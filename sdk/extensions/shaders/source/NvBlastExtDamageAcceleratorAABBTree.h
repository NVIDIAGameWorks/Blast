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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.

#pragma once

#include "NvBlastExtDamageAcceleratorInternal.h"
#include "NvBlast.h"
#include "NvBlastArray.h"


namespace Nv
{
namespace Blast
{

class ExtDamageAcceleratorAABBTree final : public ExtDamageAcceleratorInternal
{
public:
	//////// ctor ////////

	ExtDamageAcceleratorAABBTree() :
		 m_root(nullptr)
	{
	}

	virtual ~ExtDamageAcceleratorAABBTree()
	{
	}

	static ExtDamageAcceleratorAABBTree* create(const NvBlastAsset* asset);


	//////// interface ////////

	virtual void release() override;

	virtual void findBondCentroidsInBounds(const physx::PxBounds3& bounds, ResultCallback& resultCallback) const override
	{
		const_cast<ExtDamageAcceleratorAABBTree*>(this)->findInBounds(bounds, resultCallback, false);
	}

	virtual void findBondSegmentsInBounds(const physx::PxBounds3& bounds, ResultCallback& resultCallback) const override
	{
		const_cast<ExtDamageAcceleratorAABBTree*>(this)->findInBounds(bounds, resultCallback, true);

	}

	virtual void findBondSegmentsPlaneIntersected(const physx::PxPlane& plane, ResultCallback& resultCallback) const override;

	virtual Nv::Blast::DebugBuffer fillDebugRender(int depth, bool segments) override;

	virtual void* getImmediateScratch(size_t size) override
	{
		m_scratch.resizeUninitialized(size);
		return m_scratch.begin();
	}


private:
	// no copy/assignment
	ExtDamageAcceleratorAABBTree(ExtDamageAcceleratorAABBTree&);
	ExtDamageAcceleratorAABBTree& operator=(const ExtDamageAcceleratorAABBTree& tree);

	// Tree node 
	struct Node
	{
		int child[2];
		uint32_t first;
		uint32_t last;
		physx::PxBounds3 pointsBound;
		physx::PxBounds3 segmentsBound;
	};


	void build(const NvBlastAsset* asset);

	int createNode(uint32_t startIdx, uint32_t endIdx, uint32_t depth);

	void pushResult(ResultCallback& callback, uint32_t pointIndex) const
	{
		callback.push(pointIndex, m_bonds[pointIndex].node0, m_bonds[pointIndex].node1);
	}

	void findInBounds(const physx::PxBounds3& bounds, ResultCallback& callback, bool segments) const;

	void findPointsInBounds(const Node& node, ResultCallback& callback, const physx::PxBounds3& bounds) const;

	void findSegmentsInBounds(const Node& node, ResultCallback& callback, const physx::PxBounds3& bounds) const;

	void findSegmentsPlaneIntersected(const Node& node, ResultCallback& callback, const physx::PxPlane& plane) const;

	void fillDebugBuffer(const Node& node, int currentDepth, int depth, bool segments);


	//////// data ////////

	Node*					              m_root;
	Array<Node>::type		              m_nodes;
	Array<uint32_t>::type	              m_indices;

	Array<physx::PxVec3>::type		      m_points;

	struct Segment
	{
		physx::PxVec3	p0;
		physx::PxVec3	p1;
	};
	Array<Segment>::type			      m_segments;

	struct BondData
	{
		uint32_t		node0;
		uint32_t		node1;
	};
	Array<BondData>::type		          m_bonds;

	Array<Nv::Blast::DebugLine>::type     m_debugLineBuffer;

	Array<char>::type					  m_scratch;
};


} // namespace Blast
} // namespace Nv
