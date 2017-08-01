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
#include "D3D11RenderInterface.h"

#include "D3D11TextureResource.h"
#include "D3D11Shaders.h"
#include "D3D11Buffer.h"
#include "Nv.h"
#ifdef USE_11ON12_WRAPPER
#include "D3D11on12Wrapper.h"
#endif // USE_11ON12_WRAPPER
namespace RenderInterfaceD3D11
{
	using namespace RenderInterface;

	ID3D11SamplerState*			m_pSamplerStates[SAMPLER_TYPE_END];
	ID3D11BlendState*			m_pBlendStates[BLEND_STATE_END];
	ID3D11DepthStencilState*	m_pDepthStencilStates[DEPTH_STENCIL_STATE_END];
	ID3D11RasterizerState*		m_pRasterizerStates[RASTERIZER_STATE_END];

#ifdef USE_11ON12_WRAPPER
#else
	ID3D11DeviceContext*		g_d3dDeviceContext = 0;
	ID3D11Device*				g_d3dDevice = 0;
	IDXGIFactory1*				g_pDXGIFactory1 = 0;
	IDXGIDevice*				g_dxgiDevice = 0;
	IDXGIAdapter *				g_pAdapter = 0;
#endif // USE_11ON12_WRAPPER


/////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11DeviceContext* GetDeviceContext()
{
#ifdef USE_11ON12_WRAPPER
	return D3D11on12Wrapper::GetDeviceContext();
#else
	return g_d3dDeviceContext;
#endif // USE_11ON12_WRAPPER
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11Device* GetDevice()
{
#ifdef USE_11ON12_WRAPPER
	return D3D11on12Wrapper::GetDevice11();
#else
	return g_d3dDevice;
#endif // USE_11ON12_WRAPPER
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
IDXGIFactory1* GetDXGIFactory()
{
#ifdef USE_11ON12_WRAPPER
	return D3D11on12Wrapper::GetDXGIFactory();
#else
	return g_pDXGIFactory1;
#endif // USE_11ON12_WRAPPER
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
IDXGIDevice* GetDXGIDevice()
{
#ifdef USE_11ON12_WRAPPER
	return nullptr;
#else
	return g_dxgiDevice;
#endif // USE_11ON12_WRAPPER
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
IDXGIAdapter* GetAdapter()
{
#ifdef USE_11ON12_WRAPPER
	return D3D11on12Wrapper::GetAdapter();
#else
	return g_pAdapter;
#endif // USE_11ON12_WRAPPER
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
bool IsNvDeviceID(UINT id)
{
	return id == 0x10DE;
}

IDXGIAdapter* FindAdapter(IDXGIFactory* IDXGIFactory_0001, const WCHAR* targetName, bool& isNv)
{
	IDXGIAdapter* targetAdapter = nullptr;
	std::vector<IDXGIAdapter*> adapters;
	// check current adapter first. EnumAdapters could fail on some device 
	IDXGIAdapter* pAdapter = nullptr;
	ID3D11Device* pD3dDevice = nullptr;
	ID3D11DeviceContext* pD3dDeviceContext = nullptr;
	DWORD createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL fl;
	// This following code is the robust way to get all possible feature levels while handling DirectX 11.0 systems:
	// please read https://blogs.msdn.microsoft.com/chuckw/2014/02/05/anatomy-of-direct3d-11-create-device/
	D3D_FEATURE_LEVEL lvl[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1 };
	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		createDeviceFlags, lvl, _countof(lvl),
		D3D11_SDK_VERSION, &pD3dDevice, &fl, &pD3dDeviceContext);
	if (pD3dDevice)
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = pD3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			hr = dxgiDevice->GetAdapter(&pAdapter);
			if (pAdapter)
			{
				adapters.push_back(pAdapter);
			}
			SAFE_RELEASE(dxgiDevice);
		}
		SAFE_RELEASE(pD3dDeviceContext);
		SAFE_RELEASE(pD3dDevice);
	}

	// Enum Adapters
	unsigned int adapterNo = 0;
	HRESULT hres = S_OK;
	while (SUCCEEDED(hres = IDXGIFactory_0001->EnumAdapters(adapterNo, (IDXGIAdapter**)&pAdapter)))
	{
		adapters.push_back(pAdapter);
		adapterNo++;
	}
	if (wcslen(targetName) != 0)
	{
		// find the adapter with specified name
		for (int i = 0; i < adapters.size(); ++i)
		{
			IDXGIAdapter* pAdapter = adapters[i];
			DXGI_ADAPTER_DESC aDesc;
			pAdapter->GetDesc(&aDesc);
			std::wstring aName = aDesc.Description;
			if (aName.find(targetName) != std::string::npos)
			{
				targetAdapter = pAdapter;
				isNv = IsNvDeviceID(aDesc.VendorId);
			}
		}
	}
	else
	{
		// no name specified, find one NV adapter
		for (int i = 0; i < adapters.size(); ++i)
		{
			IDXGIAdapter* pAdapter = adapters[i];
			DXGI_ADAPTER_DESC aDesc;
			pAdapter->GetDesc(&aDesc);
			std::wstring aName = aDesc.Description;
			if (IsNvDeviceID(aDesc.VendorId))
			{
				targetAdapter = pAdapter;
				isNv = true;
			}
		}
	}
	if (targetAdapter == nullptr)
		targetAdapter = adapters[0];
	for (int i = 0; i < adapters.size(); ++i)
	{
		IDXGIAdapter* pAdapter = adapters[i];
		if(pAdapter != targetAdapter)
		{
			pAdapter->Release();
		}
	}

	return targetAdapter;
}

HRESULT UseGoodGPUDevice()
{
#ifdef USE_11ON12_WRAPPER
	return S_OK;
#else
	// create factory
	if (g_pDXGIFactory1 == NV_NULL)
	{
		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&g_pDXGIFactory1));
		if (FAILED(hr))
			return hr;
	}

	DWORD createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED; // 0;  // I changed only this line.
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	bool bNVAdapter = false;
	WCHAR* pwName = L"";
	g_pAdapter = FindAdapter(g_pDXGIFactory1, pwName, bNVAdapter);

#ifdef _DEBUG
	DXGI_ADAPTER_DESC adapterDesc;
	g_pAdapter->GetDesc(&adapterDesc);
	std::wstring adapterName = adapterDesc.Description;
#endif

	D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_9_1;
	HRESULT hr = 0;

	hr = D3D11CreateDevice(g_pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
		createDeviceFlags, 0, 0,
		D3D11_SDK_VERSION, &g_d3dDevice, &fl, &g_d3dDeviceContext);

	if (g_d3dDevice == nullptr)
	{
		// here is the codes to make it run on a WARP device(Windows DirectX CPU - based emulation).
		if (g_pAdapter)
		{
			g_pAdapter->Release();
			g_pAdapter = nullptr;
		}
		hr = D3D11CreateDevice(g_pAdapter, D3D_DRIVER_TYPE_WARP, nullptr,
			createDeviceFlags, 0, 0,
			D3D11_SDK_VERSION, &g_d3dDevice, &fl, &g_d3dDeviceContext);
	}

	if(g_d3dDevice)
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = g_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			g_dxgiDevice = dxgiDevice;
			if (g_pAdapter == nullptr)
			{
				// when running on WARP device, need find out adapter.
				hr = dxgiDevice->GetAdapter(&g_pAdapter);
			}
			return hr;
		}
		else
		{
			SAFE_RELEASE(g_d3dDevice);
			SAFE_RELEASE(g_pAdapter);
			SAFE_RELEASE(g_pDXGIFactory1);
		}
	}
	return hr;
