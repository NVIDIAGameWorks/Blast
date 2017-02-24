#include <windows.h>
#include <SDL.h>
#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QString>
#include <QtCore/QList>

#include "Gamepad.h"

#include "Camera.h"
#include "SimpleScene.h"
#include "AppMainWindow.h"
#include "DisplayLightPanel.h"
#include "DisplayScenePanel.h"

#include "DXUT.h"
//#include "D3D11RendererWindow.h"
#include "sdkmisc.h"
#include "GlobalSettings.h"
#include "Light.h"
#include "DisplayPreferencesPanel.h"
#include "ViewerOutput.h"
#include "D3DWidget.h"
#include <stdarg.h>
#include <vector>
#include <string>

#ifndef NV_ARTISTTOOLS
#include "BlastToolbar.h"
#include "DisplayVisualizersPanel.h"
#include "FurCharacter.h"
#include "HairParams.h"
#endif // NV_ARTISTTOOLS

const bool bSaveLogToDisk = true;
bool bDefaultDemoMode = false;

static QList<QString> demoProjects;

static int curentPrjIdx = -1;
static int iQuitState = 0;

static std::string strAppDir;

SimpleScene* theScene = NV_NULL;

bool bUseOldLightCam = false, bUseOldSceneCam = false;

enum ButtonState
{
	BUTTON_NOTHING,
	BUTTON_DOWN,
	BUTTON_HOLDING,
	BUTTON_UP,
};

class GamepadHandler
{
public:
	GamepadHandler()
		:joystick(0)
	{
	}

	~GamepadHandler()
	{
		DisConnect();
	}

	void DisConnect()
	{
		if(joystick)
		{
			SDL_JoystickClose(joystick);
			joystick = NV_NULL;
		}
	}

	void Process();

	inline ButtonState ButtonState(int button)
	{
		if (button >= 0 && button < iButtonState.size())
		{
			if (iButtonState[button] && iButtonStateOld[button] == 0)
				return BUTTON_DOWN;
			else if (iButtonState[button] && iButtonStateOld[button])
				return BUTTON_HOLDING;
			else if (iButtonState[button] == 0 && iButtonStateOld[button])
				return BUTTON_UP;
		}
		return BUTTON_NOTHING;
	}

	inline int AxisChange(int idx)
	{
		if (idx >= 0 && idx < iAxisState.size())
		{
			return (iAxisState[idx] - iAxisStateAtBegin[idx]);
		}
		return 0;
	}

	const char* pjsName;
	int numAxis;
	int numTrackballs;
	int numPov;
	int numButtons;

	std::vector<int> bKeyHolding; 
	std::vector<int> iAxisStateAtBegin;
	std::vector<int> iAxisStateOld;
	std::vector<int> iAxisState;
	std::vector<int> iButtonStateOld;
	std::vector<int> iButtonState;

	SDL_Joystick* joystick;
	SDL_JoystickGUID jsGuid;
};

std::vector<GamepadHandler> gHandlers;

//
// Gamepad thresholds taken from XINPUT API
//
//#define GAMEPAD_LEFT_THUMB_DEADZONE  7849
//#define GAMEPAD_RIGHT_THUMB_DEADZONE  8689
//#define GAMEPAD_TRIGGER_THRESHOLD    30

#define GAMEPAD_BUMPER_DEADZONE  9000
inline int ClampAxis(int value)
{
	if((value>GAMEPAD_BUMPER_DEADZONE) || (value<-GAMEPAD_BUMPER_DEADZONE))
		return value;
	return 0;
}

std::vector<std::string> keylog;

void LogAdd(const char *fmt,...)
{
	if(bSaveLogToDisk)
	{
		static char logBuf[2048];
		va_list arg;
		va_start( arg, fmt );
		vsprintf(logBuf, fmt, arg);
		va_end(arg);
		keylog.push_back(logBuf);
	}
}

void SaveLog()
{
	if(keylog.size())
	{
		std::string fn = strAppDir + "\\buttonLog.txt";
		FILE* fp = fopen(fn.c_str(), "w+");
		if(fp)
		{
			for(int i = 0; i < keylog.size(); ++i)
			{
				std::string& info = keylog[i];
				fputs(info.c_str(), fp);
				fputs("\n", fp);
			}
			fclose(fp);
		}
	}
}

//XBOX 360 AXIS
static int	AXIS_LS_X = 0;
static int	AXIS_LS_Y = 1;
static int	AXIS_RS_X = 2;
static int	AXIS_RS_Y = 3;
static int	AXIS_LT = 4;
static int	AXIS_RT = 5;

// XBox 360 Key codes
static int GAMEPAD_KEY_DPAD_UP = 0;
static int GAMEPAD_KEY_DPAD_DOWN = 1;
static int GAMEPAD_KEY_DPAD_LEFT = 2;
static int GAMEPAD_KEY_DPAD_RIGHT = 3;
static int GAMEPAD_KEY_START = 4;
static int GAMEPAD_KEY_BACK = 5;
static int GAMEPAD_KEY_LS = 6;
static int GAMEPAD_KEY_RS = 7;
static int GAMEPAD_KEY_LEFT_BUMP = 8;
static int GAMEPAD_KEY_RIGHT_BUMP = 9;
static int GAMEPAD_KEY_A = 10;
static int GAMEPAD_KEY_B = 11;
static int GAMEPAD_KEY_X = 12;
static int GAMEPAD_KEY_Y = 13;

