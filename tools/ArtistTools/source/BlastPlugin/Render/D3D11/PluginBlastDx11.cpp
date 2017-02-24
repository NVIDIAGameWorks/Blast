#include "PluginBlastDx11.h"
#include "D3D11RenderShader.h"
#include "D3D11RenderInterface.h"
#include "LightShaderParam.h"

PluginBlast* CreateRenderBlast(void)
{
	return new PluginBlastDx11;
}

PluginBlastDx11::PluginBlastDx11()
{
}

PluginBlastDx11::~PluginBlastDx11()
{
}

// D3D11Shaders
bool PluginBlastDx11::D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap)
{
	return true;
}