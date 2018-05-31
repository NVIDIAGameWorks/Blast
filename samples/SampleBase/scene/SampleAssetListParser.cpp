// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


#include "SampleAssetListParser.h"
#include <PsFastXml.h>
#include "Sample.h"
#include "PxVec4.h"
#include "PxInputDataFromPxFileBuf.h"
#include <bitset>


using namespace physx;


const float DEGREE_TO_RAD = acos(-1.0) / 180.0;

class AssetListParser : public physx::shdfnd::FastXml::Callback
{
public:
	AssetListParser(AssetList& assetList): m_assetList(assetList){}
protected:

	// encountered a comment in the XML
	virtual bool processComment(const char* /*comment*/)
	{
		return true;
	}

	virtual bool processClose(const char* elementName, unsigned int /*depth*/, bool& /*isError*/)
	{
		if (::strcmp(elementName, "Box") == 0)
		{
			m_assetList.boxes.push_back(m_boxTemp);
			m_boxTemp = AssetList::BoxAsset();
		}
		else if (::strcmp(elementName, "Composite") == 0)
		{
			m_assetList.composites.push_back(m_compositeTemp);
			m_compositeTemp = AssetList::CompositeAsset();
		}
		return true;
	}

	// return true to continue processing the XML document, false to skip.
	virtual bool processElement(const char* elementName, // name of the element
		const char* elementData, // element data, null if none
		const physx::shdfnd::FastXml::AttributePairs& attr,
		int /*lineno*/) // line number in the source XML file
	{
		if (::strcmp(elementName, "Model") == 0)
		{
			m_assetList.models.resize(m_assetList.models.size() + 1);
			auto& model = m_assetList.models.back();
			for (int i = 0; i < attr.getNbAttr(); ++i)
			{
				if (::strcmp(attr.getKey(i), "id") == 0)
				{
					model.id = std::string(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "file") == 0)
				{
					model.file = std::string(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "name") == 0)
				{
					model.name = std::string(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "isSkinned") == 0)
				{
					std::string str = attr.getValue(i);
					if (::strcmp(&str[0], "true") == 0)
					{
						model.isSkinned = true;
					}
				}
			}

			model.transform = parseTransform(attr);

			if (model.name.empty())
			{
				model.name = model.file;
			}
			if (model.id.empty())
			{
				model.id = model.name;
			}
		}
		else if (::strcmp(elementName, "Box") == 0)
		{
			for (int i = 0; i < attr.getNbAttr(); ++i)
			{
				if (::strcmp(attr.getKey(i), "id") == 0)
				{
					m_boxTemp.id = std::string(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "name") == 0)
				{
					m_boxTemp.name = std::string(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "jointAllBonds") == 0)
				{
					std::string str = attr.getValue(i);
					if (::strcmp(&str[0], "true") == 0)
					{
						m_boxTemp.jointAllBonds = true;
					}
				}
				else if (::strcmp(attr.getKey(i), "extents") == 0)
				{
					std::string str = attr.getValue(i);
					sscanf(&str[0], "%f %f %f", &m_boxTemp.extents.x, &m_boxTemp.extents.y, &m_boxTemp.extents.z);
				}
				else if (::strcmp(attr.getKey(i), "bondFlagsMask") == 0)
				{
					std::string str = attr.getValue(i);
					std::bitset<8> bondFlags(str);
					m_boxTemp.bondFlags = static_cast<uint32_t>(bondFlags.to_ulong());
				}
			}

			if (m_boxTemp.id.empty())
			{
				m_boxTemp.id = m_boxTemp.name;
			}
		}
		else if (::strcmp(elementName, "Level") == 0)
		{
			m_boxTemp.levels.push_back(AssetList::BoxAsset::Level());
			auto& level = m_boxTemp.levels.back();
			for (int i = 0; i < attr.getNbAttr(); ++i)
			{
				if (::strcmp(attr.getKey(i), "slices") == 0)
				{
					std::string str = attr.getValue(i);
					sscanf(&str[0], "%d %d %d", &level.x, &level.y, &level.z);
				}
				if (::strcmp(attr.getKey(i), "isSupport") == 0)
				{
					std::string str = attr.getValue(i);
					if (::strcmp(&str[0], "true") == 0)
					{
						level.isSupport = true;
					}
				}
			}
		}
		else if (::strcmp(elementName, "Composite") == 0)
		{
			for (int i = 0; i < attr.getNbAttr(); ++i)
			{
				if (::strcmp(attr.getKey(i), "id") == 0)
				{
					m_compositeTemp.id = std::string(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "name") == 0)
				{
					m_compositeTemp.name = std::string(attr.getValue(i));
				}
			}
			m_compositeTemp.transform = parseTransform(attr);

			if (m_compositeTemp.id.empty())
			{
				m_compositeTemp.id = m_compositeTemp.name;
			}
		}
		else if (::strcmp(elementName, "AssetRef") == 0)
		{
			m_compositeTemp.assetRefs.push_back(AssetList::CompositeAsset::AssetRef());
			AssetList::CompositeAsset::AssetRef& assetRef = m_compositeTemp.assetRefs.back();
			for (int i = 0; i < attr.getNbAttr(); ++i)
			{
				if (::strcmp(attr.getKey(i), "id") == 0)
				{
					assetRef.id = attr.getValue(i);
				}
			}
			assetRef.transform = parseTransform(attr);
		}
		else if (::strcmp(elementName, "Joint") == 0)
		{
			m_compositeTemp.joints.push_back(AssetList::CompositeAsset::Joint());
			AssetList::CompositeAsset::Joint& joint = m_compositeTemp.joints.back();
			for (int i = 0; i < attr.getNbAttr(); ++i)
			{
				if (::strcmp(attr.getKey(i), "asset0") == 0)
				{
					joint.assetIndices[0] = std::stoi(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "asset1") == 0)
				{
					joint.assetIndices[1] = std::stoi(attr.getValue(i));
				}
				if (::strcmp(attr.getKey(i), "chunk0") == 0)
				{
					joint.chunkIndices[0] = std::stoi(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "chunk1") == 0)
				{
					joint.chunkIndices[1] = std::stoi(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "position0") == 0)
				{
					joint.attachPositions[0] = parsePosition(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "position1") == 0)
				{
					joint.attachPositions[1] = parsePosition(attr.getValue(i));
				}
			}
		}
		return true;
	}

private:
	PxTransform parseTransform(const physx::shdfnd::FastXml::AttributePairs& attr)
	{
		PxTransform transform(PxIdentity);
		for (int i = 0; i < attr.getNbAttr(); ++i)
		{
			if (::strcmp(attr.getKey(i), "position") == 0)
			{
				transform.p = parsePosition(attr.getValue(i));
			}
			else if (::strcmp(attr.getKey(i), "rotation") == 0)
			{
				transform.q = parseRotation(attr.getValue(i));
			}
		}
		return transform;
	}

	PxVec3 parsePosition(const char* value)
	{
		PxVec3 ps;
		sscanf(value, "%f %f %f", &ps.x, &ps.y, &ps.z);
		return ps;
	}

	PxQuat parseRotation(const char* value)
	{
		PxVec4 ps;
		sscanf(value, "%f %f %f %f", &ps.x, &ps.y, &ps.z, &ps.w);
		ps.w = ps.w * DEGREE_TO_RAD;
		return PxQuat(ps.w, PxVec3(ps.x, ps.y, ps.z).getNormalized());;
	}

	AssetList::BoxAsset			m_boxTemp;
	AssetList::CompositeAsset	m_compositeTemp;
	AssetList&					m_assetList;
};


void parseAssetList(AssetList& assetList, std::string filepath)
{
	physx::PsFileBuffer fileBuffer(filepath.c_str(), physx::general_PxIOStream2::PxFileBuf::OPEN_READ_ONLY);
	if (!fileBuffer.isOpen())
	{
		return;
	}
	PxInputDataFromPxFileBuf inputData(fileBuffer);
	AssetListParser parser(assetList);
	physx::shdfnd::FastXml* xml = physx::shdfnd::createFastXml(&parser);
	xml->processXml(inputData, false);
	xml->release();
}