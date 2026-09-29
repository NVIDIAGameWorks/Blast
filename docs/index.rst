Blast SDK Documentation
=======================

.. image:: images/blast.png
    :width: 100%
    :alt: Blast Intro

Blast is a NVIDIA Omniverse destruction library.  It consists of three layers: the low-level (NvBlast), a high-level "toolkit"
wrapper (NvBlastTk), and extensions (prefixed with NvBlastExt).  This layered API is designed to allow short ramp-up time for
first usage (through the Ext and Tk APIs) while also allowing for customization and optimization by experienced users through the
low-level API.

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
* Generates "worker" objects to process damage, which the user may call from multiple threads.
* Uses an event system to inform the user of actor splitting and chunk fracturing.
* Introduces a joint representation which uses the event system to allow the user to update physical joints between actors.

Notably absent from NvBlast and NvBlastTk:

* There is no physics or collision representation.
* There is no graphics representation.

Blast, at the low-level and toolkit layer, is physics and graphics agnostic.  It is entirely up to the user to create such representations
when Blast objects are created.  Updates to those objects (such as actor splitting) are passed to the user as the output of a split
function in the low-level API, or through a split event in the toolkit API.  This allows Blast to be used with any physics SDK and any
rendering library.

In order to help the user get started quickly, however, there is a PhysX-specific Blast extension which uses BlastTk and manages PhysX actors
and joints.  The source code for this extension, like all Blast extensions, is intended to be a reference implementation.

Current Blast extensions:

* ExtAssetUtils - NvBlastAsset utility functions. Add external bonds, merge assets, and transform geometric data. 
* ExtAuthoring - a set of geometric tools which can split a mesh hierarchically and create a Blast asset, along with collision geometry and chunk graphics meshes in a separate files.
* ExtExporter - standard mesh and collision writer tools in fbx, obj, and json formats. 
* ExtSerialization and ExtTkSerialization - serialization extensions for low-level and Tk layers. Uses Cap'n Proto to provide robust serialization across different platforms.
* ExtShaders - sample damage shaders to pass to both the low-level and Tk actor damage functions.
* ExtStress - a toolkit for performing stress calculations on low-level Blast actors, using a minimal API to assign masses and apply forces. Does not use any external physics library. 

..
    Additional Blast extensions.  These depend on PhysX:

    * ExtPhysX - a physics manager using PhysX which keeps PxActors and PxJoints updated in a user-supplied PxScene.  It handles impact damage (through the contact callback), includes a stress solver wrapper, and provides a listener that enables multiple clients to keep their state synchronized.
    * ExtPxSerialization - serialization extensions for ExtPhysX.  Uses Cap'n Proto to provide robust serialization across different platforms.

Contents
--------

* `Introduction <api/introduction.rst>`_
* `Low Level API (NvBlast) <api/api_ll_users_guide.rst>`_
* `Globals API (NvBlastGlobals) <api/api_globals_users_guide.rst>`_
* `High Level API (NvBlastTk) <api/api_hl_users_guide.rst>`_
* `Extensions (NvBlastExt) <api/extensions/index.rst>`_
* `Definitions <api/definitions.rst>`_
* `Copyrights <api/copyrights.rst>`_
* `C++ API headers <../include/>`_

Gallery
-------

Tower Explosion
####################################

.. image:: images/tower_explode.png
    :width: 100%
    :alt: Blast Intro

Bunny Impact Damage
####################################

.. image:: images/bunny_impact.png
    :width: 100%
    :alt: Blast Sample: tower explode

Layered Cube Explosion
####################################

.. image:: images/cube_explode.png
    :width: 100%
    :alt: Blast Sample: bunny impact

Table Impact Damage
####################################

.. image:: images/table_impact_wireframe.png
    :width: 100%
    :alt: Blast Sample: cube explode

Tower Slice
####################################

.. image:: images/tower_slice.png
    :width: 100%
    :alt: Blast Sample: table impact

Wall Impact Damage
####################################

.. image:: images/wall_impact.png
    :width: 100%
    :alt: Blast Sample: tower slice

Stress Solver
####################################

.. image:: images/stress.png
    :width: 100%
    :alt: Blast Sample: wall impact

Joints
####################################

.. image:: images/joints.png
    :width: 100%
    :alt: Blast Sample: joints
