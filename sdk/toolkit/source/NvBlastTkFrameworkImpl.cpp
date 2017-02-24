/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastAssert.h"

#include "NvBlastTkFrameworkImpl.h"
#include "NvBlastTkAssetImpl.h"
#include "NvBlastTkFamilyImpl.h"
#include "NvBlastTkGroupImpl.h"
#include "NvBlastTkActorImpl.h"
#include "NvBlastTkJointImpl.h"
#include "NvBlastTkTypeImpl.h"

#include "PxAllocatorCallback.h"
#include "PxErrorCallback.h"
#include "PxFileBuf.h"

#include <algorithm>


using namespace physx;
using namespace physx::shdfnd;
using namespace physx::general_PxIOStream2;


NV_INLINE bool operator < (const NvBlastID& id1, const NvBlastID& id2)
{
	return memcmp(&id1, &id2, sizeof(NvBlastID)) < 0;
}


namespace Nv
{
namespace Blast
{

//////// Local definitions ////////

// Map type ID to static type data
#define NVBLASTTK_REGISTER_TYPE(_name)										\
	if (!Tk##_name##Impl::s_type.indexIsValid())							\
	{																		\
		Tk##_name##Impl::s_type.setIndex(TkTypeIndex::_name);				\
	}																		\
	m_types[TkTypeIndex::_name] = &Tk##_name##Impl::s_type;					\
	m_typeIDToIndex[Tk##_name##Impl::s_type.getID()] = TkTypeIndex::_name


#define NVBLASTTK_RELEASE_TYPE(_name)										\
	{ 																		\
		TkTypeImpl& type = Tk##_name##Impl::s_type;							\
		auto& toRelease = m_objects[type.getIndex()];						\
		for (TkObject* obj : toRelease) 									\
		{ 																	\
			obj->release();													\
		}																	\
	}


//////// TkFrameworkImpl static variables ////////

TkFrameworkImpl* TkFrameworkImpl::s_framework = nullptr;


//////// TkFrameworkImpl static function ////////

TkFrameworkImpl* TkFrameworkImpl::get()
{
	return s_framework;
}


bool TkFrameworkImpl::set(TkFrameworkImpl* framework)
{
	if (s_framework != nullptr)
	{
		if (framework != nullptr)
		{
			NVBLASTTK_LOG_ERROR("TkFrameworkImpl::set: framework already set.  Pass NULL to this function to destroy framework.");
			return false;
		}

		PxAllocatorCallback& allocator = s_framework->getAllocatorCallbackInternal();
		s_framework->~TkFrameworkImpl();
		allocator.deallocate(s_framework);
	}

	s_framework = framework;

	return true;
}


void TkFrameworkImpl::log(int type, const char* msg, const char* file, int line)
{
	if (s_framework == nullptr)
	{
		return;
	}

	PxErrorCode::Enum pxErrorCode = PxErrorCode::eNO_ERROR;
	switch (type)
	{
	case NvBlastMessage::Error:		pxErrorCode = PxErrorCode::eINVALID_OPERATION;	break;
	case NvBlastMessage::Warning:	pxErrorCode = PxErrorCode::eDEBUG_WARNING;		break;
	case NvBlastMessage::Info:		pxErrorCode = PxErrorCode::eDEBUG_INFO;			break;
	case NvBlastMessage::Debug:		pxErrorCode = PxErrorCode::eNO_ERROR;			break;
	}

	s_framework->getErrorCallback().reportError(pxErrorCode, msg, file, line);
}


void* TkFrameworkImpl::alloc(size_t size)
{
	if (s_framework == nullptr)
	{
		return nullptr;
	}

	NV_COMPILE_TIME_ASSERT(Alignment > 0 && Alignment <= 256);

	unsigned char* mem = reinterpret_cast<unsigned char*>(s_framework->m_allocatorCallback->allocate(size + (size_t)Alignment, "NvBlast", __FILE__, __LINE__));

	const unsigned char offset = (unsigned char)((uintptr_t)Alignment - (uintptr_t)mem % (size_t)Alignment - 1);
	mem += offset;
	*mem++ = offset;

	return mem;
}


void TkFrameworkImpl::free(void* mem)
{
	if (s_framework == nullptr)
	{
		return;
	}

	unsigned char* ptr = reinterpret_cast<unsigned char*>(mem);
	const unsigned char offset = *--ptr;

	return s_framework->m_allocatorCallback->deallocate(ptr - offset);
}


//////// TkFrameworkImpl methods ////////

TkFrameworkImpl::TkFrameworkImpl(const TkFrameworkDesc& desc)
	: TkFramework()
	, m_errorCallback(desc.errorCallback)
	, m_allocatorCallback(desc.allocatorCallback)
{
	// Static create() function should ensure these pointers are not NULL
	NVBLAST_ASSERT(m_errorCallback != nullptr);
	NVBLAST_ASSERT(m_allocatorCallback != nullptr);

	// Register types
	m_types.resize(TkTypeIndex::TypeCount);
	m_objects.resize(TkTypeIndex::TypeCount);
	NVBLASTTK_REGISTER_TYPE(Asset);
	NVBLASTTK_REGISTER_TYPE(Family);
	NVBLASTTK_REGISTER_TYPE(Group);
}


TkFrameworkImpl::~TkFrameworkImpl()
{
}


void TkFrameworkImpl::release()
{
	// Special release of joints, which are not TkIdentifiable:
	TkArray<TkJointImpl*>::type joints;	// Since the EraseIterator is not exposed
	joints.reserve(m_joints.size());
	for (auto j = m_joints.getIterator(); !j.done(); ++j)
	{
		joints.pushBack(*j);
	}
	for (uint32_t i = 0; i < joints.size(); ++i)
	{
		joints[i]->release();
	}
	NVBLAST_ASSERT(m_joints.size() == 0);
	joints.reset();	// Since we will be deleting the allocator

	NVBLASTTK_RELEASE_TYPE(Group);
	NVBLASTTK_RELEASE_TYPE(Asset);
	set(nullptr);
	Nv::Blast::TkAllocator::s_allocatorCallback = nullptr;
}


physx::PxErrorCallback& TkFrameworkImpl::getErrorCallback() const
{
	return getErrorCallbackInternal();
}


physx::PxAllocatorCallback& TkFrameworkImpl::getAllocatorCallback() const
{
	return getAllocatorCallbackInternal();
}


NvBlastLog TkFrameworkImpl::getLogFn() const
{
	return TkFrameworkImpl::log;
}


TkSerializable* TkFrameworkImpl::deserialize(PxFileBuf& stream)
{
	// Read framework ID
	if (stream.readDword() != ClassID)
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::deserialize: stream does not contain a BlastTk object.");
		return nullptr;
	}

	// Read object class ID and get class type data
	const auto it = m_typeIDToIndex.find(stream.readDword());
	if (it == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::deserialize: BlastTk object type unrecognized.");
		return nullptr;
	}

	const uint32_t index = (*it).second;
	NVBLAST_ASSERT(index < m_types.size());

	const TkTypeImpl* type = m_types[index];

	// Read object class version and ensure it's current
	if (stream.readDword() != type->getVersionInternal())
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::deserialize: BlastTk object version does not equal the current version for the loaded type.");
		return nullptr;
	}

	// Object ID
	NvBlastID id;
	stream.read(&id, sizeof(NvBlastID));

	// Serializable user data
	const uint32_t lsq = stream.readDword();
	const uint32_t msq = stream.readDword();

	// All checks out, deserialize
	TkSerializable* object = type->getDeserializeFn()(stream, id);

	// Set serializable user data if deserialization was successful
	if (object != nullptr)
	{
		object->userIntData = static_cast<uint64_t>(msq) << 32 | static_cast<uint64_t>(lsq);
	}

	return object;
}


const TkType* TkFrameworkImpl::getType(TkTypeIndex::Enum typeIndex) const
{
	if (typeIndex < 0 || typeIndex >= TkTypeIndex::TypeCount)
	{
		NVBLASTTK_LOG_WARNING("TkFrameworkImpl::getType: invalid typeIndex.");
		return nullptr;
	}

	return m_types[typeIndex];
}


TkIdentifiable* TkFrameworkImpl::findObjectByID(const NvBlastID& id) const
{
	TkIdentifiable* object = findObjectByIDInternal(id);

	if (object == nullptr)
	{
		NVBLASTTK_LOG_WARNING("TkFrameworkImpl::findObjectByID: object not found.");
	}

	return object;
}


uint32_t TkFrameworkImpl::getObjectCount(const TkType& type) const
{
	const uint32_t index = static_cast<const TkTypeImpl&>(type).getIndex();

	if (index >= m_objects.size())
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::getObjectCount: BlastTk object type unrecognized.");
		return 0;

	}