double GetSeconds()
{
	static LARGE_INTEGER lastTime;
	static LARGE_INTEGER freq;
	static bool first = true;

	if (first)
	{	
		QueryPerformanceCounter(&lastTime);
		QueryPerformanceFrequency(&freq);

		first = false;
	}

	static double time = 0.0;

	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);

	__int64 delta = t.QuadPart-lastTime.QuadPart;
	double deltaSeconds = double(delta) / double(freq.QuadPart);

	time += deltaSeconds;

	lastTime = t;

	return time;

}

Gamepad::Gamepad()
{
}

Gamepad::~Gamepad()
{
}

Gamepad& Gamepad::Instance()
{
	static Gamepad gmpad;
	return gmpad;
}

void ChangeMode()
{
	AppMainWindow::Inst().shortcut_expert();
}

void ToggleSimulation()
{
#ifndef NV_ARTISTTOOLS
	BlastToolbar* toolbar = AppMainWindow::Inst().GetMainToolbar();
	toolbar->on_btnEnableSimulation_clicked();
	// read simulation state; have to call it twice to avoid change it.
	GlobalSettings::Inst().toggleSimulation();

	bool anim = GlobalSettings::Inst().toggleSimulation();
	DisplayFurVisualizersPanel*  furPanel = AppMainWindow::Inst().GetFurVisualizersPanel();
	if(furPanel)
	{
		furPanel->on_btnShowHair_stateChanged(anim);
		furPanel->updateValues();
	}
#else
	CoreLib::Inst()->Gamepad_ToggleSimulation();
#endif // NV_ARTISTTOOLS
}

void StartAnimation()
{
#ifndef NV_ARTISTTOOLS
	GlobalSettings::Inst().toggleSimulation();
	bool simulating = GlobalSettings::Inst().toggleSimulation();  // call it twice to get right state

	GlobalSettings::Inst().toggleAnimation();
	bool animating = GlobalSettings::Inst().toggleAnimation();  // call it twice to get right state

	BlastToolbar* toolbar = AppMainWindow::Inst().GetMainToolbar();
	if(!simulating)
		toolbar->on_btnEnableSimulation_clicked();

	if(!animating)
		toolbar->on_btnPlayAnimation_clicked();

	// reset animation
	//toolbar->on_btnResetAnimation_clicked();
#else
	CoreLib::Inst()->Gamepad_StartAnimation();
#endif // NV_ARTISTTOOLS
}

void ButtonAPressed()
{
	// Play/Pause the animation
	Gamepad::PlayPauseAnimation();
}

void ButtonBPressed()
{
	// Stops and re-starts animation
	Gamepad::ResetAnimation();
}

void Gamepad::ShowProjectName()
{
	bool bExpertMode = AppMainWindow::IsExpertMode();
	// need change title
	char msg[1024];
	// show title
	if(curentPrjIdx != -1)
	{
		QString fn = demoProjects.at(curentPrjIdx);
		sprintf(msg, "Blast Viewer - %s %s", fn.toUtf8().data(), (bExpertMode ? "Demo Mode" : ""));
		AppMainWindow::Inst().setWindowTitle(msg);
	}
	else
	{
		if (bExpertMode)
		{
			sprintf(msg, "Blast Viewer - Expert Mode");
		}
		else
		{
			sprintf(msg, "Blast Viewer");
		}
		AppMainWindow::Inst().setWindowTitle(msg);
	}
}

