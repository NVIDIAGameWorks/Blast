#include "D3D12RenderTarget.h"
#include "D3D12RenderContext.h"

D3D12RenderTarget::D3D12RenderTarget(int renderTargetIndex, int nRenderTargetCount)
{
	m_RenderTargetIndex = renderTargetIndex;

	m_RenderTargetCount = nRenderTargetCount;

	m_BackBuffers = nullptr;
	m_RenderTargets = nullptr;
	m_DepthStencil = nullptr;

	m_pRenderContext = D3D12RenderContext::Instance();

	if (m_RenderTargetCount > 0)
	{
		m_BackBuffers = new ID3D12Resource*[m_RenderTargetCount];

		for (int n = 0; n < m_RenderTargetCount; n++)
		{
			m_BackBuffers[n] = nullptr;
		}

		m_RenderTargets = new ID3D12Resource*[m_RenderTargetCount];

		for (int n = 0; n < m_RenderTargetCount; n++)
		{
			m_RenderTargets[n] = nullptr;
		}
	}

	if (renderTargetIndex == 0)
	{
		m_rtvClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_rtvClearValue.Color[0] = 0.0;
		m_rtvClearValue.Color[1] = 0.0;
		m_rtvClearValue.Color[2] = 0.0;
		m_rtvClearValue.Color[3] = 1.0;
	}
	else
	{
		m_rtvClearValue.Format = DXGI_FORMAT_R32_FLOAT;
		m_rtvClearValue.Color[0] = FLT_MAX;
		m_rtvClearValue.Color[1] = FLT_MAX;
		m_rtvClearValue.Color[2] = FLT_MAX;
		m_rtvClearValue.Color[3] = FLT_MAX;
	}

	if (renderTargetIndex == 0)
	{
		m_dsvClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	}
	else
	{
		m_dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	}
	m_dsvClearValue.DepthStencil.Depth = 1.0f;
	m_dsvClearValue.DepthStencil.Stencil = 0;
}

D3D12RenderTarget::~D3D12RenderTarget()
{
	if (nullptr != m_BackBuffers)
	{
		delete[] m_BackBuffers;
		m_BackBuffers = nullptr;
	}

	if (nullptr != m_RenderTargets)
	{
		delete[] m_RenderTargets;
		m_RenderTargets = nullptr;
	}
}

ID3D12Resource* D3D12RenderTarget::GetDepthStencilResource()
{
	return m_DepthStencil;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderTarget::GetRenderTargetViewHandle()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	return rtvHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderTarget::GetDepthStencilViewHandle()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	return dsvHandle;
}

void D3D12RenderTarget::CreateResource(int nWidth, int nHeight)
{
	ID3D12Device* m_device = m_pRenderContext->GetDevice();
	int nSampleCount = m_pRenderContext->GetSampleCount();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	if (m_RenderTargetIndex == 0)
	{
		IDXGISwapChain3* m_swapChain = m_pRenderContext->GetSwapChain();
		
		// Create a RTV for each frame.
		for (int n = 0; n < m_RenderTargetCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_BackBuffers[n])));

			if (nSampleCount > 1)
			{
				// If we are multi-sampling - create a render target separate from the back buffer
				CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
				D3D12_RESOURCE_DESC desc = m_BackBuffers[n]->GetDesc();

				desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				desc.SampleDesc.Count = nSampleCount;
				desc.SampleDesc.Quality = 0;
				desc.Alignment = 0;

				ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET,
					&m_rtvClearValue, IID_PPV_ARGS(&m_RenderTargets[n])));
			}
			else
			{
				// The render targets and back buffers are the same thing
				m_RenderTargets[n] = m_BackBuffers[n];
			}

			m_device->CreateRenderTargetView(m_RenderTargets[n], nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}

		{
			auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_dsvClearValue.Format,
				nWidth, nHeight, 1, 1, nSampleCount, 0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // No need to read/write by CPU
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&m_dsvClearValue,
				IID_PPV_ARGS(&m_DepthStencil)));

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.ViewDimension = nSampleCount <= 1 ? D3D12_DSV_DIMENSION_TEXTURE2D : D3D12_DSV_DIMENSION_TEXTURE2DMS;
			dsvDesc.Format = m_dsvClearValue.Format;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			m_device->CreateDepthStencilView(m_DepthStencil, &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		}
	}
	else
	{
		{
			auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_rtvClearValue.Format,
				nWidth, nHeight, 1, 1, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = m_rtvClearValue.Format;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			rtvDesc.Texture2D.PlaneSlice = 0;

			for (int n = 0; n < m_RenderTargetCount; n++)
			{
				m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					&m_rtvClearValue,
					IID_PPV_ARGS(&m_RenderTargets[n]));
				m_device->CreateRenderTargetView(m_RenderTargets[n], &rtvDesc, rtvHandle);
				rtvHandle.Offset(1, m_rtvDescriptorSize);
			}
		}

		{
			auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_dsvClearValue.Format,
				nWidth, nHeight, 1, 1, 1, 0, 
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

			m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&m_dsvClearValue,
				IID_PPV_ARGS(&m_DepthStencil));

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = m_dsvClearValue.Format;
			dsvDesc.Texture2D.MipSlice = 0;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			m_device->CreateDepthStencilView(m_DepthStencil, &dsvDesc, m_dsvHeap.Get()->GetCPUDescriptorHandleForHeapStart());
		}
	}
}

