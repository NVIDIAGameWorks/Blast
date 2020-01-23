#!/usr/bin/python
import os
import sys
import subprocess
import util
import conf

def generate(target, platform, workspace = "."):
	print("Generate project: {0} {1} {2}".format(target, platform, workspace))
	
	path = os.path.join(workspace, conf.TARGETS[target]['generatePath'])
	
	args = [conf.TARGETS[target]['generate' + conf.get_cli(platform)], platform]

	# add msys to run shell on windows
	if os.name == 'nt' and conf.get_cli(platform) == 'Shell':
		msys = os.path.relpath(os.path.join(workspace, conf.MSYS_PATH), path)
		args = [msys] + args 
		
	util.run_cmd(path, args)

def main():
	if len(sys.argv) < 2:
		print('Error: too few arguments')
		print('Usage example: generate.py Blast-SDK win32 D:/perforce_depot')
		sys.exit(-1)

	target = sys.argv[1]
	platform = sys.argv[2]
	workspace = sys.argv[3] if len(sys.argv) > 3 else '.'
	generate(target, platform, workspace)	
	
if __name__ == "__main__":
    main()
