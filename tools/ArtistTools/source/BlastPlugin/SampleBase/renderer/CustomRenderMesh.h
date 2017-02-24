/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef CUSTOM_RENDER_MESH_H
#define CUSTOM_RENDER_MESH_H

#include "Renderable.h"


class CustomRenderMesh : public IRenderMesh
{
public:
	const std::vector<D3D11_INPUT_ELEMENT_DESC>& getInputElementDesc() const { return m_inputDesc; }
	void render(ID3D11DeviceContext& context) const;

	CustomRenderMesh(const void* vertices, uint32_t numVertices, uint32_t vertexSize, std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, const uint16_t* faces = nullptr, uint32_t numFaces = 0);
	virtual ~CustomRenderMesh();

protected:
	CustomRenderMesh();
	void initialize(const void* vertices, uint32_t numVertices, uint32_t vertexSize, std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, const uint16_t* faces, uint32_t numFaces);

private:
	ID3D11Buffer* m_vertexBuffer;
	ID3D11Buffer* m_indexBuffer;
	uint32_t      m_numFaces;
	uint32_t      m_numVertices;
	uint32_t      m_vertexSize;

	std::vector<D3D11_INPUT_ELEMENT_DESC> m_inputDesc;
};


#endif //CUSTOM_RENDER_MESH_H