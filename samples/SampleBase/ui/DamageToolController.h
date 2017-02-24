/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef DAMAGE_TOOL_CONTROLLER_H
#define DAMAGE_TOOL_CONTROLLER_H

#include "SampleManager.h"
#include "NvBlastTypes.h"
#include <DirectXMath.h>
#include <functional>
#include "PxVec3.h"


class Renderable;
class RenderMaterial;

namespace Nv
{
namespace Blast
{
class ExtPxActor;
}
}



class DamageToolController : public ISampleController
{
public:
	DamageToolController();
	virtual ~DamageToolController();

	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Animate(double dt);
	void drawUI();


	virtual void onInitialize();
	virtual void onSampleStart();
	virtual void onSampleStop();

	bool isDamageMode() const
	{
		return m_damageMode;
	}

private:
	DamageToolController& operator= (DamageToolController&);


	//////// private methods ////////

	void damage(physx::PxVec3 position, physx::PxVec3 normal);

	void setDamageProfile(uint32_t profile);
	uint32_t getDamageProfile() const
	{ 
		return m_damageProfile; 
	}

	void changeDamageRadius(float dr);

	void setDamageMode(bool enabled);


	//////// used controllers ////////

	Renderer& getRenderer() const
	{
		return getManager()->getRenderer();
	}

	PhysXController& getPhysXController() const
	{
		return getManager()->getPhysXController();
	}

	BlastController& getBlastController() const
	{
		return getManager()->getBlastController();
	}


	//////// internal data ////////

	Renderable* m_pickPointerRenderable;
	RenderMaterial* m_pickPointerRenderMaterial;
	DirectX::XMFLOAT4 m_pickPointerColor;

	float m_damageRadius;
	float m_compressiveDamage;
	float m_explosiveImpulse;
	float m_stressForceFactor;
	uint32_t m_damageProfile;

	struct Damager
	{
		const char* uiName;
		NvBlastDamageProgram program;
		DirectX::XMFLOAT4 pointerColor;
		std::function<void(const Damager* damager, Nv::Blast::ExtPxActor* actor, physx::PxVec3 position, physx::PxVec3 normal)> executeFunction;

		void execute(Nv::Blast::ExtPxActor* actor, physx::PxVec3 position, physx::PxVec3 normal)
		{
			executeFunction(this, actor, position, normal);
		}
	};

	std::vector<Damager> m_armory;
	std::vector<const char*> m_armoryNames;

	bool m_damageMode;
};

#endif