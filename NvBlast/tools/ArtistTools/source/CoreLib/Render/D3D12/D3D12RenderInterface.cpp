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
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
#include "D3D12RenderInterface.h"

#include "D3D12TextureResource.h"
#include "D3D12Shaders.h"
#include "D3D12Buffer.h"

#include "D3D12RenderContext.h"

namespace RenderInterfaceD3D12
{
	using namespace RenderInterface;

	D3D12_BLEND_DESC			m_BlendStates[BLEND_STATE_END];
	D3D12_DEPTH_STENCIL_DESC	m_DepthStencilStates[DEPTH_STENCIL_STATE_END];
	D3D12_RASTERIZER_DESC		m_RasterizerStates[RASTERIZER_STATE_END];

	D3D12RenderContext *pRenderContext = D3D12RenderContext::Instance();

	SHADER_TYPE m_ShaderType;
	RenderShaderStateKey m_ShaderStateKey;

	map<RenderShaderStateKey, RenderShaderState*> m_RenderShaderStates;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
bool InitDevice(int deviceID)
{
	pRenderContext->InitDevice();
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool Initialize()
{
	if (!pRenderContext)
		return false;

	InitializeShadersD3D12();
	InitializeRenderStates();
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
void Shutdown()
{
	DestroyShadersD3D12();
}

RenderShaderState* GetShaderState()
{
	RenderShaderState* pShaderState = nullptr;
	map<RenderShaderStateKey, RenderShaderState*>::iterator it = m_RenderShaderStates.find(m_ShaderStateKey);
	if (it != m_RenderShaderStates.end())
	{
		pShaderState = it->second;
	}
	else
	{
		pShaderState = new RenderShaderState;
		pShaderState->BlendState = m_BlendStates[m_ShaderStateKey.BlendState];
		pShaderState->DepthStencilState = m_DepthStencilStates[m_ShaderStateKey.DepthStencilState];
		pShaderState->RasterizerState = m_RasterizerStates[m_ShaderStateKey.RasterizerState];
		pShaderState->PrimitiveTopologyType = m_ShaderStateKey.PrimitiveTopologyType;
		if (m_ShaderStateKey.ForShadow)
		{
			pShaderState->RTVFormat = DXGI_FORMAT_R32_FLOAT;
			pShaderState->DSVFormat = DXGI_FORMAT_D32_FLOAT;
		}
		else
		{
			pShaderState->RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			pShaderState->DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			pShaderState->SampleCount = D3D12RenderContext::Instance()->GetSampleCount();
		}

		m_RenderShaderStates[m_ShaderStateKey] = pShaderState;
	}
	return pShaderState;
}
//////////////////////////////////////////////////////////////////////////////////////
void InitializeRenderStates()
{
	/////////////////////////////////////////////////////////////////////////////////////////
	// alpha blending state descriptors
	/////////////////////////////////////////////////////////////////////////////////////////

	// alpha blending enabled
	{
		D3D12_BLEND_DESC desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;

		D3D12_RENDER_TARGET_BLEND_DESC &rtDesc = desc.RenderTarget[0];
		{
			rtDesc.BlendEnable = true;
			rtDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rtDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			rtDesc.BlendOp = D3D12_BLEND_OP_ADD;
			rtDesc.SrcBlendAlpha = D3D12_BLEND_ZERO;
			rtDesc.DestBlendAlpha = D3D12_BLEND_ONE;
			rtDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			rtDesc.RenderTargetWriteMask = 0x0f;
		}
		m_BlendStates[BLEND_STATE_ALPHA] = desc;
	}

	// no alpha blending
	{
		D3D12_BLEND_DESC desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;

		D3D12_RENDER_TARGET_BLEND_DESC &rtDesc = desc.RenderTarget[0];
		{
			rtDesc.BlendEnable = false;
			rtDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rtDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			rtDesc.BlendOp = D3D12_BLEND_OP_ADD;
			rtDesc.SrcBlendAlpha = D3D12_BLEND_ZERO;
			rtDesc.DestBlendAlpha = D3D12_BLEND_ONE;
			rtDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			rtDesc.RenderTargetWriteMask = 0x0f;
		}
		m_BlendStates[BLEND_STATE_NONE] = desc;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// depth and stencil
	///////////////////////////////////////////////////////////////////////////////////////////////
	D3D12_DEPTH_STENCIL_DESC depthTestDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	{
		depthTestDesc.DepthEnable = true;
		depthTestDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthTestDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthTestDesc.StencilEnable = false;
		depthTestDesc.StencilReadMask = 0xff;
		depthTestDesc.StencilWriteMask = 0xff;
	}

	m_DepthStencilStates[DEPTH_STENCIL_DEPTH_TEST] = depthTestDesc;

	D3D12_DEPTH_STENCIL_DESC depthNone = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	{
		depthNone.DepthEnable = false;
		depthNone.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthNone.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthNone.StencilEnable = false;
		depthNone.StencilReadMask = 0xff;
		depthNone.StencilWriteMask = 0xff;
	}

	m_DepthStencilStates[DEPTH_STENCIL_DEPTH_NONE] = depthNone;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// rasterizer
	///////////////////////////////////////////////////////////////////////////////////////////////
	D3D12_RASTERIZER_DESC rsDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	// solid cull front
	{
		rsDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rsDesc.CullMode = D3D12_CULL_MODE_FRONT;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 10;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ForcedSampleCount = 0;

		m_RasterizerStates[RASTERIZER_STATE_FILL_CULL_FRONT] = rsDesc;
	};

	// solid cull back
	{
		rsDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rsDesc.CullMode = D3D12_CULL_MODE_BACK;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 10;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ForcedSampleCount = 0;

		m_RasterizerStates[RASTERIZER_STATE_FILL_CULL_BACK] = rsDesc;
	}

	// solid cull none
	{
		rsDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rsDesc.CullMode = D3D12_CULL_MODE_NONE;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 10;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ForcedSampleCount = 0;

		m_RasterizerStates[RASTERIZER_STATE_FILL_CULL_NONE] = rsDesc;
	}

	// wireframe cull none
	{
		rsDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		rsDesc.CullMode = D3D12_CULL_MODE_NONE;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.FrontCounterClockwise = 0;
		rsDesc.DepthBias = 0;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ForcedSampleCount = 0;
	};

	m_RasterizerStates[RASTERIZER_STATE_WIRE] = rsDesc;
}

//////////////////////////////////////////////////////////////////////////////////////
void SetPrimitiveTopologyTriangleStrip()
{
	ID3D12GraphicsCommandList* pCommandList = pRenderContext->GetGraphicsCommandList();
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

//////////////////////////////////////////////////////////////////////////////////////
void SetPrimitiveTopologyTriangleList()
{
	ID3D12GraphicsCommandList* pCommandList = pRenderContext->GetGraphicsCommandList();
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//////////////////////////////////////////////////////////////////////////////////////
void SetPrimitiveTopologyLineList()
{
	ID3D12GraphicsCommandList* pCommandList = pRenderContext->GetGraphicsCommandList();
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
}

//////////////////////////////////////////////////////////////////////////////////////
void SetVertexBuffer(GPUBufferResource* pBuffer, UINT stride, UINT offset)
{
	D3D12_VERTEX_BUFFER_VIEW* vertexView = GPUBufferD3D12::GetVertexView(pBuffer);
	if (!vertexView)
		return;

	vertexView->StrideInBytes = stride;
	ID3D12GraphicsCommandList* m_commandList = pRenderContext->GetGraphicsCommandList();
	m_commandList->IASetVertexBuffers(0, 1, vertexView);
}

///////////////////////////////////////////////////////////////////////////////
void ApplyDepthStencilState(DEPTH_STENCIL_STATE state)
{
	m_ShaderStateKey.DepthStencilState = state;
}

///////////////////////////////////////////////////////////////////////////////
void ApplyRasterizerState(RASTERIZER_STATE state)
{
	m_ShaderStateKey.RasterizerState = state;
}

///////////////////////////////////////////////////////////////////////////////
void ApplyBlendState(BLEND_STATE state)
{
	m_ShaderStateKey.BlendState = state;
}

void ApplyPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType)
{
	m_ShaderStateKey.PrimitiveTopologyType = PrimitiveTopologyType;
}

void ApplyForShadow(int ForShadow)
{
	m_ShaderStateKey.ForShadow = ForShadow;
}

void SwitchToDX11()
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	pContext->SwitchToDX11();
}

void FlushDX11()
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	pContext->FlushDX11();
}

void FlushDX12()
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	pContext->Flush();
}

void SubmitGpuWork()
{
	D3D12RenderContext::Instance()->SubmitGpuWork();
}

void WaitForGpu()
{
	D3D12RenderContext* context = D3D12RenderContext::Instance();
	context->WaitForGpu();
	context->UpdateGpuWorkCompleted();
}

/////////////////////////////////////////////////////////////////////////////////////////
void SetViewport(const RenderInterface::Viewport& vp)
{
	ID3D12GraphicsCommandList* pCommandList = pRenderContext->GetGraphicsCommandList();

	D3D12_VIEWPORT d3dViewport;

	d3dViewport.TopLeftX = vp.TopLeftX;
	d3dViewport.TopLeftY = vp.TopLeftY;

	d3dViewport.Width = vp.Width;
	d3dViewport.Height = vp.Height;

	d3dViewport.MinDepth = vp.MinDepth;
	d3dViewport.MaxDepth = vp.MaxDepth;

	pRenderContext->SetViewport(d3dViewport);
}

/////////////////////////////////////////////////////////////////////////////////////////
void GetViewport(RenderInterface::Viewport& vp)
{
	D3D12_VIEWPORT d3dViewport;
	pRenderContext->GetViewport(d3dViewport);

	vp.TopLeftX = d3dViewport.TopLeftX;
	vp.TopLeftY = d3dViewport.TopLeftY;

	vp.Width = d3dViewport.Width;
	vp.Height = d3dViewport.Height;

	vp.MinDepth = d3dViewport.MinDepth;
	vp.MaxDepth = d3dViewport.MaxDepth;
}

///////////////////////////////////////////////////////////////////////////////
void Draw(unsigned int vertexCount, unsigned int startCount)
{
	ID3D12GraphicsCommandList* pCommandList = pRenderContext->GetGraphicsCommandList();
	pCommandList->DrawInstanced(vertexCount, 1, startCount, 0);
}

///////////////////////////////////////////////////////////////////////////////
GPUBufferResource* CreateVertexBuffer( 
	unsigned int ByteWidth, void* pSysMem)
{
	ID3D12Device* pDevice = D3D12RenderContext::Instance()->GetDevice();
	if (!pDevice)
		return false;

	ID3D12Resource* pBuffer = nullptr;
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(ByteWidth),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pBuffer)));
	if (nullptr == pBuffer)
	{
		return false;
	}

	void* pData;
	ThrowIfFailed(pBuffer->Map(0, nullptr, &pData));
	memcpy(pData, pSysMem, ByteWidth);
	pBuffer->Unmap(0, nullptr);

	return GPUBufferD3D12::Create(pBuffer, ByteWidth);
}