#endif // USE_11ON12_WRAPPER
}

bool InitDevice(int deviceID)
{
#ifdef USE_11ON12_WRAPPER
	D3D11on12Wrapper::InitDevice();
#else
	if (deviceID == -1)
	{
		HRESULT hResult = UseGoodGPUDevice();
		if (FAILED(hResult))
			return false;

		return true;
	}

	D3D_FEATURE_LEVEL featureLvl;

	// create factory
	if (g_pDXGIFactory1 == NV_NULL)
	{
		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&g_pDXGIFactory1));
		if (FAILED(hr))
			return false;
	}

	// create adapter for selected device
	if (g_pAdapter == NV_NULL)
	{
		HRESULT hr = g_pDXGIFactory1->EnumAdapters(deviceID, &g_pAdapter);
		if (FAILED(hr))
			return false;
	}

	UINT deviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	
	// create device
	HRESULT hResult = D3D11CreateDevice(
		g_pAdapter,
		D3D_DRIVER_TYPE_UNKNOWN, //D3D_DRIVER_TYPE_HARDWARE, 
		0, 
		deviceFlags, 
		NV_NULL,
		0, 
		D3D11_SDK_VERSION, 
		&g_d3dDevice, 
		&featureLvl, 
		&g_d3dDeviceContext);
	
	if(FAILED(hResult)) 
		return false;
