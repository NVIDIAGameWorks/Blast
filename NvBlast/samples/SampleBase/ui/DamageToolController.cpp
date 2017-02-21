/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "DamageToolController.h"
#include "RenderUtils.h"
#include "BlastController.h"
#include "Renderer.h"
#include "PhysXController.h"
#include "SampleProfiler.h"

#include <imgui.h>

#include "NvBlastTkActor.h"
#include "NvBlastExtDamageShaders.h"
#include "NvBlastExtPxActor.h"

#include "PxRigidDynamic.h"
#include "PxScene.h"


using namespace Nv::Blast;
using namespace physx;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const DirectX::XMFLOAT4 PICK_POINTER_ACTIVE_COLOR(1.0f, 0.f, 0.f, 0.6f);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


DamageToolController::DamageToolController()
	: m_damageRadius(5.0f),	m_compressiveDamage(1.0f), m_pickPointerColor(1.0f, 1.0f, 1.0f, 0.4f), 
	m_pickPointerRenderMaterial(nullptr), m_pickPointerRenderable(nullptr), m_explosiveImpulse(100), m_damageProfile(0), m_stressForceFactor(1.0f)
{
	// Damage functions
	auto radialDamageExecute = [&](const Damager* damager, ExtPxActor* actor, PxVec3 position, PxVec3 normal)
	{
		NvBlastExtRadialDamageDesc desc =
		{
			m_compressiveDamage,
			{ position.x, position.y, position.z },
			m_damageRadius,
			m_damageRadius + 2.0f
		};

		actor->getTkActor().damage(damager->program, &desc, sizeof(desc));
	};
	auto shearDamageExecute = [&](const Damager* damager, ExtPxActor* actor, PxVec3 position, PxVec3 normal)
	{
		PxVec3 force = -2 * normal;

		NvBlastExtShearDamageDesc desc =
		{
			{ force.x, force.y, force.z },
			{ position.x, position.y, position.z }
		};

		actor->getTkActor().damage(damager->program, &desc, sizeof(desc));
	};
	auto stressDamageExecute = [&](const Damager* damager, ExtPxActor* actor, PxVec3 position, PxVec3 normal)
	{
		PxVec3 force = -m_stressForceFactor * normal * actor->getPhysXActor().getMass();

		getBlastController().stressDamage(actor, position, force);
	};

	// Damage Tools:
	{
		Damager dam;
		dam.uiName = "Radial Damage (Falloff)";
		dam.program = NvBlastDamageProgram { NvBlastExtFalloffGraphShader, NvBlastExtFalloffSubgraphShader };
		dam.pointerColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.4f);
		dam.executeFunction = radialDamageExecute;
		m_armory.push_back(dam);
	}
	{
		Damager dam;
		dam.uiName = "Radial Damage (Cutter)";
		dam.program = NvBlastDamageProgram { NvBlastExtCutterGraphShader, NvBlastExtCutterSubgraphShader };
		dam.pointerColor = DirectX::XMFLOAT4(0.5f, 0.5f, 1.0f, 0.4f);
		dam.executeFunction = radialDamageExecute;
		m_armory.push_back(dam);
	}

	{
		Damager dam;
		dam.uiName = "Shear Damage";
		dam.program = NvBlastDamageProgram { NvBlastExtShearGraphShader, NvBlastExtShearSubgraphShader };
		dam.pointerColor = DirectX::XMFLOAT4(0.5f, 1.0f, 0.5f, 0.4f);
		dam.executeFunction = shearDamageExecute;
		m_armory.push_back(dam);
	}

	{
		Damager dam;
		dam.uiName = "Stress Damage";
		dam.program = { nullptr, nullptr };
		dam.pointerColor = DirectX::XMFLOAT4(0.5f, 0.5f, 1.0f, 0.4f);
		dam.executeFunction = stressDamageExecute;
		m_armory.push_back(dam);
	}

	for (const Damager& d : m_armory)
	{
		m_armoryNames.push_back(d.uiName);
	}
}

DamageToolController::~DamageToolController()
{
}

