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


#include "Renderable.h"
#include "Renderer.h"
#include "RenderUtils.h"

const DirectX::XMFLOAT4 DEFAULT_COLOR(0.5f, 0.5f, 0.5f, 1.0f);

Renderable::Renderable(IRenderMesh& mesh, RenderMaterial& material) : m_mesh(mesh), m_scale(1, 1, 1), m_color(DEFAULT_COLOR), m_hidden(false), m_transform(PxIdentity)
{
	setMaterial(material);
}

void Renderable::setMaterial(RenderMaterial& material)
{
	m_materialInstance = material.getMaterialInstance(&m_mesh);
}

void Renderable::render(Renderer& renderer, bool depthStencilOnly) const
{
	if (!m_materialInstance->isValid())
	{
		PX_ALWAYS_ASSERT();
		return;
	}

	m_materialInstance->bind(*renderer.m_context, 0, depthStencilOnly);

	// setup object CB
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		renderer.m_context->Map(renderer.m_objectCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		Renderer::CBObject* objectBuffer = (Renderer::CBObject*)mappedResource.pData;
		objectBuffer->world = PxMat44ToXMMATRIX(getModelMatrix());
		objectBuffer->color = getColor();
		renderer.m_context->Unmap(renderer.m_objectCB, 0);
	}

	m_mesh.render(*renderer.m_context);
}
