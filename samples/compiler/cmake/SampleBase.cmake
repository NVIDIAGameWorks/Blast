#
# Build SampleBase Common
#


SET(SB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/SampleBase)
SET(SB_BLAST_SOURCE_DIR ${SB_SOURCE_DIR}/blast)
SET(SB_CORE_SOURCE_DIR ${SB_SOURCE_DIR}/core)
SET(SB_PHYSX_SOURCE_DIR ${SB_SOURCE_DIR}/physx)
SET(SB_RENDERER_SOURCE_DIR ${SB_SOURCE_DIR}/renderer)
SET(SB_SCENE_SOURCE_DIR ${SB_SOURCE_DIR}/scene)
SET(SB_UI_SOURCE_DIR ${SB_SOURCE_DIR}/ui)
SET(SB_UTILS_SOURCE_DIR ${SB_SOURCE_DIR}/utils)
SET(SHARED_UTILS_SOURCE_DIR ${BLAST_ROOT_DIR}shared/utils)


FIND_PACKAGE(PhysXSDK $ENV{PM_PhysX_VERSION} REQUIRED)
FIND_PACKAGE(PxSharedSDK $ENV{PM_PxShared_VERSION} REQUIRED)
FIND_PACKAGE(DXUT $ENV{PM_DXUT_VERSION} REQUIRED)
FIND_PACKAGE(DirectXTex $ENV{PM_DirectXTex_VERSION} REQUIRED)
FIND_PACKAGE(imgui $ENV{PM_imgui_VERSION} REQUIRED)
FIND_PACKAGE(tinyObjLoader $ENV{PM_tinyObjLoader_VERSION} REQUIRED)
FIND_PACKAGE(tclap $ENV{PM_tclap_VERSION} REQUIRED)
FIND_PACKAGE(hbao_plus $ENV{PM_hbao_plus_VERSION} REQUIRED)
FIND_PACKAGE(shadow_lib $ENV{PM_shadow_lib_VERSION} REQUIRED)
FIND_PACKAGE(d3dcompiler $ENV{PM_d3dcompiler_VERSION} REQUIRED)
FIND_PACKAGE(FBXSDK $ENV{PM_FBXSDK_VERSION} REQUIRED)
FIND_PACKAGE(nvToolsExt $ENV{PM_nvToolsExt_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/SampleBase.cmake)

SET(BLAST_FILES
	${SB_BLAST_SOURCE_DIR}/BlastFamily.cpp
	${SB_BLAST_SOURCE_DIR}/BlastFamily.h
	${SB_BLAST_SOURCE_DIR}/BlastFamilyBoxes.cpp
	${SB_BLAST_SOURCE_DIR}/BlastFamilyBoxes.h
	${SB_BLAST_SOURCE_DIR}/BlastFamilyModelSimple.cpp
	${SB_BLAST_SOURCE_DIR}/BlastFamilyModelSimple.h
	${SB_BLAST_SOURCE_DIR}/BlastFamilyModelSkinned.cpp
	${SB_BLAST_SOURCE_DIR}/BlastFamilyModelSkinned.h
	${SB_BLAST_SOURCE_DIR}/BlastAsset.cpp
	${SB_BLAST_SOURCE_DIR}/BlastAsset.h
	${SB_BLAST_SOURCE_DIR}/BlastAssetBoxes.cpp
	${SB_BLAST_SOURCE_DIR}/BlastAssetBoxes.h
	${SB_BLAST_SOURCE_DIR}/BlastAssetModel.cpp
	${SB_BLAST_SOURCE_DIR}/BlastAssetModel.h
	${SB_BLAST_SOURCE_DIR}/BlastAssetModelSimple.cpp
	${SB_BLAST_SOURCE_DIR}/BlastAssetModelSimple.h
	${SB_BLAST_SOURCE_DIR}/BlastAssetModelSkinned.cpp
	${SB_BLAST_SOURCE_DIR}/BlastAssetModelSkinned.h
	${SB_BLAST_SOURCE_DIR}/BlastController.cpp
	${SB_BLAST_SOURCE_DIR}/BlastController.h
	${SB_BLAST_SOURCE_DIR}/BlastModel.cpp
	${SB_BLAST_SOURCE_DIR}/BlastModel.h
	${SB_BLAST_SOURCE_DIR}/BlastReplay.cpp
	${SB_BLAST_SOURCE_DIR}/BlastReplay.h
)

SET(CORE_FILES
	${SB_CORE_SOURCE_DIR}/Application.cpp
	${SB_CORE_SOURCE_DIR}/Application.h
	${SB_CORE_SOURCE_DIR}/DeviceManager.cpp
	${SB_CORE_SOURCE_DIR}/DeviceManager.h
	${SB_CORE_SOURCE_DIR}/SampleController.cpp
	${SB_CORE_SOURCE_DIR}/SampleController.h
	${SB_CORE_SOURCE_DIR}/SampleManager.cpp
	${SB_CORE_SOURCE_DIR}/SampleManager.h
)

SET(PHYSX_FILES
	${SB_PHYSX_SOURCE_DIR}/PhysXController.cpp
	${SB_PHYSX_SOURCE_DIR}/PhysXController.h
)

SET(RENDERER_FILES
	${SB_RENDERER_SOURCE_DIR}/ConvexRenderMesh.cpp
	${SB_RENDERER_SOURCE_DIR}/ConvexRenderMesh.h
	${SB_RENDERER_SOURCE_DIR}/CustomRenderMesh.cpp
	${SB_RENDERER_SOURCE_DIR}/CustomRenderMesh.h
	${SB_RENDERER_SOURCE_DIR}/DebugRenderBuffer.h
	${SB_RENDERER_SOURCE_DIR}/Mesh.cpp
	${SB_RENDERER_SOURCE_DIR}/Mesh.h
	${SB_RENDERER_SOURCE_DIR}/PrimitiveRenderMesh.cpp
	${SB_RENDERER_SOURCE_DIR}/PrimitiveRenderMesh.h
	${SB_RENDERER_SOURCE_DIR}/Renderable.cpp
	${SB_RENDERER_SOURCE_DIR}/Renderable.h
	${SB_RENDERER_SOURCE_DIR}/Renderer.cpp
	${SB_RENDERER_SOURCE_DIR}/Renderer.h
	${SB_RENDERER_SOURCE_DIR}/RendererHBAO.cpp
	${SB_RENDERER_SOURCE_DIR}/RendererHBAO.h
	${SB_RENDERER_SOURCE_DIR}/RendererShadow.cpp
	${SB_RENDERER_SOURCE_DIR}/RendererShadow.h
	${SB_RENDERER_SOURCE_DIR}/RenderMaterial.cpp
	${SB_RENDERER_SOURCE_DIR}/RenderMaterial.h
	${SB_RENDERER_SOURCE_DIR}/RenderUtils.h
	${SB_RENDERER_SOURCE_DIR}/ResourceManager.cpp
	${SB_RENDERER_SOURCE_DIR}/ResourceManager.h
	${SB_RENDERER_SOURCE_DIR}/ShaderUtils.h
	${SB_RENDERER_SOURCE_DIR}/SkinnedRenderMesh.cpp
	${SB_RENDERER_SOURCE_DIR}/SkinnedRenderMesh.h
)

SET(SCENE_FILES
	${SB_SCENE_SOURCE_DIR}/SampleAssetListParser.cpp
	${SB_SCENE_SOURCE_DIR}/SampleAssetListParser.h
	${SB_SCENE_SOURCE_DIR}/SceneController.cpp
	${SB_SCENE_SOURCE_DIR}/SceneController.h
)

SET(UI_FILES
	${SB_UI_SOURCE_DIR}/CommonUIController.cpp
	${SB_UI_SOURCE_DIR}/CommonUIController.h
	${SB_UI_SOURCE_DIR}/DamageToolController.cpp
	${SB_UI_SOURCE_DIR}/DamageToolController.h
	${SB_UI_SOURCE_DIR}/imgui_impl_dx11.cpp
	${SB_UI_SOURCE_DIR}/imgui_impl_dx11.h
)

SET(UTIL_FILES
	${SHARED_UTILS_SOURCE_DIR}/AssetGenerator.cpp
	${SHARED_UTILS_SOURCE_DIR}/AssetGenerator.h
	${SB_UTILS_SOURCE_DIR}/SampleProfiler.cpp
	${SB_UTILS_SOURCE_DIR}/SampleProfiler.h
	${SB_UTILS_SOURCE_DIR}/SampleTime.h
	${SB_UTILS_SOURCE_DIR}/UIHelpers.h
	${SB_UTILS_SOURCE_DIR}/Utils.cpp
	${SB_UTILS_SOURCE_DIR}/Utils.h
	${SB_UTILS_SOURCE_DIR}/PxInputDataFromPxFileBuf.h
)

SET(ROOT_FILES
	${SB_SOURCE_DIR}/Sample.h
)

ADD_LIBRARY(SampleBase STATIC 
	${BLAST_FILES}
	${CORE_FILES}
	${PHYSX_FILES}
	${RENDERER_FILES}
	${SCENE_FILES}
	${UI_FILES}
	${UTIL_FILES}
	${ROOT_FILES}
	
	
	${IMGUI_SOURCE_FILES}
)

SOURCE_GROUP("Source" FILES ${ROOT_FILES})

SOURCE_GROUP("Source\\blast" FILES ${BLAST_FILES})
SOURCE_GROUP("Source\\core" FILES ${CORE_FILES})
SOURCE_GROUP("Source\\imgui" FILES ${IMGUI_SOURCE_FILES})
SOURCE_GROUP("Source\\physx" FILES ${PHYSX_FILES})
SOURCE_GROUP("Source\\renderer" FILES ${RENDERER_FILES})
SOURCE_GROUP("Source\\scene" FILES ${SCENE_FILES})
SOURCE_GROUP("Source\\ui" FILES ${UI_FILES})
SOURCE_GROUP("Source\\utils" FILES ${UTIL_FILES})

# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(SampleBase 
	PRIVATE ${SAMPLEBASE_PLATFORM_INCLUDES}

	PRIVATE ${DIRECTXTEX_INCLUDE_DIRS}
	PRIVATE ${DXUT_INCLUDE_DIRS}
	
	PRIVATE ${IMGUI_INCLUDE_DIRS}
	
	PRIVATE ${TINYOBJLOADER_INCLUDE_DIRS}
	PRIVATE ${TCLAP_INCLUDE_DIRS}
	PRIVATE ${HBAO_PLUS_INCLUDE_DIRS}
	PRIVATE ${SHADOW_LIB_INCLUDE_DIRS}

	PRIVATE ${SB_SOURCE_DIR}
	PRIVATE ${SB_SOURCE_DIR}/blast
	PRIVATE ${SB_SOURCE_DIR}/core
	PRIVATE ${SB_SOURCE_DIR}/physx
	PRIVATE ${SB_SOURCE_DIR}/renderer
	PRIVATE ${SB_SOURCE_DIR}/scene
	PRIVATE ${SB_SOURCE_DIR}/ui
	PRIVATE ${SB_SOURCE_DIR}/utils
	
	PRIVATE ${SHARED_UTILS_SOURCE_DIR}
	
	PRIVATE ${PHYSXSDK_INCLUDE_DIRS}
	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
	PRIVATE ${FBXSDK_INCLUDE_DIRS}	
    
    PRIVATE $<$<OR:$<CONFIG:debug>,$<CONFIG:checked>,$<CONFIG:profile>>:${NVTOOLSEXT_INCLUDE_DIRS}>
)

