#include "PluginBlastDx12.h"

PluginBlast* CreateRenderBlast(void)
{
	return new PluginBlastDx12;
}

PluginBlastDx12::PluginBlastDx12()
{
}

PluginBlastDx12::~PluginBlastDx12()
{
}

// D3D11Shaders
bool PluginBlastDx12::D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap)
{
	return true;
}