void DamageToolController::onSampleStart()
{
	// pick pointer
	m_pickPointerRenderMaterial = new RenderMaterial(getRenderer().getResourceManager(), "physx_primitive_transparent", "", RenderMaterial::BLEND_ALPHA_BLENDING);
	IRenderMesh* mesh = getRenderer().getPrimitiveRenderMesh(PrimitiveRenderMeshType::Sphere);
	m_pickPointerRenderable = getRenderer().createRenderable(*mesh, *m_pickPointerRenderMaterial);
	m_pickPointerRenderable->setScale(PxVec3(m_damageRadius));

	// default tool
	setDamageProfile(0);

	// start with damage mode by default
	setDamageMode(true);
}

void DamageToolController::onInitialize()
{
}


void DamageToolController::onSampleStop() 
{
	getRenderer().removeRenderable(m_pickPointerRenderable);
	SAFE_DELETE(m_pickPointerRenderMaterial);
}

void DamageToolController::Animate(double dt)
{
	PROFILER_SCOPED_FUNCTION();

	m_pickPointerColor = XMFLOAT4Lerp(m_pickPointerColor, m_armory[m_damageProfile].pointerColor, dt * 5.0f);
	m_pickPointerRenderable->setColor(m_pickPointerColor);
}


LRESULT DamageToolController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PROFILER_SCOPED_FUNCTION();

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONUP)
	{
		float mouseX = (short)LOWORD(lParam) / getRenderer().getScreenWidth();
		float mouseY = (short)HIWORD(lParam) / getRenderer().getScreenHeight();
		bool press = uMsg == WM_LBUTTONDOWN;

		// damage mode
		if (m_damageMode && m_pickPointerRenderable)
		{
			PxVec3 eyePos, pickDir;
			getPhysXController().getEyePoseAndPickDir(mouseX, mouseY, eyePos, pickDir);
			pickDir = pickDir.getNormalized();

			PxRaycastHit hit; hit.shape = NULL;
			PxRaycastBuffer hit1;
			getPhysXController().getPhysXScene().raycast(eyePos, pickDir, PX_MAX_F32, hit1, PxHitFlag::ePOSITION | PxHitFlag::eNORMAL);
			hit = hit1.block;

			if (hit.shape)
			{
				PxRigidActor* actor = hit.actor;
				m_pickPointerRenderable->setHidden(false);
				m_pickPointerRenderable->setTransform(PxTransform(hit.position));

				if (press)
				{
					damage(hit.position, hit.normal);
					m_pickPointerColor = PICK_POINTER_ACTIVE_COLOR;
				}
			}
			else
			{
				m_pickPointerRenderable->setHidden(true);
			}
		}
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
			uint32_t num = PxClamp<uint32_t>(iKeyPressed - '1', 0, (uint32_t)m_armory.size() - 1);
			setDamageProfile(num);
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
	ImGui::DragFloat("Compressive Damage", &m_compressiveDamage, 0.05f);
	ImGui::DragFloat("Explosive Impulse", &m_explosiveImpulse);
	ImGui::DragFloat("Damage Radius (Mouse WH)", &m_damageRadius);
	ImGui::DragFloat("Stress Damage Force", &m_stressForceFactor);

	// - - - - - - - -
	ImGui::Spacing();

	// Armory
	if (ImGui::Combo("Damage Profile", (int*)&m_damageProfile, m_armoryNames.data(), (int)m_armoryNames.size(), -1))
	{
		setDamageProfile(m_damageProfile);
	}
}


void DamageToolController::setDamageMode(bool enabled)
{
	m_damageMode = enabled;

	getPhysXController().setDraggingEnabled(!m_damageMode);

	if (!m_damageMode)
	{
		m_pickPointerRenderable->setHidden(true);
	}
}


void DamageToolController::setDamageProfile(uint32_t profile) 
{ 
	m_damageProfile = profile; 
}


void DamageToolController::changeDamageRadius(float dr)
{
	m_damageRadius += dr;
	m_damageRadius = PxMax(1.0f, m_damageRadius);
	m_pickPointerRenderable->setScale(PxVec3(m_damageRadius));
}


void DamageToolController::damage(physx::PxVec3 position, physx::PxVec3 normal)
{
	auto damageFunction = [&](ExtPxActor* actor)
	{
		auto t0 = actor->getPhysXActor().getGlobalPose();
		PxTransform t(t0.getInverse());
		PxVec3 localNormal = t.rotate(normal);
		PxVec3 localPosition = t.transform(position);
		Damager& damager = m_armory[m_damageProfile];
		damager.execute(actor, localPosition, localNormal);
	};

	this->getBlastController().blast(position, m_damageRadius, m_explosiveImpulse, damageFunction);

}
