/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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