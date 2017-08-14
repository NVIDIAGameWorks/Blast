// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#include "DamageToolController.h"
#include "RenderUtils.h"
#include "BlastController.h"
#include "Renderer.h"
#include "PhysXController.h"
#include "SampleProfiler.h"

#include <imgui.h>

#include "NvBlastTkActor.h"
#include "NvBlastTkFamily.h"
#include "NvBlastExtDamageShaders.h"
#include "NvBlastExtPxActor.h"

#include "PxRigidDynamic.h"
#include "PxScene.h"
#include "SimpleScene.h"
#include "BlastSceneTree.h"
#include "DefaultDamagePanel.h"
#include "ViewerOutput.h"


using namespace Nv::Blast;
using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const DirectX::XMFLOAT4 PICK_POINTER_ACTIVE_COLOR(1.0f, 0.f, 0.f, 0.6f);
static const PxVec3 WEAPON_POSITION_IN_VIEW(0, -7, 23);
static const float SEGMENT_DAMAGE_MAX_DISTANCE = 100.0f;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DamageToolController* gDamageToolController = nullptr;
DamageToolController* DamageToolController::ins()
{
	return gDamageToolController;
}
void DamageToolController::setDamageAmount(float value)
{
	m_damage = value;
}
void DamageToolController::setExplosiveImpulse(float value)
{
	m_explosiveImpulse = value;
}
void DamageToolController::setStressForceFactor(float value)
{
	m_stressForceFactor = value;
}
void DamageToolController::setDamagerIndex(int index)
{
	m_damagerIndex = index;
}
void DamageToolController::setRadius(float value)
{
	m_damagers[m_damagerIndex].radius = value;
}
void DamageToolController::setDamageWhilePressed(bool value)
{
	m_damagers[m_damagerIndex].damageWhilePressed = value;
}

