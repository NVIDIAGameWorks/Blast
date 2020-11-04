#
# Build LegacyConverter common
#

SET(LEGACYCONVERTER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/LegacyConverter/src)

FIND_PACKAGE(tclap $ENV{PM_tclap_VERSION} REQUIRED)
FIND_PACKAGE(PxSharedSDK ${PM_physxsdk_VERSION} REQUIRED)
FIND_PACKAGE(PhysXSDK ${PM_pxshared_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/LegacyConverter.cmake)

SET(COMMON_FILES
	${LEGACYCONVERTER_PLATFORM_COMMON_FILES}
	
	${LEGACYCONVERTER_SOURCE_DIR}/Main.cpp
)

ADD_EXECUTABLE(LegacyConverter 
	${COMMON_FILES}
)

set_target_properties(LegacyConverter 
	PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
	CHECKED_POSTFIX ${CMAKE_CHECKED_POSTFIX}
	RELEASE_POSTFIX ${CMAKE_RELEASE_POSTFIX}
	PROFILE_POSTFIX ${CMAKE_PROFILE_POSTFIX}
)


SOURCE_GROUP("src" FILES ${COMMON_FILES})

# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(LegacyConverter 
	PRIVATE ${LEGACYCONVERTER_PLATFORM_INCLUDES}
	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
	PRIVATE ${BLAST_ROOT_DIR}/source/common

	PRIVATE ${TCLAP_INCLUDE_DIRS}
)

TARGET_COMPILE_DEFINITIONS(LegacyConverter 
	PRIVATE ${LEGACYCONVERTER_COMPILE_DEFS}
)

SET_TARGET_PROPERTIES(LegacyConverter PROPERTIES 
	PDB_NAME_DEBUG "LegacyConverter${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "LegacyConverter${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "LegacyConverter${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "LegacyConverter${CMAKE_RELEASE_POSTFIX}"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(LegacyConverter NvBlast NvBlastTk NvBlastExtPhysX NvBlastExtImport NvBlastExtExporter NvBlastExtSerialization NvBlastExtTkSerialization NvBlastExtPxSerialization)


ADD_CUSTOM_COMMAND(TARGET LegacyConverter POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	${PHYSXSDK_DLLS}
	${BL_EXE_OUTPUT_DIR}
)
