#Platform specific compile flags and project includes

SET(CMAKE_CXX_FLAGS "-Wextra -fdiagnostics-show-option -fno-rtti -fno-exceptions -m64 -msse2 -mfpmath=sse -fpic -ffast-math -fno-exceptions -fno-rtti -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-multichar -Wno-sign-compare -std=c++0x")

if (CMAKE_COMPILER_IS_GNUCXX)
    #Starting with 6.0 -fPIE is the default and our gtest build can't link with this
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "6.0")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie")
    endif()
endif()

SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -gdwarf-2")
SET(CMAKE_CXX_FLAGS_CHECKED "-O3 -g3 -gdwarf-2 -fno-strict-aliasing")
SET(CMAKE_CXX_FLAGS_PROFILE "-O3 -fno-strict-aliasing")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -fno-strict-aliasing")

SET(BLASTTESTS_SLN_COMPILE_DEFS)
#NvBlastExt doesn't have the _CONSOLE flag

SET(BLASTTESTS_SLN_DEBUG_COMPILE_DEFS _DEBUG;NV_DEBUG=1;)
SET(BLASTTESTS_SLN_CHECKED_COMPILE_DEFS NDEBUG;NV_CHECKED=1;)
SET(BLASTTESTS_SLN_PROFILE_COMPILE_DEFS NDEBUG;NV_PROFILE=1;)
SET(BLASTTESTS_SLN_RELEASE_COMPILE_DEFS NDEBUG;)

SET(CMAKE_EXECUTABLE_SUFFIX ".elf")


# Include all of the projects
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/BlastUnitTests.cmake)
INCLUDE(${PROJECT_CMAKE_FILES_DIR}/BlastPerfTests.cmake)


