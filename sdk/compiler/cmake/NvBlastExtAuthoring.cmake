#
# Build NvBlastExt Common
#


SET(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/common)

SET(AUTHORING_COMMON_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/authoringCommon/source)
SET(AUTHORING_COMMON_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/authoringCommon/include)

SET(AUTHORING_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/authoring/source)
SET(COMMON_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/common/source)
SET(AUTHORING_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/authoring/include)

FIND_PACKAGE(PxSharedSDK ${PM_physxsdk_VERSION} REQUIRED)
FIND_PACKAGE(PhysXSDK ${PM_pxshared_VERSION} REQUIRED)
FIND_PACKAGE(BoostMultiprecision $ENV{PM_BoostMultiprecision_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/NvBlastExtAuthoring.cmake)

SET(COMMON_FILES
	${BLASTEXT_PLATFORM_COMMON_FILES}
	${COMMON_SOURCE_DIR}/NvBlastPxSharedHelpers.h
	#${COMMON_SOURCE_DIR}/NvBlastAssert.cpp
	#${COMMON_SOURCE_DIR}/NvBlastAssert.h
)

SET(PUBLIC_FILES
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringBondGenerator.h
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringFractureTool.h
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoring.h
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringMeshCleaner.h
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringCutout.h

	${AUTHORING_COMMON_EXT_INCLUDE_DIR}/NvBlastExtAuthoringConvexMeshBuilder.h	
	${AUTHORING_COMMON_EXT_INCLUDE_DIR}/NvBlastExtAuthoringPatternGenerator.h
	${AUTHORING_COMMON_EXT_INCLUDE_DIR}/NvBlastExtAuthoringMesh.h
	${AUTHORING_COMMON_EXT_INCLUDE_DIR}/NvBlastExtAuthoringTypes.h
	${AUTHORING_COMMON_EXT_INCLUDE_DIR}/NvBlastExtAuthoringAccelerator.h
)

SET(EXT_AUTHORING_FILES
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringBondGeneratorImpl.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringBondGeneratorImpl.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringBooleanTool.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringBooleanTool.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringCollisionBuilderImpl.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringCollisionBuilderImpl.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringPerlinNoise.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringTriangulator.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringTriangulator.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringVSA.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringFractureToolImpl.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringFractureToolImpl.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtTriangleProcessor.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringMeshNoiser.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringMeshNoiser.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtTriangleProcessor.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtApexSharedParts.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtApexSharedParts.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoring.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringMeshCleanerImpl.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringMeshCleanerImpl.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringCutoutImpl.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringCutoutImpl.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringPatternGeneratorImpl.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringPatternGeneratorImpl.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringMeshUtils.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringMeshUtils.cpp
	
	${AUTHORING_COMMON_EXT_SOURCE_DIR}/NvBlastExtAuthoringAccelerator.cpp
	${AUTHORING_COMMON_EXT_SOURCE_DIR}/NvBlastExtAuthoringMeshImpl.cpp
	${AUTHORING_COMMON_EXT_SOURCE_DIR}/NvBlastExtAuthoringMeshImpl.h
	${AUTHORING_COMMON_EXT_SOURCE_DIR}/NvBlastExtAuthoringInternalCommon.h	
)

SET(VHACD_SOURCE_FILES
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/btAlignedAllocator.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/btConvexHullComputer.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/FloatMath.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/FloatMath.inl
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/VHACD.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/VHACD-ASYNC.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/vhacdICHull.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/vhacdManifoldMesh.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/vhacdMesh.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/vhacdRaycastMesh.cpp
	${AUTHORING_EXT_SOURCE_DIR}/VHACD/src/vhacdVolume.cpp
)

SET_SOURCE_FILES_PROPERTIES(${VHACD_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "${VHACD_COMPILE_FLAGS}")


ADD_LIBRARY(NvBlastExtAuthoring ${BLAST_EXT_SHARED_LIB_TYPE} 
	${COMMON_FILES}
	${PUBLIC_FILES}

	${EXT_AUTHORING_FILES}
	${VHACD_SOURCE_FILES}
)

SOURCE_GROUP("common" FILES ${COMMON_FILES})
SOURCE_GROUP("public" FILES ${PUBLIC_FILES})
SOURCE_GROUP("src" FILES ${EXT_AUTHORING_FILES})
SOURCE_GROUP("VHACD/src" FILES ${VHACD_SOURCE_FILES})


# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(NvBlastExtAuthoring 
	PRIVATE ${BLASTEXT_PLATFORM_INCLUDES}

	PUBLIC ${PROJECT_SOURCE_DIR}/lowlevel/include
	PUBLIC ${AUTHORING_EXT_INCLUDE_DIR}
	PUBLIC ${AUTHORING_COMMON_EXT_INCLUDE_DIR}
	PRIVATE ${AUTHORING_COMMON_EXT_SOURCE_DIR}
	
	PRIVATE ${PROJECT_SOURCE_DIR}/toolkit/include
		
	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${COMMON_EXT_SOURCE_DIR}
	
	PRIVATE ${AUTHORING_EXT_SOURCE_DIR}

	PRIVATE ${PROJECT_SOURCE_DIR}/extensions/assetutils/include
	PRIVATE ${PROJECT_SOURCE_DIR}/extensions/RT/source

	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
	PRIVATE ${PHYSXSDK_INCLUDE_DIRS}

	PRIVATE ${BOOSTMULTIPRECISION_INCLUDE_DIRS}
	
	PRIVATE ${AUTHORING_EXT_SOURCE_DIR}/VHACD/inc
	PRIVATE ${AUTHORING_EXT_SOURCE_DIR}/VHACD/public
)

TARGET_COMPILE_DEFINITIONS(NvBlastExtAuthoring
	PRIVATE ${BLASTEXT_COMPILE_DEFS}
)

# Warning disables for Capn Proto
TARGET_COMPILE_OPTIONS(NvBlastExtAuthoring
	PRIVATE ${BLASTEXT_PLATFORM_COMPILE_OPTIONS}
)

SET_TARGET_PROPERTIES(NvBlastExtAuthoring PROPERTIES 
	PDB_NAME_DEBUG "NvBlastExtAuthoring${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "NvBlastExtAuthoring${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "NvBlastExtAuthoring${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "NvBlastExtAuthoring${CMAKE_RELEASE_POSTFIX}"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(NvBlastExtAuthoring 
	PUBLIC NvBlast NvBlastGlobals
	PUBLIC ${BLASTEXT_PLATFORM_LINKED_LIBS}
)