#endif // USE_11ON12_WRAPPER

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool Initialize()
{
	if (!GetDevice() || !GetDeviceContext())
		return false;

	D3D11_COMPARISON_FUNC depthFunc = D3D11_COMPARISON_LESS;

	InitializeShadersD3D11();
	InitializeRenderStates();
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
void Shutdown()
{
	DestroyShadersD3D11();
	ClearRenderStates();

	// release d3d resources
	IDXGIFactory1* pDxgiFactory = GetDXGIFactory();
	SAFE_RELEASE(pDxgiFactory);

	IDXGIAdapter* pAdapter = GetAdapter();
	SAFE_RELEASE(pAdapter);

	IDXGIDevice* pDXGIDevice = GetDXGIDevice();
	SAFE_RELEASE(pDXGIDevice);

	ID3D11DeviceContext* pContext = GetDeviceContext();
	SAFE_RELEASE(pContext);

	ID3D11Device* pDevice = GetDevice();
#if defined(DEBUG) || defined(_DEBUG)
	// output d3d leak
	ID3D11Debug *d3dDebug;
	HRESULT hr = pDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug));
	if (SUCCEEDED(hr))
	{
		hr = d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}
	if (d3dDebug != nullptr)
		d3dDebug->Release();
#endif

	SAFE_RELEASE(pDevice);
}

