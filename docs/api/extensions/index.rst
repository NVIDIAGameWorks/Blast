.. _pageextapi:

Extensions (NvBlastExt)
=======================

These are the current Blast extensions:

:ref:`pageextshaders` - Standard damage shaders (radial, shear, line segment) which can be used in NvBlast and NvBlastTk damage functions.

:ref:`pageextstress` - A toolkit for performing stress calculations on low-level Blast actors, using a minimal API to assign masses and apply forces.  Does not use any external physics library.

:ref:`pageextassetutils` - NvBlastAsset utility functions.  Add world bonds, merge assets, and transform geometric data.

:ref:`pageextauthoring` - Powerful tools for cleaning and fracturing meshes using voronoi, clustered voronoi, and slicing methods.

..
   :ref:`pageextexporter` - Standard mesh and collision writer tools in fbx, obj, and json formats.

:ref:`pageextserialization` - Blast object serialization manager.  With the ExtTkSerialization and ExtPxSerialization extensions, can serialize assets for low-level, Tk, and ExtPhysX libraries using a variety of encodings.  This extension comes with low-level serializers built-in.

:ref:`pageexttkserialization` - This module contains serializers for NvBlastTk objects.  Use in conjunction  with ExtSerialization.

..
   :ref:`pageextpxserialization` - This module contains serializers for ExtPhysX objects.  Use in conjunction  with ExtSerialization.

..
   :ref:`pageextphysx` - A reference implementation of a physics manager, using the PhysXSDK.  Creates and manages actors and joints, and handles impact damage and uses the stress solver (ExtStress) to handle stress calculations.

To use them, include the appropriate headers in include/extensions (each extension will describe which headers are necessary),
and link to the desired NvBlastExt*{config}{arch} library in the lib folder.  Here, config is the usual DEBUG/CHECKED/PROFILE (or nothing for release),
and {arch} distinguishes achitecture, if needed (such as _x86 or _x64).

.. toctree::
   :maxdepth: 2
   :caption: Contents
   :glob:

   ext_shaders.rst
   ext_stress.rst
   ext_assetutils.rst
   ext_authoring.rst
   ext_serialization.rst
   ext_tkserialization.rst

..
   ext_exporter.rst
   ext_pxserialization.rst
   ext_physx.rst
