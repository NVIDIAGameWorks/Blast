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
#include "NvBlastExtPxFamily.h"

#include "PxRigidDynamic.h"
#include "PxScene.h"


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

DamageToolController::DamageToolController()
	: m_damage(100.0f), m_toolColor(1.0f, 1.0f, 1.0f, 0.4f), 
	m_toolRenderMaterial(nullptr), m_sphereToolRenderable(nullptr), m_lineToolRenderable(nullptr), 
	m_explosiveImpulse(100), m_damagerIndex(0), m_stressForceFactor(1.0f), m_isMousePressed(false), m_damageCountWhilePressed(0)
{
	// damage amount calc using NvBlastExtMaterial
	auto getDamageAmountFn = [](const float damage, ExtPxActor* actor)
	{
		const NvBlastExtMaterial* material = actor->getFamily().getMaterial();
		return material ? material->getNormalizedDamage(damage) : 0.f;
	};

	// Damage functions
	auto radialDamageExecute = [&](const Damager* damager, ExtPxActor* actor, BlastFamily& family, const Damager::DamageData& data)
	{
		const float damage = getDamageAmountFn(m_damage, actor);
		if (damage > 0.f)
		{
			NvBlastExtRadialDamageDesc desc =
			{
				damage,
				{ data.hitPosition.x, data.hitPosition.y, data.hitPosition.z },
				damager->radius,
				damager->radius * 1.6f
			};

			getBlastController().deferDamage(actor, family, damager->program, &desc, sizeof(desc));
		}
	};
	auto sliceDamageExecute = [&](const Damager* damager, ExtPxActor* actor, BlastFamily& family, const Damager::DamageData& data)
	{
		const float damage = getDamageAmountFn(m_damage, actor);
		if (damage > 0.f)
		{
			PxVec3 farEnd = data.origin + data.weaponDir * 1000.0f;
			PxVec3 farEndPrev = data.origin + data.previousWeaponDir * 1000.0f;

			NvBlastExtTriangleIntersectionDamageDesc desc =
			{
				damage,
				{ data.origin.x, data.origin.y, data.origin.z },
				{ farEnd.x, farEnd.y, farEnd.z },
				{ farEndPrev.x, farEndPrev.y, farEndPrev.z },
			};

			getBlastController().deferDamage(actor, family, damager->program, &desc, sizeof(desc));
		}
	};
	auto capsuleDamageExecute = [&](const Damager* damager, ExtPxActor* actor, BlastFamily& family, const Damager::DamageData& data)
	{
		const float damage = getDamageAmountFn(m_damage, actor);
		if (damage > 0.f)
		{
			PxVec3 dir = (data.hitPosition - data.origin).getNormalized();
			PxVec3 farEnd = data.origin + dir * 10000.0f;

			NvBlastExtCapsuleRadialDamageDesc desc =
			{
				damage,
				{ data.origin.x, data.origin.y, data.origin.z },
				{ farEnd.x, farEnd.y, farEnd.z },
				damager->radius,
				damager->radius * 1.6f
			};

			getBlastController().deferDamage(actor, family, damager->program, &desc, sizeof(desc));
		}
	};
	auto impulseSpreadDamageExecute = [&](const Damager* damager, ExtPxActor* actor, BlastFamily& family, const Damager::DamageData& data)
	{
		const float damage = m_damage;
		if (damage > 0.f)
		{
			PxVec3 impactNormal = -data.hitNormal;

			NvBlastExtImpactSpreadDamageDesc desc =
			{
				damage,
				{ data.hitPosition.x, data.hitPosition.y, data.hitPosition.z },
				damager->radius,
				damager->radius * 1.6f
			};

			getBlastController().immediateDamage(actor, family, damager->program, &desc);
		}
	};
	auto shearDamageExecute = [&](const Damager* damager, ExtPxActor* actor, BlastFamily& family, const Damager::DamageData& data)
	{
		const float damage = getDamageAmountFn(m_damage, actor);
		if (damage > 0.f)
		{
			PxVec3 impactNormal = -data.hitNormal;

			NvBlastExtShearDamageDesc desc =
			{
				damage,
				{ impactNormal.x, impactNormal.y, impactNormal.z },
				{ data.hitPosition.x, data.hitPosition.y, data.hitPosition.z },
				damager->radius,
				damager->radius * 1.6f
			};

			getBlastController().deferDamage(actor, family, damager->program, &desc, sizeof(desc));
		}
	};
	auto stressDamageExecute = [&](const Damager* damager, ExtPxActor* actor, BlastFamily& family, const Damager::DamageData& data)
	{
		PxVec3 force = -m_stressForceFactor * data.hitNormal * actor->getPhysXActor().getMass();

		getBlastController().stressDamage(actor, data.hitPosition, force);
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
		dam.uiName = "Slice Damage";
		dam.program = NvBlastDamageProgram{ NvBlastExtTriangleIntersectionGraphShader, NvBlastExtTriangleIntersectionSubgraphShader };
		dam.pointerType = Damager::PointerType::Line;
		dam.pointerColor = DirectX::XMFLOAT4(0.1f, 1.0f, 0.1f, 0.4f);
		dam.executeFunction = sliceDamageExecute;
		dam.damageWhilePressed = true;
		dam.radius = .2f;
		dam.radiusLimit = .2f;
		m_damagers.push_back(dam);
	}

	{
		Damager dam;
		dam.uiName = "Capsule Damage (Falloff)";
		dam.program = NvBlastDamageProgram{ NvBlastExtCapsuleFalloffGraphShader, NvBlastExtCapsuleFalloffSubgraphShader };
		dam.pointerType = Damager::PointerType::Line;
		dam.pointerColor = DirectX::XMFLOAT4(0.1f, 1.0f, 0.1f, 0.4f);
		dam.executeFunction = capsuleDamageExecute;
		dam.damageWhilePressed = true;
		dam.radius = .2f;
		dam.radiusLimit = 20.0f;
		m_damagers.push_back(dam);
	}

	{
		Damager dam;
		dam.uiName = "Impact Spread Damage";
		dam.program = NvBlastDamageProgram { NvBlastExtImpactSpreadGraphShader, NvBlastExtImpactSpreadSubgraphShader };
		dam.pointerType = Damager::PointerType::Sphere;
		dam.pointerColor = DirectX::XMFLOAT4(0.5f, 1.0f, 0.5f, 0.4f);
		dam.executeFunction = impulseSpreadDamageExecute;
		m_damagers.push_back(dam);
	}

	{
		Damager dam;
		dam.uiName = "Shear Damage";
		dam.program = NvBlastDamageProgram{ NvBlastExtShearGraphShader, NvBlastExtShearSubgraphShader };
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

	for (const Damager& d : m_damagers)
	{
		m_damagerNames.push_back(d.uiName);
	}
}

DamageToolController::~DamageToolController()
{
}

void DamageToolController::onSampleStart()
{
	// damage tool pointer
	m_toolRenderMaterial = new RenderMaterial(getRenderer().getResourceManager(), "physx_primitive_transparent", "", RenderMaterial::BLEND_ALPHA_BLENDING);
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
	setDamageMode(true);
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
		const Damager& damager = m_damagers[m_damagerIndex];

		// ray cast according to camera + mouse ray
		PxVec3 eyePos, pickDir;
		getPhysXController().getEyePoseAndPickDir(m_lastMousePos.x, m_lastMousePos.y, eyePos, pickDir);
		pickDir = pickDir.getNormalized();

		PxRaycastHit hit; hit.shape = NULL;
		PxRaycastBuffer hit1;
		getPhysXController().getPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, hit1, PxHitFlag::ePOSITION | PxHitFlag::eNORMAL);
		hit = hit1.block;

		if (hit.shape || (m_prevWasHit && damager.pointerType == Damager::Line))
		{
			PxMat44 cameraViewInv = XMMATRIXToPxMat44(getRenderer().getCamera().GetViewMatrix()).inverseRT();
			PxVec3 weaponOrigin = eyePos + cameraViewInv.rotate(WEAPON_POSITION_IN_VIEW);
			PxVec3 weaponDir = (hit.position - weaponOrigin).getNormalized();

			// damage function
			auto damageFunction = [&](ExtPxActor* actor, BlastFamily& family)
			{
				auto t0 = actor->getPhysXActor().getGlobalPose();
				PxTransform t(t0.getInverse());
				Damager::DamageData data;
				data.hitNormal = t.rotate(hit.normal);
				data.hitPosition = t.transform(hit.position);
				data.origin = t.transform(weaponOrigin);
				data.weaponDir = t.rotate(weaponDir);
				data.previousWeaponDir = t.rotate(m_previousPickDir);
				damager.executeFunction(&damager, actor, family, data);
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

			m_previousPickDir = weaponDir;
			m_prevWasHit = true;
		}
		else
		{
			m_prevWasHit = false;
		}
	}
}


LRESULT DamageToolController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONUP)
	{
		m_lastMousePos.x = (short)LOWORD(lParam) / getRenderer().getScreenWidth();
		m_lastMousePos.y = (short)HIWORD(lParam) / getRenderer().getScreenHeight();
	}

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
	{
		m_isMousePressed = (uMsg == WM_LBUTTONDOWN);
	}

	if (uMsg == WM_MOUSEWHEEL)
	{
		int delta = int((short)HIWORD(wParam)) / WHEEL_DELTA;
		changeDamageRadius(delta * 0.3f);
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
			setDamageMode(!isDamageMode());
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

	getPhysXController().setDraggingEnabled(!m_damageMode);

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
