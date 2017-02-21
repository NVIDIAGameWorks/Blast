// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013-2015 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#include "SimpleRenderable.h"

#include "RenderInterface.h"

#include <vector>

#include "Nv.h"

static std::vector<SimpleRenderable*> g_SimpleShapes;

struct DefaultVertexType
{
	atcore_float3 pos;
	atcore_float4 color;
};

///////////////////////////////////////////////////////////////////////////////
SimpleRenderable::SimpleRenderable()
	: m_pVertexBuffer(NV_NULL)
	, m_numIndices(0)
	, m_numVertices(0)
{

}

///////////////////////////////////////////////////////////////////////////////
SimpleRenderable::~SimpleRenderable()
{
	Free();
}

///////////////////////////////////////////////////////////////////////////////
void SimpleRenderable::Free()
{
	SAFE_RELEASE(m_pVertexBuffer);
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleRenderable::Initialize()
{
	//////////////////////////////////////////////////////////////////////////////
	// create icon shape
	g_SimpleShapes.resize(NUM_SHAPE_TYPES);

	g_SimpleShapes[GROUND_YUP] = new SimpleRenderable;
	g_SimpleShapes[GROUND_YUP]->InitGroundGeometry(false);

	g_SimpleShapes[GROUND_ZUP] = new SimpleRenderable;
	g_SimpleShapes[GROUND_ZUP]->InitGroundGeometry(true);

	g_SimpleShapes[AXIS_YUP] = new SimpleRenderable;
	g_SimpleShapes[AXIS_YUP]->InitAxisGeometry(false);

	g_SimpleShapes[AXIS_ZUP] = new SimpleRenderable;
	g_SimpleShapes[AXIS_ZUP]->InitAxisGeometry(true);

	g_SimpleShapes[WIND_YUP] = new SimpleRenderable;
	g_SimpleShapes[WIND_YUP]->InitWindGeometry();

	g_SimpleShapes[WIND_ZUP] = new SimpleRenderable;
	g_SimpleShapes[WIND_ZUP]->InitWindGeometry();

	g_SimpleShapes[LIGHT] = new SimpleRenderable;
	g_SimpleShapes[LIGHT]->InitLightGeometry();

	g_SimpleShapes[LIGHT_RAY] = new SimpleRenderable;
	g_SimpleShapes[LIGHT_RAY]->InitLightRayGeometry();

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void SimpleRenderable::Draw(SHAPE_TYPE t, bool depthTest)
{
	if (t >= g_SimpleShapes.size())
		return;

	g_SimpleShapes[t]->Draw(depthTest);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void SimpleRenderable::Shutdown()
{
	for (int i = 0; i < g_SimpleShapes.size(); i++)
	{
		if (g_SimpleShapes[i])
		{
			g_SimpleShapes[i]->Free();
			delete g_SimpleShapes[i];
		}
	}

	g_SimpleShapes.clear();
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleRenderable::InitGroundGeometry(bool zUP)
{
	const int nSeed = 10;
	const int nRow = nSeed * 2 + 1;
	const int nLines = nRow * 2;
	const float fLen = 25.0f;
	const float fSpace = fLen/nSeed;
	float groundHeight = 0.0f;

	const atcore_float4 color1 = gfsdk_makeFloat4(0.65, 0.65, 0.65, 1.0f);
	const atcore_float4 color2 = gfsdk_makeFloat4(0.1f, 0.1f, 0.1f, 1.0f);

	DWORD offset;

	const int numVerts = nLines * 2;
	DefaultVertexType* buf = new DefaultVertexType[numVerts];
	DefaultVertexType* pData = buf;

	//build ground lines/verts
	for(int i = 0; i < nSeed; i++)
	{
		if ( zUP )
		{
			// lines parallel to y
			pData->pos = gfsdk_makeFloat3(-fSpace * (i+1), -fLen, groundHeight);
			pData->color = color1;
			pData++;

			pData->pos = gfsdk_makeFloat3(-fSpace * (i+1), fLen, groundHeight);
			pData->color = color1;
			pData++;

			// lines parallel to y
			pData->pos = gfsdk_makeFloat3(fSpace * (i+1), -fLen, groundHeight);
			pData->color = color1;
			pData++;

			pData->pos = gfsdk_makeFloat3(fSpace * (i+1), fLen, groundHeight);
			pData->color = color1;
			pData++;

			// line x
			pData->pos = gfsdk_makeFloat3(-fLen, -fSpace * (i+1), groundHeight);
			pData->color = color1;
			pData++;

			pData->pos = gfsdk_makeFloat3(fLen, -fSpace * (i+1), groundHeight);
			pData->color = color1;
			pData++;

			// line x
			pData->pos = gfsdk_makeFloat3(-fLen, fSpace * (i+1), groundHeight);
			pData->color = color1;
			pData++;

			pData->pos = gfsdk_makeFloat3(fLen, fSpace * (i+1), groundHeight);
			pData->color = color1;
			pData++;
		}
		else
		{
			// lines parallel to z
			pData->pos = gfsdk_makeFloat3(-fSpace * (i+1), groundHeight, -fLen);
			pData->color = color1;
			pData++;

			pData->pos = gfsdk_makeFloat3(-fSpace * (i+1), groundHeight, fLen);
			pData->color = color1;
			pData++;

			// lines parallel to z
			pData->pos = gfsdk_makeFloat3(fSpace * (i+1), groundHeight, -fLen);
			pData->color = color1;
			pData++;

			pData->pos = gfsdk_makeFloat3(fSpace * (i+1), groundHeight, fLen);
			pData->color = color1;
			pData++;

			// line x
			pData->pos = gfsdk_makeFloat3(-fLen, groundHeight, -fSpace * (i+1));
			pData->color = color1;
			pData++;

			pData->pos = gfsdk_makeFloat3(fLen, groundHeight, -fSpace * (i+1));
			pData->color = color1;
			pData++;

			// line x
			pData->pos = gfsdk_makeFloat3(-fLen, groundHeight, fSpace * (i+1));
			pData->color = color1;
			pData++;

			pData->pos = gfsdk_makeFloat3(fLen, groundHeight, fSpace * (i+1));
			pData->color = color1;
			pData++;
		}
	}

	if ( zUP )
	{
		// line y
		pData->pos = gfsdk_makeFloat3(-fLen, 0.0f, groundHeight);
		pData->color = color2;
		pData++;

		pData->pos = gfsdk_makeFloat3(fLen, 0.0f, groundHeight);
		pData->color = color2;
		pData++;

		// line x
		pData->pos = gfsdk_makeFloat3(0.0f, -fLen, groundHeight);
		pData->color = color2;
		pData++;

		pData->pos = gfsdk_makeFloat3(0.0f, fLen, groundHeight);
		pData->color = color2;
		pData++;
	}
	else
	{
		// line z
		pData->pos = gfsdk_makeFloat3(-fLen, groundHeight, 0.0f);
		pData->color = color2;
		pData++;

		pData->pos = gfsdk_makeFloat3(fLen, groundHeight, 0.0f);
		pData->color = color2;
		pData++;

		// line x
		pData->pos = gfsdk_makeFloat3(0.0f, groundHeight, -fLen);
		pData->color = color2;
		pData++;

		pData->pos = gfsdk_makeFloat3(0.0f, groundHeight, fLen);
		pData->color = color2;
		pData++;
	}

	int bufBytes = sizeof(DefaultVertexType) * numVerts;

	m_pVertexBuffer = RenderInterface::CreateVertexBuffer(bufBytes, buf);

	m_numVertices = numVerts;

	return (NV_NULL != m_pVertexBuffer);
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleRenderable::InitAxisGeometry(bool zUP)
{
	const atcore_float4 colorRed = gfsdk_makeFloat4(1.0f, 0, 0, 1.0f);
	const atcore_float4 colorGreen = gfsdk_makeFloat4(0, 1.0f, 0, 1.0f);
	const atcore_float4 colorBlue = gfsdk_makeFloat4(0, 0, 1.0f, 1.0f);
	const float dist = 1.0f;
	const int numVerts = 3 * 2;
	DefaultVertexType* buf = new DefaultVertexType[numVerts];
	DefaultVertexType* pData = buf;

	{
		// x axis
		pData->pos = gfsdk_makeFloat3(0, 0, 0);
		pData->color = colorRed;
		pData++;

		pData->pos = gfsdk_makeFloat3(dist, 0, 0);
		pData->color = colorRed;
		pData++;

		// y axis
		pData->pos = gfsdk_makeFloat3(0, 0, 0);
		pData->color = colorGreen;
		pData++;

		pData->pos = gfsdk_makeFloat3(0, dist, 0);
		pData->color = colorGreen;
		pData++;

		// z axis
		pData->pos = gfsdk_makeFloat3(0, 0, 0);
		pData->color = colorBlue;
		pData++;

		pData->pos = gfsdk_makeFloat3(0, 0, dist);
		pData->color = colorBlue;
		pData++;
	}

	int bufBytes = sizeof(DefaultVertexType) * numVerts;

	m_pVertexBuffer = RenderInterface::CreateVertexBuffer(bufBytes, buf);

	m_numVertices = numVerts;

	return (NV_NULL != m_pVertexBuffer);
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleRenderable::InitLightGeometry()
{
	// The following geometry data are generated by external/ObjLoad tool
#include "GeometryData/LightGeometryData.h"
	const atcore_float4 colorYellow = gfsdk_makeFloat4(1.0f, 1.0f, 0, 1.0f);
	const int numVerts = num_faces*3*2;
	DefaultVertexType* buf = new DefaultVertexType[numVerts];
	DefaultVertexType* pData = buf;

	const float modelScale = 0.25f;
	for (int fi = 0; fi < num_faces; ++fi)
	{
		float* v0 = &vertices[(fi*3+0)*8];
		float* v1 = &vertices[(fi*3+1)*8];
		float* v2 = &vertices[(fi*3+2)*8];

		// flip Y
		v0[2] *= -1;
		v1[2] *= -1;
		v2[2] *= -1;
		
		// line 0
		pData->pos = modelScale * gfsdk_makeFloat3(v0[0], v0[1], v0[2]);
		pData->color = colorYellow;
		pData++;

		pData->pos = modelScale * gfsdk_makeFloat3(v1[0], v1[1], v1[2]);
		pData->color = colorYellow;
		pData++;

		// line 1
		pData->pos = modelScale * gfsdk_makeFloat3(v1[0], v1[1], v1[2]);
		pData->color = colorYellow;
		pData++;

		pData->pos = modelScale * gfsdk_makeFloat3(v2[0], v2[1], v2[2]);
		pData->color = colorYellow;
		pData++;

		// line 2
		pData->pos = modelScale * gfsdk_makeFloat3(v2[0], v2[1], v2[2]);
		pData->color = colorYellow;
		pData++;

		pData->pos = modelScale * gfsdk_makeFloat3(v0[0], v0[1], v0[2]);
		pData->color = colorYellow;
		pData++;
	}

	int bufBytes = sizeof(DefaultVertexType) * numVerts;

	m_pVertexBuffer = RenderInterface::CreateVertexBuffer(bufBytes, buf);

	m_numVertices = numVerts;

	return (NV_NULL != m_pVertexBuffer);
}


///////////////////////////////////////////////////////////////////////////////
bool SimpleRenderable::InitLightRayGeometry()
{
	const atcore_float4 colorRed = gfsdk_makeFloat4(1.0f, 0, 0, 1.0f);
	const int numVerts = 2;
	DefaultVertexType* buf = new DefaultVertexType[numVerts];
	DefaultVertexType* pData = buf;

	pData->pos = gfsdk_makeFloat3(0, 0, 0);
	pData->color = colorRed;
	pData++;

	pData->pos = gfsdk_makeFloat3(0, 0, 0);
	pData->color = colorRed;
	pData++;

	int bufBytes = sizeof(DefaultVertexType) * numVerts;

	m_pVertexBuffer = RenderInterface::CreateVertexBuffer(bufBytes, buf);

	m_numVertices = numVerts;

	return (NV_NULL != m_pVertexBuffer);
}

///////////////////////////////////////////////////////////////////////////////
bool SimpleRenderable::InitWindGeometry()
{
	// The following geometry data are generated by external/ObjLoad tool
#include "GeometryData/WindGeometryData.h"
	const atcore_float4 colorBlue = gfsdk_makeFloat4(0, 0, 1.0f, 1.0f);
	const int numVerts = num_faces*3*2;
	DefaultVertexType* buf = new DefaultVertexType[numVerts];
	DefaultVertexType* pData = buf;

	const float modelScale = 0.5f;
	for (int fi = 0; fi < num_faces; ++fi)
	{
		float* v0 = &vertices[(fi*3+0)*8];
		float* v1 = &vertices[(fi*3+1)*8];
		float* v2 = &vertices[(fi*3+2)*8];
		
		pData->pos = modelScale * gfsdk_makeFloat3(v0[0], v0[1], v0[2]);
		pData->color = colorBlue;
		pData++;

		pData->pos = modelScale * gfsdk_makeFloat3(v1[0], v1[1], v1[2]);
		pData->color = colorBlue;
		pData++;

		pData->pos = modelScale * gfsdk_makeFloat3(v1[0], v1[1], v1[2]);
		pData->color = colorBlue;
		pData++;

		pData->pos = modelScale * gfsdk_makeFloat3(v2[0], v2[1], v2[2]);
		pData->color = colorBlue;
		pData++;

		pData->pos = modelScale * gfsdk_makeFloat3(v2[0], v2[1], v2[2]);
		pData->color = colorBlue;
		pData++;

		pData->pos = modelScale * gfsdk_makeFloat3(v0[0], v0[1], v0[2]);
		pData->color = colorBlue;
		pData++;
	}

	int bufBytes = sizeof(DefaultVertexType) * numVerts;

	m_pVertexBuffer = RenderInterface::CreateVertexBuffer(bufBytes, buf);

	m_numVertices = numVerts;

	return (NV_NULL != m_pVertexBuffer);
}

///////////////////////////////////////////////////////////////////////////////
void SimpleRenderable::Draw(bool depthTest)
{
	RenderInterface::ApplyPrimitiveTopologyLine();
	
	RenderInterface::ApplyShader(RenderInterface::SHADER_TYPE_SIMPLE_COLOR);
	RenderInterface::DrawLineList(m_pVertexBuffer, m_numVertices, sizeof(DefaultVertexType));

	RenderInterface::FlushDX12();
	RenderInterface::ApplyPrimitiveTopologyTriangle();
}

///////////////////////////////////////////////////////////////////////////////
void SimpleRenderable::DrawLine(const atcore_float3& from, const atcore_float3& to)
{
	SimpleRenderable* pShape = g_SimpleShapes[LIGHT_RAY];

	DefaultVertexType buf[2];
	DefaultVertexType* pData = buf;
		
	// start position
	pData->pos = from;
	pData->color = gfsdk_makeFloat4(1.0f, 1.0f, 0, 1.0f);
	pData++;

	// end position
	pData->pos = to;
	pData->color = gfsdk_makeFloat4(1.0f, 1.0f, 0, 1.0f);
	pData++;

	RenderInterface::CopyToDevice(pShape->m_pVertexBuffer, buf, 2 * sizeof(DefaultVertexType));
	
	pShape->Draw(true);
}

