/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
// Add By Lixu Begin
	virtual void setScale(PxMat44 scale, bool replace) {};
// Add By Lixu End
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

// Add By Lixu Begin
	void setSelected(bool selected)
	{
		m_selected = selected;
	}

	bool isSelected() const
	{
		return m_selected;
	}

	void setMeshScale(PxMat44 scale, bool replace)
	{
		m_mesh.setScale(scale, replace);
	}
// Add By Lixu End

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
// Add By Lixu Begin
	bool m_selected;
// Add By Lixu End
};

#endif //RENDERABLE_H