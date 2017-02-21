/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef DEBUGRENDERBUFFER_H
#define DEBUGRENDERBUFFER_H

#include "PxRenderBuffer.h"
#include <vector>

using namespace physx;


/**
Simple PxRenderBuffer implementation for easy debug primitives adding
*/
class DebugRenderBuffer : public PxRenderBuffer
{
public:
	~DebugRenderBuffer() {}

	virtual PxU32 getNbPoints() const { return 0; }
	virtual const PxDebugPoint* getPoints() const { return nullptr; }

	virtual PxU32 getNbLines() const { return static_cast<PxU32>(m_lines.size()); }
	virtual const PxDebugLine* getLines() const { return m_lines.data(); }

	virtual PxU32 getNbTriangles() const { return 0; }
	virtual const PxDebugTriangle* getTriangles() const { return nullptr; }

	virtual PxU32 getNbTexts() const { return 0; }
	virtual const PxDebugText* getTexts() const { return nullptr; }

	virtual void append(const PxRenderBuffer& other) {}
	virtual void clear()
	{
		m_lines.clear();
	}

	std::vector<PxDebugLine> m_lines;
};


#endif //DEBUGRENDERBUFFER_H