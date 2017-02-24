/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTIMPULSESTRESSSOLVER_H
#define NVBLASTEXTIMPULSESTRESSSOLVER_H

#include "NvBlastExtStressSolver.h"
#include "NvBlastExtPxManager.h"
#include "NvBlastExtPxListener.h"
#include "NvBlastTypes.h"
#include <NvBlastExtArray.h>
#include <NvBlastExtHashSet.h>
#include <NvBlastExtHashMap.h>

namespace Nv
{
namespace Blast
{


struct ExtStressNodeCachedData
{
	physx::PxVec3 localPos;
	bool isStatic;
};


struct ExtStressBondCachedData
{
	uint32_t bondIndex;
};

class SupportGraphProcessor;

/**
*/
class ExtImpulseStressSolver : public ExtStressSolver, ExtPxListener
{
	NV_NOCOPY(ExtImpulseStressSolver)

public:
	ExtImpulseStressSolver(ExtPxFamily& family, ExtStressSolverSettings settings);
	virtual void							release() override;


	//////// ExtStressSolver interface ////////

	virtual void							setSettings(const ExtStressSolverSettings& settings) override
	{
		m_settings = settings;
	}

	virtual const ExtStressSolverSettings&	getSettings() const override
	{
		return m_settings;
	}

	virtual void							applyImpulse(ExtPxActor& actor, physx::PxVec3 position, physx::PxVec3 force) override;

	virtual void							update(bool doDamage) override;

	void									reset() override
	{
		m_reset = true;
	}

	virtual float							getStressErrorLinear() const override
	{
		return m_errorLinear;
	}

	virtual float							getStressErrorAngular() const override
	{
		return m_errorAngular;
	}

	virtual uint32_t						getIterationCount() const override;

	virtual uint32_t						getFrameCount() const override
	{
		return m_framesCount;
	}

	virtual uint32_t						getBondCount() const override;

	virtual void							fillDebugRender(const std::vector<uint32_t>& nodes, std::vector<physx::PxDebugLine>& lines, DebugRenderMode mode, float scale) override;


	//////// ExtPxListener interface ////////

	virtual void							onActorCreated(ExtPxFamily& family, ExtPxActor& actor) final;

	virtual void							onActorDestroyed(ExtPxFamily& family, ExtPxActor& actor) final;


private:
	~ExtImpulseStressSolver();


	//////// private methods ////////

	void									solve();

	void									applyDamage();

	void									initialize();

	NV_INLINE void							iterate();

	void									syncSolver();

	template<class T>
	NV_INLINE T*							getScratchArray(uint32_t size);


	//////// data ////////

	struct ImpulseData
	{
		physx::PxVec3 position;
		physx::PxVec3 impulse;
	};

	ExtPxFamily&														m_family;
	ExtHashSet<ExtPxActor*>::type										m_actors;
	ExtStressSolverSettings												m_settings;
	NvBlastSupportGraph													m_graph;
	bool																m_isDirty;
	bool																m_reset;
	const float*														m_bondHealths;
	SupportGraphProcessor*												m_graphProcessor;
	float																m_errorAngular;
	float																m_errorLinear;
	uint32_t															m_framesCount;
	ExtArray<NvBlastBondFractureData>::type								m_bondFractureBuffer;
	ExtHashMap<const ExtPxActor*, ExtArray<ImpulseData>::type>::type	m_impulseBuffer;
	ExtArray<uint8_t>::type												m_scratch;
};


template<class T>
NV_INLINE T* ExtImpulseStressSolver::getScratchArray(uint32_t size)
{
	const uint32_t scratchSize = sizeof(T) * size;
	if (m_scratch.size() < scratchSize)
	{
		m_scratch.resize(scratchSize);
	}
	return reinterpret_cast<T*>(m_scratch.begin());
}


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTIMPULSESTRESSSOLVER_H
