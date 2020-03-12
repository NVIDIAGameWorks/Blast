# - Try to find FBX SDK
# - Sets FBXSDK_LIBRARIES - list of the libraries found
# - Sets FBXSDK_INCLUDE_DIRS 
# - Sets FBXSDK_DLLS - List of the DLLs to copy to the bin directory of projects that depend on this


include(FindPackageHandleStandardArgs)

# Find the includes

# TODO: Do the version stuff properly!
find_path(FBXSDK_PATH include/fbxsdk.h
	PATHS 
	$ENV{PM_FBXSDK_PATH}
	${GW_DEPS_ROOT}/FBXSDK/${FBXSDK_FIND_VERSION}
)

if (STATIC_WINCRT)
	SET(FBXSDK_CRT_SUFFIX "mt")
else()
	SET(FBXSDK_CRT_SUFFIX "md")
endif()


# If the project pulling in this dependency needs the static crt, then append that to the path.
if (CMAKE_CL_64)
	SET(FBXSDK_ARCH_FOLDER "x64")
else()
	SET(FBXSDK_ARCH_FOLDER "x86")
endif()

# What compiler version do we want?

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 18.0.0.0 AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0.0.0)
	SET(VS_STR "vs2013")
elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.0.0.0)
	SET(VS_STR "vs2015")
else()
	MESSAGE(FATAL_ERROR "Failed to find compatible FBXSDK - Only supporting VS2013 and VS2015")
endif()

# Now find all of the PhysX libs in the lib directory

SET(LIB_PATH ${FBXSDK_PATH}/lib/${VS_STR}/${FBXSDK_ARCH_FOLDER})

find_library(FBX_LIB
	NAMES libfbxsdk-${FBXSDK_CRT_SUFFIX}
	PATHS ${LIB_PATH}/release
)
find_library(FBX_LIB_DEBUG
	NAMES libfbxsdk-${FBXSDK_CRT_SUFFIX}
	PATHS ${LIB_PATH}/debug
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(FBXSDK
	DEFAULT_MSG
	FBXSDK_PATH
	FBX_LIB
	
	FBX_LIB_DEBUG
)

if (FBXSDK_FOUND)
	
	SET(FBXSDK_INCLUDE_DIRS 
		${FBXSDK_PATH}/include 
		${FBXSDK_PATH}/include/fbxsdk
	)
	
	SET(FBXSDK_LIBRARIES "" CACHE STRING "")
	
	LIST(APPEND FBXSDK_LIBRARIES optimized ${FBX_LIB} debug ${FBX_LIB_DEBUG})
endif()