void LoadSamples(bool bNext)
{
	bUseOldLightCam = bUseOldSceneCam = false;
	int prjCount = demoProjects.size();
	if(prjCount==0)
	{
		DisplayPreferencesPanel*  pPanel = AppMainWindow::Inst().GetDisplayPreferencesPanel();
		if (pPanel)
		{
			pPanel->assignPlayPlaylistToGamepad();
		}
		if (0)
		{
			static int iLimitSampleCount = 20;
			QString appDir = strAppDir.c_str();
			QString projectPath;
			// load from a specific file first
			QString fn = appDir + "\\samples.txt";
			FILE* fp = fopen(fn.toUtf8().data(), "r");
			if (fp)
			{
				char Line[1024], FileName[2014];
				while (fgets(Line, sizeof(Line), fp) != NV_NULL)
				{
					QString t = QString(Line).trimmed();
					if (t.length() == 0)
						continue;
					QString fn = t.toLower();
					std::string strFN = fn.toUtf8().data();
					const char* ptest = strstr(strFN.c_str(), ".furproj");
					if (ptest)
					{
						const char* pchar = strchr(strFN.c_str(), ':');
						if (pchar)
						{
							demoProjects.append(fn);
						}
						else
						{
							const char* pFirst = strFN.c_str();
							if (pFirst[0] == '/' || pFirst[0] == '\\')
							{
								fn = appDir + fn;
								demoProjects.append(fn);
							}
							else
							{
								fn = appDir + QString("/") + fn;
								demoProjects.append(fn);
							}
						}
					}
				}
				fclose(fp);
			}
		}
		if(0)
		{
			QString appDir = strAppDir.c_str();
			QString projectPath;
			// search some relative folders
			QDir dirTmp(appDir);
			if(dirTmp.cd("./media"))
				projectPath = dirTmp.absolutePath();
			else if(dirTmp.cd("../media"))
				projectPath = dirTmp.absolutePath();
			else if(dirTmp.cd("../../media"))
				projectPath = dirTmp.absolutePath();
			else if(dirTmp.cd("../../media"))
				projectPath = dirTmp.absolutePath();
			if(!projectPath.isEmpty())
			{
				QStringList filters; 
				filters<<QString("*.furproj"); 
				QDirIterator dir_iterator(projectPath, filters, QDir::Files | QDir::NoSymLinks,QDirIterator::Subdirectories);
				while(dir_iterator.hasNext())
				{
					dir_iterator.next();
					QFileInfo file_info = dir_iterator.fileInfo();
					QString absolute_file_path = file_info.absoluteFilePath();
					demoProjects.append(absolute_file_path);
					//if(demoProjects.size()>iLimitSampleCount)
					//	break;
				}
			}
			else
			{
				const char* msg = "Fail to find any Blast projects!";
				viewer_msg(msg);
			}
		}
		prjCount = demoProjects.size();
	}
	if(prjCount)
	{
		if (bNext)
		{
			++curentPrjIdx;
			if (curentPrjIdx >= prjCount)
				curentPrjIdx = 0;
		}
		else
		{
			--curentPrjIdx;
			if (curentPrjIdx < 0)
				curentPrjIdx = prjCount - 1;
		}
		// load sample
		QString fn = demoProjects[curentPrjIdx];
		AppMainWindow::Inst().openProject(fn);
	}
	Gamepad::ShowProjectName();
}

void StartButtonPressed()
{
	Gamepad& gp = Gamepad::Instance();
	gp.ShowUsage();
}

void BackButtonPressed()
{
	Gamepad::DemoEscape();
	return;

	++iQuitState;
	switch(iQuitState)
	{
	case 0:
		// do nothing
		break;
	case 1:
		{
			AppMainWindow::Inst().startProgress();
			AppMainWindow::Inst().setProgressMaximum(2);
			AppMainWindow::Inst().setProgress("Press Back again to quit. Press other buttons to cancel it.", 1);
		}
		break;
	case 2:
		{
			AppMainWindow::Inst().setProgress("Quitting...", 2);
			//AppMainWindow::Inst().close();
			Gamepad::DemoEscape();
		}
		break;
	default:
		iQuitState = 0;
		break;
	}
}

