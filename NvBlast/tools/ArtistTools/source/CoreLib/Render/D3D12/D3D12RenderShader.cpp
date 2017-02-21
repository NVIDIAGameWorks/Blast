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

#include "D3D12RenderShader.h"

#include "D3D12RenderInterface.h"
#include "D3D12Wrapper.h"
#include "D3D12RenderContext.h"
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) { if (x) x->Release(); }
#endif

///////////////////////////////////////////////////////////////////////////////
D3D12RenderShader::D3D12RenderShader()
{
	m_vertexShader = 0;
	m_vertexShaderSize = 0;
	m_pixelShader = 0;
	m_pixelShaderSize = 0;

	m_inputElementDescs = 0;
	m_inputElementDescsNum = 0;

	m_pRootSignature = 0;

	m_scuHeap = 0;
	m_scuDescriptorSize = 0;

	m_samplerHeap = 0;
	m_samplerDescriptorSize = 0;

	m_PipelineStates.clear();

	for (int i = 0; i < 2; i++)
	{
		m_ConstantBuffer[i] = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
D3D12RenderShader::~D3D12RenderShader()
{
	for (int i = 0; i < 1; i++)
	{
		SAFE_RELEASE(m_ConstantBuffer[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////
D3D12RenderShader* D3D12RenderShader::Create(
	const char *name, 
	void* vertexShader, size_t vertexShaderSize, void* pixelShader, size_t pixelShaderSize,
	UINT  cbufferSize0, UINT  cbufferSize1,
	D3D12_INPUT_ELEMENT_DESC* pElemDesc, UINT numElements,
	int ShaderResourceNum, int UnorderedAccessNum, int SamplerNum)
{
	D3D12RenderContext* pAdapter = D3D12RenderContext::Instance();
	ID3D12Device* m_device = pAdapter->GetDevice();

	D3D12RenderShader* pShader = new D3D12RenderShader;

	int sizeofuint = sizeof(UINT8);
	pShader->m_vertexShaderSize = vertexShaderSize;
	if (vertexShaderSize > 0)
	{
		pShader->m_vertexShader = new UINT8[vertexShaderSize / sizeofuint];
		memcpy(pShader->m_vertexShader, vertexShader, vertexShaderSize);
	}
	pShader->m_pixelShaderSize = pixelShaderSize;
	if (pixelShaderSize > 0)
	{
		pShader->m_pixelShader = new UINT8[pixelShaderSize / sizeofuint];
		memcpy(pShader->m_pixelShader, pixelShader, pixelShaderSize);
	}

	pShader->m_inputElementDescsNum = numElements;
	pShader->m_inputElementDescs = pElemDesc;

	CD3DX12_DESCRIPTOR_RANGE ranges[3];
	CD3DX12_ROOT_PARAMETER rootParameters[2];

	int rangesindex = 0;
	int rootParametersindex = 0;

	if (ShaderResourceNum > 0)
	{
		ranges[rangesindex++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, ShaderResourceNum, 0);
	}
	int ConstantBufferNum = 0;
	if (cbufferSize0 > 0)
	{
		ConstantBufferNum++;
	}
	if (cbufferSize1 > 0)
	{
		ConstantBufferNum++;
	}
	if (ConstantBufferNum > 0)
	{
		ranges[rangesindex++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, ConstantBufferNum, 0);
	}
	if (UnorderedAccessNum > 0)
	{
		ranges[rangesindex++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UnorderedAccessNum, 0);
	}
	if (rangesindex > 0)
	{
		rootParameters[rootParametersindex++].InitAsDescriptorTable(rangesindex, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	}

	if (SamplerNum > 0)
	{
		ranges[rangesindex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, SamplerNum, 0);
		rootParameters[rootParametersindex++].InitAsDescriptorTable(1, &ranges[rangesindex], D3D12_SHADER_VISIBILITY_ALL);
	}

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(rootParametersindex, rootParameters, 0, nullptr, rootSignatureFlags);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pShader->m_pRootSignature)));

	int numSCU = ShaderResourceNum + ConstantBufferNum + UnorderedAccessNum;
	if (numSCU > 0)
	{
		D3D12_DESCRIPTOR_HEAP_DESC scuHeapDesc = {};
		scuHeapDesc.NumDescriptors = numSCU;
		scuHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		scuHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&scuHeapDesc, IID_PPV_ARGS(&pShader->m_scuHeap)));
		pShader->m_scuDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (ConstantBufferNum > 0)
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE scuHandle(pShader->m_scuHeap->GetCPUDescriptorHandleForHeapStart(),
				ShaderResourceNum, pShader->m_scuDescriptorSize);

			if (cbufferSize0 > 0)
			{
				pShader->CreateParamBuffer(cbufferSize0, 0);

				// Describe and create a constant buffer view.
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				// CB size is required to be 256-byte aligned.
				cbvDesc.SizeInBytes = (cbufferSize0 + 255) & ~255;
				cbvDesc.BufferLocation = pShader->m_ConstantBuffer[0]->GetGPUVirtualAddress();
				m_device->CreateConstantBufferView(&cbvDesc, scuHandle);
				scuHandle.Offset(pShader->m_scuDescriptorSize);
			}
			if (cbufferSize1 > 0)
			{
				pShader->CreateParamBuffer(cbufferSize1, 1);

				// Describe and create a constant buffer view.
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				// CB size is required to be 256-byte aligned.
				cbvDesc.SizeInBytes = (cbufferSize1 + 255) & ~255;
				cbvDesc.BufferLocation = pShader->m_ConstantBuffer[1]->GetGPUVirtualAddress();
				m_device->CreateConstantBufferView(&cbvDesc, scuHandle);
			}
		}
	}

	if (SamplerNum > 0)
	{
		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
		samplerHeapDesc.NumDescriptors = SamplerNum;
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&pShader->m_samplerHeap)));
		pShader->m_samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(pShader->m_samplerHeap->GetCPUDescriptorHandleForHeapStart());

		if(SamplerNum == 1)
		{ 
			D3D12_SAMPLER_DESC pointClampSamplerDesc[1] = {
				D3D12_FILTER_MIN_MAG_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				0.0, 0, D3D12_COMPARISON_FUNC_NEVER,
				0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, D3D12_FLOAT32_MAX,
			};
			m_device->CreateSampler(pointClampSamplerDesc, samplerHandle);
		}
		else if(SamplerNum == 2)
		{
			D3D12_SAMPLER_DESC linearSamplerDesc[1] = {
				D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				0.0, 0, D3D12_COMPARISON_FUNC_NEVER, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, D3D12_FLOAT32_MAX,
			};
			m_device->CreateSampler(linearSamplerDesc, samplerHandle);
			samplerHandle.Offset(pShader->m_samplerDescriptorSize);

			D3D12_SAMPLER_DESC pointClampSamplerDesc[1] = {
				D3D12_FILTER_MIN_MAG_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				0.0, 0, D3D12_COMPARISON_FUNC_NEVER,
				0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, D3D12_FLOAT32_MAX,
			};
			m_device->CreateSampler(pointClampSamplerDesc, samplerHandle);
		}
	}

	return pShader;
}

