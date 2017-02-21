/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTDAMAGESHADERS_H
#define NVBLASTEXTDAMAGESHADERS_H

#include "NvBlastTypes.h"
#include "NvBlastPreprocessor.h"

/**
A few example damage shader implementations.
*/


///////////////////////////////////////////////////////////////////////////////
//  Common Material 
///////////////////////////////////////////////////////////////////////////////

/**
Specific parameters for the material functions here present.

Material function implementers may choose their own set.
*/
struct NvBlastExtMaterial
{
	float	singleChunkThreshold;		//!<	subsupport chunks only take damage surpassing this value
	float	graphChunkThreshold;		//!<	support chunks only take damage surpassing this value
	float	bondTangentialThreshold;	//!<	bond only take damage surpassing this value
	float	bondNormalThreshold;		//!<	currently unused - forward damage propagation
	float	damageAttenuation;			//!<	factor of damage attenuation while forwarding
};


///////////////////////////////////////////////////////////////////////////////
//  Radial Damage
///////////////////////////////////////////////////////////////////////////////

/**
Radial Damage Desc
*/
struct NvBlastExtRadialDamageDesc
{
	float	compressive;	//!<	compressive (radial) damage component
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
//  Shear Damage
///////////////////////////////////////////////////////////////////////////////

/**
Shear Damage Desc
*/
struct NvBlastExtShearDamageDesc
{
	float	shear[3];		//!<	directional damage component
	float	position[3];	//!<	origin of damage action
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
