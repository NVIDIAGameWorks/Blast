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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTDAMAGESHADERS_H
#define NVBLASTEXTDAMAGESHADERS_H

#include "NvBlastTypes.h"


/**
A few example damage shader implementations.
*/


///////////////////////////////////////////////////////////////////////////////
//  Common Material 
///////////////////////////////////////////////////////////////////////////////

/**
Example of simple material. It is passed into damage shader, thus it is not used 
currently in any of them. The user can use it to filter and normalize before applying.

Material function implementers may choose their own set.
*/
struct NvBlastExtMaterial
{
	NvBlastExtMaterial() : health(100.f), minDamageThreshold(0.0f), maxDamageThreshold(1.0f) {}

	float	health;					//!<	health
	float	minDamageThreshold;		//!<	min damage fraction threshold to be applied. Range [0, 1]. For example 0.1 filters all damage below 10% of health.
	float	maxDamageThreshold;		//!<	max damage fraction threshold to be applied. Range [0, 1]. For example 0.8 won't allow more then 80% of health damage to be applied.

	/**
	Helper to normalize damage.
	
	Pass damage defined in health, damage in range [0, 1] is returned, where 0 basically 
	indicates that the threshold wasn't reached and there is no point in applying it.

	\param[in]		damageInHealth			Damage defined in terms of health amount to be reduced.

	\return normalized damage
	*/
	float getNormalizedDamage(float damageInHealth) const
	{
		const float damage = health > 0.f ? damageInHealth / health : 1.0f;
		return damage > minDamageThreshold ? (damage < maxDamageThreshold ? damage : maxDamageThreshold) : 0.f;
	}
};


///////////////////////////////////////////////////////////////////////////////
//  Point Radial Damage
///////////////////////////////////////////////////////////////////////////////

/**
Radial Damage Desc
*/
struct NvBlastExtRadialDamageDesc
{
	float	damage;			//!<	normalized damage amount, range: [0, 1] (maximum health value to be reduced)
	float	position[3];	//!<	origin of damage action
	float	minRadius;		//!<	inner radius of damage action
	float	maxRadius;		//!<	outer radius of damage action
};

/**
Radial Falloff and Radial Cutter damage for both graph and subgraph shaders.

NOTE: The signature of shader functions are equal to NvBlastGraphShaderFunction and NvBlastSubgraphShaderFunction respectively. 
They are not expected to be called directly.
@see NvBlastGraphShaderFunction, NvBlastSubgraphShaderFunction
*/
NVBLAST_API void NvBlastExtFalloffGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params);
NVBLAST_API void NvBlastExtFalloffSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params);
NVBLAST_API void NvBlastExtCutterGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params);
NVBLAST_API void NvBlastExtCutterSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params);


/**
Helper Radial Falloff Damage function.

Basically it calls NvBlastActorGenerateFracture and then NvBlastActorApplyFracture with Radial Falloff shader.

\param[in,out]	actor				The NvBlastActor to apply fracture to.
\param[in,out]	buffers				Target buffers to hold applied command events.
\param[in]		damageDescBuffer	Damage descriptors array.
\param[in]		damageDescCount		Size of damage descriptors array.
\param[in]		material			Material to use.
\param[in]		logFn				User-supplied message function (see NvBlastLog definition).  May be NULL.
\param[in,out]	timers				If non-NULL this struct will be filled out with profiling information for the step, in profile build configurations.

\return true iff any fracture was applied.
*/
NVBLAST_API bool NvBlastExtDamageActorRadialFalloff(NvBlastActor* actor, NvBlastFractureBuffers* buffers, const NvBlastExtRadialDamageDesc* damageDescBuffer, uint32_t damageDescCount, const NvBlastExtMaterial* material, NvBlastLog logFn, NvBlastTimers* timers);


///////////////////////////////////////////////////////////////////////////////
//  Segment Radial Damage
///////////////////////////////////////////////////////////////////////////////

/**
Segment Radial Damage Desc
*/
struct NvBlastExtSegmentRadialDamageDesc
{
	float	damage;			//!<	normalized damage amount, range: [0, 1] (maximum health value to be reduced)
	float	position0[3];	//!<	damage segment point A position
	float	position1[3];	//!<	damage segment point B position
	float	minRadius;		//!<	inner radius of damage action
	float	maxRadius;		//!<	outer radius of damage action
};

/**
Segment Radial Falloff damage for both graph and subgraph shaders.

For every bond/chunk damage is calculated from the distance to line segment AB described in NvBlastExtSegmentRadialDamageDesc.
If distance is smaller then minRadius, full compressive amount of damage is applied. From minRadius to maxRaidus it linearly falls off to zero.

NOTE: The signature of shader functions are equal to NvBlastGraphShaderFunction and NvBlastSubgraphShaderFunction respectively.
They are not expected to be called directly.
@see NvBlastGraphShaderFunction, NvBlastSubgraphShaderFunction
*/
NVBLAST_API void NvBlastExtSegmentFalloffGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params);
NVBLAST_API void NvBlastExtSegmentFalloffSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params);


///////////////////////////////////////////////////////////////////////////////
//  Shear Damage
///////////////////////////////////////////////////////////////////////////////

/**
Shear Damage Desc
*/
struct NvBlastExtShearDamageDesc
{
	float	damage;			//!<	normalized damage amount, range: [0, 1] (maximum health value to be reduced)

	float	normal[3];		//!<	directional damage component
	float	position[3];	//!<	origin of damage action

	float	minRadius;		//!<	inner radius of damage action
	float	maxRadius;		//!<	outer radius of damage action
};

/**
Shear Damage Shaders

NOTE: The signature of shader functions are equal to NvBlastGraphShaderFunction and NvBlastSubgraphShaderFunction respectively.
They are not expected to be called directly.
@see NvBlastGraphShaderFunction, NvBlastSubgraphShaderFunction
*/
NVBLAST_API void NvBlastExtShearGraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastGraphShaderActor* actor, const NvBlastProgramParams* params);
NVBLAST_API void NvBlastExtShearSubgraphShader(NvBlastFractureBuffers* commandBuffers, const NvBlastSubgraphShaderActor* actor, const NvBlastProgramParams* params);


#endif // NVBLASTEXTDAMAGESHADERS_H
