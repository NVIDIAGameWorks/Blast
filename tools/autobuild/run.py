#!/usr/bin/python
import os
import sys
import subprocess
import util
import conf
import tempfile
import re
import shutil
import socket
import time

# common runner (for win, linux)
def runCommon(target, platform, config, tool, workspace = "."):
	print('runCommon')

	projectDir = conf.get_project_folder(target, platform, tool)
	dir = os.path.join(workspace, conf.TARGETS[target]['runPath'], projectDir)
	cmd = conf.TARGETS[target]['run' + conf.get_cli(platform)]
	testsFile = conf.get_bin_file_name(target, config)
	args = [s.format(testsFile) for s in cmd]

	# test output (remove previous results)
	testOutputFilePath = os.path.join(dir, testsFile + '.xml')
	if os.path.exists(testOutputFilePath):
		os.unlink(testOutputFilePath)
	
	# set LD_LIBRARY_PATH to PhysX and PxShared on linux:
	if os.name == 'posix':
		physxLdPath = os.path.relpath(os.path.join(workspace, os.path.join(conf.PHYSX_PATH, 'Bin', projectDir)), dir)
		pxSharedLdPath = os.path.relpath(os.path.join(workspace, os.path.join(conf.PX_SHARED_PATH, 'bin', platform)), dir)
		os.environ['LD_LIBRARY_PATH'] = os.getenv('LD_LIBRARY_PATH', '') + ':' + physxLdPath + ':' + pxSharedLdPath
		print('LD_LIBRARY_PATH:' + os.environ['LD_LIBRARY_PATH'])
	
	# set GTEST_FILTER
	os.environ['GTEST_FILTER'] = conf.GTEST_FILTER
	
	# run
	util.run_cmd(dir, args)

# android runner
'''
def runAndroid(target, platform, config, tool, workspace = "."):
	print('runAndroid')

	util.add_to_path(conf.ADB_PATH, workspace)
	dir = os.path.join(workspace, conf.APEX_MEDIA_PATH)

	# kill adb process
	util.run_cmd('.', ['taskkill', '/f', '/im', 'adb.exe'], autofail = False)

	# adb kill-server
	util.run_cmd(dir, ['adb', 'kill-server'], autofail = False)

	# adb root 
	util.run_cmd('.', ['adb', 'root'])
	
	# sync media
	args = ['python', '-u', conf.ADB_SYNC_PATH, '.', conf.ANDROID_MEDIA_PATH]
	util.run_cmd(dir, args)
	
	# push test
	projectDir = conf.get_project_folder(target, platform, tool)
	dir = os.path.join(workspace, conf.TARGETS[target]['runPath'], projectDir)
	testsFile = conf.get_bin_file_name(target, config)
	testsFileDevice = os.path.join(conf.ANDROID_TESTS_PATH, testsFile)
	util.run_cmd(dir, ['adb', 'push', testsFile, testsFileDevice])
	util.run_cmd('.', ['adb', 'shell', 'chmod', '777', testsFileDevice])

	# test output (remove previous results)
	testOutputFilePath = os.path.join(dir, testsFile + '.xml')
	if os.path.exists(testOutputFilePath):
		os.unlink(testOutputFilePath)
	
	# run
	testsFileDeviceOutput = testsFileDevice + '.xml'
	res = util.run_cmd('.', ['adb', 'shell', testsFileDevice, 
		'--gtest_filter=' + conf.GTEST_FILTER, '--gtest_output=xml:' + testsFileDeviceOutput], autofail = False)
		
	util.run_cmd(dir, ['adb', 'pull', testsFileDeviceOutput])
	
	# adb kill-server
	util.run_cmd(dir, ['adb', 'kill-server'], autofail = False)
	
	if res != 0:
		sys.exit(-1)
'''
	
def run(target, platform, config, tool, workspace = "."):
	util.add_to_path(os.path.dirname(os.path.realpath(sys.argv[0])), workspace)

	print('Run: {0} {1} {2} {3} {4}'.format(target, platform, config, tool, workspace))
	
	if 'android' in platform:
		runAndroid(target, platform, config, tool, workspace)
	else:
		runCommon(target, platform, config, tool, workspace)
	
	print('Ruild succeeded')

def main():
	if len(sys.argv) < 4:
		print('Error: too few arguments')
		print('Usage example: run.py Blast-Tests win32 debug vc11 D:/perforce_depot')
		sys.exit(-1)

	target = sys.argv[1]
	platform = sys.argv[2]
	config = sys.argv[3]
	tool = sys.argv[4]
	workspace = sys.argv[5] if len(sys.argv) > 5 else '.'
	run(target, platform, config, tool, workspace)

if __name__ == "__main__":
    main()	