DamageToolController::DamageToolController()
	: m_damage(100.0f), m_toolColor(1.0f, 1.0f, 1.0f, 0.4f), 
	m_toolRenderMaterial(nullptr), m_sphereToolRenderable(nullptr), m_lineToolRenderable(nullptr), 
	m_explosiveImpulse(100), m_damagerIndex(0), m_stressForceFactor(1.0f), m_isMousePressed(false), m_damageCountWhilePressed(0)
{
	// damage amount calc using NvBlastExtMaterial
	auto getDamageAmountFn = [](const float damage, ExtPxActor* actor)
	{
		const void* material = actor->getTkActor().getFamily().getMaterial();
		return material ? reinterpret_cast<const NvBlastExtMaterial*>(material)->getNormalizedDamage(damage) : 0.f;
	};

	// Damage functions
	auto radialDamageExecute = [&](const Damager* damager, ExtPxActor* actor, PxVec3 origin, PxVec3 position, PxVec3 normal)
	{
		const float damage = getDamageAmountFn(m_damage, actor);
		if (damage > 0.f)
		{
			NvBlastExtRadialDamageDesc desc =
			{
				damage,
				{ position.x, position.y, position.z },
				damager->radius,
				damager->radius * 1.6f
			};

			actor->getTkActor().damage(damager->program, &desc, sizeof(desc));
		}
	};
	auto lineSegmentDamageExecute = [&](const Damager* damager, ExtPxActor* actor, PxVec3 origin, PxVec3 position, PxVec3 normal)
	{
		const float damage = getDamageAmountFn(m_damage, actor);
		if (damage > 0.f)
		{
			PxVec3 dir = (position - origin).getNormalized();
			PxVec3 farEnd = origin + dir * 10000.0f;

			NvBlastExtSegmentRadialDamageDesc desc =
			{
				damage,
				{ origin.x, origin.y, origin.z },
				{ farEnd.x, farEnd.y, farEnd.z },
				damager->radius,
				damager->radius * 1.6f
			};

			actor->getTkActor().damage(damager->program, &desc, sizeof(desc));
		}
	};
	auto shearDamageExecute = [&](const Damager* damager, ExtPxActor* actor, PxVec3 origin, PxVec3 position, PxVec3 normal)
	{
		const float damage = getDamageAmountFn(m_damage, actor);
		if (damage > 0.f)
		{
			PxVec3 impactNormal = -normal;

			NvBlastExtShearDamageDesc desc =
			{
				damage,
				{ impactNormal.x, impactNormal.y, impactNormal.z },
				{ position.x, position.y, position.z },
				damager->radius,
				damager->radius * 1.6f
			};

			actor->getTkActor().damage(damager->program, &desc, sizeof(desc));
		}
	};
	auto stressDamageExecute = [&](const Damager* damager, ExtPxActor* actor, PxVec3 origin, PxVec3 position, PxVec3 normal)
	{
		PxVec3 force = -m_stressForceFactor * normal * actor->getPhysXActor().getMass();

		getBlastController().stressDamage(actor, position, force);
	};

	// Damage Tools:
	{
		Damager dam;
		dam.uiName = "Radial Damage (Falloff)";
		dam.program = NvBlastDamageProgram { NvBlastExtFalloffGraphShader, NvBlastExtFalloffSubgraphShader };
		dam.pointerType = Damager::PointerType::Sphere;
		dam.pointerColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.4f);
		dam.executeFunction = radialDamageExecute;
		m_damagers.push_back(dam);
	}
	{
		Damager dam;
		dam.uiName = "Radial Damage (Cutter)";
		dam.program = NvBlastDamageProgram { NvBlastExtCutterGraphShader, NvBlastExtCutterSubgraphShader };
		dam.pointerType = Damager::PointerType::Sphere;
		dam.pointerColor = DirectX::XMFLOAT4(0.5f, 0.5f, 1.0f, 0.4f);
		dam.executeFunction = radialDamageExecute;
		m_damagers.push_back(dam);
	}

	{
		Damager dam;
		dam.uiName = "Segment Damage (Falloff)";
		dam.program = NvBlastDamageProgram{ NvBlastExtSegmentFalloffGraphShader, NvBlastExtSegmentFalloffSubgraphShader };
		dam.pointerType = Damager::PointerType::Line;
		dam.pointerColor = DirectX::XMFLOAT4(0.1f, 1.0f, 0.1f, 0.4f);
		dam.executeFunction = lineSegmentDamageExecute;
		dam.damageWhilePressed = true;
		dam.radius = .2f;
		dam.radiusLimit = 20.0f;
		m_damagers.push_back(dam);
	}

	{
		Damager dam;
		dam.uiName = "Shear Damage";
		dam.program = NvBlastDamageProgram { NvBlastExtShearGraphShader, NvBlastExtShearSubgraphShader };
		dam.pointerType = Damager::PointerType::Sphere;
		dam.pointerColor = DirectX::XMFLOAT4(0.5f, 1.0f, 0.5f, 0.4f);
		dam.executeFunction = shearDamageExecute;
		m_damagers.push_back(dam);
	}

	{
		Damager dam;
		dam.uiName = "Stress Damage";
		dam.program = { nullptr, nullptr };
		dam.pointerType = Damager::PointerType::Sphere;
		dam.pointerColor = DirectX::XMFLOAT4(0.5f, 0.5f, 1.0f, 0.4f);
		dam.executeFunction = stressDamageExecute;
		m_damagers.push_back(dam);
	}

	// UI
	DefaultDamagePanel* pDefaultDamagePanel = DefaultDamagePanel::ins();
	pDefaultDamagePanel->setUpdateData(false);
	QComboBox* comboBoxDamageProfile = pDefaultDamagePanel->getDamageProfile();
	comboBoxDamageProfile->clear();

	// project
	BPPDefaultDamage& damage = BlastProject::ins().getParams().defaultDamage;
	damage.damageAmount = m_damage;
	damage.explosiveImpulse = m_explosiveImpulse;
	damage.stressDamageForce = m_stressForceFactor;
	int count = m_damagers.size();
	if (damage.damageStructs.buf != nullptr && damage.damageStructs.arraySizes[0] != count)
	{
		delete[] damage.damageStructs.buf;
		damage.damageStructs.buf = nullptr;
		damage.damageStructs.arraySizes[0] = 0;
	}
	if (damage.damageStructs.buf == nullptr)
	{
		damage.damageStructs.buf = new BPPDamageStruct[count];
		damage.damageStructs.arraySizes[0] = count;
	}
	damage.damageProfile = 0;
	int damageIndex = 0;

	for (const Damager& d : m_damagers)
	{
		m_damagerNames.push_back(d.uiName);

		// UI
		comboBoxDamageProfile->addItem(d.uiName);

		// project
		BPPDamageStruct& damageStruct = damage.damageStructs.buf[damageIndex++];
		damageStruct.damageRadius = d.radius;
		damageStruct.continuously = d.damageWhilePressed;
	}

	pDefaultDamagePanel->updateValues();

	gDamageToolController = this;
}

