#include <windows.h>

#include <QtWidgets/QApplication>

#include "AppMainWindow.h"
//#include "SimpleScene.h"

#include "OpenAutomate.h"
#include "Automate.h"
#include "Settings.h"

//#define OPEN_CONSOLE
#ifdef OPEN_CONSOLE
#include <Windows.h>
#endif

#include <cassert>
#include <string>

#include <Nv/Common/NvCoCommon.h>

#define KEYSTR  ("Software\\OpenAutomate\\RegisteredApps\\NVIDIA\\FurViewer\\v1.0\\")

const oaChar *Benchmarks[] = {
	"HairBall",
	"Teapot",
	"Manjaladon",
	"WitcherHorse",
	NV_NULL
};
oaInt NumBenchmarks = 1;

static void ParseArgs(int argc, char *argv[]);

static void OAMainLoop(const char *opt);
static bool RegisterAppItself();
static int RunApp(QApplication& a);
static void GetAllOptions(void);
static void GetCurrentOptions(void);

static bool g_isAutomateMode = false;
static int InstallModeFlag = 0;
static char *OAOpt = NV_NULL;

bool AutomateInstallApp()
{
	return RegisterAppItself();
}

static bool RegisterAppItself()
{
	char exePath[MAX_PATH];
	if(!GetExePath(exePath))
	{
		fprintf(stderr, "Cannot get exe path\r\n");
		return false; 
	}

	HKEY hKey;
	DWORD dwRes = RegOpenKeyExA(
		HKEY_LOCAL_MACHINE, 
		KEYSTR, 
		0, 
		KEY_WRITE | KEY_READ, 
		&hKey);

	DWORD dwSize = MAX_PATH;
	char buf[MAX_PATH];
	if(dwRes == ERROR_SUCCESS)
	{
		dwRes = RegQueryValueExA(
			hKey,
			"INSTALL_ROOT_PATH",
			NV_NULL,
			NV_NULL,
			(LPBYTE)buf,
			&dwSize);
		if(dwRes == ERROR_SUCCESS && !lstrcmpiA(buf, exePath))
		{
			RegCloseKey(hKey);
			return true;      
		}
	}
	else
	{
		dwRes = RegCreateKeyExA(
			HKEY_LOCAL_MACHINE,
			KEYSTR,
			0,
			NV_NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_WRITE, NV_NULL,
			&hKey,
			&dwSize);
		if(ERROR_SUCCESS != dwRes)
		{
			RegCloseKey(hKey);
			return false;
		}
	}

	bool bRet = false;
	do 
	{
		dwRes = RegSetValueExA(
			hKey,
			"INSTALL_ROOT_PATH",
			0,
			REG_SZ,
			(BYTE*)exePath,
			(DWORD)strlen(exePath));
		if(ERROR_SUCCESS != dwRes)
		{
			fprintf(stderr, "Write INSTALL_ROOT_PATH Failed\r\n");
			break;
		}

#ifdef _WIN64
#ifdef _DEBUG
		strcat(exePath, "FurViewer.win64.D.exe");
#else
		strcat(exePath, "FurViewer.win64.exe");
#endif
#else
#ifdef _DEBUG
		strcat(exePath, "FurViewer.win32.D.exe");
#else
		strcat(exePath, "FurViewer.win32.exe");
#endif
#endif

		dwRes = RegSetValueExA(
			hKey,
			"ENTRY_EXE",
			0,
			REG_SZ,
			(BYTE*)exePath,
			(DWORD)strlen(exePath));
		if(ERROR_SUCCESS != dwRes)
		{
			fprintf(stderr, "Write ENTRY_EXE Failed\r\n");
			break;
		}

		ZeroMemory(buf, sizeof(buf));
		SYSTEMTIME  tm;
		GetLocalTime(&tm);
		sprintf( 
			buf,
			"%04d-%02d-%02d %02d:%02d:%02d",
			tm.wYear,
			tm.wMonth,
			tm.wDay,
			tm.wHour,
			tm.wMinute,
			tm.wSecond);
		dwRes = RegSetValueExA(
			hKey,
			"INSTALL_DATETIME",
			0,
			REG_SZ,
			(BYTE*)buf,
			(DWORD)strlen(buf));
		if(ERROR_SUCCESS != dwRes)
		{
			fprintf(stderr, "Write INSTALL_DATETIME Failed\r\n");
			break;
		}

		dwRes = RegSetValueExA(
			hKey,
			"REGION",
			0,
			REG_SZ,
			(BYTE*)"en_US",
			(DWORD)strlen("en_US"));
		if(ERROR_SUCCESS != dwRes)
		{
			fprintf(stderr, "Write REGION Failed\r\n");
			break;
		}  

		bRet = true;
	} while(0);

	RegCloseKey(hKey);
	return bRet;
}

