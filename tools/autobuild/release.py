import os
import sys
import util
import shutil
import stat
import argparse

# Filters work recursively
# Exclude filter has higher priority than include filter
# 'include' filter can disable 'exclude' filter which applied on ancestor folder
ARTIFACTS = [
	# source windows-like platforms
	{
		'file' 	  	: 'blast_source_{0}_{1}',
		'platforms' : ['win32', 'win64', 'xboxone', 'ps4'],
		'tools'		: ['vc11', 'vc12', 'vc14'],
		'include' 	:
		{
			'.' 								 						: [".h", ".cpp", ".vsprops"],
			'./source/compiler/%tool%%platform%' 						: [".sln", ".vcxproj", ".filters", ".vsprops"],
			'./toolapps/compiler/%tool%%platform%' 						: [".sln", ".vcxproj", ".filters", ".vsprops"],
			'./shared'							 						: ["*"],
			'./docs/source_docs'					 					: ["*"],
			'./docs/api_docs'					 						: ["*"],
		},
		'exclude' 	:
		{
		
			'./tools'								: ["*"],
			'./shared/external/GraphicsLib'		 	: ["*"],
			'./test'								: ["*"],
			'./source/waiting'						: ["*"],
			'./samples'								: ["*"],
		},
		'files' : [
		'././dllcopy.bat',
		'./docs/release_notes.txt',
		]
	},

	# source unix-like platforms
	{
		'file' 	  	: 'blast_source_{0}_{1}',
		'platforms' : ['linux32', 'linux64'],
		'tools'		: ['make'],
		'include' 	:
		{
			'.' 								: [".h", ".cpp"],
			'./source/compiler/%tool%%platform%': ["", ".mk"],
			'./docs/source_docs'					 					: ["*"],
			'./docs/api_docs'					 						: ["*"],
		},
		'exclude' 	:
		{
			'./tools'							: ["*"],
			'./samples'							: ["*"],
			'./shared/external/hbao'			: ["*"],
			'./shared/external/shadow_lib'		: ["*"],
			'./shared/external/tinyObjLoader'	: ["*"],
			'./shared/external/GraphicsLib'		: ["*"],
			'./test'							: ["*"],
			'./source/waiting'					: ["*"],
		},
		
		'files' : [
		'./docs/release_notes.txt',
		]
	},

	# binary windows-like platforms
	{
		'file' 	  	: 'blast_binary_{0}_{1}',
		'platforms' : ['win32', 'win64'],
		'tools'		: ['vc11', 'vc12', 'vc14'],
		'include' 	:
		{
			'./bin/%tool%%platform%': [".exe", ".dll"],
			'./lib/%tool%%platform%': [".lib"],
			'./samples/resources'	: [".obj", ".collision", ".blast", ".xml", ".mtl", ".hlsl", ".dds"],
			'./include' : ["*"],
		},
		
		'exclude' 	:
		{
			'./tools'			: ["*"],
		},
		
		'skipwithsubstr':
		{
			'./bin/%tool%%platform%': ['SDL2']
		}
	},
	# SDK only for windows-like platforms
	{
		'file' 	  	: 'blast_sdk_only_{0}_{1}',
		'platforms' : ['win32', 'win64', 'xboxone'],
		'tools'		: ['vc11', 'vc12', 'vc14'],
		'include' 	:
		{
			'./bin/%tool%%platform%': [".dll"],
			'./lib/%tool%%platform%': [".lib"],
			'./include' : ["*"],
			'./docs/api_docs'					 						: ["*"],
		},
		
		'exclude' 	:
		{
			'./tools'			: ["*"],
		},
		
		'skipwithsubstr' 	:
		{
			'./bin/%tool%%platform%': ['AntTweakBar','Assimp32', 'PhysX', 'GFSDK', 'd3dcompiler_47', 'nvToolsExt', 'SDL2']
		},
		
		'files' : [
			'./docs/release_notes.txt',
		],
		
	},
	# SDK only for PS4
	{
		'file' 	  	: 'blast_sdk_only_{0}_{1}',
		'platforms' : ['ps4'],
		'tools'		: ['vc11', 'vc12', 'vc14'],
		'include' 	:
		{
			'./lib/%tool%%platform%': [".a"],
			'./include' : ["*"],
			'./docs/api_docs'					 						: ["*"],
		},
		
		'exclude' 	:
		{
			'./tools'			: ["*"],
		},
		
		'skipwithsubstr' 	:
		{
			'./bin/%tool%%platform%': ['AntTweakBar', 'Assimp32', 'PhysX', 'GFSDK', 'd3dcompiler_47', 'nvToolsExt', 'SDL2']
		},
		
		'files' : [
			'./docs/release_notes.txt',
		],
	},
	# Tools and samples
	{
		'file' 	  	: 'blast_tools_and_samples_{0}_{1}',
		'platforms' : ['win32', 'win64'],
		'tools'		: ['vc12'],
		'include' 	:
		{
			'./bin/%tool%%platform%': [".dll"],
			'./samples/resources'	: [".obj", ".collision", ".blast", ".xml", ".mtl", ".hlsl", ".dds"],
		},		
		'exclude' 	:
		{
			'./tools'			: ["*"],
		},
		
		'files' : [
		'./bin/%tool%%platform%/SampleAssetViewer.exe',
		'./bin/%tool%%platform%/SampleAssetViewerPROFILE.exe',
		'./bin/%tool%%platform%/AuthoringTool.exe',
		'./bin/%tool%%platform%/DataConverter.exe',	
		],
		
		'skipwithsubstr' 	:
		{
			'./bin/%tool%%platform%': ['DEBUG', 'CHECKED', 'SDL2']
		}
	},

	# SDK only unix-like platforms
	{
		'file' 	  	: 'blast_sdk_only_{0}_{1}',
		'platforms' : ['linux32', 'linux64'],
		'tools'		: ['make'],
		'include' 	:
		{
			'./bin/%tool%%platform%': [".so"],
			'./lib/%tool%%platform%': [".a"],
			'./include' 			: ["*"],
			'./docs/api_docs'		: ["*"],
		},
		
		'exclude' 	:
		{
			'./tools'			: ["*"],
		},
		
		'skipwithsubstr' 	:
		{
			'./bin/%tool%%platform%': ['AntTweakBar','Assimp32', 'PhysX', 'GFSDK_SSAO', 'd3dcompiler_47', 'nvToolsExt']
		},
		
		'files' : [
		'./docs/release_notes.txt',
		]
	}
]