void D3D12RenderTarget::OnCreate(int nWidth, int nHeight)
{
	ID3D12Device* m_device = m_pRenderContext->GetDevice();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = m_RenderTargetCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

		m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	OnResize(nWidth, nHeight);
}

void D3D12RenderTarget::OnResize(int nWidth, int nHeight)
{
	CreateResource(nWidth, nHeight);

	// viewport and scissor rect
	{
		memset(&m_viewport, 0, sizeof(m_viewport));
		m_viewport.Width = nWidth;
		m_viewport.Height = nHeight;
		m_viewport.MaxDepth = 1.0;

		memset(&m_scissorRect, 0, sizeof(m_scissorRect));
		m_scissorRect.right = nWidth;
		m_scissorRect.bottom = nHeight;
	}
}

void D3D12RenderTarget::OnDestroy()
{
	for (int n = 0; n < m_RenderTargetCount; n++)
	{
		Safe_Release(m_RenderTargets[n]);

		if (m_pRenderContext->GetSampleCount() > 1)
		{
			Safe_Release(m_BackBuffers[n]);
		}
	}
	Safe_Release(m_DepthStencil);
}

void D3D12RenderTarget::PreRender(bool doClear)
{
	ID3D12GraphicsCommandList* m_commandList = m_pRenderContext->GetGraphicsCommandList();
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	int numSamples = 1;
	UINT m_frameIndex = 0;	
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	if (m_RenderTargetIndex == 0)
	{
		numSamples = m_pRenderContext->GetSampleCount();
		m_frameIndex = m_pRenderContext->GetFrameIndex();
		if (numSamples <= 1)
		{
			state = D3D12_RESOURCE_STATE_PRESENT;
		}
		else
		{
			state = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		}
	}

	ID3D12Resource* pRenderTarget = m_RenderTargets[m_frameIndex];
	
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRenderTarget, state, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	if (doClear)
	{				
		m_commandList->ClearRenderTargetView(rtvHandle, m_rtvClearValue.Color, 0, nullptr);
		m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 
			m_dsvClearValue.DepthStencil.Depth, m_dsvClearValue.DepthStencil.Stencil, 0, nullptr);
	}
}

void D3D12RenderTarget::PostRender()
{
	ID3D12GraphicsCommandList* m_commandList = m_pRenderContext->GetGraphicsCommandList();

	int numSamples = 1;
	UINT m_frameIndex = 0;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	if (m_RenderTargetIndex == 0)
	{
		numSamples = m_pRenderContext->GetSampleCount();
		m_frameIndex = m_pRenderContext->GetFrameIndex();
		state = D3D12_RESOURCE_STATE_PRESENT;
	}

	if (numSamples <= 1)
	{
		ID3D12Resource* renderTarget = m_RenderTargets[m_frameIndex];
		CD3DX12_RESOURCE_BARRIER barrier(CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, state));
		m_commandList->ResourceBarrier(1, &barrier);
	}
	else
	{
		ID3D12Resource* backBuffer = m_BackBuffers[m_frameIndex];
		ID3D12Resource* renderTarget = m_RenderTargets[m_frameIndex];

		// Barriers to wait for the render target, and the backbuffer to be in correct state
		{
			D3D12_RESOURCE_BARRIER barriers[] =
			{
				CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST),
			};
			m_commandList->ResourceBarrier(2, barriers);
		}
		// Do the resolve...
		m_commandList->ResolveSubresource(backBuffer, 0, renderTarget, 0, m_rtvClearValue.Format);
		// Barrier until can present
		{
			CD3DX12_RESOURCE_BARRIER barrier(CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));
			m_commandList->ResourceBarrier(1, &barrier);
		}
	}
}

void D3D12RenderTarget::SetClearColor(float r, float g, float b, float a, float depth, float stencil)
{
	m_rtvClearValue.Color[0] = r;
	m_rtvClearValue.Color[1] = g;
	m_rtvClearValue.Color[2] = b;
	m_rtvClearValue.Color[3] = a;

	m_dsvClearValue.DepthStencil.Depth = depth;
	m_dsvClearValue.DepthStencil.Stencil = stencil;
}

ID3D12Resource* D3D12RenderTarget::GetTexture(int nIndex, bool bRenderTarget)
{
	if (nIndex < 0 || nIndex > m_RenderTargetCount)
		return nullptr;

	if (bRenderTarget)
	{
		return m_RenderTargets[nIndex];
	}
	else
	{
		return m_BackBuffers[nIndex];
	}
}