static const char *FormatDataType(oaOptionDataType value_type)
{
	switch(value_type)
	{
	case OA_TYPE_STRING:
		return "String";
	case OA_TYPE_INT:
		return "Int";
	case OA_TYPE_FLOAT:
		return "Float";
	case OA_TYPE_ENUM:
		return "Enum";
	case OA_TYPE_BOOL:
		return "Bool";
	default:
		return "Unknown";
	}
}

static const char *FormatCompareOp(oaComparisonOpType op_value)
{  
	switch(op_value)
	{
	case OA_COMP_OP_EQUAL:
		return "Equal";
	case OA_COMP_OP_NOT_EQUAL:
		return "NotEqual";
	case OA_COMP_OP_GREATER:
		return "Greater";
	case OA_COMP_OP_LESS:
		return "Less";
	case OA_COMP_OP_GREATER_OR_EQUAL:
		return "GreaterOrEqual";
	case OA_COMP_OP_LESS_OR_EQUAL:
		return "LessOrEqual";
	default:
		return "Unknown";
	}
}

static void ConvertOAValue(oaOptionDataType value_type, 
	const oaValue *value,
	char *result)
{
	switch(value_type)
	{
	case OA_TYPE_STRING:
	case OA_TYPE_ENUM:
		if (value->String) strcpy(result, value->String);
		else result[0] = '\0';
		break;
	case OA_TYPE_INT:
		sprintf(result, "%d", value->Int);
		break;
	case OA_TYPE_FLOAT:
		sprintf(result, "%f", value->Float);
		break;
	case OA_TYPE_BOOL:
		sprintf(result, "%1d", value->Bool);
		break;
	default:
		return;
	}
}

static void PrintOptions(const oaNamedOption *option)
{
	char buf[MAX_PATH];
	ConvertOAValue(option->DataType, &option->Value, buf);

	fprintf(stderr, "name=%s,type=%s,value=%s\r\n", option->Name, FormatDataType(option->DataType), buf);
	if(option->MaxValue.Int && option->MinValue.Int)
	{
		fprintf(stderr,"  numsteps=%d", option->NumSteps);
		ConvertOAValue(option->DataType, &option->MinValue, buf);
		fprintf(stderr, "minvalue=%s", buf);
		ConvertOAValue(option->DataType, &option->MaxValue, buf);
		fprintf(stderr,"maxvalue=%s\r\n", buf);
	}

	if(option->Dependency.ParentName)
	{
		ConvertOAValue(option->Dependency.ComparisonValType, &option->Dependency.ComparisonVal, buf);
		fprintf(stderr, "  parentname=%s,comparisionop=%s,comparisiontype=%s,comparisionvalue=%s\r\n", 
			option->Dependency.ParentName,
			FormatCompareOp(option->Dependency.ComparisonOp),
			FormatDataType(option->Dependency.ComparisonValType),
			buf);
	}
}

#if (0) // Remove this!
int main(int argc, char *argv[])
{
	SetConsoleTitleA("OA - simple application");
	ParseArgs(argc, argv);

	AppSettings::Inst().InitOptions();

	if(InstallModeFlag)
	{
		InstallApp();
	}
	else if(OAModeFlag)
	{
		OAMainLoop(OAOpt);
	}
	else
	{
		RunApp();
	}

	return(0);
}
#endif

void GetAllOptions(void)
{
	fprintf(stderr, "All Options: \r\n");
	for(oaInt i=0; i < AppSettings::Inst().GetNumOptions(); ++i)
	{
		oaNamedOption* option = AppSettings::Inst().GetOption(i);
		if (!strcmp(option->Name, "User/ProjectPath"))
			continue;
		if (!strcmp(option->Name, "User/FurAssetPath"))
			continue;
		PrintOptions(AppSettings::Inst().GetOption(i));
		oaAddOption(AppSettings::Inst().GetOption(i));
	}
}

void GetCurrentOptions(void)
{
	fprintf(stderr, "Current Options: \r\n");

	std::map<std::string, OptionValue>::const_iterator Iter = AppSettings::Inst().GetCurrentOptionMap()->begin();

	oaNamedOption option;
	oaInitOption(&option);

	for(; Iter != AppSettings::Inst().GetCurrentOptionMap()->end(); ++Iter)
	{
		if (!strcmp(Iter->second.Name, "User/ProjectPath"))
			continue;
		if (!strcmp(Iter->second.Name, "User/FurAssetPath"))
			continue;

		option.Name = Iter->second.Name;
		option.Value = Iter->second.Value;
		option.DataType = Iter->second.Type;
		PrintOptions(&option);

		oaAddOptionValue(Iter->second.Name,
			Iter->second.Type,
			&Iter->second.Value);
	}
}

