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
#include "RenderPlugin.h"

class CORERENDER_EXPORT D3D11RenderShader
{
public:
	D3D11RenderShader();
	~D3D11RenderShader();

	static D3D11RenderShader* Create(
		const char *name, 
		void* pVSBlob, size_t vsBlobSize, 
		void* pPSBlob, size_t psBlobSize, 
		UINT  cbufferSize0 = 0,	UINT  cbufferSize1 = 0,
		D3D11_INPUT_ELEMENT_DESC* pElemDesc = 0, UINT numElements = 0);

	void MakeCurrent();
	void Disable();
	void SetConstantBuffer();

	void* MapParam(UINT slot = 0);
	void  UnmapParam(UINT slot = 0);

	ID3D11VertexShader*		getVertexShader() { return m_pVertexShader; }
	ID3D11PixelShader*		getPixelShader() { return m_pPixelShader; }
	ID3D11InputLayout*		getInputLayout() { return m_pInputLayout; }
	ID3D11Buffer*			getParamBuffer(UINT slot = 0) { return m_pParamBuffers[slot]; }

protected:
	bool CreateVSFromBlob(void* pBlob, size_t blobSize, 
	D3D11_INPUT_ELEMENT_DESC *desc, int elemCount);
	bool CreatePSFromBlob(void* pBlob, size_t blobSize);
	bool CreateParamBuffer(UINT sizeBuffer, UINT slot = 0);

private:

	ID3D11VertexShader*		m_pVertexShader;
	ID3D11PixelShader*		m_pPixelShader;
	ID3D11InputLayout*		m_pInputLayout;
	ID3D11Buffer*			m_pParamBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT];
};

