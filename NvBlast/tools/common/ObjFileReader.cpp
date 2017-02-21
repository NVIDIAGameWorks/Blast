#include "ObjFileReader.h"

#pragma warning(push)
#pragma warning(disable:4706)
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#pragma warning(pop)


#include <iostream>
#include "PxVec3.h"
#include "PxVec2.h"

using physx::PxVec3;
using physx::PxVec2;
using Nv::Blast::Mesh;


ObjFileReader::ObjFileReader()
{
	setConvertToUE4(false);
}

std::shared_ptr<Nv::Blast::Mesh> ObjFileReader::loadFromFile(std::string filename)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> mats;
	std::string err;
	std::string mtlPath;
	bool ret = tinyobj::LoadObj(shapes, mats, err, filename.c_str());
	// can't load?
	if (!ret)
		return nullptr;

	if (shapes.size() > 1)
	{
		std::cout << "Can load only one object per mesh" << std::endl;
	}

	std::vector<PxVec3> positions;
	std::vector<PxVec3> normals;
	std::vector<PxVec2> uv;

	auto& psVec = shapes[0].mesh.positions;
	for (uint32_t i = 0; i < psVec.size() / 3; ++i)
	{
		positions.push_back(PxVec3(psVec[i * 3], psVec[i * 3 + 1], psVec[i * 3 + 2]));
	}
	auto& nmVec = shapes[0].mesh.normals;
	for (uint32_t i = 0; i < nmVec.size() / 3; ++i)
	{
		normals.push_back(PxVec3(nmVec[i * 3], nmVec[i * 3 + 1], nmVec[i * 3 + 2]));
	}
	auto& txVec = shapes[0].mesh.texcoords;
	for (uint32_t i = 0; i < txVec.size() / 2; ++i)
	{
		uv.push_back(PxVec2(txVec[i * 2], txVec[i * 2 + 1]));
	}
	PxVec3* nr = (!normals.empty()) ? normals.data() : 0;
	PxVec2* uvp = (!uv.empty()) ? uv.data() : 0;

	return std::make_shared<Mesh>(positions.data(), nr, uvp, static_cast<uint32_t>(positions.size()), shapes[0].mesh.indices.data(), static_cast<uint32_t>(shapes[0].mesh.indices.size()));
}
