#include "RenderPluginDx12.h"

#include "D3D12RenderInterface.h"
#include "D3D12Util.h"
#include "D3D12Shaders.h"
#include "D3D12GPUProfiler.h"
#include "D3D12ShadowMap.h"
#include "D3D12RenderContext.h"

RenderPlugin* CreateRenderPlugin(void)
{
	return new RenderPluginDx12;
}

RenderPluginDx12::RenderPluginDx12()
{
	m_RenderApi = "Dx12";
}

RenderPluginDx12::~RenderPluginDx12()
{
}

// interface
bool RenderPluginDx12::InitDevice(int deviceID) 
{ 
	return RenderInterfaceD3D12::InitDevice(deviceID);
}

bool RenderPluginDx12::Initialize() 
{ 
	return RenderInterfaceD3D12::Initialize();
}

void RenderPluginDx12::Shutdown() 
{
	D3D12Util::DestroyRenderWindow();
	RenderInterfaceD3D12::Shutdown();
}

void RenderPluginDx12::CopyToDevice(GPUBufferResource *pDevicePtr, 
	void* pSysMem, unsigned int	ByteWidth) 
{
	RenderInterfaceD3D12::CopyToDevice(pDevicePtr, pSysMem, ByteWidth);
}

void RenderPluginDx12::ApplyDepthStencilState(DEPTH_STENCIL_STATE state) 
{
	RenderInterfaceD3D12::ApplyDepthStencilState(state);
}

void RenderPluginDx12::ApplyRasterizerState(RASTERIZER_STATE state) 
{
	RenderInterfaceD3D12::ApplyRasterizerState(state);
}

void RenderPluginDx12::ApplySampler(int slot, SAMPLER_TYPE state)
{
}

void RenderPluginDx12::ApplyBlendState(BLEND_STATE state)
{
	RenderInterfaceD3D12::ApplyBlendState(state);
}

void RenderPluginDx12::GetViewport(Viewport& vp)
{
	RenderInterfaceD3D12::GetViewport(vp);
}

void RenderPluginDx12::SetViewport(const Viewport& vp)
{
	RenderInterfaceD3D12::SetViewport(vp);
}

void RenderPluginDx12::BindVertexShaderResources(int startSlot, 
	int numSRVs, GPUShaderResource** ppSRVs) 
{
}

void RenderPluginDx12::BindPixelShaderResources(int startSlot, 
	int numSRVs, GPUShaderResource** ppSRVs) 
{
}

void RenderPluginDx12::ClearVertexShaderResources(int startSlot, int numSRVs) 
{
}

void RenderPluginDx12::ClearPixelShaderResources(int startSlot, int numSRVs) 
{
}

void RenderPluginDx12::ClearInputLayout() 
{
}

void RenderPluginDx12::SetVertexBuffer(GPUBufferResource* pBuffer, UINT stride, UINT offset) 
{
	RenderInterfaceD3D12::SetVertexBuffer(pBuffer, stride, offset);
}

void RenderPluginDx12::SetPrimitiveTopologyTriangleStrip() 
{
	RenderInterfaceD3D12::SetPrimitiveTopologyTriangleStrip();
}

void RenderPluginDx12::SetPrimitiveTopologyTriangleList() 
{
	RenderInterfaceD3D12::SetPrimitiveTopologyTriangleList();
}

void RenderPluginDx12::SetPrimitiveTopologyLineList() 
{
	RenderInterfaceD3D12::SetPrimitiveTopologyLineList();
}

void RenderPluginDx12::Draw(unsigned int vertexCount, unsigned int startCount)
{
	RenderInterfaceD3D12::Draw(vertexCount, startCount);
}

GPUBufferResource* RenderPluginDx12::CreateVertexBuffer(unsigned int ByteWidth, void* pSysMem)
{ 
	return RenderInterfaceD3D12::CreateVertexBuffer(ByteWidth, pSysMem);
}

GPUShaderResource* RenderPluginDx12::CreateShaderResource(unsigned int stride, 
	unsigned int numElements, void* pSysMem, NVHairReadOnlyBuffer* pReadOnlyBuffer)
{ 
	return RenderInterfaceD3D12::CreateShaderResource(stride, numElements, pSysMem, pReadOnlyBuffer);
}

void RenderPluginDx12::ApplyForShadow(int ForShadow) 
{
	RenderInterfaceD3D12::ApplyForShadow(ForShadow);
}

void RenderPluginDx12::SwitchToDX11()
{
	RenderInterfaceD3D12::SwitchToDX11();
}

void RenderPluginDx12::FlushDX11()
{
	RenderInterfaceD3D12::FlushDX11();
}

void RenderPluginDx12::FlushDX12()
{
	RenderInterfaceD3D12::FlushDX12();
}

void RenderPluginDx12::ApplyPrimitiveTopologyLine()
{
	RenderInterfaceD3D12::ApplyPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
}

void RenderPluginDx12::ApplyPrimitiveTopologyTriangle()
{
	RenderInterfaceD3D12::ApplyPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
}

void RenderPluginDx12::SubmitGpuWork()
{
	RenderInterfaceD3D12::SubmitGpuWork();
}

void RenderPluginDx12::WaitForGpu()
{
	RenderInterfaceD3D12::WaitForGpu();
}

// util
bool RenderPluginDx12::CreateRenderWindow(HWND hWnd, int nSamples) 
{ 	
	return D3D12Util::CreateRenderWindow(hWnd, nSamples);
}

bool RenderPluginDx12::ResizeRenderWindow(int w, int h) 
{
	return D3D12Util::ResizeRenderWindow(w, h);
}