///////////////////////////////////////////////////////////////////////////////
void D3D12RenderShader::SetConstantBuffer()
{
	return;
}

ID3D12PipelineState* D3D12RenderShader::GetPipelineState(RenderShaderState* pShaderState)
{
	ID3D12PipelineState* pPipelineState = nullptr;
	map<RenderShaderState*, ID3D12PipelineState*>::iterator it = m_PipelineStates.find(pShaderState);
	if (it == m_PipelineStates.end())
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { m_inputElementDescs, m_inputElementDescsNum };
		psoDesc.pRootSignature = m_pRootSignature;
		psoDesc.VS = { reinterpret_cast<UINT8*>(m_vertexShader), m_vertexShaderSize };
		psoDesc.PS = { reinterpret_cast<UINT8*>(m_pixelShader), m_pixelShaderSize };
		psoDesc.BlendState = pShaderState->BlendState;
		psoDesc.RasterizerState = pShaderState->RasterizerState;
		psoDesc.DepthStencilState = pShaderState->DepthStencilState;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = pShaderState->PrimitiveTopologyType;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = pShaderState->RTVFormat;
		psoDesc.DSVFormat = pShaderState->DSVFormat;
		psoDesc.SampleDesc.Count = pShaderState->SampleCount;

		ID3D12Device* pDevice = D3D12RenderContext::Instance()->GetDevice();
		ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPipelineState)));

		m_PipelineStates[pShaderState] = pPipelineState;
	}
	else
	{
		pPipelineState = it->second;
	}
	return pPipelineState;
}

