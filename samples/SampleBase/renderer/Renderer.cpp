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


#include "Renderer.h"
#include "RenderUtils.h"
#include "UIHelpers.h"
#include "SampleProfiler.h"

#include "PxRenderBuffer.h"

#include <set>


const float CAMERA_CLIP_NEAR = 1.0f;
const float CAMERA_CLIP_FAR = 1000.00f;

const float CLEAR_SCENE_COLOR[4] = { 0.0f, 0.0f, 0.0f, 0.0f };


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Renderer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Renderer::Renderer()
: m_cameraCB(nullptr)
, m_worldCB(nullptr)
, m_objectCB(nullptr)
, m_RSState(nullptr)
, m_opaqueRenderDSState(nullptr)
, m_transparencyRenderDSState(nullptr)
, m_DSTexture(nullptr)
, m_DSView(nullptr)
, m_DSTextureSRV(nullptr)
, m_pointSampler(nullptr)
, m_linearSampler(nullptr)
, m_wireframeMode(false)
, m_debugPrimitiveVB(nullptr)
, m_debugPrimitiveVBVerticesCount(0)
, m_shadowEnabled(true)
, m_HBAOEnabled(true)
, m_visibleOpaqueRenderablesCount(0)
, m_visibleTransparentRenderablesCount(0)
{
	m_worldCBData.ambientColor = DirectX::XMFLOAT3(0.21f, 0.21f, 0.22f);
	m_worldCBData.pointLightColor = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	m_worldCBData.pointLightPos = DirectX::XMFLOAT3(0.0f, 30.0f, 12.0f);
	m_worldCBData.dirLightColor = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_worldCBData.dirLightDir = DirectX::XMFLOAT3(-0.08f, -0.34f, -0.91f);
	m_worldCBData.specularPower = 140.0f;
	m_worldCBData.specularIntensity = 0.4f;

	toggleCameraSpeed(false);
}

Renderer::~Renderer()
{
}

void Renderer::initializeDefaultRSState()
{
	SAFE_RELEASE(m_RSState);
	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.CullMode = D3D11_CULL_FRONT;
	desc.FillMode = m_wireframeMode ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	desc.AntialiasedLineEnable = FALSE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0;
	desc.DepthClipEnable = TRUE;
	desc.FrontCounterClockwise = FALSE;
	desc.MultisampleEnable = TRUE;
	desc.ScissorEnable = FALSE;
	desc.SlopeScaledDepthBias = 0;

	V(m_device->CreateRasterizerState(&desc, &m_RSState));
}

HRESULT Renderer::DeviceCreated(ID3D11Device* device)
{
	m_device = device;

	// Camera constant buffer
	{
		D3D11_BUFFER_DESC buffer_desc;
		ZeroMemory(&buffer_desc, sizeof(buffer_desc));
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.ByteWidth = sizeof(CBCamera);
		_ASSERT((buffer_desc.ByteWidth % 16) == 0);

		V(device->CreateBuffer(&buffer_desc, nullptr, &m_cameraCB));
	}

	// World constant buffer
	{
		D3D11_BUFFER_DESC buffer_desc;
		ZeroMemory(&buffer_desc, sizeof(buffer_desc));
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.ByteWidth = sizeof(CBWorld);
		_ASSERT((buffer_desc.ByteWidth % 16) == 0);

		V(device->CreateBuffer(&buffer_desc, nullptr, &m_worldCB));
	}

	// Object constant buffer
	{
		D3D11_BUFFER_DESC buffer_desc;
		ZeroMemory(&buffer_desc, sizeof(buffer_desc));
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.ByteWidth = sizeof(CBObject);
		_ASSERT((buffer_desc.ByteWidth % 16) == 0);

		V(device->CreateBuffer(&buffer_desc, nullptr, &m_objectCB));
	}

	// Opaque Render Depth-Stencil state
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.StencilEnable = FALSE;
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		V(device->CreateDepthStencilState(&desc, &m_opaqueRenderDSState));
	}

	// Transparency Render Depth-Stencil state
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.StencilEnable = FALSE;
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		V(device->CreateDepthStencilState(&desc, &m_transparencyRenderDSState));
	}

	// Linear sampler
	{
		D3D11_SAMPLER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		V(device->CreateSamplerState(&desc, &m_linearSampler));
	}

	// Point sampler
	{
		D3D11_SAMPLER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		V(device->CreateSamplerState(&desc, &m_pointSampler));
	}

	// Rasterizer state
	initializeDefaultRSState();

	// init primitive render meshes
	for (uint32_t i = 0; i < PrimitiveRenderMeshType::Count; i++)
	{
		m_primitiveRenderMeshes[i] = nullptr;
	}

	// init shadows
	ID3D11DeviceContext* pd3dDeviceContext;
	m_device->GetImmediateContext(&pd3dDeviceContext);
	m_shadow.createResources(m_device, pd3dDeviceContext, &m_camera);

	// init hbao
	m_HBAO.createResources(m_device);

	return S_OK;
}

