/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKFRAMEWORKIMPL_H
#define NVBLASTTKFRAMEWORKIMPL_H

#include "NvBlastTkFramework.h"
#include "NvBlastProfilerInternal.h"

#include "NvBlastTkCommon.h"

#include "NvBlastTkArray.h"
#include "NvBlastTkHashMap.h"
#include "NvBlastTkHashSet.h"


//////// Log macros that use the TkFrameworkImpl::log function ////////

#define NVBLASTTK_LOG_ERROR(_msg)	NVBLAST_LOG_ERROR(TkFrameworkImpl::log, _msg)
#define NVBLASTTK_LOG_WARNING(_msg)	NVBLAST_LOG_WARNING(TkFrameworkImpl::log, _msg)
#define NVBLASTTK_LOG_INFO(_msg)	NVBLAST_LOG_INFO(TkFrameworkImpl::log, _msg)
#define NVBLASTTK_LOG_DEBUG(_msg)	NVBLAST_LOG_DEBUG(TkFrameworkImpl::log, _msg)

#define NVBLASTTK_CHECK(_expr, _messageType, _msg, _onFail)																\
	{																													\
		if(!(_expr))																									\
		{																												\
			TkFrameworkImpl::log(_messageType, _msg, __FILE__, __LINE__);												\
			{ _onFail; };																								\
		}																												\
	}																													

#define NVBLASTTK_CHECK_ERROR(_expr, _msg, _onFail)		NVBLASTTK_CHECK(_expr, NvBlastMessage::Error, _msg, _onFail)
#define NVBLASTTK_CHECK_WARNING(_expr, _msg, _onFail)	NVBLASTTK_CHECK(_expr, NvBlastMessage::Warning, _msg, _onFail)
#define NVBLASTTK_CHECK_INFO(_expr, _msg, _onFail)		NVBLASTTK_CHECK(_expr, NvBlastMessage::Info, _msg, _onFail)
#define NVBLASTTK_CHECK_DEBUG(_expr, _msg, _onFail)		NVBLASTTK_CHECK(_expr, NvBlastMessage::Debug, _msg, _onFail)


//////// Allocator macros ////////

#define NVBLASTTK_ALLOC(_size, _name)	TkFrameworkImpl::get()->getAllocatorCallbackInternal().allocate(_size, _name, __FILE__, __LINE__)
#define NVBLASTTK_FREE(_mem)			TkFrameworkImpl::get()->getAllocatorCallbackInternal().deallocate(_mem)

#define NVBLASTTK_NEW(T)	new (NVBLASTTK_ALLOC(sizeof(T), #T)) T
#define NVBLASTTK_DELETE(obj, T)	\
	(obj)->~T();					\
	NVBLASTTK_FREE(obj)





namespace Nv
{
namespace Blast
{

// Forward declarations
class TkTypeImpl;
class TkJointImpl;

/**
Implementation of TkFramework
*/
class TkFrameworkImpl : public TkFramework
{
public:
										TkFrameworkImpl(const TkFrameworkDesc& desc);
										~TkFrameworkImpl();

	// Begin TkFramework
	virtual void						release() override;

	virtual physx::PxErrorCallback&		getErrorCallback() const override;

	virtual physx::PxAllocatorCallback&	getAllocatorCallback() const override;

	virtual NvBlastLog					getLogFn() const override;

	virtual TkSerializable*				deserialize(physx::general_PxIOStream2::PxFileBuf& stream) override;

	virtual const TkType*				getType(TkTypeIndex::Enum typeIndex) const override;

	virtual TkIdentifiable*				findObjectByID(const NvBlastID& id) const override;

	virtual uint32_t					getObjectCount(const TkType& type) const override;

	virtual uint32_t					getObjects(TkIdentifiable** buffer, uint32_t bufferSize, const TkType& type, uint32_t indexStart = 0) const override;

	virtual bool						reorderAssetDescChunks(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount, NvBlastBondDesc* bondDescs, uint32_t bondCount, uint32_t* chunkReorderMap = nullptr) const override;

	virtual bool						ensureAssetExactSupportCoverage(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount) const override;

	virtual TkAsset*					createAsset(const TkAssetDesc& desc) override;

	virtual TkAsset*					createAsset(const NvBlastAsset* assetLL, Nv::Blast::TkAssetJointDesc* jointDescs = nullptr, uint32_t jointDescCount = 0, bool ownsAsset = false) override;

	virtual TkGroup*					createGroup(const TkGroupDesc& desc) override;

