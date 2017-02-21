#pragma once

#include "D3D12RenderTarget.h"
#include "RenderPlugin.h"

#include "d3dx12.h"
#include <wrl.h>
using namespace Microsoft::WRL;

struct NVHairHeap
{
	ComPtr<ID3D12DescriptorHeap> m_pHeap;
	UINT m_sizeHeap;
	UINT m_currentIndex;
	UINT m_sizeDescriptor;
	std::vector<UINT> m_availableIndexes;

	void Init(ID3D12Device* pd3dDevice, UINT size, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	{
		HRESULT hr;

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = size;
		srvHeapDesc.Flags = flags;
		srvHeapDesc.Type = type;
		hr = pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_pHeap.ReleaseAndGetAddressOf()));

		if (SUCCEEDED(hr))
		{
			m_sizeHeap = size;
			m_sizeDescriptor = pd3dDevice->GetDescriptorHandleIncrementSize(type);
		}
		else
		{
			m_sizeHeap = 0;
			m_sizeDescriptor = 0;
		}
		m_currentIndex = 0;
		m_availableIndexes.clear();
	}

	UINT allocate()
	{
		int availables = m_availableIndexes.size();
		if (availables > 0)
		{
			UINT index = m_availableIndexes[availables - 1];
			m_availableIndexes.pop_back();
			return index;
		}
		UINT index = m_currentIndex++;
		if (m_sizeHeap > m_currentIndex)
			return index;
		return (UINT)-1;
	}

	void deallocate(UINT availableIndex)
	{
		m_availableIndexes.push_back(availableIndex);
	}

	void Release()
	{
		if (m_pHeap) m_pHeap = nullptr;
		m_sizeHeap = 0;
		m_currentIndex = 0;
		m_sizeDescriptor = 0;
		m_availableIndexes.clear();
	}
};

struct NVHairVertexBuffer
{
	ComPtr<ID3D12Resource> m_pBuffer;
	ComPtr<ID3D12Resource> m_pBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	NVHairHeap* m_pSrvUavHeap;
	UINT m_uavIndex;

	void Init(NVHairHeap* pHeap)
	{
		m_pBuffer = nullptr;
		m_pBufferUpload = nullptr;
		memset(&m_vertexBufferView, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
		m_pSrvUavHeap = pHeap;

		m_uavIndex = m_pSrvUavHeap->allocate();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE getUavCpuHandle()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_pSrvUavHeap->m_pHeap.Get()->GetCPUDescriptorHandleForHeapStart(), m_uavIndex, m_pSrvUavHeap->m_sizeDescriptor);
		return uavHandle;
	}

	void Release()
	{
		if (m_pBuffer) m_pBuffer = nullptr;
		if (m_pBufferUpload) m_pBufferUpload = nullptr;
		memset(&m_vertexBufferView, 0, sizeof(D3D12_VERTEX_BUFFER_VIEW));
		if (m_pSrvUavHeap) m_pSrvUavHeap = nullptr;
		m_uavIndex = (UINT)-1;
	}
};

struct NVHairReadOnlyBuffer
{
	ComPtr<ID3D12Resource> m_pBuffer;
	ComPtr<ID3D12Resource> m_pBufferUpload;
	NVHairHeap* m_pSrvUavHeap;
	UINT m_srvIndex;

	void Init(NVHairHeap* pHeap)
	{
		m_pBuffer = nullptr;
		m_pBufferUpload = nullptr;
		m_pSrvUavHeap = pHeap;

		m_srvIndex = m_pSrvUavHeap->allocate();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE getSrvCpuHandle()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_pSrvUavHeap->m_pHeap.Get()->GetCPUDescriptorHandleForHeapStart(), m_srvIndex, m_pSrvUavHeap->m_sizeDescriptor);
		return srvHandle;
	}

	void Release()
	{
		m_pSrvUavHeap->deallocate(m_srvIndex);

		if (m_pBuffer) m_pBuffer = nullptr;
		if (m_pBufferUpload) m_pBufferUpload = nullptr;
		if (m_pSrvUavHeap) m_pSrvUavHeap = nullptr;
		m_srvIndex = (UINT)-1;
	}
};

