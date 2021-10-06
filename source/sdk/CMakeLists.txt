cmake_minimum_required(VERSION 3.3)

project(BlastSDK CXX)

CMAKE_POLICY(SET CMP0057 NEW) # Enable IN_LIST

IF(NOT DEFINED BLAST_ROOT_DIR)
	
	STRING(REPLACE "\\" "/" BRD_TEMP $ENV{BLAST_ROOT_DIR})
	
	# This env variable is set by GenerateProjects.bat, and is no longer available when CMake rebuilds, so this stores it in the cache
	SET(BLAST_ROOT_DIR ${BRD_TEMP} CACHE INTERNAL "Root of the Blast source tree")

ENDIF()

IF(NOT EXISTS ${BLAST_ROOT_DIR})
	MESSAGE(FATAL_ERROR "BLAST_ROOT_DIR environment variable wasn't set or was invalid.")
ENDIF()

SET(GW_DEPS_ROOT $ENV{PM_PACKAGES_ROOT})

# Add the project specific CMake modules to the module path
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/compiler/cmake/modules/)

IF(NOT DEFINED CMAKEMODULES_VERSION)
	SET(CMAKEMODULES_VERSION $ENV{PM_CMakeModules_VERSION} CACHE INTERNAL "CMakeModules version from generation batch")
ENDIF()

IF(NOT EXISTS $ENV{PM_CMakeModules_PATH})
	MESSAGE(FATAL_ERROR "Could not find $ENV{PM_CMakeModules_PATH}")
ENDIF()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{PM_CMakeModules_PATH}")

MESSAGE("Module path:" ${CMAKE_MODULE_PATH})

IF(CMAKE_CONFIGURATION_TYPES)
	SET(CMAKE_CONFIGURATION_TYPES debug profile checked release)
	SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
		"Reset config to what we need" 
		FORCE)
	
	SET(CMAKE_SHARED_LINKER_FLAGS_CHECKED "")
	SET(CMAKE_SHARED_LINKER_FLAGS_PROFILE "")
	
ENDIF()

# Default to appending "DEBUG", "PROFILE", etc to produced artifacts
IF(NOT DEFINED APPEND_CONFIG_NAME)
	SET(APPEND_CONFIG_NAME ON)
ENDIF()

IF (APPEND_CONFIG_NAME)
	MESSAGE("Appending config to output names")

	SET(CMAKE_DEBUG_POSTFIX "")
	SET(CMAKE_PROFILE_POSTFIX "")
	SET(CMAKE_CHECKED_POSTFIX "")
	SET(CMAKE_RELEASE_POSTFIX "")
ENDIF()

INCLUDE(SetOutputPaths)

# Either have to define a single output path, or each one.

IF(NOT DEFINED BL_OUTPUT_DIR)
	IF (NOT DEFINED BL_LIB_OUTPUT_DIR)
		MESSAGE(FATAL_ERROR "BL_LIB_OUTPUT_DIR not defined - Define either BL_OUTPUT_DIR or BL_LIB_OUTPUT_DIR and BL_DLL_OUTPUT_DIR and BL_EXE_OUTPUT_DIR")
	ENDIF()

	IF (NOT DEFINED BL_DLL_OUTPUT_DIR)
		MESSAGE(FATAL_ERROR "BL_DLL_OUTPUT_DIR not defined - Define either BL_OUTPUT_DIR or BL_LIB_OUTPUT_DIR and BL_DLL_OUTPUT_DIR and BL_EXE_OUTPUT_DIR")
	ENDIF()

	IF (NOT DEFINED BL_EXE_OUTPUT_DIR)
		MESSAGE(FATAL_ERROR "BL_EXE_OUTPUT_DIR not defined - Define either BL_OUTPUT_DIR or BL_LIB_OUTPUT_DIR and BL_DLL_OUTPUT_DIR and BL_EXE_OUTPUT_DIR")
	ENDIF()
	
	SetLibOutputPath(${BL_LIB_OUTPUT_DIR})
	SetDllOutputPath(${BL_DLL_OUTPUT_DIR})
	SetExeOutputPath(${BL_EXE_OUTPUT_DIR})
ELSE()
	SetSingleOutputPath(${BL_OUTPUT_DIR})
ENDIF()

SET(PROJECT_CMAKE_FILES_DIR compiler/cmake/)

# Include the platform specific configuration
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/CMakeLists.txt)