void Renderer::DeviceDestroyed()
{
	SAFE_RELEASE(m_cameraCB);
	SAFE_RELEASE(m_worldCB);
	SAFE_RELEASE(m_objectCB);
	SAFE_RELEASE(m_RSState);
	SAFE_RELEASE(m_opaqueRenderDSState);
	SAFE_RELEASE(m_transparencyRenderDSState);
	SAFE_RELEASE(m_pointSampler);
	SAFE_RELEASE(m_linearSampler);
	SAFE_RELEASE(m_debugPrimitiveVB);
	SAFE_RELEASE(m_DSTexture);
	SAFE_RELEASE(m_DSView);
	SAFE_RELEASE(m_DSTextureSRV);

	for (uint32_t i = 0; i < PrimitiveRenderMeshType::Count; i++)
	{
		SAFE_DELETE(m_primitiveRenderMeshes[i]);
	}
}

void Renderer::onInitialize()
{
	// search paths
	m_resourceManager.addSearchDir("..\\..\\samples\\resources");
	m_resourceManager.addSearchDir("..\\..\\..\\samples\\resources");
	for (const std::string& d : getManager()->getConfig().additionalResourcesDir)
	{
		m_resourceManager.addSearchDir(d.c_str());
	}

	// debug primitive render material and input layout
	{
		m_debugPrimitiveRenderMaterial = new RenderMaterial(m_resourceManager, "debug_primitive", "");

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		m_debugPrimitiveRenderMaterialInstance = m_debugPrimitiveRenderMaterial->getMaterialInstance(layout, ARRAYSIZE(layout));
	}
}

void Renderer::onTerminate()
{
	SAFE_DELETE(m_debugPrimitiveRenderMaterial);
}

void Renderer::BackBufferResized(ID3D11Device* /*device*/, const DXGI_SURFACE_DESC* sd)
{
	// Setup the camera's projection parameters
	m_screenWidth = sd->Width;
	m_screenHeight = sd->Height;
	float fAspectRatio = m_screenWidth / m_screenHeight;
	m_camera.SetProjParams(DirectX::XM_PIDIV4, fAspectRatio, CAMERA_CLIP_NEAR, CAMERA_CLIP_FAR);

	SAFE_RELEASE(m_DSTexture);
	SAFE_RELEASE(m_DSView);
	SAFE_RELEASE(m_DSTextureSRV);

	// create a new Depth-Stencil texture 
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = sd->Width;
		desc.Height = sd->Height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32_TYPELESS; // Use a typeless type here so that it can be both depth-stencil and shader resource.
		desc.SampleDesc.Count = sd->SampleDesc.Count;
		desc.SampleDesc.Quality = sd->SampleDesc.Quality;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		V(m_device->CreateTexture2D(&desc, NULL, &m_DSTexture));
	}

	// create Depth-Stencil view
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.ViewDimension = sd->SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Format = DXGI_FORMAT_D32_FLOAT;	// Make the view see this as D32_FLOAT instead of typeless
		desc.Texture2D.MipSlice = 0;
		V(m_device->CreateDepthStencilView(m_DSTexture, &desc, &m_DSView));
	}

	// create Depth-Stencil shader resource view
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Format = DXGI_FORMAT_R32_FLOAT;	// Make the shaders see this as R32_FLOAT instead of typeless
		desc.ViewDimension = sd->SampleDesc.Count > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = 1;
		desc.Texture2D.MostDetailedMip = 0;
		V(m_device->CreateShaderResourceView(m_DSTexture, &desc, &m_DSTextureSRV));
	}

	// setup viewport
	m_viewport.Width = (FLOAT)sd->Width;
	m_viewport.Height = (FLOAT)sd->Height;
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;

	// setup shadows
	m_shadow.setScreenResolution(0, sd->Width, sd->Height, sd->SampleDesc.Count, nullptr);
}

