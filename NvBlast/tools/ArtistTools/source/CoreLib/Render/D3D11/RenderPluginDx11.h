#pragma once

#include "RenderPlugin.h"

extern "C" CORERENDER_EXPORT RenderPlugin* CreateRenderPlugin(void);

class CORERENDER_EXPORT RenderPluginDx11 : public RenderPlugin
{
public:
	RenderPluginDx11();
	~RenderPluginDx11();

	// interface
	virtual bool InitDevice(int deviceID);
	virtual bool Initialize();
	virtual void Shutdown();
	virtual void CopyToDevice(GPUBufferResource *pDevicePtr, void* pSysMem, unsigned int	ByteWidth);
	virtual void ApplyDepthStencilState(DEPTH_STENCIL_STATE state);
	virtual void ApplyRasterizerState(RASTERIZER_STATE state);
	virtual void ApplySampler(int slot, SAMPLER_TYPE st);
	virtual void ApplyBlendState(BLEND_STATE st);
	virtual void GetViewport(Viewport& vp);
	virtual void SetViewport(const Viewport& vp);
	virtual void BindVertexShaderResources(int startSlot, int numSRVs, GPUShaderResource** ppSRVs);
	virtual void BindPixelShaderResources(int startSlot, int numSRVs, GPUShaderResource** ppSRVs);
	virtual void ClearVertexShaderResources(int startSlot, int numSRVs);
	virtual void ClearPixelShaderResources(int startSlot, int numSRVs);
	virtual void ClearInputLayout();
	virtual void SetVertexBuffer(GPUBufferResource* pBuffer, UINT stride, UINT offset = 0);
	virtual void SetPrimitiveTopologyTriangleStrip();
	virtual void SetPrimitiveTopologyTriangleList();
	virtual void SetPrimitiveTopologyLineList();
	virtual void Draw(unsigned int vertexCount, unsigned int startCount = 0);
	virtual GPUBufferResource* CreateVertexBuffer(unsigned int ByteWidth, void* pSysMem = 0);
	virtual GPUShaderResource* CreateShaderResource(unsigned int stride, unsigned int numElements, void* pSysMem, NVHairReadOnlyBuffer* pReadOnlyBuffer = NULL);
	virtual void ApplyForShadow(int ForShadow);
	virtual void SwitchToDX11();
	virtual void FlushDX11();
	virtual void FlushDX12();
	virtual void ApplyPrimitiveTopologyLine();
	virtual void ApplyPrimitiveTopologyTriangle();
	virtual void SubmitGpuWork();
	virtual void WaitForGpu();

	// util
	virtual bool CreateRenderWindow(HWND hWnd, int nSamples);
	virtual bool ResizeRenderWindow(int w, int h);
	virtual void PresentRenderWindow();
	virtual void ClearRenderWindow(float r, float g, float b);
	virtual bool GetDeviceInfoString(wchar_t *str);
	virtual GPUShaderResource* CreateTextureSRV(const char* texturename);
	virtual void TxtHelperBegin();
	virtual void TxtHelperEnd();
	virtual void TxtHelperSetInsertionPos(int x, int y);
	virtual void TxtHelperSetForegroundColor(float r, float g, float b, float a = 1.0f);
	virtual void TxtHelperDrawTextLine(wchar_t* str);

	// shader
	virtual bool InitializeShaders();
	virtual void DestroyShaders();
	virtual void ApplyShader(SHADER_TYPE st);
	virtual void DisableShader(SHADER_TYPE st);
	virtual void BindShaderResources(SHADER_TYPE st, int numSRVs, GPUShaderResource** ppSRVs);
	virtual void CopyShaderParam(SHADER_TYPE st, void* pSysMem, unsigned int bytes, unsigned int slot = 0);

	// GPUProfiler
	virtual GPUProfiler* CreateGPUProfiler();

	// ShadowMap
	virtual ShadowMap* CreateShadowMap(int resolution);

	// D3D12RenderContext
	virtual void PreRender();
	virtual void PostRender();

	// GPUMeshResources
	virtual GPUMeshResources* GPUMeshResourcesCreate(MeshData* pMeshData, const SkinData& skinData);
	virtual void GPUMeshResourcesRelease(GPUMeshResources* pResource);

	// Get devices related
	virtual D3DHandles& GetDeviceHandles(D3DHandles& deviceHandles);
};