	return m_objects[index].size();
}


uint32_t TkFrameworkImpl::getObjects(TkIdentifiable** buffer, uint32_t bufferSize, const TkType& type, uint32_t indexStart /* = 0 */) const
{
	const uint32_t index = static_cast<const TkTypeImpl&>(type).getIndex();

	if (index >= m_objects.size())
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::getObjectCount: BlastTk object type unrecognized.");
		return 0;
	}

	const auto& objectArray = m_objects[index];

	uint32_t objectCount = objectArray.size();
	if (objectCount <= indexStart)
	{
		NVBLASTTK_LOG_WARNING("TkFrameworkImpl::getObjects: indexStart beyond end of object list.");
		return 0;
	}

	objectCount -= indexStart;
	if (objectCount > bufferSize)
	{
		objectCount = bufferSize;
	}

	memcpy(buffer, objectArray.begin() + indexStart, objectCount * sizeof(TkObject*));

	return objectCount;
}


bool TkFrameworkImpl::reorderAssetDescChunks(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount, NvBlastBondDesc* bondDescs, uint32_t bondCount, uint32_t* chunkReorderMap /*= nullptr*/) const
{
	uint32_t* map = chunkReorderMap != nullptr ? chunkReorderMap : static_cast<uint32_t*>(NVBLASTTK_ALLOC(chunkCount * sizeof(uint32_t), "reorderAssetDescChunks:chunkReorderMap"));
	void* scratch = NVBLASTTK_ALLOC(chunkCount * sizeof(NvBlastChunkDesc), "reorderAssetDescChunks:scratch");
	const bool result = NvBlastReorderAssetDescChunks(chunkDescs, chunkCount, bondDescs, bondCount, map, scratch, log);
	NVBLASTTK_FREE(scratch);
	if (chunkReorderMap == nullptr)
	{
		NVBLASTTK_FREE(map);
	}
	return result;
}


