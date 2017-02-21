#include "RenderPluginDx11.h"

#include "D3D11RenderInterface.h"
#include "D3D11Util.h"
#include "D3D11Shaders.h"
#include "D3D11GPUProfiler.h"
#include "D3D11ShadowMap.h"

RenderPlugin* CreateRenderPlugin(void)
{
	return new RenderPluginDx11;
}

RenderPluginDx11::RenderPluginDx11()
{
	m_RenderApi = "Dx11";
}

RenderPluginDx11::~RenderPluginDx11()
{
}

// interface
bool RenderPluginDx11::InitDevice(int deviceID)
{
	return RenderInterfaceD3D11::InitDevice(deviceID);
}

bool RenderPluginDx11::Initialize()
{
	return RenderInterfaceD3D11::Initialize();
}

void RenderPluginDx11::Shutdown()
{
	D3D11Util::DestroyRenderWindow();
	RenderInterfaceD3D11::Shutdown();
}

void RenderPluginDx11::CopyToDevice(GPUBufferResource *pDevicePtr,
	void* pSysMem, unsigned int	ByteWidth)
{
	RenderInterfaceD3D11::CopyToDevice(pDevicePtr, pSysMem, ByteWidth);
}

void RenderPluginDx11::ApplyDepthStencilState(DEPTH_STENCIL_STATE state)
{
	RenderInterfaceD3D11::ApplyDepthStencilState(state);
}

void RenderPluginDx11::ApplyRasterizerState(RASTERIZER_STATE state)
{
	RenderInterfaceD3D11::ApplyRasterizerState(state);
}

void RenderPluginDx11::ApplySampler(int slot, SAMPLER_TYPE state)
{
	RenderInterfaceD3D11::ApplySampler(slot, state);
}

void RenderPluginDx11::ApplyBlendState(BLEND_STATE state)
{
	RenderInterfaceD3D11::ApplyBlendState(state);
}

void RenderPluginDx11::GetViewport(Viewport& vp)
{
	RenderInterfaceD3D11::GetViewport(vp);
}

void RenderPluginDx11::SetViewport(const Viewport& vp)
{
	RenderInterfaceD3D11::SetViewport(vp);
}

void RenderPluginDx11::BindVertexShaderResources(int startSlot,
	int numSRVs, GPUShaderResource** ppSRVs)
{
	RenderInterfaceD3D11::BindVertexShaderResources(startSlot, numSRVs, ppSRVs);
}

void RenderPluginDx11::BindPixelShaderResources(int startSlot,
	int numSRVs, GPUShaderResource** ppSRVs)
{
	RenderInterfaceD3D11::BindPixelShaderResources(startSlot, numSRVs, ppSRVs);
}

void RenderPluginDx11::ClearVertexShaderResources(int startSlot, int numSRVs)
{
	RenderInterfaceD3D11::ClearVertexShaderResources(startSlot, numSRVs);
}

void RenderPluginDx11::ClearPixelShaderResources(int startSlot, int numSRVs)
{
	RenderInterfaceD3D11::ClearPixelShaderResources(startSlot, numSRVs);
}

void RenderPluginDx11::ClearInputLayout()
{
	RenderInterfaceD3D11::ClearInputLayout();
}

void RenderPluginDx11::SetVertexBuffer(GPUBufferResource* pBuffer, UINT stride, UINT offset)
{
	RenderInterfaceD3D11::SetVertexBuffer(pBuffer, stride, offset);
}

void RenderPluginDx11::SetPrimitiveTopologyTriangleStrip()
{
	RenderInterfaceD3D11::SetPrimitiveTopologyTriangleStrip();
}

void RenderPluginDx11::SetPrimitiveTopologyTriangleList()
{
	RenderInterfaceD3D11::SetPrimitiveTopologyTriangleList();
}

void RenderPluginDx11::SetPrimitiveTopologyLineList()
{
	RenderInterfaceD3D11::SetPrimitiveTopologyLineList();
}

void RenderPluginDx11::Draw(unsigned int vertexCount, unsigned int startCount)
{
	RenderInterfaceD3D11::Draw(vertexCount, startCount);
}

GPUBufferResource* RenderPluginDx11::CreateVertexBuffer(unsigned int ByteWidth, void* pSysMem)
{
	return RenderInterfaceD3D11::CreateVertexBuffer(ByteWidth, pSysMem);
}

GPUShaderResource* RenderPluginDx11::CreateShaderResource(unsigned int stride,
	unsigned int numElements, void* pSysMem, NVHairReadOnlyBuffer* pReadOnlyBuffer)
{
	return RenderInterfaceD3D11::CreateShaderResource(stride, numElements, pSysMem);
}

void RenderPluginDx11::ApplyForShadow(int ForShadow)
{
}

void RenderPluginDx11::SwitchToDX11()
{
}

