#!/usr/bin/python
import os
import re
import sys
import tempfile
import contextlib
import subprocess

def run_cmd(workingDir, args, autofail = True):
	print('run_cmd {0} in dir {1}'.format(args, workingDir))
	ret = 0
	cwd = os.getcwd()
	os.chdir(workingDir)
	try:
		p = subprocess.Popen(args, stdout=sys.stdout, stderr=subprocess.STDOUT)
		p.wait()
		ret = p.returncode
	except subprocess.CalledProcessError as e:
		print(e.returncode)
		print(e.output)
		sys.exit(-1)

	os.chdir(cwd)
	print('run_cmd finished with code {0}'.format(ret))

	if autofail and ret != 0:
		sys.exit(-1)

	return ret

def add_to_path(dir, workspace):
	path = dir if os.path.isabs(dir) else os.path.abspath(os.path.join(workspace, dir))
	os.environ["PATH"] = path + os.pathsep + os.environ["PATH"]
	print('PATH: +' + path)

def find_path_up(dir):
	path = os.getcwd()
	for i in range(0, 100):
		if os.path.exists(os.path.join(path, dir)):
			return path
		path = os.path.abspath(os.path.join(path, os.pardir))
	return ""