void Gamepad::HandleJoySticks()
{
	int numJoysticks = SDL_NumJoysticks();
	int numUsedJoysticks = gHandlers.size();
	if (numUsedJoysticks != numJoysticks)
	{
		gHandlers.clear();
	}
	if(numUsedJoysticks < 1)
	{
		//static int iSkipCount = 0;
		//if (++iSkipCount >= 60)
		//{
		//	iSkipCount = 0;
		static int iDoOnce = 1;
		if (iDoOnce)
		{
			iDoOnce = 0;

			Initialize();
			numJoysticks = SDL_NumJoysticks();
			for (int i = 0; i < numJoysticks; ++i)
			{
				SDL_Joystick* joystick = SDL_JoystickOpen(i);
				if (joystick)
				{
					numUsedJoysticks = gHandlers.size();
					gHandlers.resize(numUsedJoysticks + 1);
					GamepadHandler& handler = gHandlers[numUsedJoysticks];
					handler.joystick = joystick;
					handler.pjsName = SDL_JoystickName(joystick);
					handler.numAxis = SDL_JoystickNumAxes(joystick);
					handler.numTrackballs = SDL_JoystickNumBalls(joystick);
					handler.numPov = SDL_JoystickNumHats(joystick);
					handler.numButtons = SDL_JoystickNumButtons(joystick);
					handler.jsGuid = SDL_JoystickGetGUID(joystick);

					LogAdd("Device Name: %s", handler.pjsName);
					LogAdd("Num of Axis: %d", handler.numAxis);
					LogAdd("Num of Trackballs: %d", handler.numTrackballs);
					LogAdd("Num of POV: %d", handler.numPov);
					LogAdd("Num of Buttons: %d", handler.numButtons);
					LogAdd("Initial Axis States:");

					handler.iAxisState.resize(handler.numAxis);
					handler.iAxisStateAtBegin.resize(handler.numAxis);
					handler.iAxisStateOld.resize(handler.numAxis);

					handler.iButtonState.resize(handler.numButtons);
					handler.iButtonStateOld.resize(handler.numButtons);

					if (handler.numAxis == 6)
					{
						// XBOX 360 has 6 axis
						AXIS_LS_X = 0;
						AXIS_LS_Y = 1;
						AXIS_RS_X = 2;
						AXIS_RS_Y = 3;
						AXIS_LT = 4;
						AXIS_RT = 5;

						GAMEPAD_KEY_DPAD_UP = 0;
						GAMEPAD_KEY_DPAD_DOWN = 1;
						GAMEPAD_KEY_DPAD_LEFT = 2;
						GAMEPAD_KEY_DPAD_RIGHT = 3;
						GAMEPAD_KEY_START = 4;
						GAMEPAD_KEY_BACK = 5;
						GAMEPAD_KEY_LS = 6;
						GAMEPAD_KEY_RS = 7;
						GAMEPAD_KEY_LEFT_BUMP = 8;
						GAMEPAD_KEY_RIGHT_BUMP = 9;
						GAMEPAD_KEY_A = 10;
						GAMEPAD_KEY_B = 11;
						GAMEPAD_KEY_X = 12;
						GAMEPAD_KEY_Y = 13;
					}
					else if (handler.numAxis == 5)
					{
						// Betop has 5 axis when xbox mode
						AXIS_LS_X = 0;
						AXIS_LS_Y = 1;
						AXIS_RS_X = 3;
						AXIS_RS_Y = 4;
						AXIS_LT = 2;
						AXIS_RT = 2;

						GAMEPAD_KEY_DPAD_UP = 10;    // POV
						GAMEPAD_KEY_DPAD_DOWN = 11;  // POV
						GAMEPAD_KEY_DPAD_LEFT = 12;  // POV
						GAMEPAD_KEY_DPAD_RIGHT = 13; // POV
						GAMEPAD_KEY_START = 7;
						GAMEPAD_KEY_BACK = 6;
						GAMEPAD_KEY_LS = 8;
						GAMEPAD_KEY_RS = 9;
						GAMEPAD_KEY_LEFT_BUMP = 4;
						GAMEPAD_KEY_RIGHT_BUMP = 5;
						GAMEPAD_KEY_A = 0;
						GAMEPAD_KEY_B = 1;
						GAMEPAD_KEY_X = 2;
						GAMEPAD_KEY_Y = 3;

						if (handler.numPov == 1)
						{
							// BETOP xbox mode
							handler.iButtonState.resize(handler.numButtons + 4);    // POV fake buttons
							handler.iButtonStateOld.resize(handler.numButtons + 4);
							for (int i = GAMEPAD_KEY_DPAD_UP; i <= GAMEPAD_KEY_DPAD_RIGHT; ++i)
							{
								handler.iButtonStateOld[i] = handler.iButtonState[i] = 0;
							}
						}
					}
					for (int i = 0; i < handler.numAxis; ++i)
					{
						int value = ClampAxis(SDL_JoystickGetAxis(handler.joystick, i));
						handler.iAxisStateAtBegin[i] = handler.iAxisStateOld[i] = handler.iAxisState[i] = value;
						LogAdd("Axis %d State %d", i, value);
					}
					LogAdd("Initial Button States:");
					for (int i = 0; i < handler.numButtons; ++i)
					{
						int state = SDL_JoystickGetButton(handler.joystick, i);
						handler.iButtonStateOld[i] = handler.iButtonState[i] = state;
						LogAdd("Button %d State %d", i, state);
					}
					if (handler.numPov == 1)
					{
						LogAdd("Initial POV Button States:");
						Uint8 pov = SDL_JoystickGetHat(joystick, 0);
						char msg[512];
						sprintf(msg, "POV State: %d", pov);
						viewer_msg(msg);
						LogAdd(msg);
						// BETOP xbox mode
						handler.iButtonState.resize(handler.numButtons + 4);    // POV fake buttons
						handler.iButtonStateOld.resize(handler.numButtons + 4);
						for (int i = GAMEPAD_KEY_DPAD_UP; i <= GAMEPAD_KEY_DPAD_RIGHT; ++i)
						{
							handler.iButtonStateOld[i] = handler.iButtonState[i] = 0;
							LogAdd("POV Button %d State %d", i, 0);
						}
					}
					SDL_JoystickEventState(SDL_IGNORE);
				}
			}
		}
	}
	else
	{
		ManualReadGamepad();
	}
}

bool Gamepad::Initialize()
{
	if (strAppDir.size() < 1)
	{
		strAppDir = qApp->applicationDirPath().toUtf8().data();
	}

	SDL_Quit();
	// init SDL
	// my Gamepad does not work with SDL_INIT_EVERYTHING. Just init the ones we use.
	int ret = SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
	return (ret == 0);
}

void Gamepad::Free()
{
	if (bSaveLogToDisk)
	{
		SaveLog();
	}

	if(int count = gHandlers.size())
	{
		for(int i = 0; i<count; ++i)
			gHandlers[i].DisConnect();
	}
	SDL_Quit();
}

void ShowUsageInViewport()
{
	bool bDemoMode = AppMainWindow::IsExpertMode();
	if(bDemoMode && bDefaultDemoMode)
	{
		int w = theScene->GetCamera()->GetWidth();
		int h = theScene->GetCamera()->GetHeight();
		//CDXUTTextHelper* pTxtHelper = theScene->GetRenderWindow()->GetTextHelper();

		//pTxtHelper->Begin();
		//pTxtHelper->SetInsertionPos(w/5, h-80);
		//pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 0.8f));

		//pTxtHelper->DrawTextLine(L"Start: Load a sample        Back: Quit Demo");
		//pTxtHelper->DrawTextLine(L"A: Stop/Restart Animation   B: Pause/Continue Animation    X: Start/Pause Simulation      Y: Demo/Export Switch");
		//pTxtHelper->DrawTextLine(L"Left Stick: Zoom In/Out     Right Stick: Control Camera    Cross/Trigger: Scene HUD and Stats On/Off");
		//pTxtHelper->DrawTextLine(L"RB & LS: Wind Direction     RB & RS: Wind Strength         LB & LS: Light Control");
		//pTxtHelper->End();
	}
}