void RenderPluginDx11::FlushDX11()
{
}

void RenderPluginDx11::FlushDX12()
{
}

void RenderPluginDx11::ApplyPrimitiveTopologyLine()
{
}

void RenderPluginDx11::ApplyPrimitiveTopologyTriangle()
{
}

void RenderPluginDx11::SubmitGpuWork()
{
}

void RenderPluginDx11::WaitForGpu()
{
}

// util
bool RenderPluginDx11::CreateRenderWindow(HWND hWnd, int nSamples)
{
	return D3D11Util::CreateRenderWindow(hWnd, nSamples);
}

bool RenderPluginDx11::ResizeRenderWindow(int w, int h)
{
	return D3D11Util::ResizeRenderWindow(w, h);
}

void RenderPluginDx11::PresentRenderWindow()
{
	D3D11Util::PresentRenderWindow();
}

void RenderPluginDx11::ClearRenderWindow(float r, float g, float b)
{
	D3D11Util::ClearRenderWindow(r, g, b);
}

bool RenderPluginDx11::GetDeviceInfoString(wchar_t *str)
{
	return D3D11Util::GetDeviceInfoString(str);
}

GPUShaderResource* RenderPluginDx11::CreateTextureSRV(const char* texturename)
{
	return D3D11Util::CreateTextureSRV(texturename);
}

void RenderPluginDx11::TxtHelperBegin()
{
	D3D11Util::TxtHelperBegin();
}

void RenderPluginDx11::TxtHelperEnd()
{
	D3D11Util::TxtHelperEnd();
}

void RenderPluginDx11::TxtHelperSetInsertionPos(int x, int y)
{
	D3D11Util::TxtHelperSetInsertionPos(x, y);
}

void RenderPluginDx11::TxtHelperSetForegroundColor(float r, float g, float b, float a)
{
	D3D11Util::TxtHelperSetForegroundColor(r, g, b, a);
}

void RenderPluginDx11::TxtHelperDrawTextLine(wchar_t* str)
{
	D3D11Util::TxtHelperDrawTextLine(str);
}

// shader
bool RenderPluginDx11::InitializeShaders()
{
	return InitializeShadersD3D11();
}

void RenderPluginDx11::DestroyShaders()
{
	DestroyShadersD3D11();
}

void RenderPluginDx11::ApplyShader(SHADER_TYPE st)
{
	ApplyShaderD3D11(st);
}

void RenderPluginDx11::DisableShader(SHADER_TYPE st)
{
	DisableShaderD3D11(st);
}

void RenderPluginDx11::BindShaderResources(SHADER_TYPE st, int numSRVs, GPUShaderResource** ppSRVs)
{
}

void RenderPluginDx11::CopyShaderParam(SHADER_TYPE st,
	void* pSysMem, unsigned int bytes, unsigned int slot)
{
	CopyShaderParamD3D11(st, pSysMem, bytes, slot);
}

// GPUProfiler
GPUProfiler* RenderPluginDx11::CreateGPUProfiler()
{
	GPUProfiler* pProfiler = new D3D11GPUProfiler;
	pProfiler->Initialize();
	return pProfiler;
}

// ShadowMap
ShadowMap* RenderPluginDx11::CreateShadowMap(int resolution)
{
	return new D3D11ShadowMap(resolution);
}

// D3D12RenderContext
void RenderPluginDx11::PreRender()
{
}

void RenderPluginDx11::PostRender()
{
}

// GPUMeshResources

#include "MeshData.h"
#include "AnimUtil.h"

GPUMeshResources* RenderPluginDx11::GPUMeshResourcesCreate(MeshData* pMeshData, const SkinData& skinData)
{
	GPUMeshResources* resources = new GPUMeshResources;

	int numIndices = pMeshData->m_NumIndices;
	int numVertices = pMeshData->m_NumVertices;

	resources->m_pVertexBuffer = CreateVertexBuffer(
		sizeof(MeshData::MeshVertex) * numIndices, pMeshData->m_pMeshVertices);

	resources->m_pBoneIndicesSRV = CreateShaderResource(
		sizeof(atcore_float4), numVertices, skinData.m_pBoneIndices);

	resources->m_pBoneWeightsSRV = CreateShaderResource(
		sizeof(atcore_float4), numVertices, skinData.m_pBoneWeights);

	return resources;
}

void RenderPluginDx11::GPUMeshResourcesRelease(GPUMeshResources* pResource)
{
	SAFE_RELEASE(pResource->m_pVertexBuffer);
	SAFE_RELEASE(pResource->m_pBoneIndicesSRV);
	SAFE_RELEASE(pResource->m_pBoneWeightsSRV);
}

D3DHandles& RenderPluginDx11::GetDeviceHandles(D3DHandles& deviceHandles)
{
	return D3D11Util::GetDeviceHandles(deviceHandles);
}