///////////////////////////////////////////////////////////////////////////////
void D3D12RenderShader::MakeCurrent()
{
	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	ID3D12GraphicsCommandList* m_commandList = pContext->GetGraphicsCommandList();

	RenderShaderState* pShaderState = RenderInterfaceD3D12::GetShaderState();
	if (nullptr == pShaderState)
	{
		return;
	}

	ID3D12PipelineState* pPipelineState = GetPipelineState(pShaderState);

	m_commandList->SetGraphicsRootSignature(m_pRootSignature);
	m_commandList->SetPipelineState(pPipelineState);

	vector<ID3D12DescriptorHeap*> heaps;
	if (nullptr != m_scuHeap)
	{
		heaps.push_back(m_scuHeap);
	}
	if (nullptr != m_samplerHeap)
	{
		heaps.push_back(m_samplerHeap);
	}
	if (heaps.size() > 0)
	{
		m_commandList->SetDescriptorHeaps(heaps.size(), heaps.data());
	}
	int heapindex = 0;
	if (nullptr != m_scuHeap)
	{
		m_commandList->SetGraphicsRootDescriptorTable(heapindex++, m_scuHeap->GetGPUDescriptorHandleForHeapStart());
	}
	if (nullptr != m_samplerHeap)
	{
		m_commandList->SetGraphicsRootDescriptorTable(heapindex++, m_samplerHeap->GetGPUDescriptorHandleForHeapStart());
	}
}

///////////////////////////////////////////////////////////////////////////////
void D3D12RenderShader::Disable()
{
}

///////////////////////////////////////////////////////////////////////////////
bool D3D12RenderShader::CreateParamBuffer( UINT sizeBuffer, UINT slot )
{
	ID3D12Device* pDevice = D3D12RenderContext::Instance()->GetDevice();
	if (!pDevice)
		return false;

	SAFE_RELEASE(m_ConstantBuffer[slot]);

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeBuffer),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_ConstantBuffer[slot])));
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
void* D3D12RenderShader::MapParam(UINT slot)
{
	if (!m_ConstantBuffer[slot])
		return 0;

	void* pData;
	ThrowIfFailed(m_ConstantBuffer[slot]->Map(0, nullptr, &pData));
	return pData;
}

///////////////////////////////////////////////////////////////////////////////
void D3D12RenderShader::UnmapParam( UINT slot )
{
	if (!m_ConstantBuffer[slot])
		return;

	m_ConstantBuffer[slot]->Unmap(0, nullptr);
}

void D3D12RenderShader::BindShaderResource(UINT slot, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
{
	ID3D12Device* pDevice = D3D12RenderContext::Instance()->GetDevice();
	if (!pDevice)
		return;

	CD3DX12_CPU_DESCRIPTOR_HANDLE destHandle(m_scuHeap->GetCPUDescriptorHandleForHeapStart(), slot, m_scuDescriptorSize);
	if (handle.ptr != 0)
	{
		pDevice->CopyDescriptorsSimple(1, destHandle, handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}