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
// Copyright (c) 2016-2024 NVIDIA Corporation. All rights reserved.


#ifndef BLAST_REPLAY_H
#define BLAST_REPLAY_H

#include "NvBlastExtPxSync.h"
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

    ExtPxManager*                                       m_pxManager;
    TkGroup*                                            m_group;
    std::chrono::steady_clock::time_point               m_startTime;
    uint64_t                                            m_firstEventTs;
    uint32_t                                            m_nextEventIndex;
    bool                                                m_isRecording;
    bool                                                m_isPlaying;
    ExtSync*                                            m_sync;
    std::vector<ExtSyncEvent*>                          m_buffer;
};


#endif // ifndef BLAST_REPLAY_H