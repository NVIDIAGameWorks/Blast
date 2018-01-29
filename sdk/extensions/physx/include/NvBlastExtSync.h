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


#ifndef NVBLASTEXTSYNC_H
#define NVBLASTEXTSYNC_H

#include "NvBlastTk.h"
#include "foundation/PxTransform.h"
#include "NvPreprocessor.h"
#include "NvBlastGlobals.h"


namespace Nv
{
namespace Blast
{

class ExtPxFamily;
class ExtPxManager;


/**
Sync Event types
*/
struct ExtSyncEventType
{
	enum Enum
	{
		Fracture = 0, //!< Contains Fracture commands
		FamilySync,	  //!< Contains full family Family blob
		Physics,	  //!< Contains actor's physical info, like transforms

		Count
	};
};


/**
Generic Sync Event
*/
struct NV_DLL_EXPORT ExtSyncEvent
{
	ExtSyncEvent(ExtSyncEventType::Enum t) : type(t) {}
	virtual ~ExtSyncEvent() {}

	template<class T>
	const T* getEvent() const { return reinterpret_cast<const T*>(this); }

	/**
	Any Event can be copied (cloned).

	\return	the pointer to the new copy of event.
	*/
	virtual ExtSyncEvent* clone() const = 0;

	void release();

	ExtSyncEventType::Enum	type;		//!< Event type
	uint64_t				timestamp;	//!< Event timestamp
	NvBlastID				familyID;	//!< TkFamily ID
};


/**
Generic CRTP for Sync Events
*/
template <class T, ExtSyncEventType::Enum eventType>
struct ExtSyncEventInstance : public ExtSyncEvent
{
	ExtSyncEventInstance() : ExtSyncEvent(eventType) {}

	static const ExtSyncEventType::Enum EVENT_TYPE = eventType;

	ExtSyncEvent* clone() const override
	{
		return NVBLAST_NEW (T) (*(T*)this);
	}
};


/**
Fracture Sync Event
*/
struct ExtSyncEventFracture : public ExtSyncEventInstance<ExtSyncEventFracture, ExtSyncEventType::Fracture>
{
	std::vector<NvBlastBondFractureData>	bondFractures;	//!< bond fracture data
	std::vector<NvBlastChunkFractureData>	chunkFractures;	//!< chunk fracture data
};


/**
Family Sync Event
*/
struct ExtSyncEventFamilySync : public ExtSyncEventInstance<ExtSyncEventFamilySync, ExtSyncEventType::FamilySync>
{
	std::vector<char> family;	//!< family binary blob
};


/**
Physics Sync Event
*/
struct ExtSyncEventPhysicsSync : public ExtSyncEventInstance<ExtSyncEventPhysicsSync, ExtSyncEventType::Physics>
{
	struct ActorData
	{
		uint32_t			actorIndex;	//!< actor index in family
		physx::PxTransform	transform;	//!< actor world transform
	};

	std::vector<ActorData> data;		//!< actors data
};


/**
Sync Manager.

Implements TkEventListener interface. It can be directly subscribed to listen for family events.
*/
class NV_DLL_EXPORT ExtSync : public TkEventListener
{
public:
	//////// creation ////////

	/**
	Create a new ExtSync.

	\return the new ExtSync if successful, NULL otherwise.
	*/
	static ExtSync*		create();


	//////// common interface ////////

	/**
	Release Sync manager.
	*/
	virtual void		release() = 0;


	//////// server-side interface ////////

	/**
	TkEventListener interface. 

	\param[in]	events		The array of events being dispatched.
	\param[in]	eventCount	The number of events in the array.
	*/
	virtual void		receive(const TkEvent* events, uint32_t eventCount)  = 0;

	/**
	Sync family state. Writes to internal sync buffer.

	\param[in]	family		The TkFamily to sync
	*/
	virtual void		syncFamily(const TkFamily& family) = 0;

	/**
	Sync PxFamily state. Writes to internal sync buffer.

	\param[in]	family		The ExtPxFamily to sync
	*/
	virtual void		syncFamily(const ExtPxFamily& family) = 0;

	/**
	The size of internal sync buffer (events count).

	\return the number of events in internal sync buffer.
	*/
	virtual uint32_t	getSyncBufferSize() const = 0;

	/**
	Acquire internal sync buffer.

	\param[in] buffer		Reference to sync event buffer pointer to be set.
	\param[in] size			Reference to the size of the buffer array to be set.
	*/
	virtual void		acquireSyncBuffer(const ExtSyncEvent*const*& buffer, uint32_t& size) const = 0;

	/**
	Clear internal sync buffer.
	*/
	virtual void		releaseSyncBuffer() = 0;


	//////// client-side interface ////////

	/**
	Apply external sync buffer on TkFramework and possibly ExtPxManager. This function call will result in
	respective families/actors changes in order to synchronize state.

	\param[in]	framework			The TkFramework instance to be used.
	\param[in]	buffer				Sync buffer array pointer.
	\param[in]	size				Sync buffer array size.
	\param[in]	groupForNewActors	TkGroup to be used for newly created actors. Can be nullptr.
	\param[in]	manager				The ExtPxManager instance to be used. Can be nullptr, physics sync events will be ignored in that case.
	*/
	virtual void		applySyncBuffer(TkFramework& framework, const ExtSyncEvent** buffer, uint32_t size, TkGroup* groupForNewActors, ExtPxManager* manager = nullptr) = 0;

};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTSYNC_H
