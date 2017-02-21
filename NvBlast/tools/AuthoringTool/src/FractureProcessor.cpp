#include "FractureProcessor.h"
#include "NvBlastExtAuthoringCollisionBuilder.h"
#include <iostream>
#include "PxPhysicsAPI.h"
#include "NvBlastTkFramework.h"
#include "NvBlastExtAuthoringFractureTool.h"
#include "SimpleRandomGenerator.h"
#include "NvBlastExtAuthoringBondGenerator.h"
#include "NvBlastTypes.h"
#include "NvBlastIndexFns.h"
#include "NvBlast.h"
#include <ctime>
using namespace Nv::Blast;

void loggingCallback(int type, const char* msg, const char* file, int line)
{
	(void)type;

	std::cout << msg << " FILE:" << file << " Line: " << line << "\n";
}


FractureProcessor::FractureProcessor()
{
	initPhysX();

	Nv::Blast::TkFrameworkDesc frameworkDesc =
	{
		&g_errorCallback,
		&g_allocator
	};
	
	framework = NvBlastTkFrameworkCreate(frameworkDesc);
	if (framework == nullptr)
	{
		std::cout << "Failed to create TkFramework" << std::endl;
		return;
	}
}

FractureProcessor::~FractureProcessor()
{
	releasePhysX();

	if (framework != nullptr)
	{
		framework->release();
	}
}

void FractureProcessor::buildPhysicsChunks(const std::vector<std::vector<Nv::Blast::Triangle>>& chunkGeometry, std::vector<Nv::Blast::ExtPxAssetDesc::ChunkDesc>& outPhysicsChunks, std::vector<Nv::Blast::ExtPxAssetDesc::SubchunkDesc>& outPhysicsSubchunks)
{
	Nv::Blast::ConvexMeshBuilder collisionBuilder(cooking, &physics->getPhysicsInsertionCallback());

	outPhysicsChunks.resize(chunkGeometry.size());
	outPhysicsSubchunks.resize(chunkGeometry.size());

	for (uint32_t i = 0; i < chunkGeometry.size(); ++i)
	{
		std::vector<physx::PxVec3> vertices;
		for (uint32_t p = 0; p < chunkGeometry[i].size(); ++p)
		{
			vertices.push_back(chunkGeometry[i][p].a.p);
			vertices.push_back(chunkGeometry[i][p].b.p);
			vertices.push_back(chunkGeometry[i][p].c.p);
		}
		outPhysicsSubchunks[i].transform = physx::PxTransform(physx::PxIdentity);
		outPhysicsSubchunks[i].geometry = physx::PxConvexMeshGeometry(collisionBuilder.buildConvexMesh(vertices));
		outPhysicsChunks[i].isStatic = false;
		outPhysicsChunks[i].subchunkCount = 1;
		outPhysicsChunks[i].subchunks = &outPhysicsSubchunks[i];
	}

}

std::shared_ptr<FractureResult> FractureProcessor::fractureMesh(std::shared_ptr<Nv::Blast::Mesh> sourceMesh, const FractureSettings &settings)
{
	// Create FractureTool

	std::cout << "Fracture tool initialization" << std::endl;

	std::vector<NvBlastChunkDesc> chunkDesc;
	std::vector<NvBlastBondDesc> bondDescs;
	std::vector<std::vector<Nv::Blast::Triangle> > chunkMeshes;
	std::vector<bool> isSupport;

	Nv::Blast::FractureTool fTool(loggingCallback);

	fTool.setSourceMesh(sourceMesh.get());
	SimpleRandomGenerator rnd;
	rnd.seed((int32_t)time(nullptr)); // Keep the same seed to have reproducible results.

	std::cout << "Fracturing..." << std::endl;
	VoronoiSitesGenerator stGenerator(sourceMesh.get(), &rnd);
	switch (settings.mode)
	{
	case 'c':
		stGenerator.clusteredSitesGeneration(settings.clusterCount, settings.cellsCount, settings.clusterRadius);
		fTool.voronoiFracturing(0, stGenerator.getVoronoiSites(), false);
		break;
	case 's':
	{
		SlicingConfiguration slConfig;
		slConfig.x_slices = settings.slicingX;
		slConfig.y_slices = settings.slicingY;
		slConfig.z_slices = settings.slicingZ;
		slConfig.angle_variations = settings.angleVariation;
		slConfig.offset_variations = settings.offsetVariation;
		fTool.slicing(0, slConfig, false, &rnd);
		break;
	}
	case 'v':
		stGenerator.uniformlyGenerateSitesInMesh(settings.cellsCount);
		fTool.voronoiFracturing(0, stGenerator.getVoronoiSites(), false);
		break;
	default:
		std::cout << "Not supported mode" << std::endl;
		return nullptr;
	}
	std::cout << "Creating geometry" << std::endl;
	fTool.finalizeFracturing();

	chunkMeshes.resize(fTool.getChunkList().size());
	isSupport.resize(fTool.getChunkList().size());
	for (uint32_t i = 0; i < fTool.getChunkList().size(); ++i)
	{
		fTool.getBaseMesh(i, chunkMeshes[i]);
		isSupport[i] = fTool.getChunkList()[i].isLeaf;

	}
	

	BlastBondGenerator bondGenerator(cooking, &physics->getPhysicsInsertionCallback());

	BondGenerationConfig cnf;
	cnf.bondMode = BondGenerationConfig::EXACT;

	bondGenerator.buildDescFromInternalFracture(&fTool, isSupport, bondDescs, chunkDesc);

//	const uint32_t chunkCount = static_cast<uint32_t>(chunkDesc.size());
	const uint32_t bondCount = static_cast<uint32_t>(bondDescs.size());
	if (bondCount == 0)
	{
		std::cout << "Can't create bonds descriptors..." << std::endl;
		return nullptr;
	}

	return finalizeMeshProcessing(chunkDesc, bondDescs, chunkMeshes);
}

