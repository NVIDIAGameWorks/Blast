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

    virtual PxU32 getNbPoints() const { return static_cast<PxU32>(m_points.size()); }
    virtual const PxDebugPoint* getPoints() const { return m_points.data(); }
    virtual void addPoint(const PxDebugPoint& point) { m_points.push_back(point); }

    virtual PxU32 getNbLines() const { return static_cast<PxU32>(m_lines.size()); }
    virtual const PxDebugLine* getLines() const { return m_lines.data(); }
    virtual void addLine(const PxDebugLine& line) { m_lines.push_back(line); }
    virtual PxDebugLine* reserveLines(const PxU32 count)
    {
        const size_t offset = m_lines.size();
        m_lines.resize(offset + count, PxDebugLine(PxVec3(PxZero), PxVec3(PxZero), 0));
        return count == 0 ? nullptr : m_lines.data() + offset;
    }
    virtual PxDebugPoint* reservePoints(const PxU32 count)
    {
        const size_t offset = m_points.size();
        m_points.resize(offset + count, PxDebugPoint(PxVec3(PxZero), 0));
        return count == 0 ? nullptr : m_points.data() + offset;
    }

    virtual PxU32 getNbTriangles() const { return static_cast<PxU32>(m_triangles.size()); }
    virtual const PxDebugTriangle* getTriangles() const { return m_triangles.data(); }
    virtual void addTriangle(const PxDebugTriangle& triangle) { m_triangles.push_back(triangle); }

    virtual void append(const PxRenderBuffer& other)
    {
        if (other.getNbPoints() != 0)
            m_points.insert(m_points.end(), other.getPoints(), other.getPoints() + other.getNbPoints());
        if (other.getNbLines() != 0)
            m_lines.insert(m_lines.end(), other.getLines(), other.getLines() + other.getNbLines());
        if (other.getNbTriangles() != 0)
            m_triangles.insert(m_triangles.end(), other.getTriangles(), other.getTriangles() + other.getNbTriangles());
    }
    virtual void clear()
    {
        m_points.clear();
        m_lines.clear();
        m_triangles.clear();
    }
    virtual void shift(const PxVec3& delta)
    {
        for (PxDebugPoint& point : m_points) point.pos += delta;
        for (PxDebugLine& line : m_lines) { line.pos0 += delta; line.pos1 += delta; }
        for (PxDebugTriangle& triangle : m_triangles)
        {
            triangle.pos0 += delta;
            triangle.pos1 += delta;
            triangle.pos2 += delta;
        }
    }
    virtual bool empty() const { return m_points.empty() && m_lines.empty() && m_triangles.empty(); }

    std::vector<PxDebugPoint> m_points;
    std::vector<PxDebugLine> m_lines;
    std::vector<PxDebugTriangle> m_triangles;
};


#endif //DEBUGRENDERBUFFER_H
