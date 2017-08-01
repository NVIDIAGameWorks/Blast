#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFileInfo>

#include "Nv.h"

#include "AppMainWindow.h"
#include "D3DWidget.h"
#include "SimpleScene.h"
#include "Automate.h"
#include "Settings.h"
#include "RenderInterface.h"
#include "GlobalSettings.h"

#ifndef NV_ARTISTTOOLS
#include "FurCharacter.h"
#include "PluginBlast.h"
#endif // NV_ARTISTTOOLS

#include "RenderPlugin.h"

void SetupStyles()
{
	//QFile styleFile( ":/AppMainWindow/ThemeDefault.qss" );
	QString appPath = QCoreApplication::applicationDirPath();
	QString themeFilePath = appPath + "/ArtistToolsTheme.qss";
	QFile styleFile;
	if (QFile::exists(themeFilePath))
	{
		styleFile.setFileName(themeFilePath);
	}
	else
	{
		styleFile.setFileName(":/AppMainWindow/ThemeDark.qss");
	}
	styleFile.open(QFile::ReadOnly);

	QString styleSheet(styleFile.readAll());
	QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
	qApp->setStyleSheet(styleSheet);
}

int ShowErrorMsg(int errorCode)
{
	const char* errorMsg = 0;
	switch (errorCode)
	{
	case 1:
		errorMsg = "Fail to create GPU device and context. "
			"Please make sure you are using an independant GPU and choose '3D App - Game Development' in the Global Settings page in its Control Panel.";
		break;
	case 2:
		errorMsg = "Fail to initialize all the shared shaders, resources, etc. "
			"Please make sure you are using an independant GPU and choose '3D App - Game Development' in the Global Settings page in its Control Panel.";
		break;
	case 3:
		errorMsg = "Fail to create render window, target, swap chains, etc. "
			"Please make sure you are using an independant GPU and choose '3D App - Game Development' in the Global Settings page in its Control Panel.";
		break;
	case 4:
		// Unable to load/start the dll.
		errorMsg = "Unable to load and/or start the Blast dll. "
			"Please make sure the appropriate hairworks dll is the same directory as the viewer executable.";
		break;
	default:
		errorMsg = "We need define one error message!";
	}
	QMessageBox messageBox;
	messageBox.information(0, "Error", errorMsg);
	return errorCode;
}

extern void CreateAppMainWindow();
extern void ReleaseAppMainWindow();