DamageToolController::~DamageToolController()
{
}

void DamageToolController::onSampleStart()
{
	// damage tool pointer
	m_toolRenderMaterial = new RenderMaterial("", getRenderer().getResourceManager(), "physx_primitive_transparent", "", RenderMaterial::BLEND_ALPHA_BLENDING);
	{
		IRenderMesh* mesh = getRenderer().getPrimitiveRenderMesh(PrimitiveRenderMeshType::Sphere);
		m_sphereToolRenderable = getRenderer().createRenderable(*mesh, *m_toolRenderMaterial);
	}
	{
		IRenderMesh* mesh = getRenderer().getPrimitiveRenderMesh(PrimitiveRenderMeshType::Box);
		m_lineToolRenderable = getRenderer().createRenderable(*mesh, *m_toolRenderMaterial);
	}

	// default tool
	m_damagerIndex = 0;

	// start with damage mode by default
	setDamageMode(false);
}

void DamageToolController::onInitialize()
{
}


void DamageToolController::onSampleStop() 
{
	getRenderer().removeRenderable(m_sphereToolRenderable);
	getRenderer().removeRenderable(m_lineToolRenderable);
	SAFE_DELETE(m_toolRenderMaterial);
}


void DamageToolController::Animate(double dt)
{
	PROFILER_SCOPED_FUNCTION();

	m_toolColor = XMFLOAT4Lerp(m_toolColor, m_damagers[m_damagerIndex].pointerColor, dt * 5.0f);

	m_sphereToolRenderable->setHidden(true);
	m_lineToolRenderable->setHidden(true);

	// damage mode
	if (m_damageMode)
	{
		// ray cast according to camera + mouse ray
		PxVec3 eyePos, pickDir;
		getPhysXController().getEyePoseAndPickDir(m_lastMousePos.x, m_lastMousePos.y, eyePos, pickDir);
		pickDir = pickDir.getNormalized();

		PxRaycastHit hit; hit.shape = NULL;
		PxRaycastBuffer hit1;
		getPhysXController().getPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, hit1, PxHitFlag::ePOSITION | PxHitFlag::eNORMAL);
		hit = hit1.block;

		if (hit.shape)
		{
			PxMat44 cameraViewInv = XMMATRIXToPxMat44(getRenderer().getCamera().GetViewMatrix()).inverseRT();
			PxVec3 weaponOrigin = eyePos + cameraViewInv.rotate(WEAPON_POSITION_IN_VIEW);

			// damage function
			const Damager& damager = m_damagers[m_damagerIndex];
			auto damageFunction = [&](ExtPxActor* actor)
			{
				auto t0 = actor->getPhysXActor().getGlobalPose();
				PxTransform t(t0.getInverse());
				PxVec3 localNormal = t.rotate(hit.normal);
				PxVec3 localPosition = t.transform(hit.position);
				PxVec3 localOrigin = t.transform(weaponOrigin);
				damager.executeFunction(&damager, actor, localOrigin, localPosition, localNormal);
			};

			// should damage? 
			bool shouldDamage = false;
			if (m_isMousePressed)
			{
				shouldDamage = damager.damageWhilePressed || m_damageCountWhilePressed == 0;
				m_damageCountWhilePressed++;
			}
			else
			{
				m_damageCountWhilePressed = 0;
			}

			// Update tool pointer and do damage with specific overlap
			if (damager.pointerType == Damager::Sphere)
			{
				m_sphereToolRenderable->setHidden(false);
				m_sphereToolRenderable->setColor(m_toolColor);
				m_sphereToolRenderable->setScale(PxVec3(damager.radius));
				m_sphereToolRenderable->setTransform(PxTransform(hit.position));

				if (shouldDamage)
				{
					if (getBlastController().overlap(PxSphereGeometry(damager.radius), PxTransform(hit.position), damageFunction))
					{
						m_toolColor = PICK_POINTER_ACTIVE_COLOR;
					}
					getPhysXController().explodeDelayed(hit.position, damager.radius, m_explosiveImpulse);
				}
			}
			else if (damager.pointerType == Damager::Line)
			{
				m_lineToolRenderable->setHidden(false);
				m_lineToolRenderable->setColor(m_toolColor);

				PxVec3 scale(damager.radius, damager.radius, SEGMENT_DAMAGE_MAX_DISTANCE);
				PxVec3 direction = (hit.position - weaponOrigin).getNormalized();
				PxVec3 position = weaponOrigin + direction * SEGMENT_DAMAGE_MAX_DISTANCE;

				m_lineToolRenderable->setScale(scale);
				PxTransform t(position, quatLookAt(direction));
				m_lineToolRenderable->setTransform(t);

				if (shouldDamage)
				{
					if (this->getBlastController().overlap(PxBoxGeometry(scale), t, damageFunction))
					{
						m_toolColor = PICK_POINTER_ACTIVE_COLOR;
					}
				}
			}
			else
			{
				PX_ASSERT(false);
			}
		}
	}
}


