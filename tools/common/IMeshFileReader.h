#pragma once
#include <memory>
#include <string>
#include "NvBlastExtAuthoringMesh.h"


class IMeshFileReader
{
public:
	
	/*
	Load from the specified file path, returning a mesh or nullptr if failed
	*/
	virtual std::shared_ptr<Nv::Blast::Mesh> loadFromFile(std::string filename) = 0;

	virtual bool getConvertToUE4() { return bConvertToUE4; }
	virtual void setConvertToUE4(bool bConvert) { bConvertToUE4 = bConvert; }

private:
	bool		bConvertToUE4;
};