int RunApp(QApplication& app)
{
	CreateAppMainWindow();

	// resolution
	int windowWidth = 0;
	int windowHeight = 0;
	std::string resolution = AppSettings::Inst().GetOptionValue("User/Resolution")->Value.Enum;
	int width, height;
	sscanf(resolution.c_str(), "%dx%d", &windowWidth, &windowHeight);

	// aa sample count
	int sampleCount = 8;
	if (stricmp(AppSettings::Inst().GetOptionValue("User/AA")->Value.Enum, "2X") == 0)
	{
		sampleCount = 2;
	}
	else if (stricmp(AppSettings::Inst().GetOptionValue("User/AA")->Value.Enum, "4X") == 0)
	{
		sampleCount = 4;
	}
	else if (stricmp(AppSettings::Inst().GetOptionValue("User/AA")->Value.Enum, "8X") == 0)
	{
		sampleCount = 8;
	}
	else if (stricmp(AppSettings::Inst().GetOptionValue("User/AA")->Value.Enum, "Off") == 0)
	{
		sampleCount = 1;
	}

	// device id
	int deviceID = (AppSettings::Inst().GetOptionValue("User/Device")->Value.Int);
	if (deviceID < 0 || deviceID > 3)
	{
		deviceID = -1;  // when -1 to choose a good GPU
	}

	// backdoor connection mode
	int backdoor = 0;
	if (stricmp(AppSettings::Inst().GetOptionValue("User/Backdoor")->Value.String, "master") == 0)
	{
		backdoor = 1;
	}
	else if (stricmp(AppSettings::Inst().GetOptionValue("User/Backdoor")->Value.String, "slave") == 0)
	{
		backdoor = 2;
	}

	AppMainWindow::setConnectionMode(backdoor);

	SimpleScene::Inst();
#ifdef NV_ARTISTTOOLS
	CoreLib::Inst()->SimpleScene_SimpleScene();
#endif // NV_ARTISTTOOLS
	// initialize main window
	// HAIR-285 Viewer - new command line to start Viewer in full screen
	AppMainWindow::Inst().InitUI();
	if (windowWidth == 0)
		AppMainWindow::Inst().showFullScreen();
	else if (windowWidth<0)
		AppMainWindow::Inst().showMaximized();
	else
		AppMainWindow::Inst().resize(windowWidth, windowHeight);

	D3DWidget* d3dWidget = AppMainWindow::Inst().GetRenderWidget();

	HWND hWidget = (HWND)d3dWidget->winId();

	// create GPU device and context
	if (false == RenderInterface::InitDevice(deviceID))
		return ShowErrorMsg(1);

	// initialize all the shared shaders, resources, etc
	if (false == RenderInterface::Initialize())
		return ShowErrorMsg(2);

	// create render window, target, swap chains, etc.
	if (false == RenderInterface::CreateRenderWindow(hWidget, sampleCount))
		return ShowErrorMsg(3);

	SimpleScene* scene = SimpleScene::Inst();
	if (false == scene->Initialize(hWidget, backdoor))
		return ShowErrorMsg(4);

	GlobalSettings::Inst().setRenderFps(60);

	GlobalSettings::GetFrameTimer().start();

	QObject::connect(&GlobalSettings::GetFrameTimer(), SIGNAL(timeout()), d3dWidget, SLOT(Timeout()));
	QObject::connect(&app, SIGNAL(aboutToQuit()), d3dWidget, SLOT(Shutdown()));

	SetupStyles();

	AppMainWindow::Inst().show();
	AppMainWindow::Inst().update();

	std::string title = "";

#ifdef NV_ARTISTTOOLS
	title = "ArtistTools";
#else
	title = "Blast Viewer";
#endif // NV_ARTISTTOOLS

#ifdef _WIN64
	title.append(" x64");
#endif

#ifdef _DEBUG
	title.append(" (Debug)");
#else			
#endif

	std::string strApi = RenderPlugin::Instance()->GetRenderApi();
	title.append(" ");
	title.append(strApi);

	switch (backdoor)
	{
	case 1:
		title.append(" (Master Mode)");
		break;
	case 2:
		title.append(" (Slave Mode)");
		break;
	}

	AppMainWindow::Inst().setWindowTitle(title.c_str());

	OptionValue* option = AppSettings::Inst().GetOptionValue("User/HideUI");
	if (option)
	{
		if (option->Value.Bool == OA_TRUE)
		{
			AppMainWindow::Inst().shortcut_expert();
		}
	}

#ifndef NV_ARTISTTOOLS
	option = AppSettings::Inst().GetOptionValue("User/FurAssetPath");
	if (option && option->Value.String && strlen(option->Value.String))
	{
		const char* path = option->Value.String;
		QFileInfo fi(path);
		QByteArray dir = QDir::toNativeSeparators(fi.absoluteDir().absolutePath()).toLocal8Bit();
		QByteArray file = fi.fileName().toLocal8Bit();
		scene->GetFurCharacter().LoadHairAsset(dir, file);
	}
#else
	CoreLib::Inst()->CoreLib_RunApp();
#endif // NV_ARTISTTOOLS

	option = AppSettings::Inst().GetOptionValue("User/ProjectPath");
	if (option && option->Value.String && strlen(option->Value.String))
	{
		const char* path = option->Value.String;
		QFileInfo fi(path);
		QByteArray dir = QDir::toNativeSeparators(fi.absoluteDir().absolutePath()).toLocal8Bit();
		QByteArray file = fi.fileName().toLocal8Bit();
		scene->LoadProject(dir, file);
	}
	option = AppSettings::Inst().GetOptionValue("User/Play");
	if (option)
	{
		if (option->Value.Bool == OA_TRUE)
		{
			AppMainWindow::Inst().shortcut_pause();
		}
	}

	int ret = app.exec();
	// release GUI before releasing QApplication (and QWindowsContext) to avoid crash. 
	// e.g. GWDCC-415 Blast Viewer crashes when quit after clicked any main menu.
	ReleaseAppMainWindow();
	return ret;
}

