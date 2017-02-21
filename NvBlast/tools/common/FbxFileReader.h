#pragma once
#include "IMeshFileReader.h"
#include "fbxsdk.h"

class FbxFileReader: public IMeshFileReader
{
public:
	FbxFileReader();
	~FbxFileReader() = default;

	/*
	Load from the specified file path, returning a mesh or nullptr if failed
	*/
	std::shared_ptr<Nv::Blast::Mesh> loadFromFile(std::string filename) override;

private:

	// Should we convert the scene to UE4 coordinate system on load?
	bool		bConvertToUE4;

	FbxAMatrix getTransformForNode(FbxNode* node);
	void getFbxMeshes(FbxNode* node, std::vector<FbxNode*>& meshNodes);
};