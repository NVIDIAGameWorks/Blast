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
// Copyright (c) 2022-2023 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtExporterJsonCollision.h"
#include "NvBlastExtAuthoringTypes.h"
#include "NvVec3.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#define JS_NAME(name) "\"" << name << "\": "

using namespace Nv::Blast;


void serializaHullPolygon(std::ofstream& stream, const HullPolygon& p, uint32_t indent)
{
    std::string sindent(indent, '\t');
    std::string bindent(indent + 1, '\t');
    stream << sindent << "{\n" <<
        bindent << JS_NAME("mIndexBase") << p.indexBase << ",\n" <<
        bindent << JS_NAME("mPlane") << "[" << p.plane[0] << ", " << p.plane[1] << ", " << p.plane[2] << ", " << p.plane[3] << "],\n" <<
        bindent << JS_NAME("mNbVerts") << p.vertexCount << "\n" <<
        sindent << "}";
}
void serializeCollisionHull(std::ofstream& stream, const CollisionHull& hl, uint32_t indent)
{
    std::string sindent(indent, '\t');
    std::string bindent(indent + 1, '\t');

    stream << sindent << "{\n" << bindent << JS_NAME("indices") << "[";
    for (uint32_t i = 0; i < hl.indicesCount; ++i)
    {
        stream << hl.indices[i];
        if (i < hl.indicesCount - 1) stream << ", ";
    }
    stream << "],\n";
    stream << bindent << JS_NAME("points") << "[";
    for (uint32_t i = 0; i < hl.pointsCount; ++i)
    {
        auto& p = hl.points[i];
        stream << p.x << ", " << p.y << ", " << p.z;
        if (i < hl.pointsCount - 1) stream << ", ";
    }
    stream << "],\n";
    stream << bindent << JS_NAME("polygonData") << "[\n";
    for (uint32_t i = 0; i < hl.polygonDataCount; ++i)
    {
        serializaHullPolygon(stream, hl.polygonData[i], indent + 1);
        if (i < hl.polygonDataCount - 1) stream << ", ";
        stream << "\n";
    }
    stream << bindent << "]\n";
    stream << sindent << "}";
}


/**
Implementation of object which serializes collision geometry to JSON format.
*/
class JsonCollisionExporter : public IJsonCollisionExporter
{
public:
    JsonCollisionExporter() {}
    ~JsonCollisionExporter() = default;

    virtual void    release() override;

    virtual bool    writeCollision(const char* path, uint32_t chunkCount, const uint32_t* hullOffsets, const CollisionHull* const * hulls) override;
};


void
JsonCollisionExporter::release()
{
    delete this;
}


bool
JsonCollisionExporter::JsonCollisionExporter::writeCollision(const char* path, uint32_t chunkCount, const uint32_t* hullOffsets, const CollisionHull* const * hulls)
{
    std::ofstream stream(path, std::ios::out);
    stream << std::fixed << std::setprecision(8);
    if (!stream.is_open())
    {
        std::cout << "Can't open output stream" << std::endl;
        return false;
    }

    stream << "{\n" << "\t" << JS_NAME("CollisionData") << "[\n";
    for (uint32_t i = 0; i < chunkCount; ++i)
    {
        stream << "\t\t" << "[\n";
        for (uint32_t j = hullOffsets[i]; j < hullOffsets[i + 1]; ++j)
        {
            serializeCollisionHull(stream, *hulls[j], 3);
            stream << ((j < hullOffsets[i + 1] - 1) ? ",\n" : "\n");
        }
        stream << "\t\t" << ((i < chunkCount - 1) ? "], \n" : "]\n");
    }
    stream << "\t]\n}";
    stream.close();
    return true;
};


IJsonCollisionExporter* NvBlastExtExporterCreateJsonCollisionExporter()
{
    return new JsonCollisionExporter;
}
