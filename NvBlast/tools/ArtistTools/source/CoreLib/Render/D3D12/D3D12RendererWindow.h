#pragma once

#include <d3d12.h>
class D3D12RenderContext;
// DXUT stuffs for text rendering
class CDXUTDialogResourceManager;
class CDXUTTextHelper;

struct D3D12RenderWindow
{
public:
	D3D12RenderWindow();
	~D3D12RenderWindow();

	bool Create(HWND hWnd, unsigned int nSamples = 1);
	bool Resize(int w, int h);
	void Present();
	void Clear(float r, float g, float b);

	CDXUTDialogResourceManager*	m_pDialogResourceManager;
	CDXUTTextHelper*			m_pTextHelper;

private:
	void Free();
	void FreeBuffer();

	D3D12RenderContext* m_pRenderContext;
};

