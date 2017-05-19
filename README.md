Blast 1.0
=========

![Alt text](/images/blast.png?raw=true "Blast Intro")

Introduction
------------

Blast is a new NVIDIA GameWorks destruction library.  It consists of three layers: the low-level (NvBlast), a high-level "toolkit"
wrapper (NvBlastTk), and extensions (prefixed with NvBlastExt).  This layered API is designed to allow short ramp-up time for
first usage (through the Ext and Tk APIs) while also allowing for customization and optimization by experienced users through the
low-level API.

This library is intended to replace APEX Destruction.  It is being developed with the goal of addressing shortcomings in
performance, stability, and customizability of the APEX Destruction module.

Some notable features of NvBlast:
* C-style API consisting of stateless functions, with no global framework or context.
* Functions do not spawn tasks, allocate, or deallocate memory.
* A support structure may be defined that includes chunks from different hierarchical depths.
* Multiple chunk hierarchies may exist in a single asset.
* Damage behavior is completely defined by user-supplied "shader" functions.
* Has a portable memory layout for assets and actor families, which allows for memcopy cloning and direct binary serialization (on platforms with the same endianness).

Features of NvBlastTk:
* C++ API which includes a global framework.
* Manages objects, allocating and deallocating using a user-supplied callback.
* Spawns multiple tasks to process damage, through a user-supplied task manager to allow multithreading.
* Uses an event system to inform the user of actor splitting and chunk fracturing.
* Introduces a joint representation which uses the event system to allow the user to update physical joints between actors.

Notably absent from NvBlast and NvBlastTk:
* There is no physics or collision representation.
* There is no graphics representation.

Blast, at the low-level and toolkit layer are physics and graphics agnostic.  It is entirely up to the user to create such representations
when blast objects are created.  Updates to those objects (such as actor splitting) are passed to the user as the output of a split
function in the low-level API, or through a split event in the toolkit API.  This allows Blast to be used with any physics SDK and any
rendering library.

In order to help the user get started quickly, however, there is a PhysX-specific Blast extension which uses BlastTk and manages PhysX actors
and joints.  The source code for this extension, like all Blast extensions, is intended to be a reference implementation.

Current blast extensions:
* ExtAuthoring - a set of geometric tools which can split a mesh hierarchically and create a Blast asset, along with collision geometry and chunk graphics meshes in a separate files.
* ExtConverterLL - a data format converter for low-level assets and actor families.  This simple converter uses user-defined conversion functions.
* ExtImport - provides functions to import an APEX Destructible Asset to create a Blast asset.
* ExtPhysX - a physics manager using PhysX which keeps PxActors and PxJoints updated in a user-supplied PxScene.  It handles impact damage (through the contact callback), includes a stress solver, and provides a listener that enables multiple clients to keep their state synchronized.
* ExtSerialization and ExtSerializationLL - serialization extensions for Tk and the low-level, which uses Cap'n Proto to provide robust serialization across different platforms.
* ExtShaders - sample damage shaders to pass to both the low-level and Tk actor damage functions.

Documentation
-------------

See docs/api_docs/index.html for api documentation.

See docs/source_docs/index.html for full source doxygen pages.

See docs/release_notes.txt for changes.

Compiling
---------

For windows (VS2013 and VS2015):
* Run generate_projects_vcNNwinBB.bat, where NN = 12 or 14, and BB = 32 or 64, depending on which compiler
(vc12/vc14) you're using and which OS style (32 or 64 bit) you're targeting.  This step will download all necessary
dependencies that are not already downloaded into a folder NVIDIA/packman-repo at the root of your hard drive, so
this might take some time the first time one of these scripts is run (or when a dependency version changes).
* Open compiler/vcNNwinBB-cmake/BlastAll.sln.  This contains all Blast windows projects, including the
low-level, toolkit, extensions, tools, tests, and sample.
* If you run the sample, you should first run download_sample_resources.bat.  This will load complex asset
files with nontrivial graphics meshes.  Without these assets, only procedurally-generated box assets are available
in the sample.

For linux:
* Run generate_projects_linux.sh.  This step will download all necessary dependencies that are not already
downloaded into a folder NVIDIA/packman-repo at the root of your hard drive, so this might take some time the first
time the script is run (or when a dependency version changes).
* Makefiles will be generated in compiler/linux64-CONFIG-gcc, where CONFIG = debug or release.
These will build all Blast linux projects, including the low-level, toolkit, extensions, and tests.

For PS4 and XBoxOne:
* Please visit developer.nvidia.com in order to contact NVIDIA for further information.

Tools and Samples Binaries (Windows only)
-----------------------------------------

Blast tools and sample executables, along with all necessary supporting libraries, are packaged in the
blast_tools_and_samples-windows.zip file.  This allows someone without a development environment to use these
applications.

Gallery
---------

### Tower explosion
![Alt text](/images/tower_explode.png?raw=true "Blast Sample: tower explode")
### Bunny impact damage
![Alt text](/images/bunny_impact.png?raw=true "Blast Sample: bunny impact")
### Layered cube explosion
![Alt text](/images/cube_explode.png?raw=true "Blast Sample: cube explode")
### Table impact damage
![Alt text](/images/table_impact_wireframe.png?raw=true "Blast Sample: table impact")
### Tower slice
![Alt text](/images/tower_slice.png?raw=true "Blast Sample: tower slice")
### Wall impact damage
![Alt text](/images/wall_impact.png?raw=true "Blast Sample: wall impact")
### Stress solver
![Alt text](/images/stress.png?raw=true "Blast Sample: stress solver")
### Joints
![Alt text](/images/joints.png?raw=true "Blast Sample: joints")