static void ShowUsage()
{
	char message[] = {
		"FurViewer Command-line Options:\n\n"
		"Usage: FurViewer.[win64|win32].exe [file] [options]...\n\n"
		"[file]: \n"
		"\t: File to load. .blastProj, .apx or .apb are supported.\n\t ex) media/Ball/ball.blastProj\n\n"
		"[options]:\n"
		"-size <width>x<height>\n\t: Window resolution\t  ex) -size 1024x768\n"
		"-aa <1|2|4|8>\n\t: MSAA anti-aliasing options. 1 for off.\t  ex) -aa 4 or -aa 1\n"
		"-backdoor <master|slave>\n\t: Backdoor options\n"
		"-device <0|1|2|3>\n\t: Which GPU to use\n"
		"-noui \n\t: Set this option to hide UI\n"
		"-play \n\t: Play the default animation if it has\n"
		"-fullscreen \n\t: Start the application in full screen\n"
		"-maxscreen \n\t: Start the application in max sized screen\n"
		"-demoMode playlistname.plist \n\t: Start the application and demo the projects in a fur playlist stored in media/playlists folder.\n"
	};
	QMessageBox messageBox;
	messageBox.information(0, "Usage", message);
}

bool ParseCommandLineOptions(int argc, char* argv[])
{
	AppSettings& settings = AppSettings::Inst();

	for (int idx = 1; idx < argc; ++idx)
	{
		if (!stricmp(argv[idx], "-size"))
		{
			if ((idx + 1) < argc)
			{
				oaValue value;
				value.Enum = argv[idx + 1];
				settings.SetOptionValue("User/Resolution", OA_TYPE_ENUM, &value);
				idx++; // Skip next option
			}
		}
		else if (!stricmp(argv[idx], "-fullscreen"))
		{
			// HAIR-285 Viewer - new command line to start Viewer in full screen
			oaValue value;
			value.Enum = "0x0";
			settings.SetOptionValue("User/Resolution", OA_TYPE_ENUM, &value);;
		}
		else if (!stricmp(argv[idx], "-maxscreen"))
		{
			// HAIR-285 Viewer - new command line to start Viewer in full screen
			oaValue value;
			value.Enum = "-1x0";
			settings.SetOptionValue("User/Resolution", OA_TYPE_ENUM, &value);;
		}
		else if (!stricmp(argv[idx], "-backdoor"))
		{
			if ((idx + 1) < argc)
			{
				oaValue value;
				bool isValid = true;
				if (!strcmp(argv[idx + 1], "master"))
					value.String = "master";
				else if (!strcmp(argv[idx + 1], "slave"))
					value.String = "slave";
				else
					isValid = false;

				if (isValid)
					settings.SetOptionValue("User/Backdoor", OA_TYPE_STRING, &value);
				idx++; // Skip next option
			}
		}
		else if (!stricmp(argv[idx], "-aa"))
		{
			if ((idx + 1) < argc)
			{
				oaValue value;
				bool isValid = true;
				if (!strcmp(argv[idx + 1], "1"))
					value.Enum = "Off";
				else if (!strcmp(argv[idx + 1], "2"))
					value.Enum = "2X";
				else if (!strcmp(argv[idx + 1], "4"))
					value.Enum = "4X";
				else if (!strcmp(argv[idx + 1], "8"))
					value.Enum = "8X";
				else
					isValid = false;
				if (isValid)
					settings.SetOptionValue("User/AA", OA_TYPE_ENUM, &value);
				idx++; // Skip next option
			}
		}
		else if (!stricmp(argv[idx], "-device"))
		{
			if ((idx + 1) < argc)
			{
				oaValue value;
				bool isValid = true;
				if (!strcmp(argv[idx + 1], "0"))
					value.Int = 0;
				else if (!strcmp(argv[idx + 1], "1"))
					value.Int = 1;
				else if (!strcmp(argv[idx + 1], "2"))
					value.Int = 2;
				else if (!strcmp(argv[idx + 1], "3"))
					value.Int = 3;
				else
					isValid = false;
				if (isValid)
					settings.SetOptionValue("User/Device", OA_TYPE_INT, &value);
				idx++; // Skip next option
			}
		}
		else if (!stricmp(argv[idx], "-noui"))
		{
			oaValue value;
			value.Bool = OA_TRUE;
			settings.SetOptionValue("User/HideUI", OA_TYPE_BOOL, &value);
		}
		else if (!stricmp(argv[idx], "-perf"))
		{
			oaValue value;
			value.Bool = OA_TRUE;
			settings.SetOptionValue("User/PerfMode", OA_TYPE_BOOL, &value);
		}
		else if (!stricmp(argv[idx], "-play"))
		{
			oaValue value;
			value.Bool = OA_TRUE;
			settings.SetOptionValue("User/Play", OA_TYPE_BOOL, &value);
		}
		else if (!stricmp(argv[idx], "-install"))
		{
			// Install mode
			// Just ignore here
		}
		else if (!stricmp(argv[idx], "-openautomate"))
		{
			// OpenAutomate mode
			// Just ignore here
			idx++; // Skip next option
		}
		else if (!stricmp(argv[idx], "-demoMode"))
		{
			//QMessageBox messageBox;
			//messageBox.information(0, "debug", argv[idx]);
			idx++; // check next arg
			if (idx < argc)
			{
				bool isValid = false;
				char* extension = strrchr(argv[idx], '.');
				if (extension)
				{
					if (!_stricmp(extension, ".plist"))
					{
						oaValue value;
						value.String = argv[idx];
						settings.SetOptionValue("User/FurDemoPlaylist", OA_TYPE_STRING, &value);
						isValid = true;
					}
				}
			}
		}
		else // Try to load the file
		{
			bool isValid = false;
			char* extension = strrchr(argv[idx], '.');
			if (extension)
			{
				if (!_stricmp(extension, ".blastProj"))
				{
					oaValue value;
					value.String = argv[idx];
					settings.SetOptionValue("User/ProjectPath", OA_TYPE_STRING, &value);
					isValid = true;
				}
				//else if (!_stricmp(extension, ".apx") || !_stricmp(extension, ".apb"))
				//{
				//	oaValue value;
				//	value.String = argv[idx];
				//	settings.SetOptionValue("User/FurAssetPath", OA_TYPE_STRING, &value);
				//	isValid = true;
				//}
			}
			if (!isValid)
			{
				// Invalid option
				return false;
			}
		}
	}
	return true;
}

