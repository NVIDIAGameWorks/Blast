#pragma once
#include <vector>
#include "NvBlastExtAuthoringTypes.h"
#include "PxFoundation.h"
#include "PxPhysics.h"
#include "PxCooking.h"
#include "PxDefaultAllocator.h"
#include "PxDefaultErrorCallback.h"
#include <memory>
#include "NvBlastExtAuthoringMesh.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastTypes.h"
#include "NvBlastExtPxAsset.h"

struct FractureResult
{
	std::vector<std::vector<Nv::Blast::Triangle>> resultGeometry;
	std::shared_ptr<Nv::Blast::ExtPxAsset>	resultPhysicsAsset;
};

struct FractureSettings
{
	unsigned char mode;
	uint32_t cellsCount;
	uint32_t clusterCount;
	float clusterRadius;
	int32_t slicingX;
	int32_t slicingY;
	int32_t slicingZ;
	float angleVariation;
	float offsetVariation;
};

class FractureProcessor
{
public:
	FractureProcessor();
	~FractureProcessor();


	std::shared_ptr<FractureResult> fractureMesh(std::shared_ptr<Nv::Blast::Mesh> sourceMesh, const FractureSettings &settings);


	physx::PxPhysics*	getPhysics() { return physics; }
	physx::PxCooking*	getCooking() { return cooking; }
private:
	physx::PxFoundation*	foundation = nullptr;
	physx::PxPhysics*		physics = nullptr;
	physx::PxCooking*		cooking = nullptr;

	Nv::Blast::TkFramework*	framework = nullptr;

	physx::PxDefaultAllocator g_allocator;
	physx::PxDefaultErrorCallback g_errorCallback;


	void buildPhysicsChunks(const std::vector<std::vector<Nv::Blast::Triangle>>& chunkGeometry, std::vector<Nv::Blast::ExtPxAssetDesc::ChunkDesc>& outPhysicsChunks,
		std::vector<Nv::Blast::ExtPxAssetDesc::SubchunkDesc>& outPhysicsSubchunks);

	std::shared_ptr<FractureResult> finalizeMeshProcessing(std::vector<NvBlastChunkDesc> chunkDesc, std::vector<NvBlastBondDesc> bondDescs, std::vector<std::vector<Nv::Blast::Triangle> > chunkMeshes);

	bool initPhysX();
	void releasePhysX();
};