LRESULT DamageToolController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONUP)
	{
		m_lastMousePos.x = (float)LOWORD(lParam);
		m_lastMousePos.y = (float)HIWORD(lParam);
	}

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
	{
		m_isMousePressed = (uMsg == WM_LBUTTONDOWN);
		if (m_isMousePressed && !m_damageMode)
		{
			viewer_warn("damage mode is disable, please enable it first !");
		}
	}

	if (uMsg == WM_MOUSEWHEEL)
	{
		/*
		int delta = int((short)HIWORD(wParam)) / WHEEL_DELTA;
		changeDamageRadius(delta * 0.3f);
		*/
	}

	if (uMsg == WM_KEYDOWN)
	{
		int iKeyPressed = static_cast<int>(wParam);
		if (iKeyPressed == VK_OEM_PLUS)
		{
			changeDamageRadius(0.2f);
		}
		else if (iKeyPressed == VK_OEM_MINUS)
		{
			changeDamageRadius(-0.2f);
		}
		else if (iKeyPressed >= '1' && iKeyPressed <= '9')
		{
			m_damagerIndex = PxClamp<uint32_t>(iKeyPressed - '1', 0, (uint32_t)m_damagers.size() - 1);
		}
		else if (iKeyPressed == VK_SPACE)
		{
			//setDamageMode(!isDamageMode());
		}
	}

	return 1;
}

void DamageToolController::drawUI()
{
	ImGui::DragFloat("Damage Amount", &m_damage, 1.0f);
	ImGui::DragFloat("Explosive Impulse", &m_explosiveImpulse);
	ImGui::DragFloat("Stress Damage Force", &m_stressForceFactor);

	// - - - - - - - -
	ImGui::Spacing();

	// Armory
	ImGui::Combo("Damage Profile", (int*)&m_damagerIndex, m_damagerNames.data(), (int)m_damagerNames.size(), -1);
	Damager& damager = m_damagers[m_damagerIndex];
	ImGui::DragFloat("Damage Radius (Mouse WH)", &damager.radius);
	ImGui::Checkbox("Damage Continuously", &damager.damageWhilePressed);
}

void DamageToolController::setDamageMode(bool enabled)
{
	m_damageMode = enabled;

	//getPhysXController().setDraggingEnabled(!m_damageMode);

	if (!m_damageMode)
	{
		m_sphereToolRenderable->setHidden(true);
		m_lineToolRenderable->setHidden(true);
	}
}

void DamageToolController::changeDamageRadius(float dr)
{
	Damager& damager = m_damagers[m_damagerIndex];
	damager.radius += dr;
	damager.radius = PxClamp<float>(damager.radius, 0.05f, damager.radiusLimit);
}
