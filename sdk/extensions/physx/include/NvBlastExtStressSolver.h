/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTSTRESSSOLVER_H
#define NVBLASTEXTSTRESSSOLVER_H

#include "common/PxRenderBuffer.h"
#include <vector>
#include "NvPreprocessor.h"


namespace Nv
{
namespace Blast
{

// forward declarations
class ExtPxFamily;
class ExtPxActor;

/**
Stress Solver Settings

Stress on every bond is calculated as 
stress = bond.linearStress * stressLinearFactor + bond.angularStress * stressAngularFactor
where:
bond.linearStress - is linear stress force on particular bond
bond.angularStress - is angular stress force on particular bond
stressLinearFactor, stressAngularFactor - are a multiplier parameter set by this struct

Support graph reduction:
2 ^ reduction level = max node count to be aggregated during graph reduction, so 0 is 2 % 0 = 1, basically use support graph.
So N nodes graph will be simplified to contain ~ N / (2 ^ reduction level)
*/
struct ExtStressSolverSettings
{
	float		stressLinearFactor;			//!<	linear stress on bond multiplier
	float		stressAngularFactor;		//!<	angular stress on bond multiplier
	uint32_t	bondIterationsPerFrame;		//!<	number of bond iterations to perform per frame, @see getIterationsPerFrame() below
	uint32_t	graphReductionLevel;		//!<	graph reduction level

	ExtStressSolverSettings() :
		stressLinearFactor(0.00004f),
		stressAngularFactor(0.00007f),
		bondIterationsPerFrame(18000),
		graphReductionLevel(3)
	{}
};


/**
Stress Solver.

Uses ExtPxFamily, allocates and prepares it's graph once when it's created. Then it's being quickly updated on every 
actor split.
Works on both dynamic and static actor's within family.
For static actors it applies gravity.
For dynamic actors it applies centrifugal force.
Additionally applyImpulse() method can be used to apply external impulse (like impact damage).
*/
class NV_DLL_EXPORT ExtStressSolver
{
public:
	//////// creation ////////

	/**
	Create a new ExtStressSolver.

	\param[in]	family			The ExtPxFamily instance to calculate stress on.
	\param[in]	settings		The settings to be set on ExtStressSolver.

	\return the new ExtStressSolver if successful, NULL otherwise.
	*/
	static ExtStressSolver*					create(ExtPxFamily& family, ExtStressSolverSettings settings = ExtStressSolverSettings());


	//////// interface ////////

	/**
	Release this stress solver.
	*/
	virtual void							release() = 0;

	/**
	Set stress solver settings.
	Changing graph reduction level will lead to graph being rebuilt (which is fast, but still not recommended).
	All other settings are applied instantly and can be changed every frame.

	\param[in]	settings		The settings to be set on ExtStressSolver.
	*/
	virtual void							setSettings(const ExtStressSolverSettings& settings) = 0;

	/**
	Get stress solver settings.

	\return the pointer to stress solver settings currently set.
	*/
	virtual const ExtStressSolverSettings&	getSettings() const = 0;

	/**
	Apply external impulse on particular actor of family

	\param[in]	actor			The ExtPxActor to apply impulse on.
	\param[in]	position		Local position in actor's coordinates to apply impulse on.
	\param[in]	force			Impulse to apply (kg * m / s).
	*/
	virtual void							applyImpulse(ExtPxActor& actor, physx::PxVec3 position, physx::PxVec3 force) = 0;

	/**
	Update stress solver.

	Calculate stress and optionally apply damage.

	\param[in]	doDamage		If 'true' damage will be applied after stress solver.
	*/
	virtual void							update(bool doDamage = true) = 0;

	/**
	Reset stress solver.

	Stress solver uses warm start internally, calling this function will flush all previous data calculated and also zeros frame count.
	This function is to be used for debug purposes. 
	*/
	virtual void							reset() = 0;

	/**
	Debug Render Mode
	*/
	enum DebugRenderMode
	{
		STRESS_GRAPH = 0,					//!<	render only stress graph
		STRESS_GRAPH_NODES_IMPULSES = 1,	//!<	render stress graph + nodes impulses after solving stress
		STRESS_GRAPH_BONDS_IMPULSES = 2		//!<	render stress graph + bonds impulses after solving stress
	};

	/**
	Fill debug render for passed array of support graph nodes.

	\param[in]	nodes			Node indices of support graph to debug render for.
	\param[out]	lines			Lines array to fill.
	\param[in]	mode			Debug render mode.
	\param[in]	scale			Scale to be applied on impulses.
	*/
	virtual void							fillDebugRender(const std::vector<uint32_t>& nodes, std::vector<physx::PxDebugLine>& lines, DebugRenderMode mode, float scale = 1.0f) = 0;

	/**
	Get stress solver linear error.

	\return the total linear error of stress calculation.
	*/
	virtual float							getStressErrorLinear() const = 0;

	/**
	Get stress solver angular error.

	\return the total angular error of stress calculation.
	*/
	virtual float							getStressErrorAngular() const = 0;

	/**
	Get stress solver total iterations count since it was created (or reset).

	\return the iterations count.
	*/
	virtual uint32_t						getIterationCount() const = 0;

	/**
	Get stress solver total frames count (update() calls) since it was created (or reset).

	\return the frames count.
	*/
	virtual uint32_t						getFrameCount() const = 0;

	/**
	Get stress solver bonds count, after graph reduction was applied.

	\return the bonds count.
	*/
	virtual uint32_t						getBondCount() const = 0;


	//////// helpers ////////

	/**
	Get iteration per frame (update() call).

	Helper method to know how many solver iterations are made per frame.

	\return the iterations per frame count.
	*/
	uint32_t								getIterationsPerFrame() const
	{
		uint32_t perFrame = getSettings().bondIterationsPerFrame / (getBondCount() + 1);
		return perFrame > 0 ? perFrame : 1;
	}
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTSTRESSSOLVER_H