void Renderer::setAllConstantBuffers(ID3D11DeviceContext* ctx)
{
	ID3D11Buffer* cbs[3] = { m_cameraCB, m_worldCB, m_objectCB };
	ctx->VSSetConstantBuffers(0, 3, cbs);
	ctx->GSSetConstantBuffers(0, 3, cbs);
	ctx->PSSetConstantBuffers(0, 3, cbs);
}

void Renderer::Render(ID3D11Device* /*device*/, ID3D11DeviceContext* ctx, ID3D11RenderTargetView* pRTV,
						  ID3D11DepthStencilView*)
{
	PROFILER_SCOPED_FUNCTION();

	m_context = ctx;

	ctx->ClearRenderTargetView(pRTV, CLEAR_SCENE_COLOR);
	ctx->ClearDepthStencilView(m_DSView, D3D11_CLEAR_DEPTH, 1.0, 0);
	ctx->RSSetViewports(1, &m_viewport);

	// needed matrices
	DirectX::XMMATRIX viewMatrix = m_camera.GetViewMatrix();
	DirectX::XMMATRIX projMatrix = m_camera.GetProjMatrix();
	DirectX::XMMATRIX projMatrixInv = DirectX::XMMatrixInverse(NULL, projMatrix);
	DirectX::XMMATRIX viewProjMatrix = viewMatrix * projMatrix;

	// Fill Camera constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ctx->Map(m_cameraCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		CBCamera* cameraBuffer = (CBCamera*)mappedResource.pData;
		cameraBuffer->viewProjection = viewProjMatrix;
		cameraBuffer->projectionInv = projMatrixInv;
		DirectX::XMStoreFloat3(&(cameraBuffer->viewPos), m_camera.GetEyePt());
		ctx->Unmap(m_cameraCB, 0);
	}

	// Fill World constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ctx->Map(m_worldCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		CBWorld* worldBuffer = (CBWorld*)mappedResource.pData;
		memcpy(worldBuffer, &m_worldCBData, sizeof(m_worldCBData));
		//worldBuffer->ambientColor = m_CBWorldData.ambientColor;
		//worldBuffer->pointLightPos = m_CBWorldData.pointLightPos;
		//worldBuffer->pointLightColor = m_CBWorldData.pointLightColor;
		//worldBuffer->dirLightDir = m_CBWorldData.dirLightDir;
		//worldBuffer->specularPower = m_CBWorldData.specularPower;
		//worldBuffer->dirLightColor = m_CBWorldData.dirLightColor;
		//worldBuffer->specularIntensity = m_CBWorldData.specularIntensity;
		ctx->Unmap(m_worldCB, 0);
	}

	ctx->RSSetState(m_RSState);
	ctx->PSSetSamplers(0, 1, &m_linearSampler);
	ctx->PSSetSamplers(1, 1, &m_pointSampler);


	if (m_shadowEnabled)
	{
		// render depth only
		{
			ctx->OMSetRenderTargets(0, nullptr, m_DSView);
			ctx->OMSetDepthStencilState(m_opaqueRenderDSState, 0xFF);

			// set constants buffers
			setAllConstantBuffers(ctx);

			for (auto it = m_renderables.begin(); it != m_renderables.end(); it++)
			{
				if (!(*it)->isTransparent() && !(*it)->isHidden())
					(*it)->renderDepthStencilOnly(*this);
			}
		}

		// render shadow map
		m_shadow.renderShadowMaps(this);

		// render shadow buffer
		ctx->OMSetRenderTargets(0, nullptr, nullptr);
		m_shadow.renderShadowBuffer(m_DSTextureSRV, nullptr);
	}

	// Opaque render
	{
		ctx->RSSetViewports(1, &m_viewport);
		ctx->RSSetState(m_RSState);
		ctx->OMSetRenderTargets(1, &pRTV, m_DSView);
		ctx->OMSetDepthStencilState(m_opaqueRenderDSState, 0xFF);

		// set constants buffers
		setAllConstantBuffers(ctx);

		// Fill Camera constant buffer
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			ctx->Map(m_cameraCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			CBCamera* cameraBuffer = (CBCamera*)mappedResource.pData;
			cameraBuffer->viewProjection = viewProjMatrix;
			cameraBuffer->projectionInv = projMatrixInv;
			DirectX::XMStoreFloat3(&(cameraBuffer->viewPos), m_camera.GetEyePt());
			ctx->Unmap(m_cameraCB, 0);
		}

		// Render opaque renderables
		m_visibleOpaqueRenderablesCount = 0;
		for (auto it = m_renderables.begin(); it != m_renderables.end(); it++)
		{
			if (!(*it)->isTransparent() && !(*it)->isHidden())
			{
				(*it)->render(*this);
				m_visibleOpaqueRenderablesCount++;
			}
		}
	}

	// modulate shadows
	if (m_shadowEnabled)
	{
		m_shadow.modulateShadowBuffer(pRTV);
	}

	// render AO
	if (m_HBAOEnabled)
	{
		m_HBAO.renderAO(m_context, pRTV, m_DSTextureSRV, projMatrix);
	}

	ctx->RSSetViewports(1, &m_viewport);

	// render debug render buffers
	while (m_queuedRenderBuffers.size() > 0)
	{
		render(m_queuedRenderBuffers.back());
		m_queuedRenderBuffers.pop_back();
	}

	// Transparency render
	ctx->OMSetRenderTargets(1, &pRTV, m_DSView);
	ctx->OMSetDepthStencilState(m_transparencyRenderDSState, 0xFF);

	// depth as SRV isn't used now (uncommenting will produce a warning, probably need readonly depth?)
	//ctx->PSSetShaderResources(1, 1, &mDSTextureSRV);

	// Render transparent renderables
	m_visibleTransparentRenderablesCount = 0;
	for (auto it = m_renderables.begin(); it != m_renderables.end(); it++)
	{
		if ((*it)->isTransparent() && !(*it)->isHidden())
		{
			(*it)->render(*this);
			m_visibleTransparentRenderablesCount++;
		}
	}

	// shadows debug render
	if (0)
	{
		m_shadow.displayMapFrustums(pRTV, m_DSView);
	}

	// Reset RT and SRV state
	ID3D11ShaderResourceView* nullAttach[16] = { nullptr };
	ctx->PSSetShaderResources(0, 16, nullAttach);
	ctx->OMSetRenderTargets(0, nullptr, nullptr);
}

LRESULT Renderer::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PX_UNUSED(hWnd);
	PX_UNUSED(wParam);
	PX_UNUSED(lParam);

	if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP)
	{
		// Camera overspeed event
		int iKeyPressed = static_cast<int>(wParam);
		if (iKeyPressed == VK_SHIFT)
		{
			toggleCameraSpeed(uMsg == WM_KEYDOWN);
		}
	}

	// Camera events
	return m_camera.HandleMessages(hWnd, uMsg, wParam, lParam);
}

void Renderer::Animate(double fElapsedTimeSeconds)
{
	PROFILER_SCOPED_FUNCTION();

	m_camera.FrameMove((float)fElapsedTimeSeconds);
}

void Renderer::renderDepthOnly(DirectX::XMMATRIX* viewProjectionSubstitute)
{
	// Fill Camera constant buffer
	if (viewProjectionSubstitute)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		m_context->Map(m_cameraCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		CBCamera* cameraBuffer = (CBCamera*)mappedResource.pData;
		cameraBuffer->viewProjection = *viewProjectionSubstitute;
		m_context->Unmap(m_cameraCB, 0);
	}

	// set constants buffers
	setAllConstantBuffers(m_context);

	// render
	for (auto it = m_renderables.begin(); it != m_renderables.end(); it++)
	{
		if (!(*it)->isTransparent() && !(*it)->isHidden())
			(*it)->renderDepthStencilOnly(*this);
	}
}

