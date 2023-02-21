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
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.


#ifndef DEBUGRENDERBUFFER_H
#define DEBUGRENDERBUFFER_H

#include "PxRenderBuffer.h"
#include <vector>

using namespace physx;


/**
Simple PxRenderBuffer implementation for easy debug primitives adding
*/
class DebugRenderBuffer : public PxRenderBuffer
{
public:
    ~DebugRenderBuffer() {}

    virtual PxU32 getNbPoints() const { return 0; }
    virtual const PxDebugPoint* getPoints() const { return nullptr; }

    virtual PxU32 getNbLines() const { return static_cast<PxU32>(m_lines.size()); }
    virtual const PxDebugLine* getLines() const { return m_lines.data(); }

    virtual PxU32 getNbTriangles() const { return 0; }
    virtual const PxDebugTriangle* getTriangles() const { return nullptr; }

    virtual PxU32 getNbTexts() const { return 0; }
    virtual const PxDebugText* getTexts() const { return nullptr; }

    virtual void append(const PxRenderBuffer& other) {}
    virtual void clear()
    {
        m_lines.clear();
    }

    std::vector<PxDebugLine> m_lines;
};


#endif //DEBUGRENDERBUFFER_H