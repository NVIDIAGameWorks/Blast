/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef EDITION_TOOL_CONTROLLER_H
#define EDITION_TOOL_CONTROLLER_H

#include "SampleManager.h"

namespace physx
{
	class PxActor;
}

class EditionToolController : public ISampleController
{
public:
	EditionToolController();
	virtual ~EditionToolController();

	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Animate(double dt);
	void drawUI();

	virtual void onInitialize();
	virtual void onSampleStart();
	virtual void onSampleStop();

	void fracture(physx::PxActor* actor);

private:
	EditionToolController& operator= (EditionToolController&);

	//////// private methods ////////

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
};

#endif // EDITION_TOOL_CONTROLLER_H