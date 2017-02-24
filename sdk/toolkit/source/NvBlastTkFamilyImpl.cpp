/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastTkFrameworkImpl.h"
#include "NvBlastTkFamilyImpl.h"
#include "NvBlastTkGroupImpl.h"
#include "NvBlastTkAssetImpl.h"
#include "NvBlastTkActorImpl.h"
#include "NvBlastTkJointImpl.h"

#include "Px.h"
#include "PxFileBuf.h"
#include "PxAllocatorCallback.h"

#include "NvBlastIndexFns.h"
#include "NvBlastMemory.h"

using namespace physx::general_PxIOStream2;


namespace Nv
{
namespace Blast
{

//////// Static data ////////

NVBLASTTK_DEFINE_TYPE_SERIALIZABLE(Family);


//////// Member functions ////////

TkFamilyImpl::TkFamilyImpl() : m_familyLL(nullptr), m_internalJointCount(0), m_asset(nullptr), m_material(nullptr)
{
}


TkFamilyImpl::TkFamilyImpl(const NvBlastID& id) : TkFamilyType(id), m_familyLL(nullptr), m_internalJointCount(0), m_asset(nullptr), m_material(nullptr)
{
}


TkFamilyImpl::~TkFamilyImpl()
{
	if (m_familyLL != nullptr)
	{
		uint32_t familyActorCount = NvBlastFamilyGetActorCount(m_familyLL, TkFrameworkImpl::get()->log);
		if (familyActorCount != 0)
		{
			NVBLASTTK_LOG_WARNING("TkFamilyImpl::~TkFamilyImpl(): family actor count is not 0.");
		}
		TkFrameworkImpl::get()->free(m_familyLL);
	}
}


void TkFamilyImpl::release()
{
	for (TkActorImpl& actor : m_actors)
	{
		if (actor.isActive())
		{
			actor.release();
		}
	}

	m_actors.clear();

	NVBLASTTK_DELETE(this, TkFamilyImpl);
}


const NvBlastFamily* TkFamilyImpl::getFamilyLL() const
{
	return m_familyLL;
}


TkActorImpl* TkFamilyImpl::addActor(NvBlastActor* actorLL)
{
	TkActorImpl* actor = getActorByActorLL(actorLL);
	NVBLAST_ASSERT(actor);
	actor->m_actorLL = actorLL;
	actor->m_family = this;
	return actor;
}


void TkFamilyImpl::removeActor(TkActorImpl* actor)
{
	NVBLAST_ASSERT(actor != nullptr && actor->m_family == this);
	//actor->m_family = nullptr;
	actor->m_actorLL = nullptr;
}


uint32_t TkFamilyImpl::getActorCount() const
{
	return getActorCountInternal();
}


uint32_t TkFamilyImpl::getActors(TkActor** buffer, uint32_t bufferSize, uint32_t indexStart /*= 0*/) const
{
	uint32_t actorCount = getActorCount();
	if (actorCount <= indexStart)
	{
		NVBLASTTK_LOG_WARNING("TkFamilyImpl::getActors: indexStart beyond end of actor list.");
		return 0;
	}

	actorCount -= indexStart;
	if (actorCount > bufferSize)
	{
		actorCount = static_cast<uint32_t>(bufferSize);
	}

	uint32_t index = 0;
	for (const TkActorImpl& actor : m_actors)
	{
		if (actor.isActive())
		{
			if (index >= indexStart)
			{
				if ((index - indexStart) >= actorCount)
				{
					break;
				}
				else
				{
					*buffer++ = const_cast<TkActorImpl*>(&actor);
				}
			}
			index++;
		}
	}

	return actorCount;
}


NV_INLINE bool areLLActorsEqual(const NvBlastActor* actor0, const NvBlastActor* actor1, TkArray<uint32_t>::type& scratch)
{
	if (NvBlastActorGetGraphNodeCount(actor0, TkFrameworkImpl::get()->log) != NvBlastActorGetGraphNodeCount(actor1, TkFrameworkImpl::get()->log))
	{
		return false;
	}

	const uint32_t chunkCount = NvBlastActorGetVisibleChunkCount(actor0, TkFrameworkImpl::get()->log);
	if (chunkCount != NvBlastActorGetVisibleChunkCount(actor1, TkFrameworkImpl::get()->log))
	{
		return false;
	}

	scratch.resize(chunkCount * 2);
	NvBlastActorGetVisibleChunkIndices(scratch.begin(), chunkCount, actor0, TkFrameworkImpl::get()->log);
	NvBlastActorGetVisibleChunkIndices(scratch.begin() + chunkCount, chunkCount, actor1, TkFrameworkImpl::get()->log);
	return memcmp(scratch.begin(), scratch.begin() + chunkCount, chunkCount * sizeof(uint32_t)) == 0;
}


void TkFamilyImpl::reinitialize(const NvBlastFamily* newFamily, TkGroup* group)
{
	NVBLAST_ASSERT(newFamily);
#if NV_ENABLE_ASSERTS
	NvBlastID id0 = NvBlastFamilyGetAssetID(m_familyLL, TkFrameworkImpl::get()->log);
	NvBlastID id1 = NvBlastFamilyGetAssetID(newFamily, TkFrameworkImpl::get()->log);
	NVBLAST_ASSERT(TkGUIDsEqual(&id0, &id1));
#endif
	NVBLAST_ASSERT(NvBlastFamilyGetSize(m_familyLL, TkFrameworkImpl::get()->log) == NvBlastFamilyGetSize(newFamily, TkFrameworkImpl::get()->log));

	// alloc and init new family
	const uint32_t blockSize = NvBlastFamilyGetSize(newFamily, TkFrameworkImpl::get()->log);
	NvBlastFamily* newFamilyCopy = (NvBlastFamily*)TkFrameworkImpl::get()->alloc(blockSize);
	memcpy(newFamilyCopy, newFamily, blockSize);
	NvBlastFamilySetAsset(newFamilyCopy, m_asset->getAssetLL(), TkFrameworkImpl::get()->log);

	// get actors from new family
	TkArray<NvBlastActor*>::type newLLActors(NvBlastFamilyGetActorCount(newFamilyCopy, TkFrameworkImpl::get()->log));
	uint32_t actorCount = NvBlastFamilyGetActors(newLLActors.begin(), newLLActors.size(), newFamilyCopy, TkFrameworkImpl::get()->log);

	// reset actor families to nullptr (we use it as a flag later)
	for (TkActorImpl& actor : m_actors)
	{
		if (actor.isActive())
		{
			actor.m_family = nullptr;
		}
	}

	// prepare split event with new actors
	auto newActorsSplitEvent = getQueue().allocData<TkSplitEvent>();
	TkArray<TkActor*>::type children(actorCount);
	children.resizeUninitialized(0);
	newActorsSplitEvent->children = children.begin();

	// scratch
	TkArray<uint32_t>::type scratch(m_asset->getChunkCount());

	for (uint32_t i = 0; i < actorCount; ++i)
	{
		NvBlastActor* newLLActor = newLLActors[i];
		uint32_t actorIndex = NvBlastActorGetIndex(newLLActor, TkFrameworkImpl::get()->log);
		TkActorImpl& tkActor = *getActorByIndex(actorIndex);

		tkActor.m_family = this;

		if (!tkActor.isActive() || !areLLActorsEqual(newLLActor, tkActor.m_actorLL, scratch))
		{
			if (tkActor.isActive())
			{
				auto removeSplitEvent = getQueue().allocData<TkSplitEvent>();
				removeSplitEvent->parentData.family = this;
				removeSplitEvent->numChildren = 0;
				removeSplitEvent->parentData.userData = tkActor.userData;
				removeSplitEvent->parentData.index = tkActor.getIndex();
				getQueue().addEvent(removeSplitEvent);
			}

			tkActor.m_actorLL = newLLActor;

			// switch groups
			TkGroupImpl* prevGroup = tkActor.m_group;
			if (prevGroup != group)
			{
				if (prevGroup)
				{
					prevGroup->removeActor(tkActor);
				}
				if (group)
				{
					group->addActor(tkActor);
				}
			}

			children.pushBack(&tkActor);
		}
		else
		{
			tkActor.m_actorLL = newLLActor;
		}
	}

	// if m_family is still nullptr for an active actor -> remove it. It doesn't exist in new family.
	for (TkActorImpl& tkActor : m_actors)
	{
		if (tkActor.isActive() && tkActor.m_family == nullptr)
		{
			tkActor.m_family = this;
			if (tkActor.m_group)
			{
				tkActor.m_group->removeActor(tkActor);
			}

			auto removeSplitEvent = getQueue().allocData<TkSplitEvent>();
			removeSplitEvent->parentData.family = this;
			removeSplitEvent->numChildren = 0;
			removeSplitEvent->parentData.userData = tkActor.userData;
			removeSplitEvent->parentData.index = tkActor.getIndex();
			getQueue().addEvent(removeSplitEvent);

			tkActor.m_actorLL = nullptr;
		}
	}

	// add split event with all new actors
	newActorsSplitEvent->parentData.family = this;
	newActorsSplitEvent->parentData.userData = 0;
	newActorsSplitEvent->parentData.index = invalidIndex<uint32_t>();
	newActorsSplitEvent->numChildren = children.size();
	if (newActorsSplitEvent->numChildren > 0)
	{
		getQueue().addEvent(newActorsSplitEvent);
	}

	// replace family
	TkFrameworkImpl::get()->free(m_familyLL);
	m_familyLL = newFamilyCopy;

	// update joints
	for (TkActorImpl& tkActor : m_actors)
	{
		if (!tkActor.m_jointList.isEmpty())
		{
			updateJoints(&tkActor);
		}
	}

	getQueue().dispatch();
}


TkActorImpl* TkFamilyImpl::getActorByChunk(uint32_t chunk)
{
	if (chunk >= NvBlastAssetGetChunkCount(m_asset->getAssetLLInternal(), TkFrameworkImpl::get()->log))
	{
		NVBLASTTK_LOG_WARNING("TkFamilyImpl::getActorByChunk: invalid chunk index.  Returning NULL.");
		return nullptr;
	}

	NvBlastActor* actorLL = NvBlastFamilyGetChunkActor(m_familyLL, chunk, TkFrameworkImpl::get()->log);
	return actorLL ? getActorByActorLL(actorLL) : nullptr;
}


void TkFamilyImpl::applyFractureInternal(const NvBlastFractureBuffers* commands)
{
	NvBlastSupportGraph graph = getAsset()->getGraph();

	// apply bond fracture commands on relevant actors
	{
		TkActorImpl* currActor = nullptr;
		NvBlastBondFractureData* bondFractures = commands->bondFractures;
		uint32_t bondFracturesCount = 0;

		auto applyFracture = [&]()
		{
			if (bondFracturesCount > 0)
			{
				if (currActor != nullptr && currActor->isActive())
				{
					NvBlastFractureBuffers newCommands;
					newCommands.bondFractures = bondFractures;
					newCommands.bondFractureCount = bondFracturesCount;
					newCommands.chunkFractures = nullptr;
					newCommands.chunkFractureCount = 0;
					currActor->applyFracture(nullptr, &newCommands);
				}

				bondFractures += bondFracturesCount;
				bondFracturesCount = 0;
			}
		};

		for (uint32_t i = 0; i < commands->bondFractureCount; ++i, ++bondFracturesCount)
		{
			const NvBlastBondFractureData& command = commands->bondFractures[i];
			uint32_t chunk0 = graph.chunkIndices[command.nodeIndex0];
			uint32_t chunk1 = graph.chunkIndices[command.nodeIndex1];
			TkActorImpl* actor0 = getActorByChunk(chunk0);
			TkActorImpl* actor1 = getActorByChunk(chunk1);
			if (actor0 != actor1)
			{
				// skipping this event, bond already broken
				actor0 = nullptr;
			}
			if (actor0 != currActor)
			{
				applyFracture();
				currActor = actor0;
			}
		}

		if (bondFracturesCount > 0)
		{
			applyFracture();
		}
	}

	// apply chunk fracture commands on relevant actors
	{
		TkActorImpl* currActor = nullptr;
		NvBlastChunkFractureData* chunkFractures = commands->chunkFractures;
		uint32_t chunkFracturesCount = 0;

		auto applyFracture = [&]()
		{
			if (chunkFracturesCount > 0)
			{
				if (currActor != nullptr && currActor->isActive())
				{
					NvBlastFractureBuffers newCommands;
					newCommands.bondFractures = nullptr;
					newCommands.bondFractureCount = 0;
					newCommands.chunkFractures = chunkFractures;
					newCommands.chunkFractureCount = chunkFracturesCount;
					currActor->applyFracture(nullptr, &newCommands);
				}

				chunkFractures += chunkFracturesCount;
				chunkFracturesCount = 0;
			}
		};

		for (uint32_t i = 0; i < commands->chunkFractureCount; ++i, ++chunkFracturesCount)
		{
			const NvBlastChunkFractureData& command = commands->chunkFractures[i];
			TkActorImpl* actor = getActorByChunk(command.chunkIndex);
			if (actor != currActor)
			{
				applyFracture();
				currActor = actor;
			}
		}
		if (chunkFracturesCount > 0)
		{
			applyFracture();
		}
	}
}


void TkFamilyImpl::updateJoints(TkActorImpl* actor, TkEventQueue* alternateQueue)
{
	// Copy joint array for safety against implementation of joint->setActor
	TkJointImpl** joints = reinterpret_cast<TkJointImpl**>(NvBlastAlloca(sizeof(TkJointImpl*)*actor->getJointCountInternal()));
	TkJointImpl** stop = joints + actor->getJointCountInternal();
	TkJointImpl** jointHandle = joints;
	for (TkActorImpl::JointIt j(*actor); (bool)j; ++j)
	{
		*jointHandle++ = *j;
	}
	jointHandle = joints;
	while (jointHandle < stop)
	{
		TkJointImpl* joint = *jointHandle++;
		
		const TkJointData& data = joint->getDataInternal();

		TkActorImpl* actor0 = data.actors[0] != nullptr ?
			static_cast<TkActorImpl&>(*data.actors[0]).getFamilyImpl().getActorByChunk(data.chunkIndices[0]) : nullptr;

		TkActorImpl* actor1 = data.actors[1] != nullptr ?
			static_cast<TkActorImpl&>(*data.actors[1]).getFamilyImpl().getActorByChunk(data.chunkIndices[1]) : nullptr;

		joint->setActors(actor0, actor1, alternateQueue);
	}
}


const TkAsset* TkFamilyImpl::getAsset() const
{
	return m_asset;
}


bool TkFamilyImpl::serialize(PxFileBuf& stream) const
{
	TkFrameworkImpl::get()->serializeHeader(*this, stream);

	if (m_material != nullptr)
	{
		NVBLASTTK_LOG_WARNING("TkFamilyImpl::serialize(): Material pointer is not nullptr, it will be lost during serialization.");
	}

	NVBLASTTK_CHECK_ERROR(m_asset != nullptr, "TkFamilyImpl::serialize(): TkFamily asset is nullptr, can't be serialized.", return false);
	NVBLASTTK_CHECK_ERROR(m_familyLL != nullptr, "TkFamilyImpl::serialize(): TkFamily family is nullptr, can't be serialized.", return false);

	// Asset ID
	const NvBlastID& assetID = m_asset->getID();
	NVBLASTTK_CHECK_ERROR(!TkGUIDIsZero(&assetID), "TkFamilyImpl::serialize(): Associated asset doesn't have an ID set.", return false);
	stream.write(&assetID, sizeof(NvBlastID));

	// Family
	const uint32_t familySize = NvBlastFamilyGetSize(m_familyLL, TkFrameworkImpl::get()->log);
	stream.storeDword(familySize);
	stream.write(m_familyLL, familySize);

	//// Joints ////

	// Internal joint data
	stream.storeDword(m_internalJointCount);

	// External joint family ID list
	stream.storeDword(m_jointSets.size());
	for (uint32_t i = 0; i < m_jointSets.size(); ++i)
	{
		const JointSet* jointSet = m_jointSets[i];
		stream.write(&jointSet->m_familyID, sizeof(NvBlastID));
	}

	// Actor joint lists
	TkJointImpl* internalJoints = getInternalJoints();
	for (uint32_t actorNum = 0; actorNum < m_actors.size(); ++actorNum)
	{
		const TkActorImpl& actor = m_actors[actorNum];
		if (!actor.isActive())
		{
			continue;	// We may need a better way of iterating through active actors
		}

		stream.storeDword(actor.getJointCount());

		for (TkActorImpl::JointIt j(actor); (bool)j; ++j)
		{
			TkJointImpl* joint = *j;

			const TkJointData& jointData = joint->getDataInternal();
			NVBLAST_ASSERT(jointData.actors[0] == &actor || jointData.actors[1] == &actor);

			const uint32_t attachmentFlags = (uint32_t)(jointData.actors[0] == &actor) | (uint32_t)(jointData.actors[1] == &actor) << 1;
			stream.storeDword(attachmentFlags);

			const TkActorImpl* otherActor = static_cast<const TkActorImpl*>(jointData.actors[(attachmentFlags >> 1) ^ 1]);

			if (joint->m_owner == this)
			{
				// Internal joint - write internal joint index
				const uint32_t jointIndex = static_cast<uint32_t>(joint - internalJoints);
				stream.storeDword(jointIndex);
				if (otherActor != nullptr && otherActor->getIndexInternal() < actorNum)	// No need to write the joint data, it has already been written
				{
					continue;
				}
			}
			else
			{
				// External joint - write external family index and joint information
				stream.storeDword(invalidIndex<uint32_t>());	// Denotes external joint

				const FamilyIDMap::Entry* e = m_familyIDMap.find(getFamilyID(otherActor));
				NVBLASTTK_CHECK_ERROR(e != nullptr, "TkFamilyImpl::deserialize(): Bad data - attached family's ID not recorded.", return false);

				stream.storeDword(e->second);	// Write family ID index
			}

			// Write joint data
			for (int side = 0; side < 2; ++side)
			{
				stream.storeDword(jointData.chunkIndices[side]);
				const physx::PxVec3& attachPosition = jointData.attachPositions[side];
				stream.storeFloat(attachPosition.x); stream.storeFloat(attachPosition.y); stream.storeFloat(attachPosition.z);
			}
		}
	}

	return true;
}


//////// Static functions ////////

TkSerializable* TkFamilyImpl::deserialize(PxFileBuf& stream, const NvBlastID& id)
{
	// Asset resolve
	NvBlastID assetID;
	stream.read(&assetID, sizeof(NvBlastID));
	TkIdentifiable* object = TkFrameworkImpl::get()->findObjectByIDInternal(assetID);
	NVBLASTTK_CHECK_ERROR(object && object->getType() == TkAssetImpl::s_type, "TkFamilyImpl::deserialize: can't find asset with corresponding ID.", return nullptr);
	TkAssetImpl* asset = static_cast<TkAssetImpl*>(object);

	// Allocate
	TkFamilyImpl* family = NVBLASTTK_NEW(TkFamilyImpl)(id);
	NVBLASTTK_CHECK_ERROR(family != nullptr, "TkFamilyImpl::deserialize: family allocation failed.", return nullptr);

	// associate with found asset
	family->m_asset = asset;

	// Family
	const uint32_t familySize = stream.readDword();
	family->m_familyLL = static_cast<NvBlastFamily*>(TkFrameworkImpl::get()->alloc(familySize));
	stream.read(family->m_familyLL, familySize);

	if (family->m_familyLL == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkFamilyImpl::deserialize: low-level family could not be created.");
		family->release();
		return nullptr;
	}

#if NV_ENABLE_ASSERTS && 0
	NvBlastID id = NvBlastFamilyGetAssetID(family->m_familyLL, TkFrameworkImpl::get()->log);
	NVBLAST_ASSERT(TkGUIDsEqual(&id, &assetID));
#endif

	// preallocate actors
	uint32_t maxActorCount = NvBlastFamilyGetMaxActorCount(family->m_familyLL, TkFrameworkImpl::get()->log);
	family->m_actors.resize(maxActorCount);

	// get actors from family
	TkArray<NvBlastActor*>::type newLLActors(NvBlastFamilyGetActorCount(family->m_familyLL, TkFrameworkImpl::get()->log));
	uint32_t actorCount = NvBlastFamilyGetActors(newLLActors.begin(), newLLActors.size(), family->m_familyLL, TkFrameworkImpl::get()->log);

	// fill actors
	for (uint32_t i = 0; i < actorCount; ++i)
	{
		NvBlastActor* newLLActor = newLLActors[i];
		uint32_t actorIndex = NvBlastActorGetIndex(newLLActor, TkFrameworkImpl::get()->log);
		TkActorImpl& tkActor = *family->getActorByIndex(actorIndex);

		tkActor.m_family = family;
		tkActor.m_actorLL = newLLActor;
	}

	//// Create joints ////

	// internal
	family->m_internalJointCount = stream.readDword();
	family->m_internalJointBuffer.resize(family->m_internalJointCount * sizeof(TkJointImpl), '\0');
	TkJointImpl* internalJoints = family->getInternalJoints();

	// external joint family ID list
	const uint32_t jointSetCount = stream.readDword();
	family->m_jointSets.resize(jointSetCount);
	for (uint32_t i = 0; i < jointSetCount; ++i)
	{
		family->m_jointSets[i] = NVBLASTTK_NEW(JointSet);
		stream.read(&family->m_jointSets[i]->m_familyID, sizeof(NvBlastID));
		family->m_familyIDMap[family->m_jointSets[i]->m_familyID] = i;
	}

	// fill actor joint lists
	for (uint32_t actorNum = 0; actorNum < family->m_actors.size(); ++actorNum)
	{
		TkActorImpl& actor = family->m_actors[actorNum];
		if (!actor.isActive())
		{
			continue;	// We may need a better way of iterating through active actors
		}

		// Read joint information
		uint32_t jointCount = stream.readDword();
		while (jointCount--)
		{
			const uint32_t attachmentFlags = stream.readDword();
			const uint32_t jointIndex = stream.readDword();
			if (!isInvalidIndex(jointIndex))
			{
				// Internal joint
				TkJointImpl& joint = internalJoints[jointIndex];
				TkJointData& jointData = joint.getDataWritable();

				// Initialize joint if it has not been encountered yet
				NVBLAST_ASSERT((joint.m_links[0].m_joint == nullptr) == (joint.m_links[1].m_joint == nullptr));
				if (joint.m_links[0].m_joint == nullptr)
				{
					new (&joint) TkJointImpl;
					joint.m_owner = family;
					for (int side = 0; side < 2; ++side)
					{
						jointData.chunkIndices[side] = stream.readDword();
						physx::PxVec3& attachPosition = jointData.attachPositions[side];
						attachPosition.x = stream.readFloat();	attachPosition.y = stream.readFloat();	attachPosition.z = stream.readFloat();
					}
				}

				if (attachmentFlags & 1)
				{
					jointData.actors[0] = &actor;
					actor.addJoint(joint.m_links[0]);
				}

				if (attachmentFlags & 2)
				{
					jointData.actors[1] = &actor;
					if (jointData.actors[0] != jointData.actors[1])
					{
						actor.addJoint(joint.m_links[1]);
					}
				}
			}
			else
			{
				// External joint
				const uint32_t otherFamilyIndex = stream.readDword();
				NVBLASTTK_CHECK_ERROR(otherFamilyIndex < family->m_jointSets.size(), "TkFamilyImpl::deserialize: family allocation failed.", return nullptr);
				const NvBlastID& otherFamilyID = family->m_jointSets[otherFamilyIndex]->m_familyID;
				TkFamilyImpl* otherFamily = static_cast<TkFamilyImpl*>(TkFrameworkImpl::get()->findObjectByIDInternal(otherFamilyID));

				TkJointDesc jointDesc;
				for (int side = 0; side < 2; ++side)
				{
					jointDesc.chunkIndices[side] = stream.readDword();
					physx::PxVec3& attachPosition = jointDesc.attachPositions[side];
					attachPosition.x = stream.readFloat();	attachPosition.y = stream.readFloat();	attachPosition.z = stream.readFloat();
				}

				NVBLASTTK_CHECK_ERROR(attachmentFlags != 3, "TkFamilyImpl::deserialize: both attached actors are the same in an external joint.", return nullptr);

				const uint32_t attachmentIndex = attachmentFlags >> 1;

				TkJointImpl** jointHandle = family->createExternalJointHandle(otherFamilyID, jointDesc.chunkIndices[attachmentIndex], jointDesc.chunkIndices[attachmentIndex ^ 1]);
				NVBLASTTK_CHECK_ERROR(jointHandle != nullptr, "TkFamilyImpl::deserialize: joint handle could not be created.", return nullptr);

				if (otherFamily == nullptr)
				{
					// Other family does not exist yet, we'll create the joint
					jointDesc.families[attachmentIndex] = family;
					jointDesc.families[attachmentIndex ^ 1] = nullptr;

					TkJointImpl* joint = NVBLASTTK_NEW(TkJointImpl)(jointDesc, nullptr);
					NVBLASTTK_CHECK_ERROR(joint != nullptr, "TkFamilyImpl::deserialize: joint createion failed.", return nullptr);

					*jointHandle = joint;

					actor.addJoint(joint->m_links[attachmentIndex]);
				}
				else
				{
					// Other family exists, and should have created the joint
					TkJointImpl* joint = otherFamily->findExternalJoint(family, ExternalJointKey(jointDesc.chunkIndices[attachmentIndex ^ 1], jointDesc.chunkIndices[attachmentIndex]));
					NVBLASTTK_CHECK_ERROR(joint != nullptr, "TkFamilyImpl::deserialize: other family should have created joint, but did not.", return nullptr);

					*jointHandle = joint;

					// Add the joint to its actor(s)
					joint->getDataWritable().actors[attachmentIndex] = &actor;
					actor.addJoint(joint->m_links[attachmentIndex]);
				}
			}
		}
	}

	return family;
}


TkFamilyImpl* TkFamilyImpl::create(const TkAssetImpl* asset)
{
	TkFamilyImpl* family = NVBLASTTK_NEW(TkFamilyImpl);
	family->m_asset = asset;
	void* mem = TkFrameworkImpl::get()->alloc(NvBlastAssetGetFamilyMemorySize(asset->getAssetLL(), TkFrameworkImpl::get()->log));
	family->m_familyLL = NvBlastAssetCreateFamily(mem, asset->getAssetLL(), TkFrameworkImpl::get()->log);
	//family->addListener(*TkFrameworkImpl::get());

	if (family->m_familyLL == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkFamilyImpl::create: low-level family could not be created.");
		family->release();
		return nullptr;
	}

	uint32_t maxActorCount = NvBlastFamilyGetMaxActorCount(family->m_familyLL, TkFrameworkImpl::get()->log);
	family->m_actors.resize(maxActorCount);

	family->m_internalJointBuffer.resize(asset->getJointDescCountInternal() * sizeof(TkJointImpl), 0);
	family->m_internalJointCount = asset->getJointDescCountInternal();

	return family;
}


TkJointImpl** TkFamilyImpl::createExternalJointHandle(const NvBlastID& otherFamilyID, uint32_t chunkIndex0, uint32_t chunkIndex1)
{
	JointSet* jointSet;
	const FamilyIDMap::Entry* jointSetIndexEntry = m_familyIDMap.find(otherFamilyID);
	uint32_t otherFamilyIndex;
	if (jointSetIndexEntry != nullptr)
	{
		otherFamilyIndex = jointSetIndexEntry->second;
		jointSet = m_jointSets[otherFamilyIndex];
	}
	else
	{
		jointSet = NVBLASTTK_NEW(JointSet);
		NVBLASTTK_CHECK_ERROR(jointSet != nullptr, "TkFamilyImpl::addExternalJoint: failed to create joint set for other family ID.", return nullptr);
		jointSet->m_familyID = otherFamilyID;
		otherFamilyIndex = m_jointSets.size();
		m_familyIDMap[otherFamilyID] = otherFamilyIndex;
		m_jointSets.pushBack(jointSet);
	}

	const ExternalJointKey key(chunkIndex0, chunkIndex1);
	const bool jointExists = jointSet->m_joints.find(key) != nullptr;
	NVBLASTTK_CHECK_WARNING(!jointExists, "TkFamilyImpl::addExternalJoint: joint already added.", return nullptr);

	return &jointSet->m_joints[key];
}


bool TkFamilyImpl::deleteExternalJointHandle(TkJointImpl*& joint, const NvBlastID& otherFamilyID, uint32_t chunkIndex0, uint32_t chunkIndex1)
{
	const FamilyIDMap::Entry* jointSetIndexEntry = m_familyIDMap.find(otherFamilyID);
	if (jointSetIndexEntry != nullptr)
	{
		const uint32_t jointSetIndex = jointSetIndexEntry->second;
		TkHashMap<ExternalJointKey, TkJointImpl*>::type::Entry e;
		if (m_jointSets[jointSetIndex]->m_joints.erase(ExternalJointKey(chunkIndex0, chunkIndex1), e))
		{
			// Delete the joint set if it is empty
			if (m_jointSets[jointSetIndex]->m_joints.size() == 0)
			{
				NVBLASTTK_DELETE(m_jointSets[jointSetIndex], JointSet);
				m_jointSets.replaceWithLast(jointSetIndex);
				m_familyIDMap.erase(otherFamilyID);
				if (jointSetIndex < m_jointSets.size())
				{
					m_familyIDMap[m_jointSets[jointSetIndex]->m_familyID] = jointSetIndex;
				}
			}

			// Return value that was stored
			joint = e.second;
			return true;
		}
	}

	return false;
}


TkJointImpl* TkFamilyImpl::findExternalJoint(const TkFamilyImpl* otherFamily, ExternalJointKey key) const
{
	const FamilyIDMap::Entry* jointSetIndexEntry = m_familyIDMap.find(getFamilyID(otherFamily));
	if (jointSetIndexEntry != nullptr)
	{
		const TkHashMap<ExternalJointKey, TkJointImpl*>::type::Entry* e = m_jointSets[jointSetIndexEntry->second]->m_joints.find(key);
		if (e != nullptr)
		{
			return e->second;
		}
	}

	return nullptr;
}

} // namespace Blast
} // namespace Nv
