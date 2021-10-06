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


#ifndef NVBLASTTKFRAMEWORKIMPL_H
#define NVBLASTTKFRAMEWORKIMPL_H

#include "NvBlastTkFramework.h"
#include "NvBlastProfilerInternal.h"

#include "NvBlastTkCommon.h"

#include "NvBlastArray.h"
#include "NvBlastHashMap.h"
#include "NvBlastHashSet.h"


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
										TkFrameworkImpl();
										~TkFrameworkImpl();

	// Begin TkFramework
	virtual void						release() override;

	virtual const TkType*				getType(TkTypeIndex::Enum typeIndex) const override;

	virtual TkIdentifiable*				findObjectByID(const NvBlastID& id) const override;

	virtual uint32_t					getObjectCount(const TkType& type) const override;

	virtual uint32_t					getObjects(TkIdentifiable** buffer, uint32_t bufferSize, const TkType& type, uint32_t indexStart = 0) const override;

	virtual bool						reorderAssetDescChunks(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount, NvBlastBondDesc* bondDescs, uint32_t bondCount, uint32_t* chunkReorderMap = nullptr, bool keepBondNormalChunkOrder = false) const override;

	virtual bool						ensureAssetExactSupportCoverage(NvBlastChunkDesc* chunkDescs, uint32_t chunkCount) const override;

	virtual TkAsset*					createAsset(const TkAssetDesc& desc) override;

	virtual TkAsset*					createAsset(const NvBlastAsset* assetLL, Nv::Blast::TkAssetJointDesc* jointDescs = nullptr, uint32_t jointDescCount = 0, bool ownsAsset = false) override;

	virtual TkGroup*					createGroup(const TkGroupDesc& desc) override;

	virtual TkActor*					createActor(const TkActorDesc& desc) override;

	virtual TkJoint*					createJoint(const TkJointDesc& desc) override;
	// End TkFramework

	// Public methods
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

	// Access to singleton

	/** Retrieve the global singleton. */
	static TkFrameworkImpl*				get();

	/** Set the global singleton, if it's not already set, or set it to NULL.  Returns true iff successful. */
	static bool							set(TkFrameworkImpl* framework);

private:
	// Enums
	enum { ClassID = NVBLAST_FOURCC('T', 'K', 'F', 'W') };	//!< TkFramework identifier token, used in serialization

	// Static data
	static TkFrameworkImpl*														s_framework;			//!< Global (singleton) object pointer

	// Types
	InlineArray<const TkTypeImpl*, TkTypeIndex::TypeCount>::type				m_types;				//!< TkIdentifiable static type data
	HashMap<uint32_t, uint32_t>::type											m_typeIDToIndex;		//!< Map to type data keyed by ClassID

	// Objects and object names
	HashMap<NvBlastID, TkIdentifiable*>::type									m_IDToObject;			//!< Map to all TkIdentifiable objects, keyed by NvBlastID
	InlineArray<Array<TkIdentifiable*>::type, TkTypeIndex::TypeCount>::type		m_objects;				//!< Catalog of all TkIdentifiable objects, grouped by type.  (Revisit implementation.)

	// Track external joints (to do: make this a pool)
	HashSet<TkJointImpl*>::type													m_joints;				//!< All internal joints
};


//////// TkFrameworkImpl inline methods ////////

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
