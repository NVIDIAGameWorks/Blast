#include "D3D12RenderContext.h"

const int nBufferCount = 2;
//const int nBufferCount = 5;

int nRenderTargetIndex = 1;
//int nRenderTargetIndex = 4;

D3D12RenderContext::D3D12RenderContext()
{
	m_sampleCount = 1;
}

D3D12RenderContext::~D3D12RenderContext()
{
}

D3D12RenderContext* D3D12RenderContext::Instance()
{
	static D3D12RenderContext ri;
	return &ri;
}

void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	IDXGIAdapter1* pAdapter = nullptr;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = pAdapter;
}

void D3D12RenderContext::InitDevice()
{
	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	// Enable the D3D11 debug layer.
	d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)));

	GetHardwareAdapter(m_factory.Get(), &m_adapter);

	auto adapterDescription = DXGI_ADAPTER_DESC();
	m_adapter->GetDesc(&adapterDescription);

	D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
	if (nullptr == m_device)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	ThrowIfFailed(D3D11On12CreateDevice(
		m_device.Get(),
		d3d11DeviceFlags, nullptr, 0,
		reinterpret_cast<IUnknown**>(m_commandQueue.GetAddressOf()), 1, 0,
		&m_d3d11Device,
		&m_d3d11DeviceContext,
		nullptr ));

	ThrowIfFailed(m_d3d11Device.As(&m_d3d11On12Device));
}

void D3D12RenderContext::InitSwapchain(int nWidth, int nHeight, HWND hWnd)
{
	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = nBufferCount;
	swapChainDesc.BufferDesc.Width = nWidth;
	swapChainDesc.BufferDesc.Height = nHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain> swapChain;
	ThrowIfFailed(m_factory->CreateSwapChain(
		m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		&swapChainDesc,
		&swapChain
		));

	ThrowIfFailed(swapChain.As(&m_swapChain));

	// This sample does not support fullscreen transitions.
	// ThrowIfFailed(m_factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	m_pCurrentRenderTarget = new D3D12RenderTarget(0, nBufferCount);
	m_pCurrentRenderTarget->OnCreate(nWidth, nHeight);
	m_pCurrentRenderTarget->SetClearColor(0.5, 0.5, 0.5);
	m_RenderTargetMap[0] = m_pCurrentRenderTarget;

	m_pD3D11RenderTargetView = new ID3D11RenderTargetView*[nBufferCount];
	m_wrappedBackBuffers = new ID3D11Resource*[nBufferCount];
	for (int n = 0; n < nBufferCount; n++)
	{
		m_pD3D11RenderTargetView[n] = nullptr;
		m_wrappedBackBuffers[n] = nullptr;
	}

	PostCreate();
}

#ifndef Safe_Release
#define Safe_Release(p)       { if (p) { p->Release();     (p) = nullptr; } }
#endif // !Safe_Delete

