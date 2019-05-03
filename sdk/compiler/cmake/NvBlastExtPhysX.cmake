#
# Build NvBlastExtPhysX Common
#


SET(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/common)

SET(COMMON_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/common/source)
SET(PHYSX_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/physx/source)
SET(COMMON_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/common/include)
SET(PHYSX_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/physx/include)

FIND_PACKAGE(PhysXSDK $ENV{PM_PhysX_VERSION} REQUIRED)
FIND_PACKAGE(PxSharedSDK $ENV{PM_PxShared_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/NvBlastExtPhysX.cmake)

SET(COMMON_FILES
	${BLASTEXT_PLATFORM_COMMON_FILES}
	
	${COMMON_SOURCE_DIR}/NvBlastAssert.cpp
	${COMMON_SOURCE_DIR}/NvBlastAssert.h

	${COMMON_SOURCE_DIR}/NvBlastArray.h
	${COMMON_SOURCE_DIR}/NvBlastHashMap.h
	${COMMON_SOURCE_DIR}/NvBlastHashSet.h
)

SET(PUBLIC_FILES
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtImpactDamageManager.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPx.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPxActor.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPxAsset.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPxCollisionBuilder.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPxFamily.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPxListener.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPxManager.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPxStressSolver.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtPxTask.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtSync.h
)

SET(EXT_PHYSICS_FILES
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtImpactDamageManager.cpp
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxStressSolverImpl.h
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxStressSolverImpl.cpp
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxActorImpl.h
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxActorImpl.cpp
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxAssetImpl.h
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxAssetImpl.cpp
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxCollisionBuilderImpl.cpp
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxCollisionBuilderImpl.h
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxFamilyImpl.h
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxFamilyImpl.cpp
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxManagerImpl.h
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxManagerImpl.cpp
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxTaskImpl.h
	${PHYSX_EXT_SOURCE_DIR}/physics/NvBlastExtPxTaskImpl.cpp
)

SET(EXT_CALLBACKS_FILES
    ${PHYSX_EXT_INCLUDE_DIR}/NvBlastExtCustomProfiler.h
	${PHYSX_EXT_INCLUDE_DIR}/NvBlastPxCallbacks.h
)

SET(EXT_SYNC_FILES
	${PHYSX_EXT_SOURCE_DIR}/sync/NvBlastExtSync.cpp
)

ADD_LIBRARY(NvBlastExtPhysX ${BLASTEXT_PHYSX_LIBTYPE}
	${COMMON_FILES}
	${PUBLIC_FILES}

	${EXT_PHYSICS_FILES}
	${EXT_SYNC_FILES}
	${EXT_CALLBACKS_FILES}
)

SOURCE_GROUP("common" FILES ${COMMON_FILES})
SOURCE_GROUP("public" FILES ${PUBLIC_FILES})
SOURCE_GROUP("src\\physics" FILES ${EXT_PHYSICS_FILES})
SOURCE_GROUP("src\\sync" FILES ${EXT_SYNC_FILES})
SOURCE_GROUP("src\\callbacks" FILES ${EXT_CALLBACKS_FILES})


# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(NvBlastExtPhysX 
	PRIVATE ${BLASTEXT_PLATFORM_INCLUDES}

	PUBLIC ${PROJECT_SOURCE_DIR}/lowlevel/include
	PUBLIC ${PHYSX_EXT_INCLUDE_DIR}

	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${COMMON_EXT_SOURCE_DIR}
	
	PRIVATE ${PHYSX_EXT_SOURCE_DIR}/physics
	PRIVATE ${PHYSX_EXT_SOURCE_DIR}/sync
	
   	PUBLIC ${PROJECT_SOURCE_DIR}/extensions/profiler/include
	PUBLIC ${PROJECT_SOURCE_DIR}/extensions/authoringCommon/include

	PUBLIC ${PHYSXSDK_INCLUDE_DIRS}
	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
)

TARGET_COMPILE_DEFINITIONS(NvBlastExtPhysX
	PRIVATE ${BLASTEXT_COMPILE_DEFS}
)

# Warning disables for Capn Proto
TARGET_COMPILE_OPTIONS(NvBlastExtPhysX
	PRIVATE ${BLASTEXT_PLATFORM_COMPILE_OPTIONS}
)

SET_TARGET_PROPERTIES(NvBlastExtPhysX PROPERTIES 
	PDB_NAME_DEBUG "NvBlastExtPhysX${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "NvBlastExtPhysX${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "NvBlastExtPhysX${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "NvBlastExtPhysX${CMAKE_RELEASE_POSTFIX}"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(NvBlastExtPhysX 
	PUBLIC NvBlastTk NvBlastExtShaders NvBlastExtStress
	PUBLIC $<$<CONFIG:debug>:${PHYSX3_LIB_DEBUG}> $<$<CONFIG:debug>:${PHYSX3COOKING_LIB_DEBUG}> $<$<CONFIG:debug>:${PHYSX3EXTENSIONS_LIB_DEBUG}> $<$<CONFIG:debug>:${PXFOUNDATION_LIB_DEBUG}>
	PUBLIC $<$<CONFIG:checked>:${PHYSX3_LIB_CHECKED}> $<$<CONFIG:checked>:${PHYSX3COOKING_LIB_CHECKED}> $<$<CONFIG:checked>:${PHYSX3EXTENSIONS_LIB_CHECKED}> $<$<CONFIG:checked>:${PXFOUNDATION_LIB_CHECKED}>
	PUBLIC $<$<CONFIG:profile>:${PHYSX3_LIB_PROFILE}> $<$<CONFIG:profile>:${PHYSX3COOKING_LIB_PROFILE}> $<$<CONFIG:profile>:${PHYSX3EXTENSIONS_LIB_PROFILE}> $<$<CONFIG:profile>:${PXFOUNDATION_LIB_PROFILE}>
	PUBLIC $<$<CONFIG:release>:${PHYSX3_LIB}> $<$<CONFIG:release>:${PHYSX3COOKING_LIB}> $<$<CONFIG:release>:${PHYSX3EXTENSIONS_LIB}> $<$<CONFIG:release>:${PXFOUNDATION_LIB}>

	PUBLIC ${BLASTEXT_PLATFORM_LINKED_LIBS}
)
