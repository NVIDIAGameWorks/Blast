// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2024 NVIDIA Corporation. All rights reserved.


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