typedef RenderPlugin*(*Func)(void);

void GetRenderPlugins(QApplication& app, std::vector<std::string>& render_plugins)
{
	HMODULE module = NULL;
	Func CreateFunc = NULL;
	RenderPlugin* Plugin = NULL;

	QDir pluginsDir(app.applicationDirPath());
	foreach(QString fileName, pluginsDir.entryList(QDir::Files)) {
#ifdef NV_ARTISTTOOLS
		if (!fileName.startsWith("Render"))
#else
		if (!fileName.startsWith("FurRender"))
#endif
		{
			continue;
		}

#ifdef _WIN64
#ifdef _DEBUG
		if (!fileName.endsWith(".win64.d.dll"))
		{
			continue;
		}
#else						  
		if (!fileName.endsWith(".win64.dll"))
		{
			continue;
		}
#endif
#else
#ifdef _DEBUG
		if (!fileName.endsWith(".win32.d.dll"))
		{
			continue;
		}
#else
		if (!fileName.endsWith(".win32.dll"))
		{
			continue;
		}
#endif
#endif

		QByteArray tmp = pluginsDir.absoluteFilePath(fileName).toUtf8();
		const char* pName = tmp.data();		
		module = LoadLibraryA(pName);
		if (NULL == module)
			continue;

		CreateFunc = (Func)GetProcAddress(module, "CreateRenderPlugin");
		if (NULL == CreateFunc)
			continue;

		Plugin = CreateFunc();
		if (NULL != Plugin)
		{
			std::string name = Plugin->GetRenderApi();
			render_plugins.push_back(name);
			delete Plugin;
			Plugin = NULL;
		}
	}
}


#include "RenderPlugin.h"
#include "CoreLib.h"

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#define LINE 10

