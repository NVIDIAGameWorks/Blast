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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtExporterJsonCollision.h"
#include "NvBlastExtAuthoringTypes.h"
#include <PxVec3.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#define JS_NAME(name) "\"" << name << "\": "

using namespace Nv::Blast;


void serializaHullPolygon(std::ofstream& stream, const CollisionHull::HullPolygon& p, uint32_t indent)
{
	std::string sindent(indent, '\t');
	std::string bindent(indent + 1, '\t');
	stream << sindent << "{\n" <<
		bindent << JS_NAME("mIndexBase") << p.mIndexBase << ",\n" <<
		bindent << JS_NAME("mPlane") << "[" << p.mPlane[0] << ", " << p.mPlane[1] << ", " << p.mPlane[2] << ", " << p.mPlane[3] << "],\n" <<
		bindent << JS_NAME("mNbVerts") << p.mNbVerts << "\n" <<
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

	virtual void	release() override;

	virtual bool	writeCollision(const char* path, uint32_t chunkCount, const uint32_t* hullOffsets, const CollisionHull* const * hulls) override;
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
