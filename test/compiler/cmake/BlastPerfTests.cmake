#
# Build BlastPerfTests Common
#


SET(TEST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
SET(PERF_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/perf)
SET(UTILS_SOURCE_DIR ${TEST_SOURCE_DIR}/utils)
SET(SHAREDUTILS_SOURCE_DIR ${BLAST_ROOT_DIR}/shared/utils)

SET(COMMON_SOURCE_DIR ${BLAST_ROOT_DIR}/sdk/common)
SET(SOLVER_SOURCE_DIR ${BLAST_ROOT_DIR}/sdk/lowlevel/source)


FIND_PACKAGE(PxSharedSDK ${PM_physxsdk_VERSION} REQUIRED)
FIND_PACKAGE(PhysXSDK ${PM_pxshared_VERSION} REQUIRED)
FIND_PACKAGE(GoogleTestNV $ENV{PM_GoogleTest-nv_VERSION} REQUIRED)


# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/BlastPerfTests.cmake)

SET(PERF_SOURCE_FILES
	${BLASTPERFTESTS_PLATFORM_COMMON_FILES}
	
	${PERF_SOURCE_DIR}/BlastBasePerfTest.h
	${PERF_SOURCE_DIR}/SolverPerfTests.cpp
	${PERF_SOURCE_DIR}/DamagePerfTests.cpp
)

SET(SDK_COMMON_FILES
	${COMMON_SOURCE_DIR}/NvBlastAssert.cpp
	${COMMON_SOURCE_DIR}/NvBlastAssert.h
	${COMMON_SOURCE_DIR}/NvBlastAtomic.cpp
	${COMMON_SOURCE_DIR}/NvBlastAtomic.h
	${COMMON_SOURCE_DIR}/NvBlastDLink.h
	${COMMON_SOURCE_DIR}/NvBlastFixedArray.h
	${COMMON_SOURCE_DIR}/NvBlastFixedBitmap.h
	${COMMON_SOURCE_DIR}/NvBlastFixedBoolArray.h
	${COMMON_SOURCE_DIR}/NvBlastFixedPriorityQueue.h
	${COMMON_SOURCE_DIR}/NvBlastGeometry.h
	${COMMON_SOURCE_DIR}/NvBlastIndexFns.h
	${COMMON_SOURCE_DIR}/NvBlastIteratorBase.h
	${COMMON_SOURCE_DIR}/NvBlastMath.h
	${COMMON_SOURCE_DIR}/NvBlastMemory.h
	${COMMON_SOURCE_DIR}/NvBlastPreprocessorInternal.h
	${COMMON_SOURCE_DIR}/NvBlastTime.cpp
	${COMMON_SOURCE_DIR}/NvBlastTime.h
	${COMMON_SOURCE_DIR}/NvBlastTimers.cpp
)

SET(SDK_SOLVER_FILES
	${SOLVER_SOURCE_DIR}/NvBlastActor.cpp
	${SOLVER_SOURCE_DIR}/NvBlastActor.h
	${SOLVER_SOURCE_DIR}/NvBlastFamilyGraph.cpp
	${SOLVER_SOURCE_DIR}/NvBlastFamilyGraph.h
	${SOLVER_SOURCE_DIR}/NvBlastActorSerializationBlock.cpp
	${SOLVER_SOURCE_DIR}/NvBlastActorSerializationBlock.h
	${SOLVER_SOURCE_DIR}/NvBlastAsset.cpp
	${SOLVER_SOURCE_DIR}/NvBlastAsset.h
	${SOLVER_SOURCE_DIR}/NvBlastSupportGraph.h
	${SOLVER_SOURCE_DIR}/NvBlastChunkHierarchy.h
	${SOLVER_SOURCE_DIR}/NvBlastFamily.cpp
	${SOLVER_SOURCE_DIR}/NvBlastFamily.h
)

SET(UTILS_SOURCE_FILES
	${SHAREDUTILS_SOURCE_DIR}/AssetGenerator.cpp
	${SHAREDUTILS_SOURCE_DIR}/AssetGenerator.h

	${UTILS_SOURCE_DIR}/TaskDispatcher.h
	${UTILS_SOURCE_DIR}/TestAssets.cpp
	${UTILS_SOURCE_DIR}/TestAssets.h
	${UTILS_SOURCE_DIR}/TestProfiler.h
)



ADD_EXECUTABLE(BlastPerfTests 
	${PERF_SOURCE_FILES}
	${UTILS_SOURCE_FILES}
	
	${SDK_COMMON_FILES}
	${SDK_SOLVER_FILES}
)

set_target_properties(BlastPerfTests 
	PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	CHECKED_POSTFIX ${CMAKE_CHECKED_POSTFIX}
#	RELEASE_POSTFIX ${CMAKE_RELEASE_POSTFIX}
	PROFILE_POSTFIX ${CMAKE_PROFILE_POSTFIX}
)


SOURCE_GROUP("Source" FILES ${PERF_SOURCE_FILES})
SOURCE_GROUP("Utils" FILES ${UTILS_SOURCE_FILES})
SOURCE_GROUP("Sdk\\common" FILES ${SDK_COMMON_FILES})
SOURCE_GROUP("Sdk\\solver" FILES ${SDK_SOLVER_FILES})


# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(BlastPerfTests 
	PRIVATE ${BLASTPERFTESTS_PLATFORM_INCLUDES}

	PRIVATE ${TEST_SOURCE_DIR}
	PRIVATE ${UTILS_SOURCE_DIR}
	
	PRIVATE ${BLAST_ROOT_DIR}/sdk/common
	PRIVATE ${BLAST_ROOT_DIR}/sdk/profiler
	PRIVATE ${BLAST_ROOT_DIR}/sdk/lowlevel/include
	PRIVATE ${BLAST_ROOT_DIR}/sdk/lowlevel/source
	PRIVATE ${BLAST_ROOT_DIR}/shared/utils

	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
	PRIVATE ${GOOGLETEST_INCLUDE_DIRS}
	
)

TARGET_COMPILE_DEFINITIONS(BlastPerfTests
	PRIVATE ${BLASTPERFTESTS_COMPILE_DEFS}
)

SET_TARGET_PROPERTIES(BlastPerfTests PROPERTIES 
	COMPILE_PDB_NAME_DEBUG "BlastPerfTests${CMAKE_DEBUG_POSTFIX}"
	COMPILE_PDB_NAME_CHECKED "BlastPerfTests${CMAKE_CHECKED_POSTFIX}"
	COMPILE_PDB_NAME_PROFILE "BlastPerfTests${CMAKE_PROFILE_POSTFIX}"
	COMPILE_PDB_NAME_RELEASE "BlastPerfTests${CMAKE_RELEASE_POSTFIX}"
)

#TARGET_COMPILE_OPTIONS(BlastPerfTests PRIVATE /wd4005 /wd4244)
TARGET_COMPILE_OPTIONS(BlastPerfTests
	PRIVATE ${BLASTPERFTESTS_PLATFORM_COMPILE_OPTIONS}
)


TARGET_LINK_LIBRARIES(BlastPerfTests 

	PRIVATE NvBlastExtShaders NvBlastTk NvBlastExtSerialization ${GOOGLETEST_LIBRARIES} 
	PRIVATE ${BLASTPERFTESTS_PLATFORM_LINKED_LIBS}
)

# This is ugly, but have to include this after the target is defined
# This will be used for stuff like POST_BUILD commands that are platform specific
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/BlastPerfTests-AT.cmake OPTIONAL)




