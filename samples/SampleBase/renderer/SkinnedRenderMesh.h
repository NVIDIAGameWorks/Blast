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
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.


#ifndef SKINNED_RENDER_MESH_H
#define SKINNED_RENDER_MESH_H

#include "Utils.h"
#include <DirectXMath.h>

#include <vector>
#include "Renderable.h"
#include "Mesh.h"

/**
SkinnedRenderMesh:
	bonde indices are passed as vertex input,
	bone transforms are stored in texture
	max bone meshes count: SkinnedRenderMesh::MeshesCountMax
*/
class SkinnedRenderMesh : public IRenderMesh
{
public:
	//////// ctor ////////

	SkinnedRenderMesh(const std::vector<const SimpleMesh*>& meshes);
	~SkinnedRenderMesh();


	//////// const ////////

	static const uint32_t MeshesCountMax = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;


	//////// public API ////////

	void updateVisibleMeshes(const std::vector<uint32_t>& visibleMeshes);
	void updateVisibleMeshTransforms(std::vector<PxMat44>& transforms);


	//////// IRenderMesh implementation ////////

	virtual const std::vector<D3D11_INPUT_ELEMENT_DESC>& getInputElementDesc() const { return m_inputDesc; }
	virtual void render(ID3D11DeviceContext& context) const;

private:
	//////// internal data ////////

	struct MeshInfo
	{
		uint32_t firstIndex;
		uint32_t indicesCount;

		uint32_t firstVertex;
		uint32_t verticesCount;
	};

	std::vector<D3D11_INPUT_ELEMENT_DESC> m_inputDesc;

	ID3D11Device* m_device;

	ID3D11Buffer* m_vertexBuffer;
	ID3D11Buffer* m_boneIndexBuffer;
	ID3D11Buffer* m_indexBuffer;
	ID3D11Texture2D* m_boneTexture;
	ID3D11ShaderResourceView* m_boneTextureSRV;

	uint32_t m_indexCount;

	std::vector<MeshInfo> m_meshesInfo;
	std::vector<uint32_t> m_indices;
};



#endif //SKINNED_RENDER_MESH_H