bool TkFrameworkImpl::ensureAssetExactSupportCoverage(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount) const
{
	void* scratch = NVBLASTTK_ALLOC(chunkCount, "ensureAssetExactSupportCoverage:scratch");
	const bool result = NvBlastEnsureAssetExactSupportCoverage(chunkDescs, chunkCount, scratch, log);
	NVBLASTTK_FREE(scratch);
	return result;
}


TkAsset* TkFrameworkImpl::createAsset(const TkAssetDesc& desc)
{
	TkAssetImpl* asset = TkAssetImpl::create(desc);
	if (asset == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::createAsset: failed to create asset.");
	}

	return asset;
}


TkAsset* TkFrameworkImpl::createAsset(const NvBlastAsset* assetLL, Nv::Blast::TkAssetJointDesc* jointDescs, uint32_t jointDescCount, bool ownsAsset)
{
	TkAssetImpl* asset = TkAssetImpl::create(assetLL, jointDescs, jointDescCount, ownsAsset);
	if (asset == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::createAsset: failed to create asset.");
	}

	return asset;
}


TkGroup* TkFrameworkImpl::createGroup(const TkGroupDesc& desc)
{
	TkGroupImpl* group = TkGroupImpl::create(desc);
	if (group == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::createGroup: failed to create group.");
	}

	return group;
}


TkActor* TkFrameworkImpl::createActor(const TkActorDesc& desc)
{
	TkActor* actor = TkActorImpl::create(desc);
	if (actor == nullptr)
	{
		NVBLASTTK_LOG_ERROR("TkFrameworkImpl::createActor: failed to create actor.");
	}

	return actor;
}