class CORERENDER_EXPORT D3D12RenderContext
{
public:
	~D3D12RenderContext();
	static D3D12RenderContext* Instance();

	class GpuInterface
	{
		public:
		virtual void onGpuWorkSubmitted(ID3D12CommandQueue* queue) = 0;
		virtual void updateGpuWorkCompleted() = 0;
		virtual ~GpuInterface() {}
	};

	typedef void (*GpuWorkSubmitFunc)(ID3D12CommandQueue* queue, void* data);
	typedef void (*GpuUpdateCompletedFunc)(void* data);

	void InitDevice();
	void InitSwapchain(int width, int height, HWND hWnd);
	void ResizeSwapchain(int width, int height);
	void PostCreate();

	void OnDestroy();

	void PreRender();
	void PostRender();
	void Flush();
	
	void SubmitGpuWork();
	void WaitForGpu();

	void UpdateGpuWorkCompleted();

	void Present();

	int AllocRenderTargetIndex();
	D3D12RenderTarget* CreateRenderTarget(int renderTargetIndex, int nWidth, int nHeight);
	void SetClearColor(int renderTargetIndex, float r, float g, float b, float a = 1.0, float depth = 1.0, float stencil = 0.0);
	void AcquireRenderTarget(bool doClear = false, int renderTargetIndex = 0);
	void ReleaseRenderTarget(int renderTargetIndex = 0);
	ID3D12Resource* GetTexture(int renderTargetIndex, int index = 0);
	void SetViewport(D3D12_VIEWPORT& vp);
	void GetViewport(D3D12_VIEWPORT& vp);

	ID3D12Device* GetDevice() { return m_device.Get(); }
	ID3D12GraphicsCommandList* GetGraphicsCommandList() { return m_commandList.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }

	IDXGISwapChain3* GetSwapChain() { return m_swapChain.Get(); }
	UINT GetFrameIndex() { return m_frameIndex; }

	ID3D11Device* GetDevice11() { return m_d3d11Device.Get(); }
	ID3D11On12Device* GetDevice11On12() { return m_d3d11On12Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() { return m_d3d11DeviceContext.Get(); }
	
	D3D12_RESOURCE_DESC GetBackBufferDesc();
	void SwitchToDX11();
	void FlushDX11();

	ID3D12Resource* GetDepthStencilResource();
	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetViewHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilViewHandle();

	void InitBuffer(NVHairReadOnlyBuffer& buffer);
	void DestroyBuffer(NVHairReadOnlyBuffer& buffer);

	CD3DX12_CPU_DESCRIPTOR_HANDLE NVHairINT_CreateD3D12ReadOnlyBuffer(
		UINT						stride,
		UINT						numElements,
		NVHairReadOnlyBuffer*		pReadOnlyBuffer,
		void*						pSysMem);

	CD3DX12_CPU_DESCRIPTOR_HANDLE NVHairINT_CreateD3D12Texture(ID3D12Resource* pTexture, int& nIndexInHeap);
	void NVHairINT_DestroyD3D12Texture(int& nIndexInHeap);

	void SetSampleCount(int nSampleCount);
	int GetSampleCount();

	void AddGpuInterface(GpuInterface* intf);

private:
	void OnGpuWorkSubmitted(ID3D12CommandQueue* queue);
	
	D3D12RenderContext();
	
	ComPtr<IDXGIFactory4> m_factory;
	ComPtr<IDXGIAdapter1> m_adapter;

	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<IDXGISwapChain3> m_swapChain;

	ComPtr<ID3D11Device> m_d3d11Device;
	ComPtr<ID3D11On12Device> m_d3d11On12Device;
	ComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;
	ID3D11Resource** m_wrappedBackBuffers;
	ID3D11RenderTargetView** m_pD3D11RenderTargetView;

	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;

	D3D12RenderTarget* m_pCurrentRenderTarget;
	map<int, D3D12RenderTarget*> m_RenderTargetMap;

	std::vector<GpuInterface*> m_gpuInterfaces;

	NVHairHeap					m_srvUavHeap;

	// sample desc
	UINT						m_sampleCount;
};

