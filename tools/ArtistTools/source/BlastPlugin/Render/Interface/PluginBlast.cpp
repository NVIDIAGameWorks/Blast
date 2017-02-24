#include "PluginBlast.h"

PluginBlast* g_Plugin = nullptr;

PluginBlast::PluginBlast()
{
}

PluginBlast::~PluginBlast()
{
}

typedef PluginBlast*(*Func)(void);

bool PluginBlast::Create(std::string strApi)
{
	if ("" == strApi)
		return false;

	std::string pluginDll = "";
	HMODULE module = NULL;
	Func CreateFunc = NULL;

#ifdef NV_ARTISTTOOLS
	pluginDll = "RenderBlast";
#else
	pluginDll = "FurRender";
#endif

	pluginDll.append(strApi);

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

	CreateFunc = (Func)GetProcAddress(module, "CreateRenderBlast");
	if (NULL == CreateFunc)
		return false;

	g_Plugin = CreateFunc();
	return (NULL != g_Plugin);
}

PluginBlast* PluginBlast::Instance()
{
	return g_Plugin;
}

void PluginBlast::Destroy()
{
	if (nullptr == g_Plugin)
		return;

	delete g_Plugin;
	g_Plugin = nullptr;
}