///////////////////////////////////////////////////////////////////////////////
GPUShaderResource* CreateShaderResource(unsigned int stride, 
	unsigned int numElements, void* pSysMem, NVHairReadOnlyBuffer* pReadOnlyBuffer)
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	if (!pContext)
		return false;

	ID3D12Device* pDevice = pContext->GetDevice();
	ID3D12GraphicsCommandList* pCommandList = pContext->GetGraphicsCommandList();

	pContext->NVHairINT_CreateD3D12ReadOnlyBuffer(stride, numElements, pReadOnlyBuffer, pSysMem);

	int nIndexInHeap = -1;
	return D3D12TextureResource::Create(pReadOnlyBuffer->m_pBuffer.Get(), pReadOnlyBuffer->getSrvCpuHandle(), nIndexInHeap);
}

///////////////////////////////////////////////////////////////////////////////
// create read only shader resource buffer
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void CopyToDevice(
	GPUBufferResource *pGPUBuffer, 	void* pSysMem, 	unsigned int	ByteWidth)
{
	ID3D12Resource* pBuffer = GPUBufferD3D12::GetResource(pGPUBuffer);
	if (!pBuffer)
		return;

	void* pData;
	ThrowIfFailed(pBuffer->Map(0, nullptr, &pData));
	memcpy(pData, pSysMem, ByteWidth);
	pBuffer->Unmap(0, nullptr);

	/*
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	if (!pContext)
		return;

	ID3D12Device* pDevice = pContext->GetDevice();
	ID3D12GraphicsCommandList* pCommandList = pContext->GetGraphicsCommandList();

	D3D12_SUBRESOURCE_DATA data = {};
	data.pData = reinterpret_cast<UINT8*>(pSysMem);
	data.RowPitch = ByteWidth;
	data.SlicePitch = data.RowPitch;

	ID3D12Resource* m_pBufferUpload = GPUBufferD3D12::GetResourceUpload(pGPUBuffer, pDevice, ByteWidth);

	UpdateSubresources<1>(pCommandList, pBuffer, m_pBufferUpload, 0, 0, 1, &data);
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	*/
}
} // end namespace
