#pragma once

#include "PluginBlast.h"

extern "C" PLUGINBTRENDER_EXPORT PluginBlast* CreateRenderBlast(void);

class PluginBlastDx11 : public PluginBlast
{
public:
	PluginBlastDx11();
	~PluginBlastDx11();

	// D3D11Shaders
	virtual bool D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap);
};

