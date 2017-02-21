#pragma once

#include "PluginBlast.h"

extern "C" PLUGINBTRENDER_EXPORT PluginBlast* CreateRenderBlast(void);

class PluginBlastDx12 : public PluginBlast
{
public:
	PluginBlastDx12();
	~PluginBlastDx12();

	// D3D11Shaders
	virtual bool D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap);
};