#ifndef NV_ARTISTTOOLS
int CoreLib::CoreMain(int argc, char *argv[])
#else
int CoreLib::CoreMain(int argc, char *argv[], bool withPlugin)
#endif
{
#ifdef OPEN_CONSOLE
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif

	QApplication a(argc, argv);

	AppSettings::Inst().InitOptions();

	if (!ParseCommandLineOptions(argc, argv))
	{
		// Invalid option. Show the usage  and quit
		ShowUsage();
		return 0;
	}
#if (0) // Install mode for OpenAutomate is not tested yet
	bool isAutomateInstallMode = GetAutomateInstallModeOption(argc, argv);
	if (isAutomateInstallMode)
	{
		if (AutomateInstallApp())
		{
			QMessageBox messageBox;
			messageBox.information(0, "Info", "Install succeeded");
		}
		else
		{
			QMessageBox messageBox;
			messageBox.information(0, "Info", "Install failed. Did you run with admin privilege?");
		}
		return 0;
	}
#endif
	std::string openAutomateOptions = GetAutomateOption(argc, argv);
	if (!openAutomateOptions.empty())
	{
		AutomateInit(openAutomateOptions.c_str());
	}

	std::vector<std::string> render_plugins;
	std::vector<std::string>::iterator it;

	FILE *pf;
	if ((_access("RenderPlugin.txt", 0)) == -1)
	{
		GetRenderPlugins(a, render_plugins);

		if (render_plugins.size() > 1)
		{
			pf = fopen("RenderPlugin.txt", "a");
			for (it = render_plugins.begin(); it != render_plugins.end(); it++)
			{
				fprintf(pf, "%s\n", (*it).c_str());
			}
			fclose(pf);
		}
	}
	else
	{
		pf = fopen("RenderPlugin.txt", "r");
		char buf[LINE];
		while (1) {
			memset(buf, 0, LINE);
			if (fscanf(pf, "%s", buf) == -1)
				break;
			render_plugins.push_back(buf);
		}
		fclose(pf);
	}

	RenderPlugin::Load(render_plugins);

	std::string strApi = RenderPlugin::Instance()->GetRenderApi();
	if ("" == strApi)
	{
		return -1;
	}

#ifdef NV_ARTISTTOOLS
	if (withPlugin)
	{
		LoadPlugins(a);
	}
#else
	bool loaded = PluginBlast::Create(strApi);
	if (!loaded)
	{
		return -1;
	}
#endif

	return RunApp(a);
}

CoreLib::CoreLib()
{

}

CoreLib::~CoreLib()
{

}

CoreLib* CoreLib::Inst()
{
	static CoreLib coreLib;
	return &coreLib;
}

#ifdef NV_ARTISTTOOLS
#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFileInfo>

#include "Nv.h"

#include "AppMainWindow.h"
#include "D3DWidget.h"
#include "SimpleScene.h"
#include "Automate.h"
#include "Settings.h"
#include "RenderInterface.h"
#include "Light.h"

#include <QtCore/QPluginLoader> 
void CoreLib::LoadPlugins(QApplication& app)
{
	QDir pluginsDir(app.applicationDirPath());
	foreach(QString fileName, pluginsDir.entryList(QDir::Files)) {
		if (!fileName.startsWith("Plugin"))
		{
			continue;
		}

#ifdef _WIN64
#ifdef _DEBUG
		if (!fileName.endsWith(".win64.d.dll"))
		{
			continue;
		}
#else						  
		if (!fileName.endsWith(".win64.dll"))
		{
			continue;
		}
#endif
#else
#ifdef _DEBUG
		if (!fileName.endsWith(".win32.d.dll"))
		{
			continue;
		}
#else
		if (!fileName.endsWith(".win32.dll"))
		{
			continue;
		}
#endif
#endif

		PluginInterface* pInterface;
		QByteArray tmp = pluginsDir.absoluteFilePath(fileName).toUtf8();
		const char* pName = tmp.data();
		QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
		QObject *plugin = pluginLoader.instance();
		if (plugin) {
			pInterface = qobject_cast<PluginInterface *>(plugin);
			if (pInterface)
			{
				std::string strApi = RenderPlugin::Instance()->GetRenderApi();
				bool loaded = pInterface->LoadRenderPlugin(strApi);
				if (!loaded)
				{
					continue;
				}

				QString name = pInterface->GetPluginName();
				m_PluginInterfaces[name] = pInterface;
			}
		}
		else
		{
			QMessageBox::critical(nullptr, "", pluginLoader.errorString());
		}
	}
}

PluginInterface* CoreLib::GetPluginInterface(QString name)
{
	PluginInterface* pPluginInterface = 0;
	std::map<QString, PluginInterface*>::iterator it;
	it = m_PluginInterfaces.find(name);
	if (it != m_PluginInterfaces.end())
	{
		pPluginInterface = it->second;
	}
	return pPluginInterface;
}

bool CoreLib::GetBoneNames(std::vector<std::string>& BoneNames)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->GetBoneNames(BoneNames))
			break;
	}
	return true;
}

bool CoreLib::CoreLib_RunApp()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->CoreLib_RunApp())
			break;
	}

	return true;
}

