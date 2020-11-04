#
# Build NvBlastExt Common
#


SET(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/common)

SET(COMMON_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/common/source)
SET(IMPORT_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/import/source)
SET(IMPORT_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/import/include)
SET(PHYSX_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/physx/include)


SET(APEX_MODULES_DIR ${PROJECT_SOURCE_DIR}/extensions/import/apexmodules)

FIND_PACKAGE(PxSharedSDK ${PM_physxsdk_VERSION} REQUIRED)
FIND_PACKAGE(PhysXSDK ${PM_pxshared_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/NvBlastExtImport.cmake)

SET(COMMON_FILES
	${BLASTEXT_PLATFORM_COMMON_FILES}
	
	${COMMON_SOURCE_DIR}/NvBlastAssert.cpp
	${COMMON_SOURCE_DIR}/NvBlastAssert.h
)

SET(PUBLIC_FILES
	${IMPORT_EXT_INCLUDE_DIR}/NvBlastExtApexImportTool.h
)

SET(NV_PARAM_INCLUDE ${APEX_MODULES_DIR}/include)


SET(EXT_IMPORT_FILES
	${IMPORT_EXT_SOURCE_DIR}/NvBlastExtApexImportTool.cpp
)

file(GLOB_RECURSE MODULES_SOURCES ${APEX_MODULES_DIR}/modules/*.cpp ${APEX_MODULES_DIR}/modules/*.h)
file(GLOB_RECURSE NV_PARAM_SOURCES ${APEX_MODULES_DIR}/NvParameterized/*.cpp ${APEX_MODULES_DIR}/NvParameterized/*.h ${APEX_MODULES_DIR}/NvParameterized/*.inl)


ADD_LIBRARY(NvBlastExtImport STATIC
	${COMMON_FILES}
	${PUBLIC_FILES}
	${MODULES_SOURCES}
	${EXT_IMPORT_FILES}
	${NV_PARAM_SOURCES}	
)

SOURCE_GROUP("common" FILES ${COMMON_FILES})
SOURCE_GROUP("public" FILES ${PUBLIC_FILES})
SOURCE_GROUP("src" FILES ${EXT_IMPORT_FILES})

SOURCE_GROUP("modules" FILES ${MODULES_SOURCES})
SOURCE_GROUP("NvParameterized" FILES ${NV_PARAM_SOURCES})


# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(NvBlastExtImport 
	PRIVATE ${BLASTEXT_PLATFORM_INCLUDES}

	PUBLIC ${PROJECT_SOURCE_DIR}/lowlevel/include
	PUBLIC ${PROJECT_SOURCE_DIR}/toolkit/include
	PUBLIC ${PROJECT_SOURCE_DIR}/extensions/exporter/include
	
	PUBLIC ${PHYSX_EXT_INCLUDE_DIR}
	PUBLIC ${IMPORT_EXT_INCLUDE_DIR}

	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${COMMON_EXT_SOURCE_DIR}
	
	PRIVATE ${IMPORT_EXT_SOURCE_DIR}
	
	PRIVATE ${PHYSXSDK_INCLUDE_DIRS}
	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}

	PRIVATE ${APEX_MODULES_DIR}/modules/common/include/autogen
	PRIVATE ${APEX_MODULES_DIR}/modules/common/include
	PRIVATE ${APEX_MODULES_DIR}/modules/common_legacy/include/autogen
	PRIVATE ${APEX_MODULES_DIR}/modules/common_legacy/include
	
	PRIVATE ${APEX_MODULES_DIR}/modules/destructible/include/autogen
	PRIVATE ${APEX_MODULES_DIR}/modules/destructible_legacy/include/autogen
	PRIVATE ${APEX_MODULES_DIR}/modules/destructible_legacy/include
	
	PRIVATE ${APEX_MODULES_DIR}/modules/framework/include/autogen
	PRIVATE ${APEX_MODULES_DIR}/modules/framework_legacy/include/autogen
	PRIVATE ${APEX_MODULES_DIR}/modules/framework_legacy/include
	
	PRIVATE ${APEX_MODULES_DIR}/nvparutils
	
	PRIVATE ${APEX_MODULES_DIR}/NvParameterized/include
)

TARGET_COMPILE_DEFINITIONS(NvBlastExtImport
	PRIVATE ${BLASTEXT_COMPILE_DEFS}
)

# Warning disables for Capn Proto
TARGET_COMPILE_OPTIONS(NvBlastExtImport
	PRIVATE ${BLASTEXT_PLATFORM_COMPILE_OPTIONS}
)

SET_TARGET_PROPERTIES(NvBlastExtImport PROPERTIES 
	PDB_NAME_DEBUG "NvBlastExtImport${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "NvBlastExtImport${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "NvBlastExtImport${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "NvBlastExtImport${CMAKE_RELEASE_POSTFIX}"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(NvBlastExtImport 
	PRIVATE NvBlast NvBlastTk NvBlastExtAuthoring
	PUBLIC ${BLASTEXT_PLATFORM_LINKED_LIBS}
	PUBLIC $<$<CONFIG:debug>:${PSFASTXML_LIB_DEBUG}>
	PUBLIC $<$<CONFIG:checked>:${PSFASTXML_LIB_CHECKED}>
	PUBLIC $<$<CONFIG:profile>:${PSFASTXML_LIB_PROFILE}>
	PUBLIC $<$<CONFIG:release>:${PSFASTXML_LIB}>
)
