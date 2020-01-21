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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#pragma once
#include "PxIO.h"
#include "capnp/common.h"
#include "kj/io.h"
#include <memory>
#include "generated/NvBlastExtPxSerialization.capn.h"


namespace Nv
{
namespace Blast
{

/*
A wrapper around a Capn Proto Data reader.

Since it needs to behave like a stream, it's internally wrapped in a stream.

*/
class ExtKJPxInputStream : public physx::PxInputStream
{
public:
	ExtKJPxInputStream(capnp::Data::Reader inReader);
	~ExtKJPxInputStream() = default;

	virtual uint32_t read(void* dest, uint32_t count) override;

private:
	capnp::Data::Reader dataReader;
	std::shared_ptr<kj::ArrayInputStream> inputStream;
};

}	// namespace Blast
}	// namespace Nv