void Gamepad::ShowUsage()
{
	viewer_msg("Start: Show Gamepad Usage");
	viewer_msg("Back: Quit the Blast Viewer");
	viewer_msg("LBump: Load previous demo            RBump: Load next sample");
	viewer_msg("B: Reset Animation                   A: Play/Pause Animation");
	viewer_msg("Y: Demo View On/Off                  X: Show/Hide the selected Hair");
	viewer_msg("LStick: Zoom in and out              RStick: Rotate the camera");
	viewer_msg("D-Pad Up: Show/Hide HUD              D-Pad Left: Show/Hide Stats");
	viewer_msg("RTrigger & LStick: Wind Direction    RTrigger & RStick: Wind Strength");
	viewer_msg("LTrigger & LStick: Move Light");
}


void Gamepad::ResetScene()
{
#ifndef NV_ARTISTTOOLS
	BlastToolbar* toolbar = AppMainWindow::Inst().GetMainToolbar();

	GlobalSettings::Inst().toggleSimulation();
	bool bSimulating = GlobalSettings::Inst().toggleSimulation();  // call it twice to get right state
	if(bSimulating)
	{
		toolbar->on_btnEnableSimulation_clicked(); // stop simulation when stop animation
	}

	DisplayFurVisualizersPanel*  furPanel = AppMainWindow::Inst().GetFurVisualizersPanel();
	if(furPanel)
	{
		furPanel->on_btnShowHair_stateChanged(true);   // show hair
		furPanel->updateValues();
	}

	GlobalSettings::Inst().toggleAnimation();
	bool anim = GlobalSettings::Inst().toggleAnimation();  // call it twice to get right state
	if(anim)
	{
		toolbar->on_btnPlayAnimation_clicked();   // stop animation
	}

	toolbar->on_btnResetAnimation_clicked();       // reset animation

	toolbar->on_spinWindStrength_valueChanged(0.0f);
	toolbar->updateValues();
#else
	CoreLib::Inst()->Gamepad_ResetScene();
#endif // NV_ARTISTTOOLS

	Light* pLight = NV_NULL;
	pLight = Light::GetFirstSelectedLight();
	
	static Camera oldLightCam, oldSceneCam;
	// restore old camera
	if(bUseOldSceneCam)
	{
		*(theScene->GetCamera()) = oldSceneCam;
		//theScene->FitCamera();
	}
	else
	{
		oldSceneCam = *theScene->GetCamera();
		bUseOldSceneCam = true;
	}
	// restore light camera
	if(pLight)
	{
		//pLight->FitBounds(true);
		if(bUseOldLightCam)
		{
//			pLight->getCamera() = oldLightCam;
		}
		else
		{
//			oldLightCam = pLight->getCamera();
//			pLight->FitBounds();
			bUseOldLightCam = true;
		}
	}
	theScene->SetProjectModified(true);
}

bool Gamepad::IsDemoMode()
{
	return AppMainWindow::IsExpertMode();
}

void Gamepad::SetDemoProjects(QList<QString>& projects)
{
	demoProjects = projects;
}

void Gamepad::Process()
{
	if (AppMainWindow::IsExpertMode() == false)
	{
		static bool bFirstTime = true;
		if (bFirstTime)
		{
			bFirstTime = false;
			DisplayPreferencesPanel*  pPanel = AppMainWindow::Inst().GetDisplayPreferencesPanel();
			if (pPanel)
			{
				pPanel->runDemoCommandline();
				return;
			}
		}
	}

	HandleJoySticks();

	if (theScene == 0)
	{
		theScene = SimpleScene::Inst();
		return;
	}

	//ShowUsageInViewport();
}


void Gamepad::ManualReadGamepad()
{
	SDL_JoystickUpdate();
	if(int count = gHandlers.size())
	{
		for(int i = 0; i < count; ++i)
			gHandlers[i].Process();
	}
}