TkJoint* TkFrameworkImpl::createJoint(const TkJointDesc& desc)
{
	TkJointImpl** handle0 = nullptr;
	TkJointImpl** handle1 = nullptr;

	TkFamilyImpl* family0 = static_cast<TkFamilyImpl*>(desc.families[0]);
	TkFamilyImpl* family1 = static_cast<TkFamilyImpl*>(desc.families[1]);

	NVBLASTTK_CHECK_ERROR(family0 != nullptr || family1 != nullptr, "TkFrameworkImpl::createJoint: at least one family in the TkJointDesc must be valid.", return nullptr);

	NVBLASTTK_CHECK_ERROR(family0 == nullptr || desc.chunkIndices[0] < family0->getAssetImpl()->getChunkCount(), "TkFrameworkImpl::createJoint: desc.chunkIndices[0] is invalid.", return nullptr);
	NVBLASTTK_CHECK_ERROR(family1 == nullptr || desc.chunkIndices[1] < family1->getAssetImpl()->getChunkCount(), "TkFrameworkImpl::createJoint: desc.chunkIndices[1] is invalid.", return nullptr);

	const bool actorsAreTheSame = family0 == family1 && family0->getActorByChunk(desc.chunkIndices[0]) == family1->getActorByChunk(desc.chunkIndices[1]);
	NVBLASTTK_CHECK_ERROR(!actorsAreTheSame, "TkFrameworkImpl::createJoint: the chunks listed in the TkJointDesc must be in different actors.", return nullptr);

	if (family0 != nullptr)
	{
		const bool isSupportChunk = !isInvalidIndex(NvBlastAssetGetChunkToGraphNodeMap(family0->getAssetImpl()->getAssetLLInternal(), log)[desc.chunkIndices[0]]);
		NVBLASTTK_CHECK_ERROR(isSupportChunk, "TkFrameworkImpl::createJoint: desc.chunkIndices[0] is not a support chunk in the asset for desc.families[0].  Joint not created.", return nullptr);
		handle0 = family0->createExternalJointHandle(getFamilyID(family1), desc.chunkIndices[0], desc.chunkIndices[1]);
		NVBLASTTK_CHECK_ERROR(handle0 != nullptr, "TkFrameworkImpl::createJoint: could not create joint handle in family[0].  Joint not created.", return nullptr);
	}

	if (family1 != nullptr)
	{
		const bool isSupportChunk = !isInvalidIndex(NvBlastAssetGetChunkToGraphNodeMap(family1->getAssetImpl()->getAssetLLInternal(), log)[desc.chunkIndices[1]]);
		NVBLASTTK_CHECK_ERROR(isSupportChunk, "TkFrameworkImpl::createJoint: desc.chunkIndices[1] is not a support chunk in the asset for desc.families[1].  Joint not created.", return nullptr);
		if (family1 != family0)
		{
			handle1 = family1->createExternalJointHandle(getFamilyID(family0), desc.chunkIndices[1], desc.chunkIndices[0]);
			NVBLASTTK_CHECK_ERROR(handle1 != nullptr, "TkFrameworkImpl::createJoint: could not create joint handle in family[1].  Joint not created.", return nullptr);
		}
	}

	TkJointImpl* joint = NVBLASTTK_NEW(TkJointImpl)(desc, nullptr);
	NVBLASTTK_CHECK_ERROR(joint != nullptr, "TkFrameworkImpl::createJoint: failed to create joint.", return nullptr);

	const TkJointData& jointData = joint->getDataInternal();

	if (handle0 != nullptr)
	{
		*handle0 = joint;
		static_cast<TkActorImpl*>(jointData.actors[0])->addJoint(joint->m_links[0]);
	}

	if (handle1 != nullptr)
	{
		*handle1 = joint;
		if (jointData.actors[0] != jointData.actors[1])
		{
			static_cast<TkActorImpl*>(jointData.actors[1])->addJoint(joint->m_links[1]);
		}
	}

	return joint;
}


bool TkFrameworkImpl::serializeHeader(const TkSerializable& object, PxFileBuf& stream)
{
	const TkTypeImpl& type = static_cast<const TkTypeImpl&>(object.getType());

	// Tk framework identifier
	stream.storeDword(ClassID);

	// Object header
	stream.storeDword(type.getID());
	stream.storeDword(type.getVersionInternal());

	// Object ID
	stream.write(&object.getID(), sizeof(NvBlastID));

	// Serializable user data
	stream.storeDword(static_cast<uint32_t>(object.userIntData & 0xFFFFFFFF));
	stream.storeDword(static_cast<uint32_t>(object.userIntData >> 32));

	return true;
}


void TkFrameworkImpl::onCreate(TkIdentifiable& object)
{
	const TkTypeImpl& type = static_cast<const TkTypeImpl&>(object.getType());

	const uint32_t index = type.getIndex();

	if (index >= m_objects.size())
	{
		if (!isInvalidIndex(index))
		{
			NVBLASTTK_LOG_ERROR("TkFrameworkImpl::addObject: object type unrecognized.");
		}
		return;
	}

	auto& objectArray = m_objects[index];
	NVBLAST_ASSERT(objectArray.find(&object) == objectArray.end());
	objectArray.pushBack(&object);
}


void TkFrameworkImpl::onDestroy(TkIdentifiable& object)
{
	// remove from id map if present
	const auto id = object.getID();
	if (!TkGUIDIsZero(&id))
	{
		m_IDToObject.erase(id);
	}

	// remove from object list
	const TkTypeImpl& type = static_cast<const TkTypeImpl&>(object.getType());

	const uint32_t index = type.getIndex();

	if (index >= m_objects.size())
	{
		if (!isInvalidIndex(index))
		{
			NVBLASTTK_LOG_ERROR("TkFrameworkImpl::removeObject: object type unrecognized.");
		}
		return;
	}

	auto& objectArray = m_objects[index];
	objectArray.findAndReplaceWithLast(&object);
}


void TkFrameworkImpl::onCreate(TkJointImpl& joint)
{
	NVBLASTTK_CHECK_ERROR(m_joints.insert(&joint), "TkFrameworkImpl::onCreate: Joint already tracked.", return);
}


void TkFrameworkImpl::onDestroy(TkJointImpl& joint)
{
	NVBLASTTK_CHECK_ERROR(m_joints.erase(&joint), "TkFrameworkImpl::onDestroy: Joint not tracked.", return);
}


void TkFrameworkImpl::onIDChange(TkIdentifiable& object, const NvBlastID& IDPrev, const NvBlastID& IDCurr)
{
	if (!TkGUIDIsZero(&IDPrev))
	{
		if (!m_IDToObject.erase(IDPrev))
		{
			NVBLASTTK_LOG_ERROR("TkFrameworkImpl::reportIDChanged: object with previous ID doesn't exist.");
		}
	}

	if (!TkGUIDIsZero(&IDCurr))
	{
		auto& value = m_IDToObject[IDCurr];
		if (value != nullptr)
		{
			NVBLASTTK_LOG_ERROR("TkFrameworkImpl::reportIDChanged: object with new ID already exists.");
			return;
		}
		value = &object;
	}
}

} // namespace Blast
} // namespace Nv


//////// Global API implementation ////////

Nv::Blast::TkFramework* NvBlastTkFrameworkCreate(const Nv::Blast::TkFrameworkDesc& desc)
{
	if (desc.errorCallback == nullptr)
	{
		return nullptr;
	}

	if (desc.allocatorCallback == nullptr)
	{
		desc.errorCallback->reportError(PxErrorCode::eINVALID_OPERATION, "TkFramework::create: NULL allocator given in descriptor.", __FILE__, __LINE__);
		return nullptr;
	}

	if (Nv::Blast::TkFrameworkImpl::get() != nullptr)
	{
		desc.errorCallback->reportError(PxErrorCode::eINVALID_OPERATION, "TkFramework::create: framework already created.  Use TkFramework::get() to access.", __FILE__, __LINE__);
		return nullptr;
	}

	Nv::Blast::TkAllocator::s_allocatorCallback = desc.allocatorCallback;

	Nv::Blast::TkFrameworkImpl* framework = new (desc.allocatorCallback->allocate(sizeof(Nv::Blast::TkFrameworkImpl), "TkFrameworkImpl", __FILE__, __LINE__)) Nv::Blast::TkFrameworkImpl(desc);
	Nv::Blast::TkFrameworkImpl::set(framework);

	return Nv::Blast::TkFrameworkImpl::get();
}


Nv::Blast::TkFramework* NvBlastTkFrameworkGet()
{
	return Nv::Blast::TkFrameworkImpl::get();
}
