#pragma once

#include "RenderPlugin.h"
#include "blastplugin_global.h"

class PLUGINBT_EXPORT PluginBlast
{
public:
	static bool Create(std::string api);
	static PluginBlast* Instance();
	static void Destroy();

	~PluginBlast();

	// D3D11Shaders
	virtual bool D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap) = 0;
	
protected:
	PluginBlast();
};

