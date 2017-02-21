/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKJOINT_H
#define NVBLASTTKJOINT_H

#include "NvBlastTkObject.h"

#include "PxVec3.h"


namespace Nv
{
namespace Blast
{

/**
The data contained in a TkJoint.
*/
struct TkJointData
{
	TkActor*		actors[2];			//!< The TkActor objects joined by the joint
	uint32_t		chunkIndices[2];	//!< The chunk indices within the corresponding TkActor objects joined by the joint.  The indexed chunks will be support chunks.
	physx::PxVec3	attachPositions[2];	//!< The position of the joint relative to each TkActor
};


/**
The TkJoint is may join two different TkActors, or be created internally within a single TkActor.

When a TkActor is created from a TkAsset with jointed bonds (the asset is created using a TkAssetDesc with joint flags on bonds, see TkActorDesc) then
internal TkJoint objects are created and associated with every TkActor created from that TkAsset.  The user only gets notification of the internal TkJoint
objects when the TkActor is split into separate TkActor objects that hold the support chunks joined by an internal TkJoint.

The user will be notified when the TkActor objects that are attached to TkJoint objects change, or are released.  In that case, a TkEvent with
a TkJointUpdateEvent payload is dispatched to TkEventListener objects registered with the TkFamily objects to which the actors belong.
*/
class TkJoint : public TkObject
{
public:
	/**
	Retrieve data in this joint.

	\return a TkJointData containing this joint's data.
	*/
	virtual const TkJointData	getData() const = 0;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKJOINT_H
