# - Try to find GoogleTest SDK
# - Sets GOOGLETEST_LIBRARIES - lists of the libraries found
# - Sets GOOGLETEST_INCLUDE_DIRS 

include(FindPackageHandleStandardArgs)

# Find the includes

# TODO: Do the version stuff properly!
find_path(GOOGLETEST_PATH include/gtest/gtest.h
	PATHS 
	$ENV{PM_googletest_PATH}
	${GW_DEPS_ROOT}/$ENV{PM_googletest_NAME}/${GoogleTestNV_FIND_VERSION}
)

if (TARGET_BUILD_PLATFORM STREQUAL "Windows")
	# If the project pulling in this dependency needs the static crt, then append that to the path.

	if (STATIC_WINCRT)
		SET(GOOGLETEST_CRT_SUFFIX "-staticcrt")
	else()
		SET(GOOGLETEST_CRT_SUFFIX "")
	endif()

	if (CMAKE_CL_64)
		SET(GOOGLETEST_ARCH_FOLDER "win64")
	else()
		SET(GOOGLETEST_ARCH_FOLDER "win32")
	endif()
	
	if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 18.0.0.0 AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0.0.0)
		SET(VS_STR "vc12")
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.0.0.0)
		SET(VS_STR "vc14")
	else()
		MESSAGE(FATAL_ERROR "Failed to find compatible FBXSDK - Only supporting VS2013 and VS2015")
	endif()


	# Now find all of the PhysX libs in the lib directory

	SET(LIB_PATH ${GOOGLETEST_PATH}/lib/${VS_STR}${GOOGLETEST_ARCH_FOLDER}-cmake${GOOGLETEST_CRT_SUFFIX})

elseif(TARGET_BUILD_PLATFORM STREQUAL "PS4")
	SET(LIB_PATH ${GOOGLETEST_PATH}/lib/PS4)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
elseif(TARGET_BUILD_PLATFORM STREQUAL "XboxOne")
	SET(LIB_PATH ${GOOGLETEST_PATH}/lib/xboxone-${XDK_VERSION})
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
elseif(TARGET_BUILD_PLATFORM STREQUAL "linux")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.0)
		SET(LIB_PATH ${GOOGLETEST_PATH}/lib/gcc-5.4)
	else()
		SET(LIB_PATH ${GOOGLETEST_PATH}/lib/gcc-4.8)
	endif()
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
endif()

MESSAGE("GTEst libpath:" ${LIB_PATH})

if(TARGET_BUILD_PLATFORM STREQUAL "linux")
	find_library(GTEST_LIB
		NAMES gtest
		PATHS ${LIB_PATH}
	)

	find_library(GTEST_MAIN_LIB
		NAMES gtest_main
		PATHS ${LIB_PATH}
	)

    SET(GTEST_LIB_DEBUG ${GTEST_LIB})
    SET(GTEST_MAIN_LIB_DEBUG ${GTEST_MAIN_LIB})
else()
    find_library(GTEST_LIB
        NAMES gtest
        PATHS ${LIB_PATH}/Release
    )

    find_library(GTEST_LIB_DEBUG
        NAMES gtest
        PATHS ${LIB_PATH}/Debug
    )

    find_library(GTEST_MAIN_LIB
        NAMES gtest_main
        PATHS ${LIB_PATH}/Release
    )

    find_library(GTEST_MAIN_LIB_DEBUG
        NAMES gtest_main
        PATHS ${LIB_PATH}/Debug
    )
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(GOOGLETEST
	DEFAULT_MSG
	GOOGLETEST_PATH

    GTEST_LIB 
    GTEST_MAIN_LIB 
    GTEST_LIB_DEBUG 
    GTEST_MAIN_LIB_DEBUG 
)

if (GOOGLETEST_FOUND)
	# NOTE: This include list is way too long and reaches into too many internals.
	# Also may not be good enough for all users.
	SET(GOOGLETEST_INCLUDE_DIRS 
		${GOOGLETEST_PATH}/include 
		${GOOGLETEST_PATH}/include/gtest 
	)
	
	SET(GOOGLETEST_LIBS_RELEASE ${GTEST_LIB} ${GTEST_MAIN_LIB}
		CACHE STRING ""
	)
	SET(GOOGLETEST_LIBS_DEBUG ${GTEST_LIB_DEBUG} ${GTEST_MAIN_LIB_DEBUG}
		CACHE STRING ""
	)
	
	SET(GOOGLETEST_LIBRARIES "" CACHE STRING "")
	
	foreach(x ${GOOGLETEST_LIBS_RELEASE})
		list(APPEND GOOGLETEST_LIBRARIES optimized ${x})
	endforeach()
	
	foreach(x ${GOOGLETEST_LIBS_DEBUG})
		list(APPEND GOOGLETEST_LIBRARIES debug ${x})
	endforeach()
endif()
