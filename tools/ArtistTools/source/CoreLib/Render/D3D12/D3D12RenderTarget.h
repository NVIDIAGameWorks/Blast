#pragma once

class D3D12RenderContext;

#include "RenderInterface.h"
using namespace RenderInterface;

#include <d3d12.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <string>
#include <vector>
#include <map>
#include <wrl.h>
#include <shellapi.h>
using namespace std;
using namespace DirectX;
using namespace Microsoft::WRL;

#ifndef Safe_Delete
#define Safe_Delete(p)       { if (p) { delete (p);     (p) = nullptr; } }
#endif // !Safe_Delete
#ifndef Safe_Release
#define Safe_Release(p)       { if (p) { p->Release();     (p) = nullptr; } }
#endif // !Safe_Delete

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

typedef struct RenderShaderStateKey
{
	BLEND_STATE BlendState = BLEND_STATE_NONE;
	RASTERIZER_STATE RasterizerState = RASTERIZER_STATE_FILL_CULL_NONE;
	DEPTH_STENCIL_STATE DepthStencilState = DEPTH_STENCIL_DEPTH_TEST;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	int ForShadow = 0;

	bool operator <(const RenderShaderStateKey &other) const
	{
		int key = (((BlendState * 10 + RasterizerState) * 10 + DepthStencilState) * 10 + PrimitiveTopologyType) * 10 + ForShadow;
		int otherkey = (((other.BlendState * 10 + other.RasterizerState) * 10 + other.DepthStencilState) * 10 + other.PrimitiveTopologyType) * 10 + other.ForShadow;
		return key < otherkey;
	}
} RenderShaderStateKey;

typedef struct RenderShaderState
{
	D3D12_BLEND_DESC BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_RASTERIZER_DESC RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	DXGI_FORMAT RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int SampleCount = 1;
} RenderShaderState;

class D3D12RenderTarget
{
public:
	D3D12RenderTarget(int renderTargetIndex, int nRenderTargetCount = 1);
	~D3D12RenderTarget();

	void OnCreate(int nWidth, int nHeight);
	void OnResize(int nWidth, int nHeight);
	void OnDestroy();

	void PreRender(bool doClear = false);
	void PostRender();

	void SetClearColor(float r, float g, float b, float a = 1.0, float depth = 1.0, float stencil = 0.0);
	
	ID3D12Resource* GetTexture(int nIndex = 0, bool bRenderTarget = true);

	D3D12_VIEWPORT m_viewport;

	ID3D12Resource* GetDepthStencilResource();
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilViewHandle();

private:
	void CreateResource(int nWidth, int nHeight);

	int m_RenderTargetIndex;
	int m_RenderTargetCount;

	ID3D12Resource** m_BackBuffers;
	ID3D12Resource** m_RenderTargets;
	ID3D12Resource*	m_DepthStencil;

	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	UINT m_dsvDescriptorSize;

	D3D12_RECT m_scissorRect;

	D3D12RenderContext* m_pRenderContext;

	D3D12_CLEAR_VALUE m_rtvClearValue;
	D3D12_CLEAR_VALUE m_dsvClearValue;
};

