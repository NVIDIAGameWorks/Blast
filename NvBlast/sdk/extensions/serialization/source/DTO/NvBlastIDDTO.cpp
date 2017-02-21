/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastIDDTO.h"
#include "NvBlastTypes.h"
#include "NvBlastAssert.h"
#include "generated/NvBlastExtSerializationLL.capn.h"


namespace Nv
{
	namespace Blast
	{
		
		bool NvBlastIDDTO::serialize(Nv::Blast::Serialization::UUID::Builder builder, const NvBlastID * poco)
		{
			capnp::Data::Reader idArrayReader((unsigned char *)poco->data, 16);
			builder.setValue(idArrayReader);
		
			return true;
		}
		
		NvBlastID* NvBlastIDDTO::deserialize(Nv::Blast::Serialization::UUID::Reader reader)
		{
			//FIXME
			reader = reader;
			//TODO: Allocate with ExtContext and return
		
			return nullptr;
		}
		
		bool NvBlastIDDTO::deserializeInto(Nv::Blast::Serialization::UUID::Reader reader, NvBlastID * poco)
		{
			NVBLAST_ASSERT_WITH_MESSAGE(reader.getValue().size() == 16, "BlastID must be 16 bytes");
		
			memcpy(poco, reader.getValue().begin(), 16);
		
			return true;
		}
	}
}