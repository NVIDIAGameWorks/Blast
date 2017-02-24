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

#pragma once

#include <d3d11.h>
#include <d3d12.h>

#include "D3D12RenderTarget.h"

class D3D12RenderShader
{
public:
	D3D12RenderShader();
	~D3D12RenderShader();

	static D3D12RenderShader* Create(
		const char *name, 
		void* pVSBlob, size_t vsBlobSize, 
		void* pPSBlob, size_t psBlobSize, 
		UINT  cbufferSize0 = 0,	UINT  cbufferSize1 = 0,
		D3D12_INPUT_ELEMENT_DESC* pElemDesc = 0, UINT numElements = 0,
		int ShaderResourceNum = 0, int UnorderedAccessNum = 0, int SamplerNum = 0);

	void MakeCurrent();
	void Disable();
	void SetConstantBuffer();

	void* MapParam(UINT slot = 0);
	void  UnmapParam(UINT slot = 0);

	void BindShaderResource(UINT slot, CD3DX12_CPU_DESCRIPTOR_HANDLE& handle);

protected:
	bool CreateParamBuffer(UINT sizeBuffer, UINT slot = 0);

private:
	ID3D12PipelineState* GetPipelineState(RenderShaderState* pShaderState);

	void* m_vertexShader;
	SIZE_T m_vertexShaderSize;
	void* m_pixelShader;
	SIZE_T m_pixelShaderSize;

	D3D12_INPUT_ELEMENT_DESC* m_inputElementDescs;
	UINT m_inputElementDescsNum;

	ID3D12RootSignature* m_pRootSignature;

	ID3D12DescriptorHeap* m_scuHeap;
	int m_scuDescriptorSize;

	ID3D12Resource* m_ConstantBuffer[2];

	ID3D12DescriptorHeap* m_samplerHeap;
	int m_samplerDescriptorSize;

	map<RenderShaderState*, ID3D12PipelineState*> m_PipelineStates;
};

