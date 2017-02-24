/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTBINARYBLOCKCONVERTER_H
#define NVBLASTEXTBINARYBLOCKCONVERTER_H


#include "NvBlast.h"
#include <vector>
#include <memory>


namespace Nv
{
namespace Blast
{

/**
Generic binary block converter class.

BinaryBlockConverter is an abstract class, as well as it's member class VersionConverter. In order to implement your own
binary converter - implement for every version conversion BinaryBlockConverter::VersionConverter. Then implement BinaryBlockConverter
where getVersionConverters() should return all your implemented BinaryBlockConverter::VersionConverter's.
		
*/
class BinaryBlockConverter
{
public:
	class VersionConverter
	{
	public:
		virtual uint32_t getVersionFrom() const = 0;
		virtual uint32_t getVersionTo() const = 0;
		virtual bool convert(const std::vector<char>& from, std::vector<char>& to) const = 0;
	};

	typedef std::shared_ptr<VersionConverter> VersionConverterPtr;

	static bool convertBinaryBlock(std::vector<char>& outBlock, const std::vector<VersionConverterPtr>& converters, const std::vector<char>& inBlock, uint32_t outBlockVersion, uint32_t inBlockVersion);
protected:

private:
	static bool findShortestPath(std::vector<VersionConverterPtr>& conversionPath, const std::vector<VersionConverterPtr>& converters, uint32_t versionFrom, uint32_t versionTo);
};
	
} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTBINARYBLOCKCONVERTER_H
