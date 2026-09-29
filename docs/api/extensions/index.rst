.. _pageextapi:

Extensions (NvBlastExt)
=======================

These are the current Blast extensions:

`Damage Shaders <ext_shaders.rst>`_ - Standard damage shaders (radial, shear, line segment) which can be used in NvBlast and NvBlastTk damage functions.

`Stress Solver <ext_stress.rst>`_ - A toolkit for performing stress calculations on low-level Blast actors, using a minimal API to assign masses and apply forces.  Does not use any external physics library.

`Asset Utilities <ext_assetutils.rst>`_ - NvBlastAsset utility functions.  Add world bonds, merge assets, and transform geometric data.

`Asset Authoring <ext_authoring.rst>`_ - Powerful tools for cleaning and fracturing meshes using voronoi, clustered voronoi, and slicing methods.

..
   :ref:`pageextexporter` - Standard mesh and collision writer tools in fbx, obj, and json formats.

`Serialization <ext_serialization.rst>`_ - Blast object serialization manager.  With the ExtTkSerialization and ExtPxSerialization extensions, can serialize assets for low-level, Tk, and ExtPhysX libraries using a variety of encodings.  This extension comes with low-level serializers built-in.

`BlastTk Serialization <ext_tkserialization.rst>`_ - This module contains serializers for NvBlastTk objects.  Use in conjunction with ExtSerialization.

..
   :ref:`pageextpxserialization` - This module contains serializers for ExtPhysX objects.  Use in conjunction  with ExtSerialization.

..
   :ref:`pageextphysx` - A reference implementation of a physics manager, using the PhysXSDK.  Creates and manages actors and joints, and handles impact damage and uses the stress solver (ExtStress) to handle stress calculations.

To use them, include the appropriate headers in include/extensions (each extension will describe which headers are necessary),
and link to the desired NvBlastExt*{config}{arch} library in the lib folder.  Here, config is the usual DEBUG/CHECKED/PROFILE (or nothing for release),
and {arch} distinguishes architecture, if needed (such as _x86 or _x64).
