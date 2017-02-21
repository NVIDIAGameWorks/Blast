/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKASSETIMPL_H
#define NVBLASTTKASSETIMPL_H


#include "NvBlastTkCommon.h"
#include "NvBlastTkJoint.h"
#include "NvBlastTkAsset.h"
#include "NvBlastTkTypeImpl.h"
#include "NvBlastTkArray.h"


// Forward declarations
struct NvBlastAsset;


namespace Nv
{
namespace Blast
{

/**
Implementation of TkAsset
*/
NVBLASTTK_IMPL_DECLARE(Asset)
{
public:
	/**
	Enum which keeps track of the serialized data format.
	*/
	enum Version
	{
		/** Initial version */
		Initial,

		//	New formats must come before Count.  They should be given descriptive names with more information in comments.

		/** The number of serialized formats. */
		Count,

		/** The current version.  This should always be Count-1 */
		Current = Count - 1
	};

	TkAssetImpl();
	TkAssetImpl(const NvBlastID& id);
	~TkAssetImpl();

	NVBLASTTK_IMPL_DEFINE_SERIALIZABLE('A', 'S', 'S', 'T');

	// Public methods

	/**
	Factory create method.  This method creates a low-level asset and stores a reference to it.

	\param[in]	desc	Asset descriptor set by the user.

	\return a pointer to a new TkAssetImpl object if successful, NULL otherwise.
	*/
	static TkAssetImpl*					create(const TkAssetDesc& desc);
	
	/**
	Static method to create an asset from an existing low-level asset.

	\param[in]	assetLL			A valid low-level asset passed in by the user.
	\param[in]	jointDescs		Optional joint descriptors to add to the new asset.
	\param[in]	jointDescCount	The number of joint descriptors in the jointDescs array.  If non-zero, jointDescs cannot be NULL.
	\param[in]	ownsAsset		Whether or not to let this TkAssetImpl object release the low-level NvBlastAsset memory upon its own release.

	\return a pointer to a new TkAssetImpl object if successful, NULL otherwise.
	*/
	static TkAssetImpl*					create(const NvBlastAsset* assetLL, Nv::Blast::TkAssetJointDesc* jointDescs = nullptr, uint32_t jointDescCount = 0, bool ownsAsset = false);

	/**
	\return a pointer to the underlying low-level NvBlastAsset associated with this asset.
	*/
	const NvBlastAsset*					getAssetLLInternal() const;

	/**
	\return the number of internal joint descriptors stored with this asset.
	*/
	uint32_t							getJointDescCountInternal() const;

	/**
	\return the array of internal joint descriptors stored with this asset, with size given by getJointDescCountInternal().
	*/
	const TkAssetJointDesc*				getJointDescsInternal() const;

	// Begin TkAsset
	virtual const NvBlastAsset*			getAssetLL() const override;

	virtual uint32_t					getChunkCount() const override;

	virtual uint32_t					getLeafChunkCount() const override;

	virtual uint32_t					getBondCount() const override;

	virtual const NvBlastChunk*			getChunks() const override;

	virtual const NvBlastBond*			getBonds() const override;

	virtual const NvBlastSupportGraph	getGraph() const override;

	virtual uint32_t					getDataSize() const override;

	virtual uint32_t					getJointDescCount() const override;

	virtual const TkAssetJointDesc*		getJointDescs() const override;
	// End TkAsset

private:
	/**
	Utility to add a joint descriptor between the indexed chunks.  The two chunks
	must be support chunks, and there must exist a bond between them.  The joint's
	attachment positions will be the bond centroid.

	\param[in]	chunkIndex0	The first chunk index.
	\param[in]	chunkIndex1	The second chunk index.

	\return true iff successful.
	*/
	bool								addJointDesc(uint32_t chunkIndex0, uint32_t chunkIndex1);

	NvBlastAsset*					m_assetLL;		//!< The underlying low-level asset.
	TkArray<TkAssetJointDesc>::type	m_jointDescs;	//!< The array of internal joint descriptors.
	bool							m_ownsAsset;	//!< Whether or not this asset should release its low-level asset upon its own release.
};


//////// TkAssetImpl inline methods ////////

NV_INLINE const NvBlastAsset* TkAssetImpl::getAssetLLInternal() const
{
	return m_assetLL;
}


NV_INLINE uint32_t TkAssetImpl::getJointDescCountInternal() const
{
	return m_jointDescs.size();
}


NV_INLINE const TkAssetJointDesc* TkAssetImpl::getJointDescsInternal() const
{
	return m_jointDescs.begin();
}

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKASSETIMPL_H
