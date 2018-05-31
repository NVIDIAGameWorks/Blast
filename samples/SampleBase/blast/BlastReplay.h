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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef BLAST_REPLAY_H
#define BLAST_REPLAY_H

#include "NvBlastExtSync.h"
#include <chrono>

using namespace Nv::Blast;

class BlastReplay
{
public:
	BlastReplay();
	~BlastReplay();

	bool isRecording() const
	{
		return m_isRecording;
	}

	bool isPlaying() const
	{
		return m_isPlaying;
	}

	bool hasRecord() const
	{
		return m_buffer.size() > 0;
	}

	size_t getEventCount() const
	{
		return isRecording() ? m_sync->getSyncBufferSize() : m_buffer.size();
	}

	uint32_t getCurrentEventIndex() const
	{
		return m_nextEventIndex;
	}

	void addFamily(TkFamily* family);
	void removeFamily(TkFamily* family);

	void startRecording(ExtPxManager& manager, bool syncFamily, bool syncPhysics);
	void stopRecording();
	void startPlayback(ExtPxManager& manager, TkGroup* group);
	void stopPlayback();
	void update();
	void reset();

private:
	void clearBuffer();

	ExtPxManager*										m_pxManager;
	TkGroup*											m_group;
	std::chrono::steady_clock::time_point				m_startTime;
	uint64_t											m_firstEventTs;
	uint32_t											m_nextEventIndex;
	bool												m_isRecording;
	bool												m_isPlaying;
	ExtSync*									        m_sync;
	std::vector<ExtSyncEvent*>				            m_buffer;
};


#endif // ifndef BLAST_REPLAY_H