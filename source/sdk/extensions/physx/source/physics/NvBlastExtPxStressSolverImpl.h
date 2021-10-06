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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTEXTPXSTRESSSOLVERIMPL_H
#define NVBLASTEXTPXSTRESSSOLVERIMPL_H

#include "NvBlastExtPxStressSolver.h"
#include "NvBlastExtPxListener.h"
#include "NvBlastArray.h"
#include "NvBlastHashSet.h"

namespace Nv
{
namespace Blast
{


class ExtPxStressSolverImpl final : public ExtPxStressSolver, ExtPxListener
{
	NV_NOCOPY(ExtPxStressSolverImpl)

public:
	ExtPxStressSolverImpl(ExtPxFamily& family, ExtStressSolverSettings settings);


	//////// ExtPxStressSolver interface ////////

	virtual void							release() override;

	virtual ExtStressSolver&				getSolver() const override
	{
		return *m_solver;
	}

	virtual void							update(bool doDamage) override;


	//////// ExtPxListener interface ////////

	virtual void							onActorCreated(ExtPxFamily& family, ExtPxActor& actor) final;

	virtual void							onActorDestroyed(ExtPxFamily& family, ExtPxActor& actor) final;


private:
	~ExtPxStressSolverImpl();


	//////// data ////////

	ExtPxFamily&				m_family;
	ExtStressSolver*			m_solver;
	HashSet<ExtPxActor*>::type  m_actors;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXSTRESSSOLVERIMPL_H
