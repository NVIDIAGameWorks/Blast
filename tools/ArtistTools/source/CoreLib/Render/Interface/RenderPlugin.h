#pragma once

#include "RenderInterface.h"
using namespace RenderInterface;

class NVHairReadOnlyBuffer;
class GPUProfiler;
class ShadowMap;

class IDXGIAdapter;
class IUnknown;
class IDXGIFactory1;
class IDXGISwapChain;

struct D3DHandles
{
public:
	IDXGIAdapter*       pAdapter;
	IDXGIFactory1*      pFactory;
	IUnknown*           pDevice;
	IUnknown*           pDeviceContext;

	IDXGISwapChain*		pDXGISwapChain;
	IUnknown*			pD3D11BackBuffer;
	IUnknown*		    pD3D11RenderTargetView;
	IUnknown*			pD3D11DepthBuffer;
	IUnknown*		    pD3D11DepthStencilView;
};

#ifdef CORERENDER_LIB
# define CORERENDER_EXPORT Q_DECL_EXPORT
#else
# define CORERENDER_EXPORT Q_DECL_IMPORT
#endif

class CORELIB_EXPORT RenderPlugin
{
public:
	static bool Load(std::vector<std::string>& render_plugins);
	static RenderPlugin* Instance();
	static void Destroy();

	~RenderPlugin();

	// self
	virtual std::string GetRenderApi() { return m_RenderApi; }

	// interface
	virtual bool InitDevice(int deviceID) = 0;
	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;
	virtual void CopyToDevice(GPUBufferResource *pDevicePtr, void* pSysMem, unsigned int	ByteWidth) = 0;
	virtual void ApplyDepthStencilState(DEPTH_STENCIL_STATE state) = 0;
	virtual void ApplyRasterizerState(RASTERIZER_STATE state) = 0;
	virtual void ApplySampler(int slot, SAMPLER_TYPE st) = 0;
	virtual void ApplyBlendState(BLEND_STATE st) = 0;
	virtual void GetViewport(Viewport& vp) = 0;
	virtual void SetViewport(const Viewport& vp) = 0;
	virtual void BindVertexShaderResources(int startSlot, int numSRVs, GPUShaderResource** ppSRVs) = 0;
	virtual void BindPixelShaderResources(int startSlot, int numSRVs, GPUShaderResource** ppSRVs) = 0;
	virtual void ClearVertexShaderResources(int startSlot, int numSRVs) = 0;
	virtual void ClearPixelShaderResources(int startSlot, int numSRVs) = 0;
	virtual void ClearInputLayout() = 0;
	virtual void SetVertexBuffer(GPUBufferResource* pBuffer, UINT stride, UINT offset = 0) = 0;
	virtual void SetPrimitiveTopologyTriangleStrip() = 0;
	virtual void SetPrimitiveTopologyTriangleList() = 0;
	virtual void SetPrimitiveTopologyLineList() = 0;
	virtual void Draw(unsigned int vertexCount, unsigned int startCount = 0) = 0;
	virtual GPUBufferResource* CreateVertexBuffer(unsigned int ByteWidth, void* pSysMem = 0) = 0;
	virtual GPUShaderResource* CreateShaderResource(unsigned int stride, unsigned int numElements, void* pSysMem, NVHairReadOnlyBuffer* pReadOnlyBuffer = NULL) = 0;
	
	// interface dx12
	virtual void ApplyForShadow(int ForShadow) = 0;
	virtual void SwitchToDX11() = 0;
	virtual void FlushDX11() = 0;	
	virtual void FlushDX12() = 0;
	virtual void ApplyPrimitiveTopologyLine() = 0;
	virtual void ApplyPrimitiveTopologyTriangle() = 0;
	virtual void SubmitGpuWork() = 0;
	virtual void WaitForGpu() = 0;

	// util
	virtual bool CreateRenderWindow(HWND hWnd, int nSamples) = 0;
	virtual bool ResizeRenderWindow(int w, int h) = 0;
	virtual void PresentRenderWindow() = 0;
	virtual void ClearRenderWindow(float r, float g, float b) = 0;
	virtual bool GetDeviceInfoString(wchar_t *str) = 0;
	virtual GPUShaderResource* CreateTextureSRV(const char* texturename) = 0;
	virtual void TxtHelperBegin() = 0;
	virtual void TxtHelperEnd() = 0;
	virtual void TxtHelperSetInsertionPos(int x, int y) = 0;
	virtual void TxtHelperSetForegroundColor(float r, float g, float b, float a = 1.0f) = 0;
	virtual void TxtHelperDrawTextLine(wchar_t* str) = 0;

	// shader
	virtual bool InitializeShaders() = 0;
	virtual void DestroyShaders() = 0;
	virtual void ApplyShader(SHADER_TYPE st) = 0;
	virtual void DisableShader(SHADER_TYPE st) = 0;
	virtual void BindShaderResources(SHADER_TYPE st, int numSRVs, GPUShaderResource** ppSRVs) = 0;
	virtual void CopyShaderParam(SHADER_TYPE st, void* pSysMem, unsigned int bytes, unsigned int slot = 0) = 0;

	// GPUProfiler
	virtual GPUProfiler* CreateGPUProfiler() = 0;

	// ShadowMap
	virtual ShadowMap* CreateShadowMap(int resolution) = 0;

	// D3D12RenderContext
	virtual void PreRender() = 0;
	virtual void PostRender() = 0;

	// GPUMeshResources
	virtual GPUMeshResources* GPUMeshResourcesCreate(MeshData* pMeshData, const SkinData& skinData) = 0;
	virtual void GPUMeshResourcesRelease(GPUMeshResources* pResource) = 0;

	// Get devices related
	virtual D3DHandles& GetDeviceHandles(D3DHandles& deviceHandles) = 0;

protected:
	RenderPlugin();
	std::string m_RenderApi;
};

