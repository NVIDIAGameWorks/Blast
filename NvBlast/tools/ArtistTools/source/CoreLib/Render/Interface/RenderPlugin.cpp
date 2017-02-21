#include "RenderPlugin.h"

#include <io.h>
#include <stdio.h>
#include <stdlib.h>

RenderPlugin* g_Plugin = nullptr;

RenderPlugin::RenderPlugin()
{
	m_RenderApi = "";
}

typedef RenderPlugin*(*Func)(void);

bool RenderPlugin::Load(std::vector<std::string>& render_plugins)
{
	if (render_plugins.size() == 0)
	{
		return false;
	}

	std::vector<std::string>::iterator it;
	std::string pluginDll = "";	
	HMODULE module = NULL;
	Func CreateFunc = NULL;
	bool loaded = false;
	for (it = render_plugins.begin(); it != render_plugins.end(); it++)
	{
#ifdef NV_ARTISTTOOLS
		pluginDll = "RenderPlugin";
#else
		pluginDll = "FurRender";
#endif

		pluginDll.append(*it);

#ifdef _WIN64
		pluginDll.append(".win64");
#else						  
		pluginDll.append(".win32");
#endif

#ifdef _DEBUG
		pluginDll.append(".d");
#else						  
#endif

		pluginDll.append(".dll");

		module = LoadLibraryA(pluginDll.c_str());
		if (NULL == module)
			return false;

		CreateFunc = (Func)GetProcAddress(module, "CreateRenderPlugin");
		if (NULL == CreateFunc)
			return false;

		g_Plugin = CreateFunc();
		if (NULL != g_Plugin)
		{
			loaded = true;
			break;
		}
	}
	return loaded;
}

RenderPlugin::~RenderPlugin()
{
}


RenderPlugin* RenderPlugin::Instance()
{
	return g_Plugin;
}

void RenderPlugin::Destroy()
{
	if (nullptr == g_Plugin)
		return;

	delete g_Plugin;
	g_Plugin = nullptr;
}