//////////////////////////////////////////////////////////////////////////////////////
void InitializeRenderStates()
{
	ID3D11Device* pDevice = GetDevice();
	if (!pDevice)
		return;

	/////////////////////////////////////////////////////////////////////////////////////////
	// alpha blending state descriptors
	/////////////////////////////////////////////////////////////////////////////////////////

	// alpha blending enabled
	{
		D3D11_BLEND_DESC desc;
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;

		D3D11_RENDER_TARGET_BLEND_DESC &rtDesc = desc.RenderTarget[0];
		{
			rtDesc.BlendEnable		= true;
			rtDesc.SrcBlend			= D3D11_BLEND_SRC_ALPHA;
			rtDesc.DestBlend		= D3D11_BLEND_INV_SRC_ALPHA;
			rtDesc.BlendOp			= D3D11_BLEND_OP_ADD;
			rtDesc.SrcBlendAlpha	= D3D11_BLEND_ZERO;
			rtDesc.DestBlendAlpha	= D3D11_BLEND_ONE;
			rtDesc.BlendOpAlpha		= D3D11_BLEND_OP_ADD;
			rtDesc.RenderTargetWriteMask = 0x0f;
		}
		pDevice->CreateBlendState(&desc, &m_pBlendStates[BLEND_STATE_ALPHA]);
	}

	// no alpha blending
	{
		D3D11_BLEND_DESC desc;
		desc.AlphaToCoverageEnable = false;
		desc.IndependentBlendEnable = false;

		D3D11_RENDER_TARGET_BLEND_DESC &rtDesc = desc.RenderTarget[0];
		{
			rtDesc.BlendEnable		= false;
			rtDesc.SrcBlend			= D3D11_BLEND_SRC_ALPHA;
			rtDesc.DestBlend		= D3D11_BLEND_INV_SRC_ALPHA;
			rtDesc.BlendOp			= D3D11_BLEND_OP_ADD;
			rtDesc.SrcBlendAlpha	= D3D11_BLEND_ZERO;
			rtDesc.DestBlendAlpha	= D3D11_BLEND_ONE;
			rtDesc.BlendOpAlpha		= D3D11_BLEND_OP_ADD;
			rtDesc.RenderTargetWriteMask = 0x0f;
		}
		pDevice->CreateBlendState(&desc, &m_pBlendStates[BLEND_STATE_NONE]);
	}
	

	//////////////////////////////////////////////////////////////////////////////////////////////
	// depth and stencil
	///////////////////////////////////////////////////////////////////////////////////////////////
	D3D11_DEPTH_STENCIL_DESC depthTestDesc;
	{
		depthTestDesc.DepthEnable	= true;
		depthTestDesc.DepthFunc		= D3D11_COMPARISON_LESS;
		depthTestDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthTestDesc.StencilEnable = false;
		depthTestDesc.StencilReadMask = 0xff;
		depthTestDesc.StencilWriteMask = 0xff;
	}

	pDevice->CreateDepthStencilState(&depthTestDesc, &m_pDepthStencilStates[DEPTH_STENCIL_DEPTH_TEST]);

	D3D11_DEPTH_STENCIL_DESC depthNone;
	{
		depthNone.DepthEnable	= false;
		depthNone.DepthFunc		= D3D11_COMPARISON_LESS;
		depthNone.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthNone.StencilEnable = false;
		depthNone.StencilReadMask = 0xff;
		depthNone.StencilWriteMask = 0xff;
	}

	pDevice->CreateDepthStencilState(&depthNone, &m_pDepthStencilStates[DEPTH_STENCIL_DEPTH_NONE]);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// rasterizer
	///////////////////////////////////////////////////////////////////////////////////////////////
	D3D11_RASTERIZER_DESC rsDesc;

	// solid cull front
	{
		rsDesc.FillMode	= D3D11_FILL_SOLID;
		rsDesc.CullMode	= D3D11_CULL_FRONT;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 10;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ScissorEnable = 0;

		pDevice->CreateRasterizerState(&rsDesc, &m_pRasterizerStates[RASTERIZER_STATE_FILL_CULL_FRONT]);
	};

	// solid cull back
	{
		rsDesc.FillMode	= D3D11_FILL_SOLID;
		rsDesc.CullMode	= D3D11_CULL_BACK;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 10;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ScissorEnable = 0;

		pDevice->CreateRasterizerState(&rsDesc, &m_pRasterizerStates[RASTERIZER_STATE_FILL_CULL_BACK]);
	}

	// solid cull none
	{
		rsDesc.FillMode	= D3D11_FILL_SOLID;
		rsDesc.CullMode	= D3D11_CULL_NONE;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.FrontCounterClockwise = true;
		rsDesc.DepthBias = 10;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ScissorEnable = 0;

		pDevice->CreateRasterizerState(&rsDesc, &m_pRasterizerStates[RASTERIZER_STATE_FILL_CULL_NONE]);

	}

	// wireframe cull none
	{
		rsDesc.FillMode	= D3D11_FILL_WIREFRAME;
		rsDesc.CullMode	= D3D11_CULL_NONE;
		rsDesc.AntialiasedLineEnable = false;
		rsDesc.MultisampleEnable = true;
		rsDesc.FrontCounterClockwise = 0;
		rsDesc.DepthBias = 0;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0;
		rsDesc.DepthClipEnable = true;
		rsDesc.ScissorEnable = 0;
	};

	pDevice->CreateRasterizerState(&rsDesc, &m_pRasterizerStates[RASTERIZER_STATE_WIRE]);

	// samplers

	D3D11_SAMPLER_DESC linearSamplerDesc[1] = { 
		D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, 
		D3D11_TEXTURE_ADDRESS_WRAP, 
		D3D11_TEXTURE_ADDRESS_WRAP, 
		D3D11_TEXTURE_ADDRESS_WRAP, 
		0.0, 0, D3D11_COMPARISON_NEVER, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, D3D11_FLOAT32_MAX,
	};
	pDevice->CreateSamplerState(linearSamplerDesc, &m_pSamplerStates[SAMPLER_TYPE_LINEAR]);

	// create point clamp sampler for PCF sampling for hair
	D3D11_SAMPLER_DESC pointClampSamplerDesc[1] = { 
		D3D11_FILTER_MIN_MAG_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_CLAMP, 
		D3D11_TEXTURE_ADDRESS_CLAMP, 
		D3D11_TEXTURE_ADDRESS_CLAMP, 
		0.0, 0, D3D11_COMPARISON_NEVER,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, D3D11_FLOAT32_MAX,
	};
	pDevice->CreateSamplerState(pointClampSamplerDesc, &m_pSamplerStates[SAMPLER_TYPE_POINTCLAMP]);
}

//////////////////////////////////////////////////////////////////////////////////////
void ClearRenderStates()
{
	for (int i = 0; i < RASTERIZER_STATE_END; i++)
		SAFE_RELEASE(m_pRasterizerStates[i]);

	for (int i = 0; i < DEPTH_STENCIL_STATE_END; i++)
		SAFE_RELEASE(m_pDepthStencilStates[i]);

	for (int i = 0; i < SAMPLER_TYPE_END; i++)
		SAFE_RELEASE(m_pSamplerStates[i]);

	for (int i = 0; i < BLEND_STATE_END; i++)
		SAFE_RELEASE(m_pBlendStates[i]);
}