bool CoreLib::MainToolbar_updateValues()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->MainToolbar_updateValues())
			break;
	}
	return true;
}

bool CoreLib::CurveEditor_updateValues(int _paramId, float* _values)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->CurveEditor_updateValues(_paramId, _values))
			break;
	}
	return true;
}
bool CoreLib::CurveEditor_onUpdateValues(int _paramId, float* _values)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->CurveEditor_onUpdateValues(_paramId, _values))
			break;
	}
	return true;
}

bool CoreLib::DisplayMeshesPanel_updateValues()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->DisplayMeshesPanel_updateValues())
			break;
	}
	return true;
}
bool CoreLib::DisplayMeshesPanel_EmitToggleSignal(unsigned int id, bool visible)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->DisplayMeshesPanel_EmitToggleSignal(id, visible))
			break;
	}
	return true;
}

bool CoreLib::Camera_LoadParameters(void* ptr, Camera* pCamera)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->Camera_LoadParameters(ptr, pCamera))
			break;
	}
	return true;
}
bool CoreLib::Camera_SaveParameters(void* ptr, Camera* pCamera)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->Camera_SaveParameters(ptr, pCamera))
			break;
	}
	return true;
}

bool CoreLib::Light_loadParameters(NvParameterized::Handle& handle, Light* pLight)
{
 	std::map<QString, PluginInterface*>::iterator it;
 	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
 	{
 		if (!(it->second)->Light_loadParameters(handle, pLight))
 			break;
 	}
	return true;
}

bool CoreLib::Light_saveParameters(NvParameterized::Handle& handle, Light* pLight)
{
 	std::map<QString, PluginInterface*>::iterator it;
 	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
 	{
 		if (!(it->second)->Light_saveParameters(handle, pLight))
 			break;
 	}
	return true;
}

bool CoreLib::Gamepad_ToggleSimulation()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->Gamepad_ToggleSimulation())
			break;
	}
	return true;
}
bool CoreLib::Gamepad_LoadSamples(QString fn)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->Gamepad_LoadSamples(fn))
			break;
	}
	return true;
}
bool CoreLib::Gamepad_ResetScene()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->Gamepad_ResetScene())
			break;
	}
	return true;
}

bool CoreLib::Gamepad_PlaySample()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->Gamepad_PlaySample())
			break;
	}
	return true;
}

bool CoreLib::GamepadHandler_ShowHair()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->GamepadHandler_ShowHair())
			break;
	}
	return true;
}

bool CoreLib::GamepadHandler_SpinWindStrength(float windStrength)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->GamepadHandler_SpinWindStrength(windStrength))
			break;
	}
	return true;
}

bool CoreLib::Gamepad_ResetAnimation()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->Gamepad_ResetAnimation())
			break;
	}
	return true;
}

bool CoreLib::Gamepad_PlayPauseAnimation()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->Gamepad_PlayPauseAnimation())
			break;
	}
	return true;
}

bool CoreLib::SimpleScene_SimpleScene()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_SimpleScene())
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_Initialize(int backdoor)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_Initialize(backdoor))
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_Shutdown()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_Shutdown())
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_Clear()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_Clear())
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_Draw_DX12()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_Draw_DX12())
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_Draw_DX11()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_Draw_DX11())
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_FitCamera(atcore_float3& center, atcore_float3& extents)
{
	bool valid = true;
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_FitCamera(center, extents))
		{
			valid = false;
			break;
		}
	}
	return valid;
}
bool CoreLib::SimpleScene_UpdateCamera()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_UpdateCamera())
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_DrawGround()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_DrawGround())
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_DrawWind()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_DrawWind())
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_DrawAxis()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_DrawAxis())
			break;
	}
	return true;
}
void CoreLib::SimpleScene_OpenFilesByDrop(const QStringList& fileNames)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		(it->second)->SimpleScene_OpenFilesByDrop(fileNames);
	}
}
bool CoreLib::SimpleScene_LoadSceneFromFbx(const char* dir, const char* fbxName)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_LoadSceneFromFbx(dir, fbxName))
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_LoadProject(const char* dir, const char* file)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_LoadProject(dir, file))
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_SaveProject(const char* dir, const char* file)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_SaveProject(dir, file))
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_LoadParameters(NvParameterized::Interface* iface)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_LoadParameters(iface))
			break;
	}
	return true;
}
bool CoreLib::SimpleScene_SaveParameters(NvParameterized::Interface* iface)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_SaveParameters(iface))
			break;
	}
	return true;
}

