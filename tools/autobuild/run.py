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

# xboxone runner
def runXbox(target, platform, config, tool, workspace = "."):
	print('runXbox')
	
	# add path to all xdk tools
	util.add_to_path(conf.XDK_PATH, workspace)
	
	# resolve hostname ip
	ip = socket.gethostbyname(conf.XBOX_HOSTNAME)
	print(conf.XBOX_HOSTNAME + ':' + ip)
	
	# prepare xbox
	util.run_cmd('.', ['xbconnect', ip])
	util.run_cmd('.', ['xbdeploy', 'shutdown'])
	#util.run_cmd('.', ['xbcleanup', '/S']) # reboot (uncomment if needed)
	
	# setup paths
	testFile = conf.get_bin_file_name(target, config)
	testOutputFile = testFile + '.xml'
	testOutputFilePath = '\\\\{0}\\TitleScratch\\{1}'.format(conf.XBOX_HOSTNAME, testOutputFile)
	projectDir = conf.get_project_folder(target, platform, tool)
	appPath = os.path.abspath(os.path.join(workspace, conf.TARGETS[target]['xboxAppPath'].format(projectDir)))
	mediaPath = os.path.abspath(os.path.join(workspace, conf.MEDIA_PATH))

	# clean last run results
	if os.path.exists(testOutputFilePath):
		os.unlink(testOutputFilePath)
	
	# replace executable name in manifest to match config
	manifestFile = os.path.join(appPath, 'AppxManifest.xml')
	manifestContent = ''
	with open(manifestFile, 'r') as f:
		manifestContent = f.read()
	manifestContent = re.sub(r'(Executable=").*?(\")', r'\1{0}.exe\2'.format(testFile), manifestContent)
	with open(manifestFile, 'w') as f:
		f.write(manifestContent)
		
	# remake package after updating manifest
	util.run_cmd('.', ['makepkg', 'appdata', '/f', manifestFile])
		
	# generate temp mapping file for media dir
	mapFile = open(os.path.join(tempfile.gettempdir(), 'map.xml'), 'w')
	mapFile.write('''<?xml version="1.0" encoding="utf-8" ?>
		<mappings>
		  <path source="{0}" target="\media"/>
		</mappings>
	'''.format(mediaPath))
	mapFile.close()
	
	# pull deploy (see xboxone docs)
	util.run_cmd('.', ['xbdeploy', 'pull', appPath, '/mf:' + mapFile.name + ''])
	
	# run app
	util.run_cmd('.', ['xbapp', 'launch', conf.TARGETS[target]['xboxAppAUMID'], 
		'--gtest_filter=' + conf.GTEST_FILTER, '--gtest_output=xml:D:/' + testOutputFile])
	
	# listen process
	startTime = time.time()
	print('waiting for process to finish...')
	while True:
		present = False
		try:
			for line in subprocess.check_output(['xbtlist', '/x/title']).split('\n'):
				if testFile in line:
					present = True
		except:
			pass
		if not present:
			break
		time.sleep(5)
	print('process finished (took ~{0}s)'.format(int(time.time() - startTime)))
	
	# delete temp mapping file
	os.unlink(mapFile.name)
	
	# stop xbdeploy
	util.run_cmd('.', ['xbdeploy', 'stop'])
	
	# copy results from xbox by network
	time.sleep(3) # no rush!
	testOutputResultFile = os.path.join(workspace, conf.TARGETS[target]['runPath'], projectDir, testOutputFile)
	shutil.copy2(testOutputFilePath, testOutputResultFile)
	
	print('runXbox succeeded:' + testOutputResultFile)
	
def run(target, platform, config, tool, workspace = "."):
	util.add_to_path(os.path.dirname(os.path.realpath(sys.argv[0])), workspace)

	print('Run: {0} {1} {2} {3} {4}'.format(target, platform, config, tool, workspace))
	
	if 'android' in platform:
		runAndroid(target, platform, config, tool, workspace)
	elif 'xboxone' in platform:
		runXbox(target, platform, config, tool, workspace)
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