//////////////////////////////////////////////////////////////////////////////////////
void BindVertexShaderResources( int startSlot, int numSRVs, ID3D11ShaderResourceView** ppSRVs)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	pDeviceContext->VSSetShaderResources( startSlot, numSRVs, ppSRVs);
}

//////////////////////////////////////////////////////////////////////////////////////
void BindPixelShaderResources( int startSlot, int numSRVs, ID3D11ShaderResourceView** ppSRVs)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	pDeviceContext->PSSetShaderResources( startSlot, numSRVs, ppSRVs);
}

//////////////////////////////////////////////////////////////////////////////////////
void BindVertexShaderResources( int startSlot, int numSRVs, GPUShaderResource** ppSRVs)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	for (int i = startSlot; i < (startSlot + numSRVs); i++)
	{
		ID3D11ShaderResourceView* pSRV = D3D11TextureResource::GetResource(ppSRVs[i]);
		pDeviceContext->VSSetShaderResources( i, 1, &pSRV);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void BindPixelShaderResources( int startSlot, int numSRVs, GPUShaderResource** ppSRVs)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	for (int i = startSlot; i < (startSlot + numSRVs); i++)
	{
		ID3D11ShaderResourceView* pSRV = D3D11TextureResource::GetResource(ppSRVs[i]);
		pDeviceContext->PSSetShaderResources( i, 1, &pSRV);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void ClearPixelShaderResources( int startSlot, int numSRVs)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	// clean up
	ID3D11ShaderResourceView* ppSRVNull[128];
	memset(&ppSRVNull, 0, sizeof(ID3D11ShaderResourceView*)*128);

	pDeviceContext->PSSetShaderResources( startSlot, numSRVs, ppSRVNull);
}

//////////////////////////////////////////////////////////////////////////////////////
void ClearVertexShaderResources( int startSlot, int numSRVs)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	// clean up
	ID3D11ShaderResourceView* ppSRVNull[128];
	memset(&ppSRVNull, 0, sizeof(ID3D11ShaderResourceView*)*128);

	pDeviceContext->VSSetShaderResources( startSlot, numSRVs, ppSRVNull);
}

//////////////////////////////////////////////////////////////////////////////////////
void ClearInputLayout()
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	pDeviceContext->IASetInputLayout(nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////
void SetPrimitiveTopologyTriangleStrip()
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	 pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
}

//////////////////////////////////////////////////////////////////////////////////////
void SetPrimitiveTopologyTriangleList()
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	 pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
}

//////////////////////////////////////////////////////////////////////////////////////
void SetPrimitiveTopologyLineList()
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	 pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
}

//////////////////////////////////////////////////////////////////////////////////////
void SetVertexBuffer(GPUBufferResource* pBuffer, UINT stride, UINT offset)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	ID3D11Buffer* pD3D11Buffer = GPUBufferD3D11::GetResource(pBuffer);
	if (!pD3D11Buffer)
		return;

	pDeviceContext->IASetVertexBuffers( 0, 1, &pD3D11Buffer, &stride, &offset );
}

//////////////////////////////////////////////////////////////////////////////////////
void ApplySampler(int slot, RenderInterface::SAMPLER_TYPE st)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	pDeviceContext->PSSetSamplers(slot, 1, &m_pSamplerStates[st] );
}

///////////////////////////////////////////////////////////////////////////////
void ApplyDepthStencilState(DEPTH_STENCIL_STATE state)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	pDeviceContext->OMSetDepthStencilState(m_pDepthStencilStates[state], 0);
}

///////////////////////////////////////////////////////////////////////////////
void ApplyRasterizerState(RASTERIZER_STATE state)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	pDeviceContext->RSSetState(m_pRasterizerStates[state]);
}

///////////////////////////////////////////////////////////////////////////////
void ApplyBlendState(BLEND_STATE st)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	float zerov[] = {0,0,0,0};

	pDeviceContext->OMSetBlendState(m_pBlendStates[st], zerov, 0xffffffff);
}

/////////////////////////////////////////////////////////////////////////////////////////
void SetViewport(const RenderInterface::Viewport& vp)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	D3D11_VIEWPORT d3dViewport;

	d3dViewport.TopLeftX = vp.TopLeftX;
	d3dViewport.TopLeftY = vp.TopLeftY;

	d3dViewport.Width = vp.Width;
	d3dViewport.Height = vp.Height;

	d3dViewport.MinDepth = vp.MinDepth;
	d3dViewport.MaxDepth = vp.MaxDepth;

	pDeviceContext->RSSetViewports(1, &d3dViewport);
}

