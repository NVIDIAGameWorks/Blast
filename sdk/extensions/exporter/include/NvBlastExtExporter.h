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
// Copyright (c) 2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTEXPORTER_H
#define NVBLASTEXTEXPORTER_H

#include "NvBlastTypes.h"
#include "NvCTypes.h"

struct NvBlastAsset;

namespace Nv
{
namespace Blast
{
struct AuthoringResult;
struct CollisionHull;

struct Material
{
	const char* name;
	const char* diffuse_tex;
};

struct ExporterMeshData
{
	NvBlastAsset* asset; //Blast asset

	uint32_t positionsCount; //Number of positions

	uint32_t normalsCount; //Number of normals

	uint32_t uvsCount; //Number of textures uv

	NvcVec3* positions; //Array of positions

	NvcVec3* normals;  // Array of normals

	NvcVec2* uvs;  // Array of textures uv

	uint32_t meshCount; //Number of meshes (chunks)

	uint32_t submeshCount; //Number of submeshes

	Material* submeshMats; 


	/**
		Indices offsets for posIndex, normIndex and texIndex
		First position index: posIndex[submeshOffsets[meshId * submeshCount + submeshId]]
		Total number of indices: submeshOffsets[meshCount * submeshCount]
	*/
	uint32_t* submeshOffsets;

	uint32_t* posIndex; //Array of position indices

	uint32_t* normIndex; //Array of normals indices

	uint32_t* texIndex; //Array of texture indices


	/**
		Hull offsets. Contains meshCount + 1 element.
		First hull for i-th mesh: hulls[hullsOffsets[i]]
		hullsOffsets[meshCount+1] is total number of hulls
	*/
	uint32_t* hullsOffsets;

	CollisionHull** hulls; //Array of pointers to hull for all meshes
};

/**
	An interface for Blast mesh file reader
*/
class IMeshFileReader
{
public:
	
	/**
		Delete this object
	*/
	virtual void			release() = 0;

	/*
		Load from the specified file path
	*/
	virtual void			loadFromFile(const char* filename) = 0;

	/**
		Number of loaded vertices
	*/
	virtual uint32_t		getVerticesCount() const = 0;

	/**
		Number of loaded indices
	*/
	virtual uint32_t		getIndicesCount() const = 0;

	/**
		Get loaded vertex positions
	*/
	virtual NvcVec3*	getPositionArray() = 0;

	/**
		Get loaded vertex normals
	*/
	virtual NvcVec3*	getNormalsArray() = 0;

	/**
		Get loaded vertex uv-coordinates
	*/
	virtual NvcVec2*	getUvArray() = 0;

	/**
		Get loaded per triangle material ids.
	*/
	virtual int32_t*		getMaterialIds() = 0;

	/**
			Get loaded per triangle smoothing groups.
	*/
	virtual int32_t*		getSmoothingGroups() = 0;

	/**
		Get material name.
	*/
	virtual const char*		getMaterialName(int32_t id) = 0;

	/**
		Get material count.
	*/
	virtual int32_t			getMaterialCount() = 0;



	/**
		Get loaded triangle indices
	*/
	virtual uint32_t*		getIndexArray() = 0;


	/**
		Check whether file contained an collision geometry
	*/
	virtual bool			isCollisionLoaded() = 0;

	/**
		Retrieve collision geometry if it exist
		\note User should call NVBLAST_FREE for hulls and hullsOffset when it not needed anymore

		\param[out] hullsOffset		Array of hull offsets for hulls array. The size is meshCount + 1.
		\param[out] hulls			Array of hull. The first i-th mesh hull: hulls[hullsOffset[i]]. The size is written to hullsOffset[meshCount]
		\return						Number of meshes (meshCount)
	*/
	virtual uint32_t		getCollision(uint32_t*& hullsOffset, CollisionHull**& hulls) = 0;

};

/**
	An interface for fbx file reader
*/
class IFbxFileReader : public IMeshFileReader
{
public:
	/**
	Retrieve bone influence if it exist, this is a bone index for each vertex in the mesh
	\note User should call NVBLAST_FREE for out when it not needed anymore

	\param[out] out			Array of bone influences.
	\return					Number of bones influences (boneCount)
	*/
	virtual uint32_t getBoneInfluences(uint32_t*& out) = 0;

	/**
		Return number of bones in fbx file
	*/
	virtual uint32_t getBoneCount() = 0;
};

/**
	An interface for Blast mesh file writer
*/
class IMeshFileWriter
{
public:

	/**
		Delete this object
	*/
	virtual void release() = 0;

	/**
	Append rendermesh to scene. Meshes constructed from arrays of triangles.
	*/
	virtual bool appendMesh(const AuthoringResult& aResult, const char* assetName, bool nonSkinned = false) = 0;

	/**
	Append rendermesh to scene. Meshes constructed from arrays of vertices and indices
	*/
	virtual bool appendMesh(const ExporterMeshData& meshData, const char* assetName, bool nonSkinned = false) = 0;

	/**
	Save scene to file.
	*/
	virtual bool saveToFile(const char* assetName, const char* outputPath) = 0;

	/**
		Set material index for interior surface. By default new material will be created;
	*/
	virtual void setInteriorIndex(int32_t index) = 0;
};

}
}

/**
	Creates an instance of IMeshFileReader for reading obj file.
*/
NVBLAST_API Nv::Blast::IMeshFileReader* NvBlastExtExporterCreateObjFileReader();

/**
	Creates an instance of IFbxFileReader for reading fbx file.
*/
NVBLAST_API Nv::Blast::IFbxFileReader* NvBlastExtExporterCreateFbxFileReader();

/**
	Creates an instance of IMeshFileWriter for writing obj file.
*/
NVBLAST_API Nv::Blast::IMeshFileWriter* NvBlastExtExporterCreateObjFileWriter();

/**
	Creates an instance of IMeshFileWriter for writing fbx file.

	\param[in] outputFBXAscii	If true writes fbx in ascii format otherwise write in binary.
*/
NVBLAST_API Nv::Blast::IMeshFileWriter* NvBlastExtExporterCreateFbxFileWriter(bool outputFBXAscii = false);

#endif //NVBLASTEXTEXPORTER_H