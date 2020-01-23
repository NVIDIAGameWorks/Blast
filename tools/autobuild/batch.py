#!/usr/bin/python
import os
import sys
import util
import re
import argparse


###############################################################################
# env vars
###############################################################################
def set_env_paths():
	os.environ["BLAST_PATH"] = util.find_path_up("source")
	print('BLAST_PATH: {0}'.format(os.environ["BLAST_PATH"]))
	os.environ["PERFORCE_ROOT"] = util.find_path_up(os.path.join('sw', 'devrel'))
	print('PERFORCE_ROOT: {0}'.format(os.environ["PERFORCE_ROOT"]))


###############################################################################
# filters
###############################################################################
def filter_platform(p):
	isWindows = (os.name == 'nt')
	if isWindows:
		return p in ['win32', 'win64']
	else:
		return p in ['linux32', 'linux64']

def filter_tool(t):
	isWindows = (os.name == 'nt')
	if isWindows:
		return t in ['vc11', 'vc12', 'vc14']
	else:
		return t in ['make']

###############################################################################
# execute
###############################################################################
def execute(cmd, settings, target, p, c = "", t = ""):
	from generate import generate
	from build import build
	from run import run

	commands = {
		"generate" : lambda target, p, c, t, root : generate(target, p, root),
		"build" : lambda target, p, c, t, root : build(target, p, c, t, root),
		"run" : lambda target, p, c, t, root : run(target, p, c, t, root)
	}

	s = "{0}-{1}-{2}-{3}".format(target, p, t, c)
	for f in settings['filters']:
		if re.search(f, s):
			print('cmd: {0}, config: {1}'.format(cmd, s))
			if not settings['echo_only']:
				commands[cmd](target, p, c, t, os.environ['PERFORCE_ROOT'])
			break


###############################################################################
# generate all
###############################################################################
def generate_all(settings):
	import conf
	print('\n> generate_all phase:')

	PLATFORMS = ['win32', 'linux32']

	for target in conf.TARGETS:
		for p in filter(filter_platform, PLATFORMS):
			if p in conf.TARGETS[target]['platforms']:
				execute("generate", settings, target, p)

###############################################################################
# build all
###############################################################################
def build_all(settings):
	import conf
	print('\n> build_all phase:')

	for target, data in sorted(conf.TARGETS.items(), key=lambda t: t[1]['order']):
		for p in filter(filter_platform, data['platforms']):
			for t in filter(filter_tool, data['platforms'][p]):
				for c in conf.CONFIGS:
					execute("build", settings, target, p, c, t)

###############################################################################
# run all
###############################################################################
def run_all(settings):
	import conf
	print('\n> run_all phase:')

	RUNNABLE_TARGETS = ['Blast-Tests']
	PLATFORMS = ['win32', 'win64', 'linux32', 'linux64']

	for target in RUNNABLE_TARGETS:
		for p in filter(filter_platform, PLATFORMS):
			for t in filter(filter_tool, conf.TARGETS[target]['platforms'][p]):
				for c in conf.CONFIGS:
					execute("run", settings, target, p, c, t)

###############################################################################
# main
###############################################################################
def main():
	parser = argparse.ArgumentParser(description='Sequential generate, build, run calls for all configurations.')
	parser.add_argument('-g', '--generate', action='store_true', help='generate all')
	parser.add_argument('-b', '--build', action='store_true', help='build all')
	parser.add_argument('-r', '--run', action='store_true', help='run all')
	parser.add_argument('-e', '--echo_only', action='store_true', help='print all filtered configurations without actually performing build or run.', default=False)
	parser.add_argument('-f', '--filter', type=str, default='.', help="regex to filter configurations, use -e to check result before running. Example: '-f win64'", nargs='*')

	if len(sys.argv) <= 1:
		parser.print_help()
		sys.exit(-1)

	args = parser.parse_args()

	settings = {
		"filters" : args.filter,
		"echo_only" : args.echo_only
	}

	set_env_paths()

	if args.generate:
		generate_all(settings)
	if args.build:
		build_all(settings)
	if args.run:
		run_all(settings)

	if not args.echo_only:
		print('A huge success!')

if __name__ == "__main__":
    main()
