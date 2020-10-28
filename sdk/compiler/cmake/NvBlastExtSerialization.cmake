#
# Build NvBlastExtSerialization Common
#

SET(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/common)

SET(SERIAL_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/serialization/source)
SET(SERIAL_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/serialization/include)

SET(DTO_SOURCE_DIR ${SERIAL_EXT_SOURCE_DIR}/DTO)

SET(SOLVER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/lowlevel/source)

SET(SERIAL_GENERATED_SOURCE_DIR ${SERIAL_EXT_SOURCE_DIR}/generated)

FIND_PACKAGE(PxSharedSDK $ENV{PM_PxShared_VERSION} REQUIRED)
FIND_PACKAGE(PhysXSDK $ENV{PM_PhysX_VERSION} REQUIRED)

FIND_PACKAGE(CapnProtoSDK $ENV{PM_CapnProto_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/NvBlastExtSerialization.cmake)

# Compile the generated files for serialization

INCLUDE(CapnProtoGenerate)

SET(CAPNPC_OUTPUT_DIR ${SERIAL_GENERATED_SOURCE_DIR})
SET(CAPNPC_SRC_PREFIX ${SERIAL_EXT_SOURCE_DIR})
CAPNP_GENERATE_CPP(CAPNP_SRCS CAPNP_HDRS ${SERIAL_EXT_SOURCE_DIR}/NvBlastExtLlSerialization.capn)

SET(COMMON_FILES
	${BLASTEXTSERIALIZATION_PLATFORM_COMMON_FILES}
	
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
  

SET(EXT_SERIALIZATION_FILES
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtLlSerialization.capn

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtSerialization.cpp
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtLlSerialization.cpp

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtSerializationCAPN.h

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtSerializationInternal.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtLlSerializerCAPN.h
	
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtOutputStream.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtOutputStream.cpp
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtInputStream.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtInputStream.cpp
)

SET(DTO_SOURCE_FILES
	${DTO_SOURCE_DIR}/DTOMacros.h
	${DTO_SOURCE_DIR}/AssetDTO.h
	${DTO_SOURCE_DIR}/AssetDTO.cpp
	${DTO_SOURCE_DIR}/NvBlastChunkDTO.h
	${DTO_SOURCE_DIR}/NvBlastChunkDTO.cpp
	${DTO_SOURCE_DIR}/NvBlastBondDTO.h
	${DTO_SOURCE_DIR}/NvBlastBondDTO.cpp
	${DTO_SOURCE_DIR}/NvBlastIDDTO.h
	${DTO_SOURCE_DIR}/NvBlastIDDTO.cpp
)

SET(EXT_SERIALIZATION_INCLUDES
	${SERIAL_EXT_INCLUDE_DIR}/NvBlastExtSerialization.h
	${SERIAL_EXT_INCLUDE_DIR}/NvBlastExtLlSerialization.h
)

ADD_LIBRARY(NvBlastExtSerialization ${BLASTEXTSERIALIZATION_LIB_TYPE} 
	${COMMON_FILES}

	${DTO_SOURCE_FILES}
	
	${EXT_SERIALIZATION_INCLUDES}
	${EXT_SERIALIZATION_FILES}
	
	${CAPNP_SRCS}
	${CAPNP_HDRS}
	
	${MD5_FILES}

	${CAPNPROTOSDK_SOURCE_FILES}
)

SOURCE_GROUP("common" FILES ${COMMON_FILES})

SOURCE_GROUP("include" FILES ${EXT_SERIALIZATION_INCLUDES})
SOURCE_GROUP("src\\serialization" FILES ${EXT_SERIALIZATION_FILES})
SOURCE_GROUP("src\\serialization\\DTO" FILES ${DTO_SOURCE_FILES})
SOURCE_GROUP("src\\serialization\\generated" FILES ${CAPNP_SRCS} ${CAPNP_HDRS})
SOURCE_GROUP("src\\serialization\\CapnProtoSDK" FILES ${CAPNPROTOSDK_SOURCE_FILES})

# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(NvBlastExtSerialization 
	PRIVATE ${BLASTEXTSERIALIZATION_PLATFORM_INCLUDES}

	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${PROJECT_SOURCE_DIR}/lowlevel/include
	PRIVATE ${PROJECT_SOURCE_DIR}/lowlevel/source

	PRIVATE ${TK_INCLUDE_DIR}
	
	PUBLIC ${SERIAL_EXT_INCLUDE_DIR}
	PUBLIC ${SERIAL_EXT_SOURCE_DIR}
	PUBLIC ${DTO_SOURCE_DIR}
	
	PUBLIC ${CAPNPROTOSDK_INCLUDE_DIRS}
	
	PRIVATE ${COMMON_SOURCE_DIR}

	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
	PRIVATE ${PHYSXSDK_INCLUDE_DIRS}
)

TARGET_COMPILE_DEFINITIONS(NvBlastExtSerialization
	PUBLIC CAPNP_LITE=1;BLAST_LL_ALLOC=1
	PRIVATE ${BLASTEXTSERIALIZATION_COMPILE_DEFS}
)

# Warning disables for Capn Proto
TARGET_COMPILE_OPTIONS(NvBlastExtSerialization
	PRIVATE ${BLASTEXTSERIALIZATION_COMPILE_OPTIONS}
)

SET_TARGET_PROPERTIES(NvBlastExtSerialization PROPERTIES 
	PDB_NAME_DEBUG "NvBlastExtSerialization${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "NvBlastExtSerialization${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "NvBlastExtSerialization${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "NvBlastExtSerialization${CMAKE_RELEASE_POSTFIX}"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(NvBlastExtSerialization 
	PRIVATE NvBlast NvBlastGlobals ${CAPNPROTOSDK_LIBRARIES}
)
