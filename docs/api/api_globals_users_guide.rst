.. _pageglobalsapi:

Globals API (NvBlastGlobals)
----------------------------

The NvBlastGlobals library is a utility library which is used by NvBlastTk (see :ref:`pagehlapi`) and some extensions (see :ref:`pageextapi`) and samples.

It provides a global allocator, error callback, and profiler API.

.. _globalsallocator:

Allocator
=========
**Include NvBlastGlobals.h**


A global allocator with interface

.. code-block:: text

    Nv::Blast::AllocatorCallback


may be set by the user with the function

.. code-block:: text

    NvBlastGlobalSetAllocatorCallback


and accessed using

.. code-block:: text

    NvBlastGlobalGetAllocatorCallback


An internal, default allocator is used if the user does not set their own, or if NULL is passed into NvBlastGlobalSetAllocatorCallback.

This allocator is used by NvBlastTk, as well as any extension that allocates memory.  In addition, utility macros are provided such as
**NVBLAST_ALLOC**, **NVBLAST_FREE**, **NVBLAST_NEW**, and **NVBLAST_DELETE**.

.. _globalserror:

Error Callback
==============

**Include NvBlastGlobals.h**

A global error message callback with interface

.. code-block:: text

    Nv::Blast::ErrorCallback


may be set by the user with the function

.. code-block:: text

    NvBlastGlobalSetErrorCallback


and accessed using

.. code-block:: text

    NvBlastGlobalGetErrorCallback


An internal, default error callback is used if the user does not set their own, or if NULL is passed into NvBlastGlobalSetErrorCallback.

This error callback is used by NvBlastTk, as well as many extensions.  In addition, utility macros are provided such as
**NVBLAST_LOG_ERROR** and **NVBLAST_LOG_WARNING**.

Finally, a function with signature given by NvBlastLog is provided which uses the global error callback,

.. code-block:: text

    Nv::Blast::logLL


This function may be passed into any NvBlast function's log parameter.

.. _globalsprofiler:

Profiler API
============

**Include NvBlastProfiler.h**

BlastTk contains many profiling zones which use the global profiler which can be accessed in this library.  The user may implement
the interface

.. code-block:: text

    Nv::Blast::ProfilerCallback


and pass it to the globals library using

.. code-block:: text

    NvBlastProfilerSetCallback


A NULL pointer may be passed in, disabling profiling.  Profiler features are only active in checked, debug and profile builds.

The granularity of events reported can be selected with

.. code-block:: text

    NvBlastProfilerSetDetail



