# - Try to find CapnProto SDK
# - Sets CAPNPROTOSDK_LIBRARIES - list of the libraries found
# - Sets CAPNPROTOSDK_SOURCE_FILES 
# - Sets CAPNPROTOSDK_INCLUDE_DIRS 

include(FindPackageHandleStandardArgs)

# Find the includes

# TODO: Do the version stuff properly!
find_path(CAPNPROTOSDK_PATH src/capnp/message.h
	PATHS 
	$ENV{PM_CapnProto_PATH}
	${GW_DEPS_ROOT}/$ENV{PM_CapnProto_NAME}/${CapnProtoSDK_FIND_VERSION}
)

if (TARGET_BUILD_PLATFORM STREQUAL "Windows")

	if (STATIC_WINCRT)
		SET(CAPNPROTOSDK_CRT_SUFFIX "-mt")
	else()
		SET(CAPNPROTOSDK_CRT_SUFFIX "-md")
	endif()


	# If the project pulling in this dependency needs the static crt, then append that to the path.
	if (CMAKE_CL_64)
		SET(CAPNPROTOSDK_ARCH_FOLDER "win64")
	else()
		SET(CAPNPROTOSDK_ARCH_FOLDER "win32")
	endif()

	SET(LIB_PATH ${CAPNPROTOSDK_PATH}/bin/${CAPNPROTOSDK_ARCH_FOLDER})
    SET(EXE_PATH ${CAPNPROTOSDK_PATH}/tools/win32)


elseif(TARGET_BUILD_PLATFORM STREQUAL "PS4")
	SET(LIB_PATH ${CAPNPROTOSDK_PATH}/bin/ps4)
    SET(EXE_PATH ${CAPNPROTOSDK_PATH}/tools/win32)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
elseif(TARGET_BUILD_PLATFORM STREQUAL "XboxOne")
	SET(LIB_PATH ${CAPNPROTOSDK_PATH}/bin/xboxone)
    SET(EXE_PATH ${CAPNPROTOSDK_PATH}/tools/win32)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
elseif(TARGET_BUILD_PLATFORM STREQUAL "linux")
	SET(LIB_PATH ${CAPNPROTOSDK_PATH}/bin/ubuntu64)
	if (UE4_LINUX_CROSSCOMPILE)
		SET(EXE_PATH ${CAPNPROTOSDK_PATH}/tools/win32)
	else()
    	SET(EXE_PATH ${CAPNPROTOSDK_PATH}/tools/ubuntu64)
    endif()
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
endif()

find_library(CAPNPROTO_LIB
	NAMES capnp${CAPNPROTOSDK_CRT_SUFFIX}
	PATHS ${LIB_PATH}/Release
)
find_library(CAPNPROTO_LIB_DEBUG
	NAMES capnp${CAPNPROTOSDK_CRT_SUFFIX}
	PATHS ${LIB_PATH}/Debug
)

find_library(KJ_LIB
	NAMES kj${CAPNPROTOSDK_CRT_SUFFIX}
	PATHS ${LIB_PATH}/Release
)
find_library(KJ_LIB_DEBUG
	NAMES kj${CAPNPROTOSDK_CRT_SUFFIX}
	PATHS ${LIB_PATH}/Debug
)

find_program(CAPNP_EXECUTABLE
  NAMES capnp
  DOC "Cap'n Proto Command-line Tool"
  PATHS ${EXE_PATH}
)

find_program(CAPNPC_CXX_EXECUTABLE
  NAMES capnpc-c++
  DOC "Capn'n Proto C++ Compiler"
  PATHS ${EXE_PATH}
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CAPNPROTOSDK
	DEFAULT_MSG
	CAPNPROTOSDK_PATH

	CAPNP_EXECUTABLE
	CAPNPC_CXX_EXECUTABLE
	
	#CAPNPROTO_LIB
	#KJ_LIB
	
	#CAPNPROTO_LIB_DEBUG
	#KJ_LIB_DEBUG
)

if (CAPNPROTOSDK_FOUND)
	
	SET(CAPNPROTOSDK_INCLUDE_DIRS 
		${CAPNPROTOSDK_PATH}/src/
	)

	SET(CAPNP_INCLUDE_DIRS
		${CAPNPROTOSDK_INCLUDE_DIRS}
	)
	
	SET(CAPNPROTOSDK_LIBRARIES "" CACHE STRING "")
	SET(CAPNPROTOSDK_SOURCE_FILES "")
	
	IF ((TARGET_BUILD_PLATFORM STREQUAL "PLATFROM_USING_PREBUILT_LIBS"))
		LIST(APPEND CAPNPROTOSDK_LIBRARIES 
			optimized ${CAPNPROTO_LIB} debug ${CAPNPROTO_LIB_DEBUG}
			optimized ${KJ_LIB} debug ${KJ_LIB_DEBUG}
			)
	ELSE()
		#Include source files for the "lite" version only, there aren't too many it avoids needing many permutations of static libs
		LIST(APPEND CAPNPROTOSDK_SOURCE_FILES
			${CAPNPROTOSDK_PATH}/src/capnp/arena.c++
			${CAPNPROTOSDK_PATH}/src/capnp/blob.c++
			${CAPNPROTOSDK_PATH}/src/capnp/layout.c++
			${CAPNPROTOSDK_PATH}/src/capnp/message.c++
			${CAPNPROTOSDK_PATH}/src/capnp/serialize.c++

			${CAPNPROTOSDK_PATH}/src/kj/array.c++
			${CAPNPROTOSDK_PATH}/src/kj/common.c++
			${CAPNPROTOSDK_PATH}/src/kj/debug.c++
			${CAPNPROTOSDK_PATH}/src/kj/exception.c++
			${CAPNPROTOSDK_PATH}/src/kj/io.c++
			${CAPNPROTOSDK_PATH}/src/kj/mutex.c++
			${CAPNPROTOSDK_PATH}/src/kj/string.c++
			${CAPNPROTOSDK_PATH}/src/kj/units.c++
			)
	ENDIF()
endif()