std::shared_ptr<FractureResult> FractureProcessor::finalizeMeshProcessing(std::vector<NvBlastChunkDesc> chunkDesc, std::vector<NvBlastBondDesc> bondDescs, std::vector<std::vector<Nv::Blast::Triangle> > chunkMeshes)
{
	const uint32_t chunkCount = static_cast<uint32_t>(chunkDesc.size());
	const uint32_t bondCount = static_cast<uint32_t>(bondDescs.size());

	// order chunks, build map
	std::vector<uint32_t> chunkReorderInvMap;
	{
		std::vector<uint32_t> chunkReorderMap(chunkCount);
		std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
		NvBlastEnsureAssetExactSupportCoverage(chunkDesc.data(), chunkCount, scratch.data(), loggingCallback);
		NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDesc.data(), chunkCount, scratch.data(), loggingCallback);
		NvBlastApplyAssetDescChunkReorderMapInplace(chunkDesc.data(), chunkCount, bondDescs.data(), bondCount, chunkReorderMap.data(), scratch.data(), loggingCallback);
		chunkReorderInvMap.resize(chunkReorderMap.size());
		Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
	}

	std::shared_ptr<FractureResult> result = std::make_shared<FractureResult>();

	// get result geometry

	result->resultGeometry.resize(chunkMeshes.size());
	//std::vector<std::vector<Triangle>> resultGeometry(chunkMeshes.size());
	for (uint32_t i = 0; i < chunkMeshes.size(); ++i)
	{
		uint32_t chunkIndex = chunkReorderInvMap[i];
		result->resultGeometry[i] = chunkMeshes[chunkIndex];
	}

	float maxX = INT32_MIN;
	float maxY = INT32_MIN;
	float maxZ = INT32_MIN;

	float minX = INT32_MAX;
	float minY = INT32_MAX;
	float minZ = INT32_MAX;

	for (uint32_t i = 0; i < bondDescs.size(); i++)
	{
		NvBlastBondDesc bondDesc = bondDescs[i];

		minX = std::min(minX, bondDesc.bond.centroid[0]);
		maxX = std::max(maxX, bondDesc.bond.centroid[0]);
		
		minY = std::min(minY, bondDesc.bond.centroid[1]);
		maxY = std::max(maxY, bondDesc.bond.centroid[1]);

		minZ = std::min(minZ, bondDesc.bond.centroid[2]);
		maxZ = std::max(maxZ, bondDesc.bond.centroid[2]);
	}

	std::cout << "Bond bounds: " << std::endl;
	std::cout << "MIN: " << minX << ", " << minY << ", " << minZ << std::endl;
	std::cout << "MAX: " << maxX << ", " << maxY << ", " << maxZ << std::endl;

	// prepare physics data (convexes)
	std::vector<Nv::Blast::ExtPxAssetDesc::ChunkDesc> physicsChunks(chunkCount);
	std::vector<Nv::Blast::ExtPxAssetDesc::SubchunkDesc> physicsSubchunks;
	buildPhysicsChunks(result->resultGeometry, physicsChunks, physicsSubchunks);

	// build and serialize ExtPhysicsAsset
	Nv::Blast::ExtPxAssetDesc	descriptor;
	descriptor.bondCount = bondCount;
	descriptor.bondDescs = bondDescs.data();
	descriptor.chunkCount = chunkCount;
	descriptor.chunkDescs = chunkDesc.data();
	descriptor.bondFlags = nullptr;
	descriptor.pxChunks = physicsChunks.data();
	
	result->resultPhysicsAsset = std::shared_ptr<Nv::Blast::ExtPxAsset>(Nv::Blast::ExtPxAsset::create(descriptor, *framework), [=](Nv::Blast::ExtPxAsset* asset)
	{
		asset->release();
	});

	std::cout << "Done" << std::endl;

	return result;
}

bool FractureProcessor::initPhysX()
{
	foundation = PxCreateFoundation(PX_FOUNDATION_VERSION, g_allocator, g_errorCallback);
	if (!foundation)
	{
		std::cout << "Can't init PhysX foundation" << std::endl;
		return false;
	}
	physx::PxTolerancesScale scale;
	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, scale, true);
	if (!physics)
	{
		std::cout << "Can't create Physics" << std::endl;
		return false;
	}
	physx::PxCookingParams cookingParams(scale);
	cookingParams.buildGPUData = true;
	cooking = PxCreateCooking(PX_PHYSICS_VERSION, physics->getFoundation(), cookingParams);
	if (!cooking)
	{
		std::cout << "Can't create Cooking" << std::endl;
		return false;
	}
	return true;
}

void FractureProcessor::releasePhysX()
{
	cooking->release();
	cooking = 0;
	physics->release();
	physics = 0;
	foundation->release();
	foundation = 0;
}