void Renderer::render(const PxRenderBuffer* renderBuffer)
{
	// points 
	uint32_t pointsCount = renderBuffer->getNbPoints();
	if (pointsCount > 0)
	{
		RenderDebugVertex* verts = new RenderDebugVertex[pointsCount];
		const PxDebugPoint* points = renderBuffer->getPoints();
		for (uint32_t i = 0; i < pointsCount; i++)
		{
			verts[i].mPos = points[i].pos;
			verts[i].mColor = points[i].color;
		}

		renderDebugPrimitive(verts, pointsCount, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		delete[] verts;
	}

	// lines 
	uint32_t linesCount = renderBuffer->getNbLines();
	if (linesCount > 0)
	{
		RenderDebugVertex* verts = new RenderDebugVertex[linesCount * 2];
		const PxDebugLine* lines = renderBuffer->getLines();
		for (uint32_t i = 0; i < linesCount; i++)
		{
			verts[i * 2].mPos = lines[i].pos0;
			verts[i * 2].mColor = lines[i].color0;
			verts[i * 2 + 1].mPos = lines[i].pos1;
			verts[i * 2 + 1].mColor = lines[i].color1;
		}

		renderDebugPrimitive(verts, linesCount * 2, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		delete[] verts;
	}

	// triangles 
	uint32_t trianglesCount = renderBuffer->getNbTriangles();
	if (trianglesCount > 0)
	{
		RenderDebugVertex* verts = new RenderDebugVertex[trianglesCount * 3];
		const PxDebugTriangle* triangles = renderBuffer->getTriangles();
		for (uint32_t i = 0; i < trianglesCount; i++)
		{
			verts[i * 3].mPos = triangles[i].pos0;
			verts[i * 3].mColor = triangles[i].color0;
			verts[i * 3 + 1].mPos = triangles[i].pos1;
			verts[i * 3 + 1].mColor = triangles[i].color1;
			verts[i * 3 + 2].mPos = triangles[i].pos2;
			verts[i * 3 + 2].mColor = triangles[i].color2;
		}

		renderDebugPrimitive(verts, trianglesCount * 3, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		delete[] verts;
	}

	// texts?
	// ....
}

void Renderer::renderDebugPrimitive(const Renderer::RenderDebugVertex *vertices, uint32_t verticesCount, D3D11_PRIMITIVE_TOPOLOGY topology)
{
	m_context->IASetPrimitiveTopology(topology);

	m_debugPrimitiveRenderMaterialInstance->bind(*m_context, 0);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	m_context->Map(m_objectCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	CBObject* objectBuffer = (CBObject*)mappedResource.pData;

	objectBuffer->world = PxMat44ToXMMATRIX(PxMat44(PxIdentity));

	m_context->Unmap(m_objectCB, 0);

	if (m_debugPrimitiveVBVerticesCount < verticesCount)
	{
		m_debugPrimitiveVBVerticesCount = verticesCount;
		SAFE_RELEASE(m_debugPrimitiveVB);

		D3D11_BUFFER_DESC bufferDesc;

		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = sizeof(Renderer::RenderDebugVertex) * m_debugPrimitiveVBVerticesCount;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		V(m_device->CreateBuffer(&bufferDesc, NULL, &m_debugPrimitiveVB));
	}

	CD3D11_BOX box(0, 0, 0, (LONG)(sizeof(Renderer::RenderDebugVertex) * verticesCount), 1, 1);
	m_context->UpdateSubresource(m_debugPrimitiveVB, 0, &box, vertices, 0, 0);

	ID3D11Buffer* pBuffers[1] = { m_debugPrimitiveVB };
	UINT strides[1] = { sizeof(RenderDebugVertex) };
	UINT offsets[1] = { 0 };
	m_context->IASetVertexBuffers(0, 1, pBuffers, strides, offsets);

	m_context->Draw(verticesCount, 0);
}

IRenderMesh* Renderer::getPrimitiveRenderMesh(PrimitiveRenderMeshType::Enum type)
{
	if (m_primitiveRenderMeshes[type] == NULL)
	{
		switch (type)
		{
		case PrimitiveRenderMeshType::Box:
			m_primitiveRenderMeshes[type] = new BoxRenderMesh();
			break;
		case PrimitiveRenderMeshType::Plane:
			m_primitiveRenderMeshes[type] = new PlaneRenderMesh();
			break;
		case PrimitiveRenderMeshType::Sphere:
			m_primitiveRenderMeshes[type] = new SphereRenderMesh();
			break;
		default:
			PX_ALWAYS_ASSERT_MESSAGE("Unsupported PxGeometryType");
			return NULL;
		}
	}

	return m_primitiveRenderMeshes[type];
}


Renderable* Renderer::createRenderable(IRenderMesh& mesh, RenderMaterial& material)
{
	Renderable* renderable = new Renderable(mesh, material);
	m_renderables.emplace(renderable);
	return renderable;
}

void Renderer::removeRenderable(Renderable* r)
{
	m_renderables.erase(m_renderables.find(r));
	delete r;
}

void Renderer::toggleCameraSpeed(bool overspeed)
{
	m_camera.SetScalers(0.002f, overspeed ? 150.f : 25.f);
}

void Renderer::reloadShaders()
{
	// iterate Renderables materials and call reload()
	std::set<RenderMaterial*> materials;
	for (auto it = m_renderables.begin(); it != m_renderables.end(); it++)
	{
		materials.emplace(&((*it)->getMaterial()));
	}
	for (std::set<RenderMaterial*>::iterator it = materials.begin(); it != materials.end(); it++)
	{
		(*it)->reload();
	}
}

void Renderer::drawUI()
{
	// Lighting
	if (ImGui::TreeNode("Lighting"))
	{
		ImGui::ColorEdit3("Ambient Color", &(m_worldCBData.ambientColor.x));
		ImGui::ColorEdit3("Point Light Color", &(m_worldCBData.pointLightColor.x));
		ImGui::DragFloat3("Point Light Pos", &(m_worldCBData.pointLightPos.x));
		ImGui::ColorEdit3("Dir Light Color", &(m_worldCBData.dirLightColor.x));
		ImGui_DragFloat3Dir("Dir Light Dir", &(m_worldCBData.dirLightDir.x));
		ImGui::DragFloat("Specular Power", &(m_worldCBData.specularPower), 1.0f, 1.0f, 500.0f);
		ImGui::DragFloat("Specular Intensity", &(m_worldCBData.specularIntensity), 0.01f, 0.0f, 2.0f);

		ImGui::TreePop();
	}

	// Shadow
	if (ImGui::TreeNode("Shadow"))
	{
		ImGui::Checkbox("Shadows Enabled", &m_shadowEnabled);
		if (m_shadowEnabled)
		{
			m_shadow.drawUI();
		}

		ImGui::TreePop();
	}

	// HBAO+
	if (ImGui::TreeNode("HBAO+"))
	{
		ImGui::Checkbox("HBAO Enabled", &(m_HBAOEnabled));
		if (m_HBAOEnabled)
		{
			m_HBAO.drawUI();
		}

		ImGui::TreePop();
	}
}