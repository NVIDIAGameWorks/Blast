#
# Build NvBlastTk common
#

SET(TOOLKIT_DIR ${PROJECT_SOURCE_DIR}/toolkit)
SET(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/common)

FIND_PACKAGE(PxSharedSDK ${PM_physxsdk_VERSION} REQUIRED)
FIND_PACKAGE(PhysXSDK ${PM_pxshared_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/NvBlastTk.cmake)

SET(COMMON_FILES
	${BLASTTK_PLATFORM_COMMON_FILES}
	
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
#	${COMMON_SOURCE_DIR}/NvBlastIndexFns.cpp
	${COMMON_SOURCE_DIR}/NvBlastIndexFns.h
	${COMMON_SOURCE_DIR}/NvBlastIteratorBase.h
	${COMMON_SOURCE_DIR}/NvBlastMath.h
	${COMMON_SOURCE_DIR}/NvBlastMemory.h
	${COMMON_SOURCE_DIR}/NvBlastPreprocessorInternal.h
	${COMMON_SOURCE_DIR}/NvBlastTime.cpp
	${COMMON_SOURCE_DIR}/NvBlastTime.h
	${COMMON_SOURCE_DIR}/NvBlastTimers.cpp
	${COMMON_SOURCE_DIR}/NvBlastArray.h
	${COMMON_SOURCE_DIR}/NvBlastHashMap.h
	${COMMON_SOURCE_DIR}/NvBlastHashSet.h
)

SET(PUBLIC_FILES
	${TOOLKIT_DIR}/include/NvBlastTk.h
	${TOOLKIT_DIR}/include/NvBlastTkActor.h
	${TOOLKIT_DIR}/include/NvBlastTkAsset.h
	${TOOLKIT_DIR}/include/NvBlastTkEvent.h
	${TOOLKIT_DIR}/include/NvBlastTkFamily.h
	${TOOLKIT_DIR}/include/NvBlastTkFramework.h
	${TOOLKIT_DIR}/include/NvBlastTkGroup.h
	${TOOLKIT_DIR}/include/NvBlastTkIdentifiable.h
	${TOOLKIT_DIR}/include/NvBlastTkJoint.h
	${TOOLKIT_DIR}/include/NvBlastTkObject.h
	${TOOLKIT_DIR}/include/NvBlastTkType.h
)

SET(TOOLKIT_FILES
	${TOOLKIT_DIR}/source/NvBlastTkActorImpl.cpp
	${TOOLKIT_DIR}/source/NvBlastTkActorImpl.h
	${TOOLKIT_DIR}/source/NvBlastTkAssetImpl.cpp
	${TOOLKIT_DIR}/source/NvBlastTkAssetImpl.h
	${TOOLKIT_DIR}/source/NvBlastTkCommon.h
	${TOOLKIT_DIR}/source/NvBlastTkEventQueue.h
	${TOOLKIT_DIR}/source/NvBlastTkFamilyImpl.cpp
	${TOOLKIT_DIR}/source/NvBlastTkFamilyImpl.h
	${TOOLKIT_DIR}/source/NvBlastTkFrameworkImpl.cpp
	${TOOLKIT_DIR}/source/NvBlastTkFrameworkImpl.h
	${TOOLKIT_DIR}/source/NvBlastTkGroupImpl.cpp
	${TOOLKIT_DIR}/source/NvBlastTkGroupImpl.h
	${TOOLKIT_DIR}/source/NvBlastTkGUID.h
	${TOOLKIT_DIR}/source/NvBlastTkJointImpl.cpp
	${TOOLKIT_DIR}/source/NvBlastTkJointImpl.h
	${TOOLKIT_DIR}/source/NvBlastTkTaskImpl.cpp
	${TOOLKIT_DIR}/source/NvBlastTkTaskImpl.h
	${TOOLKIT_DIR}/source/NvBlastTkTypeImpl.h
)

ADD_LIBRARY(NvBlastTk ${BLASTTK_LIBTYPE} 
	${COMMON_FILES}
	${PUBLIC_FILES}
	${TOOLKIT_FILES}
)

SOURCE_GROUP("common" FILES ${COMMON_FILES})
SOURCE_GROUP("public" FILES ${PUBLIC_FILES})
SOURCE_GROUP("toolkit" FILES ${TOOLKIT_FILES})

# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(NvBlastTk 
	PRIVATE ${BLASTTK_PLATFORM_INCLUDES}

	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PUBLIC ${PROJECT_SOURCE_DIR}/lowlevel/include
	PUBLIC ${PROJECT_SOURCE_DIR}/toolkit/include
	
	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
	PRIVATE ${PHYSXSDK_INCLUDE_DIRS}
)

TARGET_COMPILE_DEFINITIONS(NvBlastTk 
	PRIVATE ${BLASTTK_COMPILE_DEFS}
)

TARGET_COMPILE_OPTIONS(NvBlastTk
	PRIVATE ${BLASTTK_PLATFORM_COMPILE_OPTIONS}
)

SET_TARGET_PROPERTIES(NvBlastTk PROPERTIES 
	PDB_NAME_DEBUG "NvBlastTk${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "NvBlastTk${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "NvBlastTk${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "NvBlastTk${CMAKE_RELEASE_POSTFIX}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BL_LIB_OUTPUT_DIR}/debug"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BL_DLL_OUTPUT_DIR}/debug"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BL_EXE_OUTPUT_DIR}/debug"
    ARCHIVE_OUTPUT_DIRECTORY_CHECKED "${BL_LIB_OUTPUT_DIR}/checked"
    LIBRARY_OUTPUT_DIRECTORY_CHECKED "${BL_DLL_OUTPUT_DIR}/checked"
    RUNTIME_OUTPUT_DIRECTORY_CHECKED "${BL_EXE_OUTPUT_DIR}/checked"
    ARCHIVE_OUTPUT_DIRECTORY_PROFILE "${BL_LIB_OUTPUT_DIR}/profile"
    LIBRARY_OUTPUT_DIRECTORY_PROFILE "${BL_DLL_OUTPUT_DIR}/profile"
    RUNTIME_OUTPUT_DIRECTORY_PROFILE "${BL_EXE_OUTPUT_DIR}/profile"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BL_LIB_OUTPUT_DIR}/release"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BL_DLL_OUTPUT_DIR}/release"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BL_EXE_OUTPUT_DIR}/release"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(NvBlastTk 
	PUBLIC NvBlast NvBlastGlobals

	PUBLIC ${BLASTTK_PLATFORM_LINKED_LIBS}
)

