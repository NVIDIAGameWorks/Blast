/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#define DTO_CLASS(_NAME, _POCO, _SERIALIZER)											\
namespace Nv {																			\
namespace Blast {																		\
class _NAME ## DTO																		\
{																						\
public:																					\
	static class physx::PxCooking* Cooking;												\
	static class physx::PxPhysics* Physics;												\
																						\
	static bool serialize(_SERIALIZER::Builder builder, const _POCO * poco);			\
 	static _POCO* deserialize(_SERIALIZER::Reader reader);								\
 	static bool deserializeInto(_SERIALIZER::Reader reader, _POCO * poco);				\
};																						\
}																						\
}																						\
																						\

#define DTO_CLASS_LL(_NAME, _POCO, _SERIALIZER)											\
namespace Nv {																			\
namespace Blast {																		\
class _NAME ## DTO																		\
{																						\
public:																					\
																						\
	static bool serialize(_SERIALIZER::Builder builder, const _POCO * poco);			\
 	static _POCO* deserialize(_SERIALIZER::Reader reader);								\
 	static bool deserializeInto(_SERIALIZER::Reader reader, _POCO * poco);				\
};																						\
}																						\
}																						\
																						\