/////////////////////////////////////////////////////////////////////////////////////////
void GetViewport(RenderInterface::Viewport& vp)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	UINT numViewports = 1;
	D3D11_VIEWPORT d3dViewport;
	pDeviceContext->RSGetViewports(&numViewports, &d3dViewport);

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
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	pDeviceContext->Draw(vertexCount, startCount);
}

///////////////////////////////////////////////////////////////////////////////
// gpu buffer management
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11Buffer* CreateD3D11Buffer( 
	D3D11_USAGE Usage, UINT ByteWidth, UINT StructureByteStride, 
	UINT BindFlags, UINT MiscFlags, 
	UINT CPUAccessFlags, void *pSysMem)
{
	ID3D11Device* pDevice = RenderInterfaceD3D11::GetDevice();
	if (!pDevice)
		return 0;

	D3D11_BUFFER_DESC desc;

	desc.Usage					= Usage;
	desc.ByteWidth				= ByteWidth;
	desc.StructureByteStride	= StructureByteStride;
	desc.BindFlags				= BindFlags;
	desc.MiscFlags				= MiscFlags;
	desc.CPUAccessFlags			= CPUAccessFlags;

	D3D11_SUBRESOURCE_DATA InitData;
	
	InitData.pSysMem = pSysMem;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	ID3D11Buffer* pBuffer = 0;

	HRESULT hr = pSysMem ? pDevice->CreateBuffer( &desc, &InitData, &pBuffer)
		: pDevice->CreateBuffer( &desc, 0, &pBuffer);

	if( FAILED(hr) )
		return 0;

	return pBuffer;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11ShaderResourceView* 
CreateD3D11ShaderResourceView( 
	ID3D11Buffer*		pBuffer, 
	DXGI_FORMAT			Format, 
	D3D11_SRV_DIMENSION ViewDimension, 
	UINT				NumElements,
	UINT				FirstElement = 0
	)
{
	ID3D11Device* pDevice = RenderInterfaceD3D11::GetDevice();
	if (!pDevice)
		return 0;

	ID3D11ShaderResourceView* pSRV = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	{
		desc.Format                    = Format;
		desc.ViewDimension             = ViewDimension;
		desc.Buffer.FirstElement	   = FirstElement; 
		desc.Buffer.NumElements        = NumElements; 
	}

	pDevice->CreateShaderResourceView( pBuffer, &desc, &pSRV);

	return pSRV;
}

///////////////////////////////////////////////////////////////////////////////
GPUBufferResource* CreateVertexBuffer( 
	unsigned int ByteWidth, void* pSysMem)
{
	ID3D11Buffer* pD3D11Buffer = CreateD3D11Buffer(D3D11_USAGE_DEFAULT,
		ByteWidth, 0, D3D11_BIND_VERTEX_BUFFER, 0, 0, pSysMem);

	return GPUBufferD3D11::Create(pD3D11Buffer);
}

///////////////////////////////////////////////////////////////////////////////
GPUShaderResource* CreateShaderResource( unsigned int stride, unsigned int numElements, void* pSysMem)
{
	unsigned int byteWidth = numElements * stride;

	ID3D11Buffer* pBuffer = CreateD3D11Buffer( 
		D3D11_USAGE_DEFAULT, byteWidth, 0,
		D3D11_BIND_SHADER_RESOURCE, 0, 0, pSysMem);

	// create SRV for bone indices
	ID3D11ShaderResourceView* pSRV =  CreateD3D11ShaderResourceView(
		pBuffer,
		DXGI_FORMAT_R32G32B32A32_FLOAT, 
		D3D11_SRV_DIMENSION_BUFFER, 
		numElements);

	SAFE_RELEASE(pBuffer); // dec ref count as this is not explicitly used later

	return D3D11TextureResource::Create(pSRV);
}

///////////////////////////////////////////////////////////////////////////////
// create read only shader resource buffer
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void CopyToDevice(
	GPUBufferResource *pDevicePtr, 	void* pSysMem, 	unsigned int	ByteWidth)
{
	ID3D11Buffer* pBuffer = GPUBufferD3D11::GetResource(pDevicePtr);
	if (!pBuffer)
		return;

	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	if (!pDeviceContext)
		return;

	pDeviceContext->UpdateSubresource(pBuffer, 0, NV_NULL, pSysMem, ByteWidth, 0);
}

} // end namespace
