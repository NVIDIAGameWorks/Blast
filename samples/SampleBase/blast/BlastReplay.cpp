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


#include "BlastReplay.h"
#include "NvBlastTk.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxFamily.h"
#include "SampleProfiler.h"


using namespace std::chrono;

BlastReplay::BlastReplay() : m_sync(nullptr)
{
	m_sync = ExtSync::create();
	reset();
}

BlastReplay::~BlastReplay()
{
	m_sync->release();
	clearBuffer();
}

void BlastReplay::addFamily(TkFamily* family)
{
	family->addListener(*m_sync);
}

void BlastReplay::removeFamily(TkFamily* family)
{
	family->removeListener(*m_sync);
}

void BlastReplay::startRecording(ExtPxManager& manager, bool syncFamily, bool syncPhysics)
{
	if (isRecording())
		return;

	m_sync->releaseSyncBuffer();

	if (syncFamily || syncPhysics)
	{
		std::vector<ExtPxFamily*> families(manager.getFamilyCount());
		manager.getFamilies(families.data(), (uint32_t)families.size());
		for (ExtPxFamily* family : families)
		{
			if (syncPhysics)
			{
				m_sync->syncFamily(*family);
			}
			else if (syncFamily)
			{
				m_sync->syncFamily(family->getTkFamily());
			}
		}
	}

	m_isRecording = true;
}

void BlastReplay::stopRecording()
{
	if (!isRecording())
		return;

	const ExtSyncEvent*const* buffer;
	uint32_t size;
	m_sync->acquireSyncBuffer(buffer, size);

	clearBuffer();
	m_buffer.resize(size);
	for (uint32_t i = 0; i < size; ++i)
	{
		m_buffer[i] = buffer[i]->clone();
	}

	// TODO: sort by ts ? make sure?
	//m_buffer.sort

	m_sync->releaseSyncBuffer();

	m_isRecording = false;
}

void BlastReplay::startPlayback(ExtPxManager& manager, TkGroup* group)
{
	if (isPlaying() || !hasRecord())
		return;

	m_isPlaying = true;
	m_startTime = steady_clock::now();
	m_nextEventIndex = 0;
	m_firstEventTs = m_buffer[0]->timestamp;
	m_pxManager = &manager;
	m_group = group;
}

void BlastReplay::stopPlayback()
{
	if (!isPlaying())
		return;

	m_isPlaying = false;
	m_pxManager = nullptr;
	m_group = nullptr;
}

void BlastReplay::update()
{
	if (isPlaying())
	{
		PROFILER_SCOPED_FUNCTION();

		auto now = steady_clock::now();
		auto mil = duration_cast<milliseconds>((now - m_startTime));
		bool stop = true;
		while (m_nextEventIndex < m_buffer.size())
		{
			const ExtSyncEvent* e = m_buffer[m_nextEventIndex];
			auto t = e->timestamp - m_firstEventTs;
			if (t < (uint64_t)mil.count())
			{
				m_sync->applySyncBuffer(m_pxManager->getFramework(), &e, 1, m_group, m_pxManager);
				m_nextEventIndex++;
			}
			else
			{
				stop = false;
				break;
			}
		}

		if (stop)
			stopPlayback();
	}
}

void BlastReplay::reset()
{
	m_isPlaying = false;
	m_isRecording = false;
	m_sync->releaseSyncBuffer();
}

void BlastReplay::clearBuffer()
{
	for (auto e : m_buffer)
	{
		e->release();
	}
	m_buffer.clear();
}
