/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef SELECTION_TOOL_CONTROLLER_H
#define SELECTION_TOOL_CONTROLLER_H

#include "SampleManager.h"
#include "PxVec2.h"
#include "PxVec3.h"
#include "DebugRenderBuffer.h"
#include "BlastFamily.h"
#include "NvBlastExtPxManager.h"
class Renderable;
class RenderMaterial;

namespace Nv
{
namespace Blast
{
class ExtPxActor;
}
}

class SelectionToolController : public ISampleController
{
public:
	SelectionToolController();
	virtual ~SelectionToolController();

	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Animate(double dt);
	void drawUI();

	virtual void onInitialize();
	virtual void onSampleStart();
	virtual void onSampleStop();

	void pointSelect(PxActor* actor, SelectMode selectMode = SM_RESET);
	void rectSelect(PxVec3 eyePos, PxVec3 lefttop, PxVec3 leftbottom, PxVec3 righttop, PxVec3 rightbottom, SelectMode selectMode = SM_RESET);
	void clearSelect();

private:
	SelectionToolController& operator= (SelectionToolController&);

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

	PxVec2 m_RectSelectScreenPos;
	PxVec3 m_RectSelectSpaceDir;
	bool m_bRectSelecting;
	DebugRenderBuffer m_RectRenderBuffer;

	void setActorSelected(const ExtPxActor& actor, bool selected);
	std::set<ExtPxActor*> m_actorsSelected;
};

#endif // SELECTION_TOOL_CONTROLLER_H