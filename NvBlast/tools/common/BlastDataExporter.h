#ifndef BLAST_DATA_EXPORTER
#define BLAST_DATA_EXPORTER


#include <NvBlastIndexFns.h>
#include <NvBlastExtAuthoringTypes.h>
#include <NvBlastExtPxAsset.h>
#include <vector>
#include <string>

using namespace Nv::Blast;

namespace physx
{
	class PxCooking;
}


struct NvBlastBondDesc;
struct NvBlastChunkDesc;

struct NvBlastAsset;
namespace Nv
{
	namespace Blast
	{
		class TkAsset;
		class ExtPxAsset;
	}
}
/**
	Tool for Blast asset creation and exporting
*/
class BlastDataExporter
{
public:
	BlastDataExporter(TkFramework* framework, physx::PxCooking* cooking, NvBlastLog log) : mFramework(framework), mCooking(cooking), m_log(log) {};

	/**
		Creates ExtPxAsset
	*/
	ExtPxAsset*		createExtBlastAsset(std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs,
		std::vector<ExtPxAssetDesc::ChunkDesc>& physicsChunks);
	/**
		Creates Low Level Blast asset 
	*/
	NvBlastAsset*	createLlBlastAsset(std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs);

	/**
		Creates Blast Toolkit Asset asset 
	*/
	TkAsset*		createTkBlastAsset(const std::vector<NvBlastBondDesc>& bondDescs, const std::vector<NvBlastChunkDesc>& chunkDescs);

	/*
		Saves Blast LL asset to given path
	*/
	bool saveBlastLLAsset(const std::string& outputFilePath, const NvBlastAsset* asset);

	/*
		Saves Blast Tk asset to given path
	*/
	bool saveBlastTkAsset(const std::string& outputFilePath, const TkAsset* asset);

	/*
		Saves Blast BPXA asset to given path
	*/
	bool saveBlastExtAsset(const std::string& outputFilePath, const ExtPxAsset* asset);

private:
	TkFramework* mFramework;
	physx::PxCooking* mCooking;
	NvBlastLog m_log;
};




#endif