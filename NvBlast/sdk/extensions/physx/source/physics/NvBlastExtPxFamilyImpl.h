/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTPXFAMILYIMPL_H
#define NVBLASTEXTPXFAMILYIMPL_H

#include "NvBlastExtPxFamily.h"
#include "NvBlastExtArray.h"
#include "NvBlastExtHashSet.h"
#include "PxTransform.h"
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

	ExtPxFamilyImpl(ExtPxManagerImpl& manager, TkFamily& tkFamily, const ExtPxAsset& pxAsset);
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

	virtual const ExtPxAsset&				getPxAsset() const override
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
	const ExtPxAsset&						m_pxAsset;
	ExtPxSpawnSettings						m_spawnSettings;
	const ExtPxShapeDescTemplate*			m_pxShapeDescTemplate;
	const ExtPxActorDescTemplate*			m_pxActorDescTemplate;
	bool									m_isSpawned;
	PxTransform								m_initialTransform;
	PxVec3									m_initialScale;
	ExtHashSet<ExtPxActor*>::type			m_actors;
	ExtArray<TkActor*>::type				m_culledActors;
	ExtInlineArray<ExtPxListener*, 4>::type	m_listeners;
	ExtArray<PxShape*>::type				m_subchunkShapes;
	ExtArray<TkActor*>::type				m_newActorsBuffer;
	ExtArray<PxActorCreateInfo>::type		m_newActorCreateInfo;
	ExtArray<PxActor*>::type				m_physXActorsBuffer;
	ExtArray<ExtPxActor*>::type				m_actorsBuffer;
	ExtArray<uint32_t>::type				m_indicesScratch;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXFAMILYIMPL_H