bool CoreLib::SimpleScene_LoadCameraBookmarks(NvParameterized::Interface* iface)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_LoadCameraBookmarks(iface))
			break;
	}
	return true;
}

bool CoreLib::SimpleScene_SaveCameraBookmarks(NvParameterized::Interface* iface)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->SimpleScene_SaveCameraBookmarks(iface))
			break;
	}
	return true;
}

bool CoreLib::D3DWidget_resizeEvent(QResizeEvent* e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_resizeEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_paintEvent(QPaintEvent* e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_paintEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_mousePressEvent(QMouseEvent* e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_mousePressEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_mouseReleaseEvent(QMouseEvent* e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_mouseReleaseEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_mouseMoveEvent(QMouseEvent* e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_mouseMoveEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_wheelEvent(QWheelEvent * e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_wheelEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_keyPressEvent(QKeyEvent* e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_keyPressEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_keyReleaseEvent(QKeyEvent* e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_keyReleaseEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_dragEnterEvent(QDragEnterEvent *e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_dragEnterEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_dragMoveEvent(QDragMoveEvent *e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_dragMoveEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_dragLeaveEvent(QDragLeaveEvent *e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_dragLeaveEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_dropEvent(QDropEvent *e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_dropEvent(e))
			break;
	}
	return true;
}
bool CoreLib::D3DWidget_contextMenuEvent(QContextMenuEvent *e)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3DWidget_contextMenuEvent(e))
			break;
	}
	return true;
}

#ifdef NV_ARTISTTOOLS
bool CoreLib::D3D11Shaders_InitializeShadersD3D11(std::map<int, D3D11RenderShader*>& ShaderMap)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->D3D11Shaders_InitializeShadersD3D11(ShaderMap))
			break;
	}
	return true;
}
#endif // NV_ARTISTTOOLS

bool CoreLib::AppMainWindow_AppMainWindow()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_AppMainWindow())
			break;
	}
	return true;
}
bool CoreLib::AppMainWindow_InitMenuItems(QMenuBar* pMenuBar)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_InitMenuItems(pMenuBar))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_InitMainTab(QWidget *displayScrollAreaContents, QVBoxLayout *displayScrollAreaLayout, int idx)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_InitMainTab(displayScrollAreaContents, displayScrollAreaLayout, idx))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_InitPluginTab(QTabWidget* pTabWidget)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_InitPluginTab(pTabWidget))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_InitUI()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_InitUI())
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_updateUI()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_updateUI())
			break;
	}
	return true;
}
bool CoreLib::AppMainWindow_updatePluginUI()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_updatePluginUI())
			break;
	}
	return true;
}
bool CoreLib::AppMainWindow_processDragAndDrop(QString fname)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_processDragAndDrop(fname))
			break;
	}
	return true;
}
bool CoreLib::AppMainWindow_closeEvent(QCloseEvent *event)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_closeEvent(event))
			break;
	}
	return true;
}


bool CoreLib::menu_item_triggered(QAction* action)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_menu_item_triggered(action))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_menu_about()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_menu_about())
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_menu_opendoc()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_menu_opendoc())
			break;
	}
	return true;
}
#if USE_CURVE_EDITOR
bool CoreLib::AppMainWindow_UpdateCurveEditor()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_UpdateCurveEditor())
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_ShowCurveEditor(int paramId)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_ShowCurveEditor(paramId))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_onCurveAttributeChanged(attribute))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_onColorAttributeChanged(attribute))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_onReloadColorAttributeTexture(attribute, reloadColorTex, selectedCtrlPntIndex))
			break;
	}
	return true;
}
#endif
bool CoreLib::AppMainWindow_InitToolbar(QWidget *pQWidget, QVBoxLayout* pLayout)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_InitToolbar(pQWidget, pLayout))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_shortcut_expert(bool mode)
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_shortcut_expert(mode))
			break;
	}
	return true;
}

bool CoreLib::AppMainWindow_updateMainToolbar()
{
	std::map<QString, PluginInterface*>::iterator it;
	for (it = m_PluginInterfaces.begin(); it != m_PluginInterfaces.end(); it++)
	{
		if (!(it->second)->AppMainWindow_updateMainToolbar())
			break;
	}
	return true;
}

#endif