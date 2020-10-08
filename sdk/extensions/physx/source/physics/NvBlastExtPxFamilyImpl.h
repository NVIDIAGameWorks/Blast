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


#ifndef NVBLASTEXTPXFAMILYIMPL_H
#define NVBLASTEXTPXFAMILYIMPL_H

#include "NvBlastExtPxFamily.h"
#include "NvBlastArray.h"
#include "NvBlastHashSet.h"
#include "foundation/PxTransform.h"
#include "NvBlastTkEvent.h"


using namespace physx;


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxManagerImpl;
class ExtPxActorImpl;
struct PxActorCreateInfo;


class ExtPxFamilyImpl final : public ExtPxFamily, TkEventListener
{
	NV_NOCOPY(ExtPxFamilyImpl)

public:
	friend ExtPxActorImpl;
	friend ExtPxManagerImpl;

	//////// ctor ////////

	ExtPxFamilyImpl(ExtPxManagerImpl& manager, TkFamily& tkFamily, ExtPxAsset& pxAsset);
	~ExtPxFamilyImpl();

	virtual void release() override;


	//////// ExtPxFamily interface ////////

//	virtual bool							spawn(const PxTransform& pose, const ExtPxSpawnSettings& settings) override;
	virtual bool							spawn(const physx::PxTransform& pose, const physx::PxVec3& scale, const ExtPxSpawnSettings& settings) override;
	virtual bool							despawn() override;


	virtual uint32_t						getActorCount() const override
	{
		return m_actors.size();
	}

	virtual uint32_t						getActors(ExtPxActor** buffer, uint32_t bufferSize) const override
	{
		uint32_t index = 0;
		for (auto it = const_cast<ExtPxFamilyImpl*>(this)->m_actors.getIterator(); !it.done() && index < bufferSize; ++it)
		{
			buffer[index++] = *it;
		}
		return index;
	}

	virtual TkFamily&						getTkFamily() const override
	{
		return m_tkFamily;
	}

	virtual const physx::PxShape* const*	getSubchunkShapes() const override
	{
		return m_subchunkShapes.begin();
	}

	virtual ExtPxAsset&						getPxAsset() const override
	{
		return m_pxAsset;
	}

	virtual void							setMaterial(PxMaterial& material) override
	{
		m_spawnSettings.material = &material;
	}

	virtual void							setPxShapeDescTemplate(const ExtPxShapeDescTemplate* pxShapeDesc) override
	{
		m_pxShapeDescTemplate = pxShapeDesc;
	}

	virtual const ExtPxShapeDescTemplate*	getPxShapeDescTemplate() const override
	{
		return m_pxShapeDescTemplate;
	}

	virtual void							setPxActorDesc(const ExtPxActorDescTemplate* pxActorDesc) override
	{
		m_pxActorDescTemplate = pxActorDesc;
	}

	virtual const ExtPxActorDescTemplate*	getPxActorDesc() const override
	{
		return m_pxActorDescTemplate;
	}

	virtual const NvBlastExtMaterial*		getMaterial() const override
	{
		return m_material;
	}

	virtual void							setMaterial(const NvBlastExtMaterial* material) override
	{
		m_material = material;
	}

	virtual void							subscribe(ExtPxListener& listener) override
	{
		m_listeners.pushBack(&listener);
	}

	virtual void							unsubscribe(ExtPxListener& listener) override
	{
		m_listeners.findAndReplaceWithLast(&listener);
	}

	virtual void							postSplitUpdate() override;

	//////// TkEventListener interface ////////

	virtual void							receive(const TkEvent* events, uint32_t eventCount) override;


	//////// events dispatch ////////

	void									dispatchActorCreated(ExtPxActor& actor);
	void									dispatchActorDestroyed(ExtPxActor& actor);


private:
	//////// private methods ////////

	void									createActors(TkActor** tkActors, const PxActorCreateInfo* pxActorInfos, uint32_t count);
	void									destroyActors(ExtPxActor** actors, uint32_t count);

	//////// data ////////

	ExtPxManagerImpl&						m_manager;
	TkFamily&								m_tkFamily;
	ExtPxAsset&								m_pxAsset;
	ExtPxSpawnSettings						m_spawnSettings;
	const ExtPxShapeDescTemplate*			m_pxShapeDescTemplate;
	const ExtPxActorDescTemplate*			m_pxActorDescTemplate;
	const NvBlastExtMaterial*				m_material;
	bool									m_isSpawned;
	PxTransform								m_initialTransform;
	PxVec3									m_initialScale;
	HashSet<ExtPxActor*>::type			    m_actors;
	Array<TkActor*>::type				    m_culledActors;
	InlineArray<ExtPxListener*, 4>::type	m_listeners;
	Array<PxShape*>::type				    m_subchunkShapes;
	Array<TkActor*>::type				    m_newActorsBuffer;
	Array<PxActorCreateInfo>::type		    m_newActorCreateInfo;
	Array<PxActor*>::type				    m_physXActorsBuffer;
	Array<ExtPxActor*>::type				m_actorsBuffer;
	Array<uint32_t>::type				    m_indicesScratch;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXFAMILYIMPL_H
