#!/usr/bin/python
import os
import sys
import fnmatch
import subprocess
import util
import conf
import multiprocessing

def searchAndDelete(path, fname):
	removedCount = 0
	for root, dirnames, filenames in os.walk(path):
		for filename in fnmatch.filter(filenames, fname):
			os.remove(os.path.join(root, filename))
			removedCount += 1
	print('Removed ' + str(removedCount) + ' manifests')

def buildWithIncrediBuild(target, platform, config, tool, workspace = "."):
	print('build with IncrediBuild')

	projectDir = conf.get_project_folder(target, platform, tool)
	path = os.path.join(workspace, conf.TARGETS[target]['generatePath'], '../', projectDir)
	
	args = conf.TARGETS[target]['solutionFile'] + ' /build /cfg="{0}" /VsVersion={1}'.format(conf.get_solution_cfg(config, platform), tool)
	util.run_cmd(path, conf.INCREDIBUILD_PATH + ' ' + args)

def buildWithMSBuild(target, platform, config, tool, workspace = "."):
	print('build with MSBuild')

	projectDir = conf.get_project_folder(target, platform, tool)
	path = os.path.join(workspace, conf.TARGETS[target]['generatePath'], '../', projectDir)
	
	args = conf.TARGETS[target]['solutionFile'] + ' /m /verbosity:quiet /p:Configuration={0};VisualStudioVersion={1}.0'.format(config, tool[2:])
	util.run_cmd(path, conf.MSBUILD_PATH + ' ' + args)

def buildWithMake(target, platform, config, tool, workspace = "."):
	print('build with make')

	projectDir = conf.get_project_folder(target, platform, tool)
	path = os.path.join(workspace, conf.TARGETS[target]['generatePath'], '../', projectDir)

	if os.name == 'nt':
		util.add_to_path(conf.CYGWIN_PATH, workspace)
	
	util.run_cmd(path, ['make', config, '-j{0}'.format(multiprocessing.cpu_count())])
	
def build(target, platform, config, tool, workspace = "."):
	print("Build: {0} {1} {2} {3} {4}".format(target, platform, config, tool, workspace))
	
	if 'vc' in tool:
		if conf.USE_INCREDIBUILD:
			buildWithIncrediBuild(target, platform, config, tool, workspace)
		else:
			buildWithMSBuild(target, platform, config, tool, workspace)
	else:
		buildWithMake(target, platform, config, tool, workspace)
	
	print('Build succeeded')

def main():
	if len(sys.argv) < 4:
		print('Error: too few arguments')
		print('Usage example: build.py Blast-SDK win32 debug vc11 D:/perforce_depot')
		sys.exit(-1)

	target = sys.argv[1]
	platform = sys.argv[2]
	config = sys.argv[3]
	tool = sys.argv[4]
	workspace = sys.argv[5] if len(sys.argv) > 5 else '.'
	build(target, platform, config, tool, workspace)

if __name__ == "__main__":
    main()	


