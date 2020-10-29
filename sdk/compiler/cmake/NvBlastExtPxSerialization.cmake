#
# Build NvBlastExtPxSerialization Common
#

SET(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/common)

SET(SERIAL_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/serialization/source)
SET(SERIAL_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/serialization/include)
SET(TK_INCLUDE_DIR ${BLAST_ROOT_DIR}/sdk/toolkit/include)
SET(PHYSX_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/physx/include)
SET(PHYSX_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/physx/source)
SET(EXT_COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/common/source)
SET(EXT_COMMON_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/common/include)

SET(DTO_SOURCE_DIR ${SERIAL_EXT_SOURCE_DIR}/DTO)

SET(SOLVER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/lowlevel/source)

SET(SERIAL_GENERATED_SOURCE_DIR ${SERIAL_EXT_SOURCE_DIR}/generated)

FIND_PACKAGE(PhysXSDK $ENV{PM_PhysX_VERSION} REQUIRED)
FIND_PACKAGE(PxSharedSDK $ENV{PM_PxShared_VERSION} REQUIRED)
FIND_PACKAGE(CapnProtoSDK $ENV{PM_CapnProto_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/NvBlastExtPxSerialization.cmake)

# Compile the generated files for serialization

INCLUDE(CapnProtoGenerate)

SET(CAPNPC_OUTPUT_DIR ${SERIAL_GENERATED_SOURCE_DIR})
SET(CAPNPC_SRC_PREFIX ${SERIAL_EXT_SOURCE_DIR})
CAPNP_GENERATE_CPP(CAPNP_SRCS CAPNP_HDRS ${SERIAL_EXT_SOURCE_DIR}/NvBlastExtLlSerialization.capn ${SERIAL_EXT_SOURCE_DIR}/NvBlastExtTkSerialization.capn ${SERIAL_EXT_SOURCE_DIR}/NvBlastExtPxSerialization.capn)

SET(COMMON_FILES
	${BLASTEXTPXSERIALIZATION_PLATFORM_COMMON_FILES}
	
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
)
  

SET(EXT_SERIALIZATION_FILES
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtPxSerialization.capn

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtPxSerialization.cpp

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtSerializationCAPN.h

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtSerializationInternal.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtPxSerializerCAPN.h
	
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtPxSerializerRAW.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtPxSerializerRAW.cpp

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtTkSerializerRAW.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtTkSerializerRAW.cpp

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtOutputStream.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtOutputStream.cpp
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtInputStream.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtInputStream.cpp

	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtKJPxInputStream.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtKJPxInputStream.cpp
	
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtKJPxOutputStream.h
	${SERIAL_EXT_SOURCE_DIR}/NvBlastExtKJPxOutputStream.cpp
)

SET(DTO_SOURCE_FILES
	${DTO_SOURCE_DIR}/DTOMacros.h
	${DTO_SOURCE_DIR}/AssetDTO.h
	${DTO_SOURCE_DIR}/AssetDTO.cpp
	${DTO_SOURCE_DIR}/TkAssetDTO.h
	${DTO_SOURCE_DIR}/TkAssetDTO.cpp
	${DTO_SOURCE_DIR}/ExtPxAssetDTO.h
	${DTO_SOURCE_DIR}/ExtPxAssetDTO.cpp
	${DTO_SOURCE_DIR}/PxVec3DTO.h
	${DTO_SOURCE_DIR}/PxVec3DTO.cpp
	${DTO_SOURCE_DIR}/NvBlastChunkDTO.h
	${DTO_SOURCE_DIR}/NvBlastChunkDTO.cpp
	${DTO_SOURCE_DIR}/NvBlastBondDTO.h
	${DTO_SOURCE_DIR}/NvBlastBondDTO.cpp
	${DTO_SOURCE_DIR}/NvBlastIDDTO.h
	${DTO_SOURCE_DIR}/NvBlastIDDTO.cpp
	${DTO_SOURCE_DIR}/TkAssetJointDescDTO.h
	${DTO_SOURCE_DIR}/TkAssetJointDescDTO.cpp
	${DTO_SOURCE_DIR}/ExtPxChunkDTO.h
	${DTO_SOURCE_DIR}/ExtPxChunkDTO.cpp
	${DTO_SOURCE_DIR}/ExtPxSubchunkDTO.h
	${DTO_SOURCE_DIR}/ExtPxSubchunkDTO.cpp
	${DTO_SOURCE_DIR}/PxQuatDTO.h
	${DTO_SOURCE_DIR}/PxQuatDTO.cpp
	${DTO_SOURCE_DIR}/PxTransformDTO.h
	${DTO_SOURCE_DIR}/PxTransformDTO.cpp
	${DTO_SOURCE_DIR}/PxMeshScaleDTO.h
	${DTO_SOURCE_DIR}/PxMeshScaleDTO.cpp
	${DTO_SOURCE_DIR}/PxConvexMeshGeometryDTO.h
	${DTO_SOURCE_DIR}/PxConvexMeshGeometryDTO.cpp
)

SET(EXT_SERIALIZATION_INCLUDES
	${SERIAL_EXT_INCLUDE_DIR}/NvBlastExtPxSerialization.h
)

ADD_LIBRARY(NvBlastExtPxSerialization ${BLASTEXTPXSERIALIZATION_LIB_TYPE} 
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

TARGET_INCLUDE_DIRECTORIES(NvBlastExtPxSerialization 
	PRIVATE ${BLASTEXTPXSERIALIZATION_PLATFORM_INCLUDES}

	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${PROJECT_SOURCE_DIR}/lowlevel/include
	PRIVATE ${PROJECT_SOURCE_DIR}/lowlevel/source

	PRIVATE ${PX_INCLUDE_DIR}
	
	PUBLIC ${SERIAL_EXT_INCLUDE_DIR}
	PUBLIC ${SERIAL_EXT_SOURCE_DIR}
	PUBLIC ${DTO_SOURCE_DIR}
	
	PRIVATE ${PHYSX_EXT_INCLUDE_DIR}
	PRIVATE ${PHYSX_EXT_SOURCE_DIR}

	PRIVATE ${EXT_COMMON_SOURCE_DIR}
	PRIVATE ${EXT_COMMON_INCLUDE_DIR}
	
	PUBLIC ${CAPNPROTOSDK_INCLUDE_DIRS}
	
	PRIVATE ${COMMON_SOURCE_DIR}

	PRIVATE ${PHYSXSDK_INCLUDE_DIRS}
	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
)

TARGET_COMPILE_DEFINITIONS(NvBlastExtPxSerialization
	PUBLIC CAPNP_LITE=1
	PRIVATE ${BLASTEXTPXSERIALIZATION_COMPILE_DEFS}
)

# Warning disables for Capn Proto
TARGET_COMPILE_OPTIONS(NvBlastExtPxSerialization
	PRIVATE ${BLASTEXTPXSERIALIZATION_COMPILE_OPTIONS}
)

SET_TARGET_PROPERTIES(NvBlastExtPxSerialization PROPERTIES
	PDB_NAME_DEBUG "NvBlastExtPxSerialization${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "NvBlastExtPxSerialization${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "NvBlastExtPxSerialization${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "NvBlastExtPxSerialization${CMAKE_RELEASE_POSTFIX}"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(NvBlastExtPxSerialization 
	PRIVATE NvBlast NvBlastGlobals NvBlastTk NvBlastExtPhysX ${CAPNPROTOSDK_LIBRARIES}	
	PUBLIC $<$<CONFIG:debug>:${PHYSX_LIB_DEBUG}> $<$<CONFIG:debug>:${PHYSXCOOKING_LIB_DEBUG}>
	PUBLIC $<$<CONFIG:checked>:${PHYSX_LIB_CHECKED}> $<$<CONFIG:checked>:${PHYSXCOOKING_LIB_CHECKED}>
	PUBLIC $<$<CONFIG:profile>:${PHYSX_LIB_PROFILE}> $<$<CONFIG:profile>:${PHYSXCOOKING_LIB_PROFILE}>
	PUBLIC $<$<CONFIG:release>:${PHYSX_LIB}> $<$<CONFIG:release>:${PHYSXCOOKING_LIB}>
)

#Hack for now to force these to build serialially to fix fighting over writing the generated code
ADD_DEPENDENCIES(NvBlastExtPxSerialization NvBlastExtTkSerialization NvBlastExtSerialization)
