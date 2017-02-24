#Platform specific compile flags and project includes

SET(CMAKE_CXX_FLAGS "-Wextra -Werror -pedantic -fdiagnostics-show-option -fno-rtti -fno-exceptions -m64 -msse2 -mfpmath=sse -fpic -ffast-math -fno-exceptions -fno-rtti -Wno-invalid-offsetof -Wno-unknown-pragmas -std=c++0x")

SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -gdwarf-2")
SET(CMAKE_CXX_FLAGS_CHECKED "-O3 -g3 -gdwarf-2 -fno-strict-aliasing")
SET(CMAKE_CXX_FLAGS_PROFILE "-O3 -fno-strict-aliasing")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -fno-strict-aliasing")

SET(BLAST_SLN_COMPILE_DEFS )

SET(BLAST_SLN_DEBUG_COMPILE_DEFS _DEBUG;NV_DEBUG=1;)
SET(BLAST_SLN_CHECKED_COMPILE_DEFS NDEBUG;NV_CHECKED=1;)
SET(BLAST_SLN_PROFILE_COMPILE_DEFS NDEBUG;NV_PROFILE=1;)
SET(BLAST_SLN_RELEASE_COMPILE_DEFS NDEBUG;)

# Include all of the projects
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlast.cmake)
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlastTk.cmake)
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlastExtConverterLL.cmake)
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlastExtShaders.cmake)
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlastExtPhysX.cmake)
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlastExtAuthoring.cmake)
#INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlastExtImport.cmake)
#INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlastExtSerialization.cmake)
#INCLUDE(${PROJECT_CMAKE_FILES_DIR}/NvBlastExtSerializationLL.cmake)

