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


#ifndef NVBLASTEXTSTRESSSOLVER_H
#define NVBLASTEXTSTRESSSOLVER_H

#include "NvBlastTypes.h"
#include "PxVec3.h"
#include <vector>


namespace Nv
{
namespace Blast
{


/**
Stress Solver Settings

Stress on every bond is calculated as 
stress = (bond.linearStress * stressLinearFactor + bond.angularStress * stressAngularFactor) / hardness;
where:
bond.linearStress = the linear stress force on particular bond
bond.angularStress = the angular stress force on particular bond
stressLinearFactor, stressAngularFactor, hardness = multiplier parameters set by this struct

Support graph reduction:
graphReductionLevel is the number of node merge passes.  The resulting graph will be
roughly 2^graphReductionLevel times smaller than the original.
*/
struct ExtStressSolverSettings
{
	float		hardness;					//!<	hardness of bond's material
	float		stressLinearFactor;			//!<	linear stress on bond multiplier
	float		stressAngularFactor;		//!<	angular stress on bond multiplier
	uint32_t	bondIterationsPerFrame;		//!<	number of bond iterations to perform per frame, @see getIterationsPerFrame() below
	uint32_t	graphReductionLevel;		//!<	graph reduction level

	ExtStressSolverSettings() :
		hardness(1000.0f),
		stressLinearFactor(0.25f),
		stressAngularFactor(0.75f),
		bondIterationsPerFrame(18000),
		graphReductionLevel(3)
	{}
};


/**
Parameter to addForce() calls, determines the exact operation that is carried out.

@see ExtStressSolver.addForce()
*/
struct ExtForceMode
{
	enum Enum
	{
		IMPULSE,	//!< parameter has unit of mass * distance /time
		VELOCITY,	//!< parameter has unit of distance / time, i.e. the effect is mass independent: a velocity change.
	};
};


/**
Stress Solver.

Uses NvBlastFamily, allocates and prepares its graph once when it's created. Then it's being quickly updated on every 
actor split. 
It uses NvBlastAsset support graph, you can apply forces on nodes and stress on bonds will be calculated as the result. 
When stress on bond exceeds it's health bond is considered broken (overstressed).
Basic usage:
1. Create it with create function once for family
2. Fill node info for every node in support graph or use setAllNodesInfoFromLL() function.
3. Use notifyActorCreated / notifyActorDestroyed whenever actors are created and destroyed in family. 
4. Every frame: Apply forces (there are different functions for it see @addForce)
5. Every frame: Call update() for actual solver to process.
6. If getOverstressedBondCount() > 0 use generateFractureCommands() functions to get FractureCommands with bonds fractured
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
	static ExtStressSolver*					create(NvBlastFamily& family, ExtStressSolverSettings settings = ExtStressSolverSettings());


	//////// interface ////////

	/**
	Release this stress solver.
	*/
	virtual void							release() = 0;

	/**
	Set node info. 

	All the required info per node for stress solver is set with this function. Call it for every node in graph or use setAllNodesInfoFromLL().

	\param[in]	graphNodeIndex	Index of the node in support graph. see NvBlastSupportGraph.
	\param[in]	mass			Node mass. For static node it is irrelevant.
	\param[in]	volume			Node volume. For static node it is irrelevant.
	\param[in]	localPosition	Node local position.
	\param[in]	isStatic		Is node static.
	*/
	virtual void							setNodeInfo(uint32_t graphNodeIndex, float mass, float volume, physx::PxVec3 localPosition, bool isStatic) = 0;

	/**
	Set all nodes info using low level NvBlastAsset data.
	Uses NvBlastChunk's centroid and volume. 
	Uses 'world' node to mark nodes as static.

	\param[in]	density			Density. Used to convert volume to mass.
	*/
	virtual void							setAllNodesInfoFromLL(float density = 1.0f) = 0;

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
	Notify stress solver on newly created actor.

	Call this function for all initial actors present in family and later upon every actor split.

	\param[in]	actor			The actor created.

