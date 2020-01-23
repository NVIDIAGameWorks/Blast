********************************************************************
						Blast Build Tools
********************************************************************						

DESCRIPTION:

Python scripts to generate, build and run Blast projects on different 
platforms. Used mainly by Jenkins build system, but also can be used 
locally from command line.

Tested on Python 2.7.10

********************************************************************

GENERAL USAGE:

Most of the scripts accept: target, platform, config, tool, workspace.

- targets: 
	PhysX-SDK
	Blast-SDK
	Blast-Tests
	
- platforms:
	win64
	linux64
	
- configs:
	debug 
	checked 
	release 
	profile
	
- tools:
	vc14
	make
	
- workspace:
	Path to perforce root, e.g. D:/perforce_depot. Default is '.'

********************************************************************

SCRIPTS:

--------------------------------------------------------------------
* conf.py
--------------------------------------------------------------------

Edit for settings.


--------------------------------------------------------------------
* generate.py 
--------------------------------------------------------------------

Solution generation script.

Syntax:
>> generate.py target platform [workspace]
Usage example: 
>> generate.py Blast-SDK win32 D:/perforce_depot


--------------------------------------------------------------------
* build.py
--------------------------------------------------------------------

Solution build script.

Syntax:
>> build.py target platform config tool workspace

Usage example: 
>> build.py Blast-SDK win32 debug vc11 D:/perforce_depot


--------------------------------------------------------------------
* run.py
--------------------------------------------------------------------

Runs tests (solution result binary).

Syntax:
>> run.py target platform config tool workspace

Usage example: 
>> run.py Blast-Tests win32 debug vc11 D:/perforce_depot


--------------------------------------------------------------------
* batch.py
--------------------------------------------------------------------

Runs batches of the previous scripts (for all platforms, configs and
 etc.) depending on arguments.

Syntax:
>> batch.py [-h] [-g] [-b] [-r]
  -h, --help      show help
  -g, --generate  generate all
  -b, --build     build all
  -r, --run       run all

Usage example: 
>> batch.py -g -b


--------------------------------------------------------------------
* release.py
--------------------------------------------------------------------

Builds and packs artifacts in zip. Made for releasing whole project.

Syntax:
release.py [-h] [-p PLATFORM] [-t TOOL] [-n] [-e]
  -h, --help      show help
  -p PLATFORM, --platform PLATFORM
                        Platform (e.g. win32, win64, linux32..)
  -t TOOL, --tool TOOL  Tool (e.g. vc11, vc12, make..)
  -n, --nophysx         Disable PhysX build
  -e, --echo_only       print directory tree for each artifact without
                        actually building or zipping.
Usage example: 
>> release.py -p win32
