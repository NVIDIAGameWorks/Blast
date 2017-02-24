#pragma once
#include <memory>
#include "IMeshFileReader.h"

class ObjFileReader: public IMeshFileReader
{
public:
	ObjFileReader();
	~ObjFileReader() = default;

	/*
	Load from the specified file path, returning a mesh or nullptr if failed
	*/
	std::shared_ptr<Nv::Blast::Mesh> loadFromFile(std::string filename);

};