TARGET_COMPILE_DEFINITIONS(SampleBase
	PRIVATE ${SAMPLEBASE_COMPILE_DEFS}
)

SET_TARGET_PROPERTIES(SampleBase PROPERTIES 
	COMPILE_PDB_NAME_DEBUG "SampleBase${CMAKE_DEBUG_POSTFIX}"
	COMPILE_PDB_NAME_CHECKED "SampleBase${CMAKE_CHECKED_POSTFIX}"
	COMPILE_PDB_NAME_PROFILE "SampleBase${CMAKE_PROFILE_POSTFIX}"
	COMPILE_PDB_NAME_RELEASE "SampleBase${CMAKE_RELEASE_POSTFIX}"
)

TARGET_COMPILE_OPTIONS(SampleBase PRIVATE /wd4005 /wd4244)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(SampleBase 
	PUBLIC NvBlast NvBlastExtShaders NvBlastExtPhysX NvBlastExtExporter NvBlastExtAssetUtils NvBlastExtSerialization NvBlastExtTkSerialization NvBlastExtPxSerialization NvBlastTk d3dcompiler.lib d3d11.lib dxgi.lib comctl32.lib 
	PUBLIC $<$<CONFIG:debug>:${PXPVDSDK_LIB_DEBUG}> $<$<CONFIG:debug>:${PXFOUNDATION_LIB_DEBUG}> $<$<CONFIG:debug>:${PXTASK_LIB_DEBUG}> $<$<CONFIG:debug>:${PSFASTXML_LIB_DEBUG}> $<$<CONFIG:debug>:${PHYSX3COMMON_LIB_DEBUG}>
	PUBLIC $<$<CONFIG:checked>:${PXPVDSDK_LIB_CHECKED}> $<$<CONFIG:checked>:${PXFOUNDATION_LIB_CHECKED}> $<$<CONFIG:checked>:${PXTASK_LIB_CHECKED}> $<$<CONFIG:checked>:${PSFASTXML_LIB_CHECKED}> $<$<CONFIG:checked>:${PHYSX3COMMON_LIB_CHECKED}>
	PUBLIC $<$<CONFIG:profile>:${PXPVDSDK_LIB_PROFILE}> $<$<CONFIG:profile>:${PXFOUNDATION_LIB_PROFILE}> $<$<CONFIG:profile>:${PXTASK_LIB_PROFILE}> $<$<CONFIG:profile>:${PSFASTXML_LIB_PROFILE}> $<$<CONFIG:profile>:${PHYSX3COMMON_LIB_PROFILE}>
	PUBLIC $<$<CONFIG:release>:${PXPVDSDK_LIB}> $<$<CONFIG:release>:${PXFOUNDATION_LIB}> $<$<CONFIG:release>:${PXTASK_LIB}> $<$<CONFIG:release>:${PSFASTXML_LIB}> $<$<CONFIG:release>:${PHYSX3COMMON_LIB}>
	PUBLIC ${HBAO_PLUS_LIB} ${SHADOW_LIB_LIB} ${DXUT_LIBRARIES} ${DIRECTXTEX_LIBRARIES}
	PUBLIC ${FBXSDK_LIBRARIES}
    PUBLIC $<$<OR:$<CONFIG:debug>,$<CONFIG:checked>,$<CONFIG:profile>>:${NVTOOLSEXT_LIB}>
	)

include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/SampleBase-AT.cmake OPTIONAL)
