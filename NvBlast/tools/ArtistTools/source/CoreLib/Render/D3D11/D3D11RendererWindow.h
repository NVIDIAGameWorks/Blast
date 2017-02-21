#pragma once

#include <d3d11.h>

// DXUT stuffs for text rendering
class CDXUTDialogResourceManager;
class CDXUTTextHelper;

struct D3D11RenderWindow
{
public:
	D3D11RenderWindow();
	~D3D11RenderWindow();

	bool Create(HWND hWnd, unsigned int nSamples = 1);
	bool Resize(int w, int h);
	void Present();
	void Clear(float r, float g, float b);
	bool CreateRenderTarget();

	// sample desc
	UINT						m_sampleCount;
	UINT						m_sampleQuality;
#ifdef USE_11ON12_WRAPPER
#else
	int							m_Height, m_Width;

	IDXGISwapChain*				m_pDXGISwapChain;
	ID3D11Texture2D*			m_pD3D11BackBuffer;
	ID3D11Texture2D*			m_pD3D11DepthBuffer;
	ID3D11RenderTargetView*		m_pD3D11RenderTargetView;
	ID3D11DepthStencilView*		m_pD3D11DepthStencilView;
#endif // USE_11ON12_WRAPPER

	CDXUTDialogResourceManager*	m_pDialogResourceManager;
	CDXUTTextHelper*			m_pTextHelper;

private:
	void Free();
	void FreeBuffer();

};