void GamepadHandler::Process()
{
	for(int i = 0; i < numAxis; ++i)
	{
		int value = ClampAxis(SDL_JoystickGetAxis(joystick, i));
		iAxisState[i] = value;
		if(iAxisStateOld[i] != value)
		{
			char msg[512];
			sprintf(msg, "Axis changes: %d %d", i, value);
#ifdef _DEBUG
			viewer_msg(msg);
#endif
			LogAdd(msg);
		}
	}

	bool bCancelQuit = false;
	for(int i = 0; i < numButtons; ++i)
	{
		int state = SDL_JoystickGetButton(joystick, i);
		iButtonState[i] = state;
		if(iButtonStateOld[i] != state)
		{
			char msg[512];
			sprintf(msg, "Button changes: %d %d (%d)", i, state, iButtonStateOld[i]);
#ifdef _DEBUG
			viewer_msg(msg);
#endif
			LogAdd(msg);
		}
		if(i != GAMEPAD_KEY_BACK && ButtonState(i) == BUTTON_UP)
		{
			//bCancelQuit = true;
		}
	}

	if (numPov == 1)
	{
		// BETOP xbox mode
		Uint8 pov = SDL_JoystickGetHat(joystick, 0);
		switch (pov)
		{
		case SDL_HAT_UP:
			iButtonState[GAMEPAD_KEY_DPAD_UP] = 1;
			break;
		case SDL_HAT_RIGHT:
			iButtonState[GAMEPAD_KEY_DPAD_RIGHT] = 1;
			break;
		case SDL_HAT_DOWN:
			iButtonState[GAMEPAD_KEY_DPAD_DOWN] = 1;
			break;
		case SDL_HAT_LEFT:
			iButtonState[GAMEPAD_KEY_DPAD_LEFT] = 1;
			break;
		case SDL_HAT_RIGHTUP:
			iButtonState[GAMEPAD_KEY_DPAD_RIGHT] = 1;
			iButtonState[GAMEPAD_KEY_DPAD_UP] = 1;
			break;
		case SDL_HAT_RIGHTDOWN:
			iButtonState[GAMEPAD_KEY_DPAD_RIGHT] = 1;
			iButtonState[GAMEPAD_KEY_DPAD_DOWN] = 1;
			break;
		case SDL_HAT_LEFTUP:
			iButtonState[GAMEPAD_KEY_DPAD_LEFT] = 1;
			iButtonState[GAMEPAD_KEY_DPAD_UP] = 1;
			break;
		case SDL_HAT_LEFTDOWN:
			iButtonState[GAMEPAD_KEY_DPAD_LEFT] = 1;
			iButtonState[GAMEPAD_KEY_DPAD_DOWN] = 1;
			break;
		default:
			for (int i = GAMEPAD_KEY_DPAD_UP; i <= GAMEPAD_KEY_DPAD_RIGHT; ++i)
			{
				iButtonState[i] = 0;
			}
		}
		if (pov != SDL_HAT_CENTERED)
		{
			char msg[512];
			sprintf(msg, "POV State: %d", pov);
			viewer_msg(msg);
			LogAdd(msg);
			for (int i = GAMEPAD_KEY_DPAD_UP; i <= GAMEPAD_KEY_DPAD_RIGHT; ++i)
			{
				if (ButtonState(i) == BUTTON_DOWN)
				{
					sprintf(msg, "POV Button Down: %d", i);
					viewer_msg(msg);
					LogAdd(msg);
				}
			}
		}
	}

	//bool bLeftBump = iButtonState[GAMEPAD_KEY_LEFT_BUMP], bRightBump = iButtonState[GAMEPAD_KEY_RIGHT_BUMP];
	bool bLeftTrigger = false, bRightTrigger = false;
	double currentTime = GetSeconds();	// real elapsed frame time
	static double lastJoyTime = currentTime;

	if(ButtonState(GAMEPAD_KEY_BACK) == BUTTON_UP)
	{
		BackButtonPressed();
	}
	if(iQuitState && bCancelQuit)
	{
		AppMainWindow::Inst().setProgress("Cancel quitting.", 2);
		iQuitState = 0;
	}
	else
	{
		bCancelQuit = false;
	}

	if(!bCancelQuit)
	{
		if(ButtonState(GAMEPAD_KEY_START) == BUTTON_UP)
		{
			StartButtonPressed();
		}
		if (ButtonState(GAMEPAD_KEY_RIGHT_BUMP) == BUTTON_UP)
		{
			// next proj
			Gamepad::DemoNext();
		}
		if (ButtonState(GAMEPAD_KEY_LEFT_BUMP) == BUTTON_UP)
		{
			// prev proj
			Gamepad::DemoPrev();
		}
		if (ButtonState(GAMEPAD_KEY_B) == BUTTON_UP)
		{
			viewer_msg("B Pressed");
			ButtonBPressed();
		}
		if(ButtonState(GAMEPAD_KEY_A) == BUTTON_UP)
		{
			viewer_msg("A Pressed");
			ButtonAPressed();
		}
		if (ButtonState(GAMEPAD_KEY_Y) == BUTTON_UP)
		{
			viewer_msg("Y Pressed");
			ChangeMode();
		}
		if(ButtonState(GAMEPAD_KEY_X) == BUTTON_UP)
		{
			viewer_msg("X Pressed");
#ifndef NV_ARTISTTOOLS
			// Show/hide selected hair
			bool v = false;
			theScene->GetFurCharacter().GetHairParam(HAIR_PARAMS_DRAW_RENDER_HAIRS, &v);
			DisplayFurVisualizersPanel*  furPanel = AppMainWindow::Inst().GetFurVisualizersPanel();
			if (furPanel)
			{
				furPanel->on_btnShowHair_stateChanged(!v);
			}
#else
			CoreLib::Inst()->GamepadHandler_ShowHair();
#endif // NV_ARTISTTOOLS
		}
		if(ButtonState(GAMEPAD_KEY_DPAD_LEFT) == BUTTON_DOWN)
		{
			// Show/Hide Stats
			Gamepad::ShowHideStats();
		}
		if(ButtonState(GAMEPAD_KEY_DPAD_RIGHT) == BUTTON_DOWN)
		{
		}
		if(ButtonState(GAMEPAD_KEY_DPAD_UP) == BUTTON_DOWN)
		{
			// show/hide HUD
			Gamepad::ShowHideHud();
		}
		if(ButtonState(GAMEPAD_KEY_DPAD_DOWN) == BUTTON_DOWN)
		{
		}
	}

	//const float g_dt = 1.0f/120.0f;	// the time delta used for simulation
	if (numAxis == 6)
	{
		// XBOX 360 has 6 axis
		int iLTChange = AxisChange(AXIS_LT);
		int iRTChange = AxisChange(AXIS_RT);
		if (iLTChange > 30000)
		{
			bLeftTrigger = true;
		}
		if (iRTChange > 30000)
		{
			bRightTrigger = true;
		}
	}
	else if(numAxis == 5)
	{
		// Betop has 5 axis when x360 mode
		int iLTChange = AxisChange(AXIS_LT);
		int iRTChange = AxisChange(AXIS_RT);
		if (iLTChange>15000)
		{
			bLeftTrigger = true;
		}
		if (iRTChange < -15000)
		{
			bRightTrigger = true;
		}
	}
	if (1)
	{
		static float zoomSpeed = 0.1f;
		static float rotateSpeed = 2.5f;

		float deltaTime = (currentTime - lastJoyTime);
		lastJoyTime = currentTime;

		const float jsFactor = 1.0f/32768.0f;
		int iLSX = AxisChange(AXIS_LS_X);
		int iLSY = AxisChange(AXIS_LS_Y);
		if(iLSX != 0 || iLSY != 0)
		{
			// left stick
			float forceLSX = iLSX * jsFactor;
			float forceLSY = iLSY * jsFactor;
			if(bLeftTrigger || bRightTrigger)
			{
				if(bRightTrigger)
				{
					// wind direction
					atcore_float3 direct = gfsdk_makeFloat3(forceLSX, forceLSY, 0.1f);
					gfsdk_normalize(direct);

					GlobalSettings::Inst().m_windDir = direct;
					theScene->SetProjectModified(true);
				}
				if(bLeftTrigger)
				{
					// light direction
					{
						Light* pLight = Light::GetFirstSelectedLight();
						if (pLight)
						{
							atcore_float2 delta = gfsdk_makeFloat2(forceLSX, forceLSY);
							delta = 10.0f * delta;
							pLight->Orbit(delta);  // unit is in degree(s)
							theScene->SetProjectModified(true);
						}
					}
				}
			}
			else
			{
				if(iLSY != 0)
				{
					// zoom in/out
					theScene->GetCamera()->Dolly(zoomSpeed * forceLSY);
					theScene->SetProjectModified(true);
				}
			}
		}
		int iRSX = AxisChange(AXIS_RS_X);
		int iRSY = AxisChange(AXIS_RS_Y);
		if(iRSX != 0 || iRSY != 0)
		{
			float forceX = iRSX * jsFactor;
			float forceY = iRSY * jsFactor;
			if(bRightTrigger)
			{
				if (iRSX != 0)
				{
					// wind strength, Press right to increase. Press left to decrease 
					static float windStrength = 0.0f;
					static float fStep = 0.1f;
					if (iRSX > 0)
						windStrength += fStep;
					else
						windStrength -= fStep;
					if (windStrength > 10.0f)
						windStrength = 10.0f;
					if (windStrength < 0.0f)
						windStrength = 0.0f;

#ifndef NV_ARTISTTOOLS
					BlastToolbar* toolbar = AppMainWindow::Inst().GetMainToolbar();
					toolbar->on_spinWindStrength_valueChanged(windStrength);
					toolbar->updateValues();
#else
					CoreLib::Inst()->GamepadHandler_SpinWindStrength(windStrength);
#endif // NV_ARTISTTOOLS
				}
			}
			else
			{
				// rotate camera
				atcore_float2 moveRightStick = gfsdk_makeFloat2(forceX, forceY);
				theScene->GetCamera()->Orbit(moveRightStick*rotateSpeed);
				theScene->SetProjectModified(true);
			}
		}
	}
	//if(numAxis>5)
	//{
		//// XBOX 360 has 6 axis
		//if((iAxisState[4]>=0) && (iAxisStateOld[4]<0))
		//{
		//	bool bShow = ! GlobalSettings::Inst().m_showHUD;
		//	DisplayScenePanel* pspanel = AppMainWindow::Inst().GetDisplayScenePanel();
		//	if(pspanel)
		//	{
		//		pspanel->on_btnShowHUD_stateChanged(bShow);
		//		pspanel->on_btnComputeStats_stateChanged(false);
		//	}
		//}
		//if((iAxisState[5]>=0) && (iAxisStateOld[5]<0))
		//{
		//	bool bShow = ! GlobalSettings::Inst().m_computeStatistics;
		//	DisplayScenePanel* pspanel = AppMainWindow::Inst().GetDisplayScenePanel();
		//	if(pspanel)
		//	{
		//		DisplayScenePanel* pspanel = AppMainWindow::Inst().GetDisplayScenePanel();
		//		if(pspanel)
		//		{
		//			// HUD controls stats
		//			pspanel->on_btnComputeStats_stateChanged(bShow);
		//			pspanel->on_btnShowHUD_stateChanged(bShow);
		//		}
		//	}
		//}
	//}
	for(int i = 0; i < numAxis; ++i)
	{
		//if(iAxisStateOld[i] != iAxisState[i])
		//{
		//	char msg[512];
		//	sprintf(msg, "Axis: %d %d", i, iAxisState[i]);
		//	viewer_msg(msg);
		//}
		iAxisStateOld[i] = iAxisState[i];
	}

	for(int i = 0; i < numButtons; ++i)
	{
		iButtonStateOld[i] = iButtonState[i];
	}

	if (numPov == 1)
	{
		for (int i = GAMEPAD_KEY_DPAD_UP; i <= GAMEPAD_KEY_DPAD_RIGHT; ++i)
		{
			iButtonStateOld[i] = iButtonState[i];
		}
	}
}