	\return true if actor will take part in stress solver process.  false if actor doesn't contain any bonds.
	*/
	virtual bool							notifyActorCreated(const NvBlastActor& actor) = 0;

	/**
	Notify stress solver on destroyed actor.

	Call this function when actor is destroyed (split futher) or deactivated.

	\param[in]	actor			The actor destroyed.
	*/
	virtual void							notifyActorDestroyed(const NvBlastActor& actor) = 0;

	/**
	Apply external impulse on particular actor of family. This function will find nearest actor's graph node to apply impulse on.

	\param[in]	actor			The actor to apply impulse on.
	\param[in]	localPosition	Local position in actor's coordinates to apply impulse on.
	\param[in]	localForce		Force to apply in local actor's coordinates.
	\param[in]	mode			The mode to use when applying the force/impulse(see #ExtForceMode)

	\return true iff node was found and force applied.
	*/
	virtual bool							addForce(const NvBlastActor& actor, physx::PxVec3 localPosition, physx::PxVec3 localForce, ExtForceMode::Enum mode = ExtForceMode::IMPULSE) = 0;

	/**
	Apply external impulse on particular node.

	\param[in]	graphNodeIndex	The graph node index to apply impulse on. See #NvBlastSupportGraph.
	\param[in]	localForce		Force to apply in local actor's coordinates.
	\param[in]	mode			The mode to use when applying the force/impulse(see #ExtForceMode)
	*/
	virtual void							addForce(uint32_t graphNodeIndex, physx::PxVec3 localForce, ExtForceMode::Enum mode = ExtForceMode::IMPULSE) = 0;

	/**
	Apply external gravity on particular actor of family. This function applies gravity on every node withing actor, so it makes sense only for static actors.

	\param[in]	actor			The actor to apply impulse on.
	\param[in]	localGravity	Gravity to apply in local actor's coordinates. ExtForceMode::VELOCITY is used.

	\return true iff force was applied on at least one node.
	*/
	virtual bool							addGravityForce(const NvBlastActor& actor, physx::PxVec3 localGravity) = 0;

	/**
	Apply centrifugal force produced by actor's angular movement.

	\param[in]	actor					The actor to apply impulse on.
	\param[in]	localCenterMass			Actor's local center of mass.
	\param[in]	localAngularVelocity	Local angular velocity of an actor.

	\return true iff force was applied on at least one node.
	*/
	virtual bool							addAngularVelocity(const NvBlastActor& actor, physx::PxVec3 localCenterMass, physx::PxVec3 localAngularVelocity) = 0;

	/**
	Update stress solver.

	Actual performance heavy stress calculation happens there. Call it after all relevant forces were applied, usually every frame.
	*/
	virtual void							update() = 0;

	/**
	Get overstressed/broken bonds count. 
	
	This count is updated after every update() call. Number of overstressed bond directly hints if any bond fracture is recommended by stress solver.

	\return the overstressed bonds count.
	*/
	virtual uint32_t						getOverstressedBondCount() const = 0;

	/**
	Generate fracture commands for particular actor.

	Calling this function if getOverstressedBondCount() == 0 or actor has no bond doesn't make sense, bondFractureCount will be '0'.
	Filled fracture commands buffer can be passed directly to NvBlastActorApplyFracture.

	IMPORTANT: NvBlastFractureBuffers::bondFractures will point to internal stress solver memory which will be valid till next call 
	of any of generateFractureCommands() functions or stress solver release() call.

	\param[in]	actor					The actor to fill fracture commands for.
	\param[in]	commands				Pointer to command buffer to fill.
	*/
	virtual void							generateFractureCommands(const NvBlastActor& actor, NvBlastFractureBuffers& commands) = 0;

	/**
	Generate fracture commands for whole family. A bit faster way to get all fractured bonds than calling generateFractureCommands() for every actor.

	Calling this function if getOverstressedBondCount() == 0 or actor has no bond doesn't make sense, bondFractureCount will be '0'.

	IMPORTANT: NvBlastFractureBuffers::bondFractures will point to internal stress solver memory which will be valid till next call
	of any of generateFractureCommands() functions or stress solver release() call.

	\param[in]	commands				Pointer to command buffer to fill.
	*/
	virtual void							generateFractureCommands(NvBlastFractureBuffers& commands) = 0;

