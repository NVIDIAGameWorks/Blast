#!/usr/bin/python
import os

# projects paths
BLAST_PATH = os.getenv('BLAST_PATH', 'sw/devrel/libdev/destruction/1.0/trunk')
PHYSX_PATH = os.getenv('PHYSX_PATH', 'sw/physx/PhysXSDK/3.4/trunk')
PX_SHARED_PATH = os.getenv('PX_SHARED_PATH', 'sw/physx/PxShared/1.0/trunk/')

# necessary tools paths 
INCREDIBUILD_PATH = r'C:\Program Files (x86)\Xoreax\IncrediBuild\BuildConsole.exe'
MSBUILD_PATH = r'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe'
CYGWIN_PATH = os.getenv('CYGWIN_PATH', 'C:\\cygwin\\bin')
MSYS_PATH = os.path.join(PHYSX_PATH, 'Tools/android-dev/run_msys.cmd')
ADB_SYNC_PATH = os.getenv('ADB_SYNC_PATH', 'C:\\tools\\adb-sync')
ADB_PATH = 'sw/physx/externals/android-sdk/20130124-win/platform-tools'
XDK_PATH = r'C:\Program Files (x86)\Microsoft Durango XDK\bin'



# global settings
USE_INCREDIBUILD = False

# GoogleTest filter
GTEST_FILTER = os.getenv('GTEST_FILTER', '-MediaTest.*') 

# media
MEDIA_PATH = 'sw/physx/media'

# target specific settings
TARGETS = { 
	'Blast-SDK': 
	{
		'order' 		: 0, 
		'generatePath' 	: os.path.join(BLAST_PATH, 'source/compiler/xpj'),
		'generateCmd' 	: 'create_projects.cmd',
		'generateShell' : './create_projects.sh',
		'solutionFile' 	: 'NvBlast.sln',
		'platforms' 	: 
		{
			'win64' 	: ['vc14'],
			'linux64' 	: ['make']
		}
	},
	'Blast-Tests': 
	{ 
		'order' 		: 1,
		'generatePath' 	: os.path.join(BLAST_PATH, 'test/compiler/xpj'),
		'generateCmd' 	: 'create_projects.cmd',
		'generateShell' : './create_projects.sh',
		'solutionFile' 	: 'Tests.sln',
		'bin' 			: 'BlastUnitTests',
		'runPath' 		: os.path.join(BLAST_PATH, 'bin'),
		'runCmd' 		: ['process_tool.exe', '-wait', '-ignore_seh_exception', 'launch', '{0}.exe', '--gtest_output=xml:'],
#		'runCmd' 		: ['{0}.exe', '--gtest_output=xml:'],
		'runShell' 		: ['./{0}', '--gtest_output=xml:'],
		'platforms' 	: 
		{
			'win64' 	: ['vc14'],
			'linux64' 	: ['make']
		}
	},
	'Blast-Tools': 
	{
		'order'			: 4, 
		'generatePath' 	: os.path.join(BLAST_PATH, 'toolapps/compiler/xpj'),
		'generateCmd' 	: 'create_projects.cmd',
		'generateShell' : './create_projects.sh',
		'solutionFile' 	: 'NvBlastTools.sln',
		'platforms' 	: 
		{
			'win32' 	: ['vc11', 'vc12', 'vc14'],
			'win64' 	: ['vc11', 'vc12', 'vc14']
		}
	},
	'Blast-Sample': 
	{
		'order'			: 5, 
		'generatePath' 	: os.path.join(BLAST_PATH, 'samples/compiler/xpj'),
		'generateCmd' 	: 'create_projects.cmd',
		'generateShell' : './create_projects.sh',
		'solutionFile' 	: 'Samples.sln',
		'platforms' 	: 
		{
			'win32' 	: ['vc12'],
			'win64' 	: ['vc12']
		}
	},
	'PhysX-SDK': 
	{
		'order' 		: 2, 
		'generatePath' 	: os.path.join(PHYSX_PATH, 'Source/compiler/xpj'),
		'generateCmd' 	: 'create_projects.cmd',
		'generateShell' : './create_projects.sh',
		'solutionFile' 	: 'PhysX.sln',
		'platforms' 	: 
		{
			'win32' 	: ['vc11', 'vc12', 'vc14'],
			'win64' 	: ['vc11', 'vc12', 'vc14'],
		}
	} ,
}

# supported configs
CONFIGS = ['checked', 'debug', 'profile', 'release']


# CLI (Cmd, Shell) from platform 
def get_cli(platform):
	if 'linux' in platform:
		return 'Shell'
	elif 'android' in platform:
		return 'Shell'
	else:
		return 'Cmd'
		
# project folder (e.g. 'vc11win64-PhysX_3.4')
def get_project_folder(target, platform, tool):
	postfix = ''
	if 'linux' in platform:
		return tool + platform + postfix
	elif 'android' in platform:
		return platform + postfix
	else:
		toolPrefix = tool if tool.startswith('vc') else ''
		return toolPrefix + platform + postfix
	
# solution config (e.g. {debug|x64}), used by VS toolchains)
def get_solution_cfg(config, platform):
	platformMap = {
		'win64' : 'x64'
	}
	platformName = platformMap.get(platform, platform)
	return '{0}|{1}'.format(config, platformName)
	
def get_bin_file_name(target, config):
	configPostFix = '' if config.lower() == 'release' else config.upper()
	return TARGETS[target]['bin'] + configPostFix