void D3D12RenderContext::ResizeSwapchain(int width, int height)
{	
	D3D12RenderTarget* rt = m_RenderTargetMap[0];

	for (int n = 0; n < nBufferCount; n++)
	{
		Safe_Release(m_pD3D11RenderTargetView[n]);
		Safe_Release(m_wrappedBackBuffers[n]);
	}
	m_d3d11DeviceContext->Flush();

	rt->OnDestroy();

	ThrowIfFailed(m_swapChain->ResizeBuffers(nBufferCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	rt->OnResize(width, height);

	for (int n = 0; n < nBufferCount; n++)
	{
		ID3D11On12Device* pDevice11On12 = GetDevice11On12();

		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		ThrowIfFailed(pDevice11On12->CreateWrappedResource(
			rt->GetTexture(n, false),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&m_wrappedBackBuffers[n])));

		ThrowIfFailed(m_d3d11Device->CreateRenderTargetView(m_wrappedBackBuffers[n],
			NULL, &m_pD3D11RenderTargetView[n]));
	}
}

void D3D12RenderContext::PostCreate()
{
	m_srvUavHeap.Init(m_device.Get(), 256, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	{
		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		WaitForGpu();
	}
}

void D3D12RenderContext::OnDestroy()
{
	WaitForGpu();
	if (nullptr != m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}

#if 0
	ID3D12DebugDevice* debugInterface;
	if (SUCCEEDED(m_device.Get()->QueryInterface(&debugInterface)))
	{
		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debugInterface->Release();
	}
#endif // 0
}

void D3D12RenderContext::PreRender()
{
	ThrowIfFailed(m_commandAllocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
}

void D3D12RenderContext::PostRender()
{
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	OnGpuWorkSubmitted(m_commandQueue.Get());

	WaitForGpu();
}

void D3D12RenderContext::SubmitGpuWork()
{
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	OnGpuWorkSubmitted(m_commandQueue.Get());
}


void D3D12RenderContext::Flush()
{
	m_pCurrentRenderTarget->PostRender();
	PostRender();
	PreRender();
	m_pCurrentRenderTarget->PreRender();
}

void D3D12RenderContext::Present()
{
	ThrowIfFailed(m_swapChain->Present(1, 0));
	//ThrowIfFailed(m_swapChain->Present(0, 0));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	//OnGpuWorkSubmitted(m_commandQueue.Get(), true);
	//WaitForGpu();
}

int D3D12RenderContext::AllocRenderTargetIndex()
{
	return nRenderTargetIndex++;
}


void D3D12RenderContext::AddGpuInterface(GpuInterface* gpuIntf)
{
	m_gpuInterfaces.push_back(gpuIntf);
}


void D3D12RenderContext::OnGpuWorkSubmitted(ID3D12CommandQueue* queue)
{
	std::vector<GpuInterface*>::const_iterator cur = m_gpuInterfaces.begin();
	std::vector<GpuInterface*>::const_iterator end = m_gpuInterfaces.end();

	for (; cur != end; cur++)
	{
		(*cur)->onGpuWorkSubmitted(queue);
	}
}

void D3D12RenderContext::UpdateGpuWorkCompleted()
{
	std::vector<GpuInterface*>::const_iterator cur = m_gpuInterfaces.begin();
	std::vector<GpuInterface*>::const_iterator end = m_gpuInterfaces.end();

	for (; cur != end; cur++)
	{
		(*cur)->updateGpuWorkCompleted();
	}
}

D3D12RenderTarget* D3D12RenderContext::CreateRenderTarget(int renderTargetIndex, int nWidth, int nHeight)
{
	D3D12RenderTarget* pRenderTarget = new D3D12RenderTarget(renderTargetIndex);
	pRenderTarget->OnCreate(nWidth, nHeight);
	m_RenderTargetMap[renderTargetIndex] = pRenderTarget;
	return pRenderTarget;
}

ID3D12Resource* D3D12RenderContext::GetTexture(int renderTargetIndex, int index)
{
	D3D12RenderTarget* pRenderTarget = m_RenderTargetMap[renderTargetIndex];
	ID3D12Resource* pTexture = pRenderTarget->GetTexture(index);
	return pTexture;
}

void D3D12RenderContext::SetViewport(D3D12_VIEWPORT& vp)
{
	m_pCurrentRenderTarget->m_viewport = vp;
	m_commandList->RSSetViewports(1, &vp);
}

void D3D12RenderContext::GetViewport(D3D12_VIEWPORT& vp)
{
	vp = m_pCurrentRenderTarget->m_viewport;
}

D3D12_RESOURCE_DESC D3D12RenderContext::GetBackBufferDesc()
{
	D3D12RenderTarget* pRenderTarget = m_RenderTargetMap[0];
	ID3D12Resource* pResource = pRenderTarget->GetTexture();
	return pResource->GetDesc();
}

void D3D12RenderContext::SwitchToDX11()
{
	ReleaseRenderTarget();
	PostRender();

	if (nullptr == m_wrappedBackBuffers[m_frameIndex])
	{
		return;
	}

	m_d3d11On12Device->AcquireWrappedResources(&m_wrappedBackBuffers[m_frameIndex], 1);

	m_d3d11DeviceContext->OMSetRenderTargets(1, &m_pD3D11RenderTargetView[m_frameIndex], nullptr);
	D3D12RenderTarget* pRenderTarget = m_RenderTargetMap[0];	
	D3D11_VIEWPORT vp;
	{
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = (float)pRenderTarget->m_viewport.Width;
		vp.Height = (float)pRenderTarget->m_viewport.Height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
	}
	m_d3d11DeviceContext->RSSetViewports(1, &vp);
}

void D3D12RenderContext::FlushDX11()
{
	if (nullptr == m_wrappedBackBuffers[m_frameIndex])
	{
		return;
	}

	m_d3d11On12Device->ReleaseWrappedResources(&m_wrappedBackBuffers[m_frameIndex], 1);
	m_d3d11DeviceContext->Flush();
}

ID3D12Resource* D3D12RenderContext::GetDepthStencilResource()
{
	D3D12RenderTarget* pRenderTarget = m_RenderTargetMap[0];
	return pRenderTarget->GetDepthStencilResource();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderContext::GetRenderTargetViewHandle()
{
	D3D12RenderTarget* pRenderTarget = m_RenderTargetMap[0];
	return pRenderTarget->GetRenderTargetViewHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderContext::GetDepthStencilViewHandle()
{
	D3D12RenderTarget* pRenderTarget = m_RenderTargetMap[0];
	return pRenderTarget->GetDepthStencilViewHandle();
}

void D3D12RenderContext::AcquireRenderTarget(bool doClear, int renderTargetIndex)
{
	m_pCurrentRenderTarget = m_RenderTargetMap[renderTargetIndex];
	m_pCurrentRenderTarget->PreRender(doClear);
}

void D3D12RenderContext::ReleaseRenderTarget(int renderTargetIndex)
{
	D3D12RenderTarget* pRenderTarget = m_RenderTargetMap[renderTargetIndex];
	pRenderTarget->PostRender();
}

void D3D12RenderContext::SetClearColor(int renderTargetIndex, float r, float g, float b, float a, float depth, float stencil)
{
	D3D12RenderTarget* pRenderTarget = m_RenderTargetMap[renderTargetIndex];
	pRenderTarget->SetClearColor(r, g, b, a, depth, stencil);
}

void D3D12RenderContext::WaitForGpu()
{
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void D3D12RenderContext::InitBuffer(NVHairReadOnlyBuffer& buffer)
{
	buffer.Init(&m_srvUavHeap);
}

void D3D12RenderContext::DestroyBuffer(NVHairReadOnlyBuffer& buffer)
{
	buffer.Release();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3D12RenderContext::NVHairINT_CreateD3D12ReadOnlyBuffer(
	UINT						stride,
	UINT						numElements,
	NVHairReadOnlyBuffer*		pReadOnlyBuffer,
	void*						pSysMem)
{
	ID3D12Device*				pd3dDevice = m_device.Get();
	ID3D12GraphicsCommandList*	pCommandList = m_commandList.Get();

	UINT bufferSize = numElements * stride;

	HRESULT hr;
	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(pReadOnlyBuffer->m_pBuffer.ReleaseAndGetAddressOf()));

	hr = pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pReadOnlyBuffer->m_pBufferUpload.ReleaseAndGetAddressOf()));

	D3D12_SUBRESOURCE_DATA data = {};
	data.pData = reinterpret_cast<UINT8*>(pSysMem);
	data.RowPitch = bufferSize;
	data.SlicePitch = data.RowPitch;

	PreRender();
	UpdateSubresources<1>(pCommandList, pReadOnlyBuffer->m_pBuffer.Get(), pReadOnlyBuffer->m_pBufferUpload.Get(), 0, 0, 1, &data);
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pReadOnlyBuffer->m_pBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	PostRender();

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(pReadOnlyBuffer->m_pSrvUavHeap->m_pHeap.Get()->GetCPUDescriptorHandleForHeapStart(), pReadOnlyBuffer->m_srvIndex, pReadOnlyBuffer->m_pSrvUavHeap->m_sizeDescriptor);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = stride;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	pd3dDevice->CreateShaderResourceView(pReadOnlyBuffer->m_pBuffer.Get(), &srvDesc, srvHandle);

	return srvHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE D3D12RenderContext::NVHairINT_CreateD3D12Texture(ID3D12Resource* pTexture, int& nIndexInHeap)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = {};

	if (!pTexture)
		return handle;

	ID3D12Device* pDevice = m_device.Get();
	if (!pDevice)
		return handle;

	nIndexInHeap = m_srvUavHeap.allocate();
	if(nIndexInHeap == -1)
		return handle;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_srvUavHeap.m_pHeap.Get()->GetCPUDescriptorHandleForHeapStart(), 
		nIndexInHeap, m_srvUavHeap.m_sizeDescriptor);

	D3D12_RESOURCE_DESC desc = pTexture->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	pDevice->CreateShaderResourceView(pTexture, &srvDesc, srvHandle);

	return srvHandle;
}

void D3D12RenderContext::NVHairINT_DestroyD3D12Texture(int& nIndexInHeap)
{
	if (nIndexInHeap != -1)
	{
		m_srvUavHeap.deallocate(nIndexInHeap);
	}
}

void D3D12RenderContext::SetSampleCount(int nSampleCount)
{
	m_sampleCount = 1;

	if (nSampleCount > D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT)
		nSampleCount = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT;

	if (nSampleCount > 1)
	{
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
		qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		qualityLevels.NumQualityLevels = 0;
		qualityLevels.SampleCount = nSampleCount;

		ID3D12Device *pDevice = GetDevice();
		pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels));

		if (qualityLevels.NumQualityLevels > 0)
		{
			m_sampleCount = qualityLevels.SampleCount;
		}
	}
}

int D3D12RenderContext::GetSampleCount()
{
	return m_sampleCount;
}