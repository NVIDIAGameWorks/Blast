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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#ifndef DAMAGE_TOOL_CONTROLLER_H
#define DAMAGE_TOOL_CONTROLLER_H

#include "SampleManager.h"
#include "NvBlastTypes.h"
#include <DirectXMath.h>
#include <functional>
#include "PxVec2.h"
#include "PxVec3.h"


class Renderable;
class RenderMaterial;
class BlastFamily;

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

	RenderMaterial*   m_toolRenderMaterial;
	Renderable*       m_sphereToolRenderable;
	DirectX::XMFLOAT4 m_toolColor;
	Renderable*       m_lineToolRenderable;

	float             m_damage;
	float             m_explosiveImpulse;
	float             m_stressForceFactor;

	struct Damager
	{
		Damager() : damageWhilePressed(false), radius(5.0f), radiusLimit(1000.0f)
		{
		}

		enum PointerType
		{
			Sphere,
			Line
		};

		struct DamageData
		{
			physx::PxVec3 origin;
			physx::PxVec3 hitPosition;
			physx::PxVec3 hitNormal;
			physx::PxVec3 weaponDir;
			physx::PxVec3 previousWeaponDir;
		};

		typedef std::function<void(const Damager* damager, Nv::Blast::ExtPxActor* actor, BlastFamily& family, const DamageData& damageData)> ExecuteFn;

		const char*				uiName;
		NvBlastDamageProgram	program;
		PointerType				pointerType;
		DirectX::XMFLOAT4		pointerColor;
		float					radius;
		float					radiusLimit;
		bool					damageWhilePressed;
		ExecuteFn				executeFunction;
	};

	std::vector<Damager>     m_damagers;
	std::vector<const char*> m_damagerNames;
	uint32_t				 m_damagerIndex;

	bool                     m_damageMode;

	physx::PxVec2            m_lastMousePos;
	bool                     m_isMousePressed;
	uint32_t                 m_damageCountWhilePressed;
	physx::PxVec3			 m_previousPickDir;
	bool					 m_prevWasHit;
};

#endif