//void Gamepad::ClearStates()
//{
//	for(int i = 0; i < iStateSize; ++i)
//	{
//		bKeyHolding[i] = 0;
//
//		iAxisStateOld[i] = 0;
//		iAxisState[i] = 0;
//		iButtonStateOld[i] = 0;
//		iButtonState[i] = 0;
//	}
//}

void Gamepad::ShowHideHud()
{
	static bool bShowHud = true;
	DisplayScenePanel* pspanel = AppMainWindow::Inst().GetDisplayScenePanel();
	if (pspanel)
	{
		// HUD controls stats
		pspanel->on_btnShowHUD_stateChanged(bShowHud);
		pspanel->on_btnComputeStats_stateChanged(false);
		bShowHud = !bShowHud;
	}
}

void Gamepad::ShowHideStats()
{
	static bool bShowStats = true;
	DisplayScenePanel* pspanel = AppMainWindow::Inst().GetDisplayScenePanel();
	if (pspanel)
	{
		// HUD controls stats
		pspanel->on_btnComputeStats_stateChanged(bShowStats);
		pspanel->on_btnShowHUD_stateChanged(bShowStats);
		bShowStats = !bShowStats;
	}
}

void Gamepad::QuitApp()
{
	AppMainWindow::Inst().close();
}

void Gamepad::ResetAnimation()
{
#ifndef NV_ARTISTTOOLS
	GlobalSettings::Inst().toggleSimulation();
	bool simulating = GlobalSettings::Inst().toggleSimulation();  // call it twice to get right state

	GlobalSettings::Inst().toggleAnimation();
	bool animating = GlobalSettings::Inst().toggleAnimation();  // call it twice to get right state

	BlastToolbar* toolbar = AppMainWindow::Inst().GetMainToolbar();
	if (!simulating)
	{
		toolbar->on_btnEnableSimulation_clicked();
	}
	if (!animating)
	{
		toolbar->on_btnPlayAnimation_clicked();
	}
	toolbar->on_btnResetAnimation_clicked();
#else
	CoreLib::Inst()->Gamepad_ResetAnimation();
#endif // NV_ARTISTTOOLS
}