	/**
	Generate fracture commands for every actor in family. 
	
	Actors and commands buffer must be passed in order to be filled. It's recommended for bufferSize to be the count of actor with more then one bond in family.

	Calling this function if getOverstressedBondCount() == 0 or actor has no bond doesn't make sense, '0' will be returned.

	IMPORTANT: NvBlastFractureBuffers::bondFractures will point to internal stress solver memory which will be valid till next call
	of any of generateFractureCommands() functions or stress solver release() call.

	\param[out]	buffer			A user-supplied array of NvBlastActor pointers to fill.
	\param[out]	commandsBuffer	A user-supplied array of NvBlastFractureBuffers to fill.
	\param[in]	bufferSize		The number of elements available to write into buffer.

	\return the number of actors and command buffers written to the buffer.
	*/
	virtual uint32_t						generateFractureCommandsPerActor(const NvBlastActor** actorBuffer, NvBlastFractureBuffers* commandsBuffer, uint32_t bufferSize) = 0;

	/**
	Reset stress solver.

	Stress solver uses warm start internally, calling this function will flush all previous data calculated and also zeros frame count.
	This function is to be used for debug purposes. 
	*/
	virtual void							reset() = 0;

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
	Get stress solver total frames count (update() calls) since it was created (or reset).

	\return the frames count.
	*/
	virtual uint32_t						getFrameCount() const = 0;

	/**
	Get stress solver bonds count, after graph reduction was applied.

	\return the bonds count.
	*/
	virtual uint32_t						getBondCount() const = 0;


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
	Used to store a single line and colour for debug rendering.
	*/
	struct DebugLine
	{
		DebugLine(const physx::PxVec3& p0, const physx::PxVec3& p1, const uint32_t& c)
			: pos0(p0), color0(c), pos1(p1), color1(c) {}

		physx::PxVec3	pos0;
		uint32_t		color0;
		physx::PxVec3	pos1;
		uint32_t		color1;
	};

	/**
	Debug Buffer
	*/
	struct DebugBuffer
	{
		const DebugLine* lines;
		uint32_t		 lineCount;
	};

	/**
	Fill debug render for passed array of support graph nodes. 
	
	NOTE: Returned DebugBuffer points into internal memory which is valid till next fillDebugRender() call.

	\param[in]	nodes			Node indices of support graph to debug render for.
	\param[in]	nodeCount		Node indices count.
	\param[in]	mode			Debug render mode.
	\param[in]	scale			Scale to be applied on impulses.

	\return debug buffer with array of lines
	*/
	virtual const DebugBuffer				fillDebugRender(const uint32_t* nodes, uint32_t nodeCount, DebugRenderMode mode, float scale = 1.0f) = 0;


	//////// helpers ////////

	/**
	Get solver iteration per frame (update() call) for particular settings and bondCount.

	Helper method to know how many solver iterations are made per frame.
	This function made so transparent to make it clear how ExtStressSolverSettings::bondIterationsPerFrame is used.

	\param[in]	settings		Debug render mode.
	\param[in]	bondCount		Scale to be applied on impulses.

	\return the iterations per frame count.
	*/
	static uint32_t							getIterationsPerFrame(const ExtStressSolverSettings& settings, uint32_t bondCount)
	{
		uint32_t perFrame = settings.bondIterationsPerFrame / (bondCount + 1);
		return perFrame > 0 ? perFrame : 1;
	}

	/**
	Get iteration per frame (update() call).

	Helper method to know how many solver iterations are made per frame.

	\return the iterations per frame count.
	*/
	uint32_t								getIterationsPerFrame() const
	{
		return getIterationsPerFrame(getSettings(), getBondCount());
	}

};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTSTRESSSOLVER_H