	virtual TkActor*					createActor(const TkActorDesc& desc) override;

	virtual TkJoint*					createJoint(const TkJointDesc& desc) override;
	// End TkFramework

	// Public methods
	/**
	Access to the error callback set by the user.
	*/
	physx::PxErrorCallback&				getErrorCallbackInternal() const;

	/**
	Access to the allocator callback set by the user.
	*/
	physx::PxAllocatorCallback&			getAllocatorCallbackInternal() const;

	/**
	To be called by any TkIdentifiable object when it is created, so the framework can track it.
	*/
	void								onCreate(TkIdentifiable& object);

	/**
	To be called by any TkIdentifiable object when it is deleted, so the framework can stop tracking it.
	*/
	void								onDestroy(TkIdentifiable& object);

	/**
	Special onCreate method for joints, since they are not TkIdentifiable.
	*/
	void								onCreate(TkJointImpl& joint);

	/**
	Special onDestroy method for joints, since they are not TkIdentifiable.
	*/
	void								onDestroy(TkJointImpl& joint);

	/**
	Must be called whenever a TkIdentifiable object's ID is changed, so that the framework can associate the new ID with it.
	*/
	void								onIDChange(TkIdentifiable& object, const NvBlastID& IDPrev, const NvBlastID& IDCurr);

	/**
	Internal (non-virtual) method to find a TkIdentifiable object based upon its NvBlastID.
	*/
	TkIdentifiable*						findObjectByIDInternal(const NvBlastID& id) const;

	/**
	Serialize a TkSerializable's standard header data, including its type ID, type version, object ID, and TkObject::userIntData.
	*/
	bool								serializeHeader(const TkSerializable& object, physx::general_PxIOStream2::PxFileBuf& stream);

	// Access to singleton

	/** Retrieve the global singleton. */
	static TkFrameworkImpl*				get();

	/** Set the global singleton, if it's not already set, or set it to NULL.  Returns true iff successful. */
	static bool							set(TkFrameworkImpl* framework);

	// Blast LL context functions
	static	void						log(int type, const char* msg, const char* file, int line);	//!< A function with the NvBlastLog signature which can be used in NvBlast low-level function calls
	static	void*						alloc(size_t size);											//!< A function with the std::malloc signature which returns 16-byte aligned memory
	static	void						free(void* mem);											//!< A function with the std::free signature which can deallocate memory created by alloc

private:
	// Enums
	enum { Alignment = 16 };									//!< Memory alignment used for allocations
	enum { ClassID = NVBLASTTK_FOURCC('T', 'K', 'F', 'W') };	//!< TkFramework identifier token, used in serialization

	// Static data
	static TkFrameworkImpl*														s_framework;			//!< Global (singleton) object pointer

	// Callbacks
	physx::PxErrorCallback*														m_errorCallback;		//!< User-supplied error callback
	physx::PxAllocatorCallback*													m_allocatorCallback;	//!< User-supplied allocator callback

	// Types
	TkInlineArray<const TkTypeImpl*, TkTypeIndex::TypeCount>::type				m_types;				//!< TkIdentifiable static type data
	TkHashMap<uint32_t, uint32_t>::type											m_typeIDToIndex;		//!< Map to type data keyed by ClassID

	// Objects and object names
	TkHashMap<NvBlastID, TkIdentifiable*>::type									m_IDToObject;			//!< Map to all TkIdentifiable objects, keyed by NvBlastID
	TkInlineArray<TkArray<TkIdentifiable*>::type, TkTypeIndex::TypeCount>::type	m_objects;				//!< Catalog of all TkIdentifiable objects, grouped by type.  (Revisit implementation.)

	// Track external joints (to do: make this a pool)
	TkHashSet<TkJointImpl*>::type												m_joints;				//!< All internal joints
};


//////// TkFrameworkImpl inline methods ////////

NV_INLINE physx::PxErrorCallback& TkFrameworkImpl::getErrorCallbackInternal() const
{
	return *m_errorCallback;
}


NV_INLINE physx::PxAllocatorCallback& TkFrameworkImpl::getAllocatorCallbackInternal() const
{
	return *m_allocatorCallback;
}


NV_INLINE TkIdentifiable* TkFrameworkImpl::findObjectByIDInternal(const NvBlastID& id) const
{
	const auto entry = m_IDToObject.find(id);
	if (entry == nullptr)
	{
		return nullptr;
	}

	return entry->second;
}

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKFRAMEWORKIMPL_H
