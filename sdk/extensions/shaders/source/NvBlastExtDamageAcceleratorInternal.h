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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.

#pragma once

#include "NvBlastExtDamageShaders.h"
#include "PxBounds3.h"


namespace Nv
{
namespace Blast
{


class ExtDamageAcceleratorInternal : public NvBlastExtDamageAccelerator
{
public:
	struct QueryBondData
	{
		uint32_t bond;
		uint32_t node0;
		uint32_t node1;
	};

	class ResultCallback
	{
	public:
		ResultCallback(QueryBondData* buffer, uint32_t count) :
			m_bondBuffer(buffer), m_bondMaxCount(count), m_bondCount(0) {}

		virtual void processResults(const QueryBondData* bondBuffer, uint32_t count) = 0;

		void push(uint32_t bond, uint32_t node0, uint32_t node1)
		{
			m_bondBuffer[m_bondCount].bond = bond;
			m_bondBuffer[m_bondCount].node0 = node0;
			m_bondBuffer[m_bondCount].node1 = node1;
			m_bondCount++;
			if (m_bondCount == m_bondMaxCount)
			{
				dispatch();
			}
		}

		void dispatch()
		{
			if (m_bondCount)
			{
				processResults(m_bondBuffer, m_bondCount);
				m_bondCount = 0;
			}
		}
		
	private:
		QueryBondData* m_bondBuffer;
		uint32_t	   m_bondMaxCount;

		uint32_t	   m_bondCount;
	};

	virtual void findBondCentroidsInBounds(const physx::PxBounds3& bounds, ResultCallback& resultCallback) const = 0;
	virtual void findBondSegmentsInBounds(const physx::PxBounds3& bounds, ResultCallback& resultCallback) const = 0;
	virtual void findBondSegmentsPlaneIntersected(const physx::PxPlane& plane, ResultCallback& resultCallback) const = 0;

	// Non-thread safe! Multiple calls return the same memory.
	virtual void* getImmediateScratch(size_t size) = 0;
};


} // namespace Blast
} // namespace Nv
