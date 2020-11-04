#
# Build ApexImporter common
#

SET(APEXIMPORTER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/ApexImporter/src)
SET(TOOLS_COMMON_DIR ${BLAST_ROOT_DIR}/tools/common)
SET(TOOLS_COMMON_DIR ${BLAST_ROOT_DIR}/tools/common)
SET(APEX_MODULES_DIR ${BLAST_ROOT_DIR}/sdk/extensions/import/apexmodules)

FIND_PACKAGE(PxSharedSDK ${PM_physxsdk_VERSION} REQUIRED)
FIND_PACKAGE(PhysXSDK ${PM_pxshared_VERSION} REQUIRED)
FIND_PACKAGE(tclap $ENV{PM_tclap_VERSION} REQUIRED)
FIND_PACKAGE(FBXSDK $ENV{PM_FBXSDK_VERSION} REQUIRED)


# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/ApexImporter.cmake)

SET(COMMON_FILES
	${APEXIMPORTER_PLATFORM_COMMON_FILES}
	
	${APEXIMPORTER_SOURCE_DIR}/Main.cpp
	${APEXIMPORTER_SOURCE_DIR}/ApexDestructibleObjExporter.cpp
	${APEXIMPORTER_SOURCE_DIR}/ApexDestructibleObjExporter.h
	${BLAST_ROOT_DIR}/tools/common/BlastDataExporter.cpp
	${BLAST_ROOT_DIR}/tools/common/BlastDataExporter.h
)

ADD_EXECUTABLE(ApexImporter 
	${COMMON_FILES}
)

set_target_properties(ApexImporter 
	PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	CHECKED_POSTFIX ${CMAKE_CHECKED_POSTFIX}
	RELEASE_POSTFIX ${CMAKE_RELEASE_POSTFIX}
	PROFILE_POSTFIX ${CMAKE_PROFILE_POSTFIX}
)

SOURCE_GROUP("src" FILES ${COMMON_FILES})

# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(ApexImporter 
	PRIVATE ${APEXIMPORTER_PLATFORM_INCLUDES}

	PRIVATE ${BLAST_ROOT_DIR}/sdk/common
	PRIVATE ${TOOLS_COMMON_DIR}
	
	PRIVATE ${PHYSXSDK_INCLUDE_DIRS}
	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
	PRIVATE ${TCLAP_INCLUDE_DIRS}
	PRIVATE ${FBXSDK_INCLUDE_DIRS}
	
	PRIVATE ${APEX_MODULES_DIR}/nvparutils
	
	PRIVATE ${APEX_MODULES_DIR}/NvParameterized/include
)

TARGET_COMPILE_DEFINITIONS(ApexImporter 
	PRIVATE ${APEXIMPORTER_COMPILE_DEFS}
)

SET_TARGET_PROPERTIES(ApexImporter PROPERTIES 
	COMPILE_PDB_NAME_DEBUG "ApexImporter${CMAKE_DEBUG_POSTFIX}"
	COMPILE_PDB_NAME_CHECKED "ApexImporter${CMAKE_CHECKED_POSTFIX}"
	COMPILE_PDB_NAME_PROFILE "ApexImporter${CMAKE_PROFILE_POSTFIX}"
	COMPILE_PDB_NAME_RELEASE "ApexImporter${CMAKE_RELEASE_POSTFIX}"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(ApexImporter 
	PRIVATE NvBlast NvBlastExtPhysX NvBlastExtAuthoring NvBlastExtImport NvBlastExtExporter NvBlastExtSerialization NvBlastExtTkSerialization NvBlastExtPxSerialization Rpcrt4 
	PRIVATE ${FBXSDK_LIBRARIES}
)

ADD_CUSTOM_COMMAND(TARGET ApexImporter POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	${PHYSXSDK_DLLS}
	${BL_EXE_OUTPUT_DIR}
)
