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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#ifndef RENDERABLE_H
#define RENDERABLE_H

#include "RenderMaterial.h"
#include <DirectXMath.h>
#include "PxMat44.h"
#include "PxVec3.h"
#include "PxVec4.h"

using namespace physx;

class Renderer;

/**
RenderMesh interface, used by Renderable
*/
class IRenderMesh
{
public:
	virtual ~IRenderMesh() {}
	virtual const std::vector<D3D11_INPUT_ELEMENT_DESC>& getInputElementDesc() const = 0;
	virtual void render(ID3D11DeviceContext& context) const = 0;
};

/**
Renderable, represents single object renderer by Renderer.
Basically Renderable = RenderMaterial + RenderMesh
*/
class Renderable
{
public:
	//////// public API ////////

	void setMaterial(RenderMaterial& material);

	PxMat44 getModelMatrix() const
	{
		return PxMat44(m_transform) * PxMat44(PxVec4(m_scale, 1));
	}

	void setTransform(PxTransform& transform)
	{
		m_transform = transform;
	}

	const PxTransform& getTransform() const
	{
		return m_transform;
	}

	void setScale(PxVec3 scale)
	{
		m_scale = scale;
	}

	const PxVec3& getScale() const
	{
		return m_scale;
	}

	void setColor(DirectX::XMFLOAT4 color)
	{
		m_color = color;
	}
	DirectX::XMFLOAT4 getColor() const
	{
		return m_color;
	}

	void setHidden(bool hidden)
	{
		m_hidden = hidden;
	}

	bool isHidden() const 
	{ 
		return m_hidden;
	}

	bool isTransparent() const
	{
		return !(m_materialInstance->getMaterial().getBlending() == RenderMaterial::BLEND_NONE);
	}

	RenderMaterial& getMaterial() const { return m_materialInstance->getMaterial(); }

private:
	//////// methods used by Renderer ////////

	friend class Renderer;

	void render(Renderer& renderer) const
	{
		render(renderer, false);
	}

	void renderDepthStencilOnly(Renderer& renderer) const
	{
		render(renderer, true);
	}

	Renderable(IRenderMesh& mesh, RenderMaterial& material);

	void render(Renderer& renderer, bool depthStencilOnly) const;


	//////// internal data ////////

	DirectX::XMFLOAT4           m_color;
	PxTransform                 m_transform;
	PxVec3                      m_scale;

	RenderMaterial::InstancePtr m_materialInstance;
	IRenderMesh&                m_mesh;
	bool                        m_hidden;
};

#endif //RENDERABLE_H