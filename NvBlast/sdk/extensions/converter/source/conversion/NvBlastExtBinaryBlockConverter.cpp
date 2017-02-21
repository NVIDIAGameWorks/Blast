/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtBinaryBlockConverter.h"
#include <iostream>
#include <algorithm>
#include <queue>
#include <limits>


namespace Nv
{
namespace Blast
{

bool BinaryBlockConverter::convertBinaryBlock(std::vector<char>& outBlock, const std::vector<VersionConverterPtr>& converters, const std::vector<char>& inBlock, uint32_t outBlockVersion, uint32_t inBlockVersion)
{
	if (inBlock.empty())
	{
		std::cerr << "Conversion failed: empty input block." << std::endl;
		return false;
	}

	// starting version
	uint32_t version;
	version = inBlockVersion;
	std::cout << "Input block version: " << version << std::endl;

	// target version
	const uint32_t targetVersion = outBlockVersion;
	std::cout << "Target version: " << targetVersion << std::endl;

	// search conversion path
	std::vector<VersionConverterPtr> conversionPath;
	if (!findShortestPath(conversionPath, converters, version, targetVersion))
	{
		std::cerr << "Conversion failed: can't find conversion path." << std::endl;
		return false;
	}

	// starting convertion loop
	std::vector<char> blockFrom(inBlock.begin(), inBlock.end());
	std::vector<char> blockTo(inBlock.size());
	for (const VersionConverterPtr converter : conversionPath)
	{
		// actual conversion call
		std::cout << "Converting from version: " << converter->getVersionFrom() << " -> " << converter->getVersionTo() << " Result: ";
		if (!converter->convert(blockFrom, blockTo))
		{
			std::cout << "Fail." << std::endl;
			std::cerr << "Conversion failed: inside converter for version: " << version << std::endl;
			return false;
		}
		else
		{
			std::cout << "Success." << std::endl;
			blockFrom.swap(blockTo);
			version = converter->getVersionTo();
		}
	}

	// copy result
	outBlock = blockFrom;

	return true;
}


/**
Finds shortest path form versionFrom to verstionTo using breadth-first search with DP
*/
bool BinaryBlockConverter::findShortestPath(std::vector<VersionConverterPtr>& conversionPath, const std::vector<VersionConverterPtr>& converters, uint32_t versionFrom, uint32_t versionTo)
{
	// find max version
	uint32_t versionMax = 0;
	for (VersionConverterPtr c : converters)
	{
		versionMax = std::max(versionMax, c->getVersionFrom());
		versionMax = std::max(versionMax, c->getVersionTo());
	}

	// dynamic programming data
	struct Node
	{
		uint32_t distance;
		VersionConverterPtr converter;

		Node() : distance(std::numeric_limits<uint32_t>::max()), converter(nullptr) {}
	};
	std::vector<Node> nodes(versionMax + 1);

	// initial state (start from versionTo)
	std::queue<uint32_t> q;
	q.emplace(versionTo);
	nodes[versionTo].distance = 0;
	nodes[versionTo].converter = nullptr;

	// breadth-first search
	bool found = false;
	while (!q.empty() && !found)
	{
		uint32_t v0 = q.front();
		q.pop();

		for (const VersionConverterPtr c : converters)
		{
			if (c->getVersionTo() == v0)
			{
				uint32_t v1 = c->getVersionFrom();
				if (nodes[v1].distance > nodes[v0].distance + 1)
				{
					nodes[v1].distance = nodes[v0].distance + 1;
					nodes[v1].converter = c;
					q.emplace(v1);
				}

				if (c->getVersionFrom() == versionFrom)
				{
					found = true;
					break;
				}
			}
		}
	}

	if (found)
	{
		// unfold found path to result conversionPath
		uint32_t v = versionFrom;
		conversionPath.clear();
		while (nodes[v].converter.get() != nullptr)
		{
			conversionPath.push_back(nodes[v].converter);
			v = nodes[v].converter->getVersionTo();
		}
		return true;
	}
	else
	{
		return false;
	}
}

} // namespace Blast
} // namespace Nv
