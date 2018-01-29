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


#ifndef NVBLASTEXTPXSTRESSSOLVER_H
#define NVBLASTEXTPXSTRESSSOLVER_H

#include "NvBlastExtStressSolver.h"
#include "common/PxRenderBuffer.h"


namespace Nv
{
namespace Blast
{

// forward declarations
class ExtPxFamily;


/**
Px Stress Solver. Px wrapper over ExtStressSolver.

Uses ExtPxFamily and ExtStressSolver. see #ExtStressSolver for more details.
Works on both dynamic and static actor's within family.
For static actors it applies gravity.
For dynamic actors it applies centrifugal force.
*/
class NV_DLL_EXPORT ExtPxStressSolver
{
public:
	//////// creation ////////

	/**
	Create a new ExtStressSolver.

	\param[in]	family			The ExtPxFamily instance to calculate stress on.
	\param[in]	settings		The settings to be set on ExtStressSolver.

	\return the new ExtStressSolver if successful, NULL otherwise.
	*/
	static ExtPxStressSolver*				create(ExtPxFamily& family, ExtStressSolverSettings settings = ExtStressSolverSettings());


	//////// interface ////////

	/**
	Release this stress solver.
	*/
	virtual void							release() = 0;

	/**
	Get actual ExtStressSolver used.

	\return the pointer to ExtStressSolver used internally.
	*/
	virtual ExtStressSolver&				getSolver() const = 0;

	/**
	Update stress solver.

	Calculate stress and optionally apply damage.

	\param[in]	doDamage		If 'true' damage will be applied after stress solver.
	*/
	virtual void							update(bool doDamage = true) = 0;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXSTRESSSOLVER_H