BUILD_CONFS= ['checked', 'debug', 'profile', 'release']

PLATFORM_DIRS = {
	# 	platform directory 		: 	platform(s) name
		'/ps4'					:	['ps4'],
		'/xboxone'				:	['xboxone'],
		'/windows'				:	['win32','win64'],
		'/unix'					:	['linux32','linux64'],
	}

def onDeleteError(f, p, inf):
	os.chmod(p, stat.S_IWRITE)
	os.remove(p)

def set_env_paths():
	os.environ["BLAST_PATH"] = util.find_path_up("source")
	print('BLAST_PATH: {0}'.format(os.environ["BLAST_PATH"]))
	os.environ["PERFORCE_ROOT"] = util.find_path_up(os.path.join('sw', 'devrel'))
	print('PERFORCE_ROOT: {0}'.format(os.environ["PERFORCE_ROOT"]))
	
def prebuild(platform, workspace, defTool, noPhysx):
	import conf
	from generate import generate
	from build import build
	print("Create projects and build them")
	blastDocDir = os.path.join(os.environ["BLAST_PATH"], 'docs/_compile')
	if ('linux' not in platform):
		util.run_cmd(blastDocDir, 'build_all.bat')
	else:
		util.run_cmd(blastDocDir, './build_all.sh')
	os.putenv('INSTALLER', 'true')
	for target, data in sorted(conf.TARGETS.items(), key=lambda t: t[1]['order']):
		if (target == 'PhysX-SDK' and noPhysx):
			continue
		if (platform in data['platforms'].keys()):
			generate(target, platform, workspace)

	for target, data in sorted(conf.TARGETS.items(), key=lambda t: t[1]['order']):
		if (target == 'PhysX-SDK' and noPhysx):
			continue
		if (platform not in data['platforms'].keys()):
			continue
		tools = data['platforms'][platform] if defTool == '.' else [defTool]
		for tool in tools:
			for buildConfig in BUILD_CONFS:
				if ((target == 'Blast-Sample' or  target == 'Blast-Tools') and (buildConfig != 'release' and buildConfig != 'profile')): # Checked, profile fail due to external projects
					continue
				build(target, platform, buildConfig, tool, workspace)	
	print("Build done...")
	
def createZip(outputName, artifact, platform, workspace, echo_only):	
	print("createZip: {0} (platform {1})".format(outputName, platform))
	#Walk and collect all files
	blastDir = os.environ["BLAST_PATH"]
	tempDir = os.path.join(blastDir, "../BLAST_INSTALLER_TEMP")
	print("Remove old temp folder")
	if (os.path.isdir(tempDir)):
		os.chmod(tempDir, stat.S_IWRITE)
		shutil.rmtree(tempDir, onerror = onDeleteError)
		
	dirTreeDebug = {}

	os.makedirs(tempDir)
	print("Copy to temp folder")
	directoriesTraverse = os.walk(blastDir)
	for dirPath, dirn, filelist in directoriesTraverse:
		relPath = os.path.join('.', os.path.relpath(dirPath, blastDir))
		relPath = relPath.replace(os.path.sep, '/')
		newDir = os.path.join(tempDir,  relPath) 
		for file in filelist:			
			filename, file_ext = os.path.splitext(file)
			#-------------------------------------------------
			# Check whether current file type should be copied
			#-------------------------------------------------
			toCopy = False
			longestPath = 0
			if 'include' in artifact.keys():
				for path, exts in artifact['include'].items():
					path = path.replace('%platform%', platform)
					for p in [path.replace('%tool%', t) for t in artifact['tools']]:
						if relPath.startswith(p) and (file_ext in exts or '*' in exts):
							toCopy = True
							longestPath = max(longestPath, len(p))
						#print("Add extension: " + file_ext + " path: " + relPath)
				
			if 'exclude' in artifact.keys():
				for path, exts in artifact['exclude'].items():
					path = path.replace('%platform%', platform)
					if relPath.startswith(path) and (file_ext in exts or '*' in exts):
						if len(path) >= longestPath:
							toCopy = False
							#print("Exclude extension: " + file_ext + " path: " + relPath)		
							
							
			if 'skipwithsubstr' in artifact.keys():
				for path, substrList in artifact['skipwithsubstr'].items():
					path = path.replace('%platform%', platform)
					for p in [path.replace('%tool%', t) for t in artifact['tools']]:
						if relPath.startswith(p):
							for sbstr in substrList:
								if (sbstr in file):
									toCopy = False
																
			if 'files' in artifact.keys():	
				for filesToSave in artifact['files']:				
					filesToSave = filesToSave.replace('%platform%', platform)
					for fn in [filesToSave.replace('%tool%', t) for t in artifact['tools']]:
						if (relPath + '/' + file == fn):
							toCopy = True;
							#print("Copied: " + fn)

			for pdir, pnames in PLATFORM_DIRS.items():
				if pdir in relPath and platform not in pnames:
					toCopy = False
					#print("skipping platform directory " + relPath)

			#---------------------------------------------------

			if toCopy:
				if (not os.path.isdir(newDir)):
					os.makedirs(newDir)
				copyFrom = os.path.join(dirPath, file)
				copyTo = os.path.join(os.path.join(tempDir, relPath), file)
				#print("Copying {0} -> {1}".format(copyFrom, copyTo))
				if not echo_only:
					shutil.copy(copyFrom, copyTo)
				if dirPath not in dirTreeDebug:
					dirTreeDebug[dirPath] = []
				if file_ext not in dirTreeDebug[dirPath]:
					dirTreeDebug[dirPath].append(file_ext)

	print("Copied dir tree:")
	for k, v in sorted(dirTreeDebug.items()):
		print("{0} : {1}".format(k, sorted(v)))

	if echo_only:
		return

	print("Compress data")
	outputArchivePath = os.path.join(blastDir, outputName)+'.zip'
	if (os.path.isfile(outputArchivePath)):
		print("Zip file with same name is already exist...Delete them")
		os.remove(outputArchivePath)
	
	if (os.name == 'nt'):
		archiverPath = os.path.join(workspace, 'sw/physx/tools/7zip/win32')	
		util.run_cmd(archiverPath, ['7za.exe', 'a', '-tzip', outputArchivePath, tempDir + '/*'])
	else:
		archiverPath = os.path.join(workspace, 'sw/physx/tools/7zip/linux')	
		util.run_cmd(archiverPath, ['./7za', 'a', '-tzip', outputArchivePath, tempDir + '/*'])

	print("Delete temporary files")
	if (os.path.isdir(tempDir)):
		os.chmod(tempDir, stat.S_IWRITE)
		shutil.rmtree(tempDir, onerror = onDeleteError)
		
	print("Done")
	
def getCL():
	VCS_ROOT_ID = 'INSTALLER_CL_NUMBER' #copied from TC
	return os.getenv('INSTALLER_CL_NUMBER', '_local_')
	
def main():
	parser = argparse.ArgumentParser(description = 'Create release builds and pack them.')
	parser.add_argument('-p', '--platform', type=str, help = 'Platform (e.g. win32, win64, linux32..)')
	parser.add_argument('-t', '--tool', type=str, default='.', help = 'Tool (e.g. vc11, vc12, vc14, make..)')
	parser.add_argument('-n', '--nophysx', action='store_true', help = 'Disable PhysX build', default=False)
	parser.add_argument('-e', '--echo_only', action='store_true', help='print directory tree for each artifact without actually building or zipping.', default=False)
	
	if len(sys.argv) <= 1:
		parser.print_help()
		sys.exit(-1)
	
	args = parser.parse_args();	

	set_env_paths()
	
	if not args.echo_only:
		prebuild(args.platform, os.environ['PERFORCE_ROOT'], args.tool, args.nophysx)
	
	# Pack zip with some extension filter
	platform = args.platform
	for artifact in ARTIFACTS:
		if platform not in artifact['platforms']:
			continue
		fileName = artifact['file'].format(platform, getCL())
		createZip(fileName, artifact, platform, os.environ['PERFORCE_ROOT'], args.echo_only)

	
if __name__ == "__main__":
	main()
	

	
	