void RenderPluginDx12::PresentRenderWindow() 
{
	D3D12Util::PresentRenderWindow();
}

void RenderPluginDx12::ClearRenderWindow(float r, float g, float b) 
{
	D3D12Util::ClearRenderWindow(r, g, b);
}

bool RenderPluginDx12::GetDeviceInfoString(wchar_t *str) 
{
	return D3D12Util::GetDeviceInfoString(str);
}

GPUShaderResource* RenderPluginDx12::CreateTextureSRV(const char* texturename)
{ 
	return D3D12Util::CreateTextureSRV(texturename);
}

void RenderPluginDx12::TxtHelperBegin()
{
	D3D12Util::TxtHelperBegin();
}

void RenderPluginDx12::TxtHelperEnd() 
{
	D3D12Util::TxtHelperEnd();
}

void RenderPluginDx12::TxtHelperSetInsertionPos(int x, int y)
{
	D3D12Util::TxtHelperSetInsertionPos(x, y);
}

void RenderPluginDx12::TxtHelperSetForegroundColor(float r, float g, float b, float a)
{
	D3D12Util::TxtHelperSetForegroundColor(r, g, b, a);
}

void RenderPluginDx12::TxtHelperDrawTextLine(wchar_t* str)
{
	D3D12Util::TxtHelperDrawTextLine(str);
}

// shader
bool RenderPluginDx12::InitializeShaders() 
{ 
	return InitializeShadersD3D12();
}

void RenderPluginDx12::DestroyShaders() 
{
	DestroyShadersD3D12();
}

void RenderPluginDx12::ApplyShader(SHADER_TYPE st) 
{
	ApplyShaderD3D12(st);
}

void RenderPluginDx12::DisableShader(SHADER_TYPE st) 
{
	DisableShaderD3D12(st);
}

void RenderPluginDx12::BindShaderResources(SHADER_TYPE st, int numSRVs, GPUShaderResource** ppSRVs)
{
	BindShaderResourcesD3D12(st, numSRVs, ppSRVs);
}

void RenderPluginDx12::CopyShaderParam(SHADER_TYPE st, 
	void* pSysMem, unsigned int bytes, unsigned int slot) 
{
	CopyShaderParamD3D12(st, pSysMem, bytes, slot);
}

// GPUProfiler
GPUProfiler* RenderPluginDx12::CreateGPUProfiler()
{
	GPUProfiler* pProfiler = new D3D12GPUProfiler;
	pProfiler->Initialize();
	return pProfiler;
}

// ShadowMap
ShadowMap* RenderPluginDx12::CreateShadowMap(int resolution)
{
	return new D3D12ShadowMap(resolution);
}

// D3D12RenderContext
void RenderPluginDx12::PreRender()
{
	D3D12RenderContext::Instance()->PreRender();
}

void RenderPluginDx12::PostRender()
{
	D3D12RenderContext::Instance()->PostRender();
}

// GPUMeshResources
#include "MeshData.h"
#include "AnimUtil.h"

class GPUMeshResourcesDx12 : public GPUMeshResources
{
public:
	NVHairReadOnlyBuffer		m_BoneIndicesBuffer;
	NVHairReadOnlyBuffer		m_BoneWeightsBuffer;
};

GPUMeshResources* RenderPluginDx12::GPUMeshResourcesCreate(MeshData* pMeshData, const SkinData& skinData)
{
	GPUMeshResources* resources = new GPUMeshResourcesDx12;

	int numIndices = pMeshData->m_NumIndices;
	int numVertices = pMeshData->m_NumVertices;

	resources->m_pVertexBuffer = CreateVertexBuffer(
		sizeof(MeshData::MeshVertex) * numIndices, pMeshData->m_pMeshVertices);

	GPUMeshResourcesDx12* resourceDx12 = (GPUMeshResourcesDx12*)resources;
	if (NULL != resourceDx12)
	{
		D3D12RenderContext* pContext = D3D12RenderContext::Instance();
		pContext->InitBuffer(resourceDx12->m_BoneIndicesBuffer);
		pContext->InitBuffer(resourceDx12->m_BoneWeightsBuffer);
		resources->m_pBoneIndicesSRV = CreateShaderResource(
			sizeof(atcore_float4), numVertices, skinData.m_pBoneIndices, &resourceDx12->m_BoneIndicesBuffer);
		resources->m_pBoneWeightsSRV = CreateShaderResource(
			sizeof(atcore_float4), numVertices, skinData.m_pBoneWeights, &resourceDx12->m_BoneWeightsBuffer);
	}

	return resources;
}

void RenderPluginDx12::GPUMeshResourcesRelease(GPUMeshResources* pResource)
{
	SAFE_RELEASE(pResource->m_pVertexBuffer);
	SAFE_RELEASE(pResource->m_pBoneIndicesSRV);
	SAFE_RELEASE(pResource->m_pBoneWeightsSRV);

	GPUMeshResourcesDx12* pResourceDx12 = (GPUMeshResourcesDx12*)pResource;
	if (NULL == pResourceDx12)
	{
		return;
	}

	D3D12RenderContext* pContext = D3D12RenderContext::Instance();
	pContext->DestroyBuffer(pResourceDx12->m_BoneIndicesBuffer);
	pContext->DestroyBuffer(pResourceDx12->m_BoneWeightsBuffer);
}

D3DHandles& RenderPluginDx12::GetDeviceHandles(D3DHandles& deviceHandles)
{
	return D3D12Util::GetDeviceHandles(deviceHandles);
}