void SetOptions(void)
{
	oaNamedOption *Option;

	fprintf(stderr, "Set Options: \r\n");
	while ((Option = oaGetNextOption()) != NV_NULL)
	{
		/*
		* Set option value to persist for subsequent runs of the game 
		* to the given value.  Option->Name will be the name of the value, 
		* and Option->Value will contain the appropriate value.
		*/
		PrintOptions(Option);
		AppSettings::Inst().SetOptionValue(Option->Name, Option->DataType, &Option->Value);
	}

	AppSettings::Inst().WriteOptionsFile();
}

void GetBenchmarks(void)
{
	/* foreach known available benchmark call oaAddBenchmark() with a unique string identifying it */
	for(oaInt i=0; i < NumBenchmarks; ++i)
		oaAddBenchmark(Benchmarks[i]);
}

void RunBenchmark(const oaChar *benchmark_name)
{
	oaValue Val;
	int i;

	bool FoundBenchmark = false;
	for(i=0; i < NumBenchmarks; ++i)
		if(!strcmp(Benchmarks[i], benchmark_name))
		{
			FoundBenchmark = true;
			break;
		}

		/* Check if the requested benchark is valid */
		if(!FoundBenchmark)
		{
			char Msg[1024];
			sprintf(Msg, "'%s' is not a valid benchmark.", benchmark_name);
			OA_RAISE_ERROR(INVALID_BENCHMARK, benchmark_name);
		}

		/* Setup everything to run the benchmark */
		
		/* oaStartBenchmark() must be called right before the first frame */ 
		oaStartBenchmark();

		/* 
		* Run the benchmark, and call oaDisplayFrame() right before the final
		* present call for each frame
		*/
		//for(i=0; i < 50; ++i)
		//{
		//	SLEEP(20);

		//	oaValue FrameValue;
		//	FrameValue.Int = i;
		//	oaValue OtherInt;
		//	OtherInt.Int = i+1000;
		//	oaChar str[100];
		//	sprintf(str, "frame number: %d\00", i);
		//	oaValue Str;
		//	Str.String = str;

		//	oaAddFrameValue("FrameCount", OA_TYPE_INT, &FrameValue);
		//	oaAddFrameValue("OtherInt", OA_TYPE_INT, &OtherInt);
		//	oaAddFrameValue("String", OA_TYPE_STRING, &Str);

		//	oaDisplayFrame((oaFloat)(i * 20) / (oaFloat)1000);
		//}
		{
//			SimpleScene* scene = SimpleScene::Inst();
			AppMainWindow::Inst().menu_clearScene();
		}

		/* Return some result values */
		Val.Int = 18249;
		oaAddResultValue("Score", OA_TYPE_INT, &Val);

		Val.Float = 29.14;
		oaAddResultValue("Some other score", OA_TYPE_FLOAT, &Val);

		/* oaStartBenchmark() must be called right after the last frame */ 
		oaEndBenchmark();
}

int AutomateInit(const char* opt)
{
	oaVersion Version;
	if (!oaInit((const oaString)opt, &Version))
	{
		Error("OA did not initialize properly.");
		return 1;
	}
	g_isAutomateMode = true;
	return 0;
}

void AutomateRun()
{
	oaCommand Command;

	oaInitCommand(&Command);
	switch(oaGetNextCommand(&Command))
	{
		/* No more commands, exit program */
	case OA_CMD_EXIT: 
		AppMainWindow::Inst().close();
		return;

		/* Run as normal */
	case OA_CMD_RUN: 
		break;

		/* Enumerate all in-game options */
	case OA_CMD_GET_ALL_OPTIONS: 
		GetAllOptions();
		break;

		/* Return the option values currently set */
	case OA_CMD_GET_CURRENT_OPTIONS:
		GetCurrentOptions();
		break;

		/* Set all in-game options */
	case OA_CMD_SET_OPTIONS: 
		SetOptions();
		break;

		/* Enumerate all known benchmarks */
	case OA_CMD_GET_BENCHMARKS: 
		GetBenchmarks();
		break;

		/* Run benchmark */
	case OA_CMD_RUN_BENCHMARK: 
		RunBenchmark(Command.BenchmarkName);
		break;
	}
}

bool IsAutomateMode()
{
	return g_isAutomateMode;
}

bool GetAutomateInstallModeOption(int argc, char* argv[])
{
	for (int idx = 1; idx < argc; ++idx)
	{
		if (!strcmp(argv[idx], "-install"))
		{
			return true;
		}
	}
	return false;
}


std::string GetAutomateOption(int argc, char* argv[])
{
	for (int idx = 1; idx < argc; ++idx)
	{
		if (!strcmp(argv[idx], "-openautomate"))
		{
			if ((idx + 1) < argc)
			{
				return argv[idx+1];
			}
			else
			{
				Error("-openautomate option must have an argument.");
				return "";
			}
		}
	}
	return "";
}