void Gamepad::PlayPauseAnimation()
{
#ifndef NV_ARTISTTOOLS
	GlobalSettings::Inst().toggleSimulation();
	bool simulating = GlobalSettings::Inst().toggleSimulation();  // call it twice to get right state

	GlobalSettings::Inst().toggleAnimation();
	bool animating = GlobalSettings::Inst().toggleAnimation();  // call it twice to get right state

	BlastToolbar* toolbar = AppMainWindow::Inst().GetMainToolbar();
	if (simulating != animating)
	{
		toolbar->on_btnEnableSimulation_clicked();
	}
	toolbar->on_btnEnableSimulation_clicked();
	toolbar->on_btnPlayAnimation_clicked();
#else
	CoreLib::Inst()->Gamepad_PlayPauseAnimation();
#endif // NV_ARTISTTOOLS
}

void Gamepad::DemoModeOnOff()
{
	ChangeMode();
}

void Gamepad::DemoNext()
{
	LoadSamples(true);
	StartAnimation();
}

void Gamepad::DemoPrev()
{
	LoadSamples(false);
	StartAnimation();
}

void Gamepad::SetDemoMode(bool onOff)
{
	//if (AppMainWindow::IsExpertMode() != onOff)
	//{
	//	ChangeMode();
	//}
	if (onOff)
	{
		curentPrjIdx = -1;  // make it start from first demo
		DemoNext();

		// turn off FPS display
		GlobalSettings::Inst().m_showFPS = false;
	}
	else
	{
		ShowUsage();
	}
	ChangeMode();
}

void Gamepad::DemoEscape()
{
	if (AppMainWindow::IsExpertMode())
	{
		ChangeMode();
		// hide HUD if it is on. for GWDCC - 393 Blast Viewer - play list stats
		DisplayScenePanel* pspanel = AppMainWindow::Inst().GetDisplayScenePanel();
		if (pspanel)
		{
			// HUD controls stats
			pspanel->on_btnShowHUD_stateChanged(false);
			pspanel->on_btnComputeStats_stateChanged(false);
		}
	}
	//else
	//{
	//	QuitApp();
	//}
}