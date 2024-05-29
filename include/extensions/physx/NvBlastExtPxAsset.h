// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2016-2024 NVIDIA Corporation. All rights reserved.

//! @file
//!
//! @brief Defines the API for the NvBlastExtPxAsset class

#ifndef NVBLASTEXTPXASSET_H
#define NVBLASTEXTPXASSET_H

#include "NvBlastTkFramework.h"
#include "PxConvexMeshGeometry.h"
#include "foundation/PxTransform.h"
#include "NvPreprocessor.h"


// Forward declarations
namespace physx
{
class PxCooking;

namespace general_PxIOStream2
{
class PxFileBuf;
}
}

class NvBlastExtDamageAccelerator;


namespace Nv
{
namespace Blast
{


/**
Descriptor for PxAsset creation.

PxAsset creates TkAsset internally, so TkAssetDesc must be filled.
In addition it needs physics chunks data. Every chunk can have any amount of Convexes (Subchunks).
*/
struct ExtPxAssetDesc : public TkAssetDesc
{
    /**
    Physics Subchunk.

    Represents convex and it's position.
    */
    struct SubchunkDesc
    {
        physx::PxTransform          transform;          //!<    convex local transform
        physx::PxConvexMeshGeometry geometry;           //!<    convex geometry
    };

    /**
    Physics Chunk.

    Contains any amount of subchunks. Empty subchunks array makes chunk invisible.
    */
    struct ChunkDesc
    {
        SubchunkDesc*   subchunks;          //!< array of subchunks for chunk, can be empty
        uint32_t        subchunkCount;      //!< size array of subchunks for chunk, can be 0
        bool            isStatic;           //!< is chunk static. Static chunk makes PxActor Kinematic.
    };

    ChunkDesc*  pxChunks;       //!< array of chunks in asset, should be of size chunkCount (@see NvBlastAssetDesc)
};


/**
Physics Subchunk. 

Represents convex and it's local position.
*/
struct ExtPxSubchunk
{
    physx::PxTransform          transform;          //!<    convex local transform
    physx::PxConvexMeshGeometry geometry;           //!<    convex geometry
};


/**
Physics Chunk.

Contains any amount of subchunks.
*/
struct ExtPxChunk
{
    uint32_t    firstSubchunkIndex; //!<    first Subchunk index in Subchunk's array in ExtPhyicsAsset
    uint32_t    subchunkCount;      //!<    Subchunk count. Can be 0.
    bool        isStatic;           //!<    is chunk static (kinematic)?.
};


/**
Asset.

Keeps all the static data needed for physics.
*/
class NV_DLL_EXPORT ExtPxAsset
{
public:

    /**
    Create a new ExtPxAsset.

    \param[in]  desc            The ExtPxAssetDesc descriptor to be used, @see ExtPxAssetDesc.
    \param[in]  framework       The TkFramework instance to be used to create TkAsset.

    \return the new ExtPxAsset if successful, NULL otherwise.
    */
    static ExtPxAsset*              create(const ExtPxAssetDesc& desc, TkFramework& framework);

    /**
    Create a new ExtPxAsset.

    \param[in]  desc            The ExtPxAssetDesc descriptor to be used, @see ExtPxAssetDesc.
    \param[in]  framework       The TkFramework instance to be used to create TkAsset.

    \return the new ExtPxAsset if successful, NULL otherwise.
    */
    static ExtPxAsset*              create(const TkAssetDesc& desc, ExtPxChunk* pxChunks, ExtPxSubchunk* pxSubchunks, TkFramework& framework);


    /*
        Factory method for deserialization

        Doesn't specify chunks or subchunks as they'll be fed in during deserialization to avoid copying stuff around.
    
    */
    static ExtPxAsset*              create(TkAsset* asset);

    /*
    Create a new ExtPxAsset.

    \param[in]  asset           TkAsset from which ExtPxAsset will be created
    \param[in]  chunks          Array of physics chunks descriptors
    \param[in]  chunkCount      Size of chunks descriptors array


    \return the new ExtPxAsset if successful, NULL otherwise.
        
    */
    static ExtPxAsset*              create(TkAsset* asset, ExtPxAssetDesc::ChunkDesc* chunks, uint32_t chunkCount);

    /**
    Release this ExtPxAsset.
    */
    virtual void                    release() = 0;

    /**
    Every ExtPxAsset has corresponding TkAsset.

    /return a pointer to TkAsset actor.
    */
    virtual const TkAsset&          getTkAsset() const = 0;

    /**
    Get the number of chunks for this asset.  May be used in conjunction with getChunks().

    \return the number of chunks for the asset.
    */
    virtual uint32_t                getChunkCount() const = 0;

    /**
    Access asset's array of chunks. Use getChunkCount() to get the size of this array.

    \return a pointer to an array of chunk of an asset.
    */
    virtual const ExtPxChunk*       getChunks() const = 0;
    
    /**
    Get the number of subchunks for this asset.  May be used in conjunction with getSubchunks().
    Subchunk count is the maximum value of ExtPxChunk: (firstSubchunkIndex + subchunkCount).

    \return the number of subchunks for the asset.
    */
    virtual uint32_t                getSubchunkCount() const = 0;
    
    /**
    Access asset's array of subchunks. Use getSubchunkCount() to get the size of this array.

    \return a pointer to an array of subchunks of an asset.
    */
    virtual const ExtPxSubchunk*    getSubchunks() const = 0;

    /**
    Get the default NvBlastActorDesc to be used when creating family from this asset. It is called 'default', 
    because it can be overwritten in ExtPxManager::createFamily(...) function.

    Initially default NvBlastActorDesc contains only uniform health values, and 'nullptr' is set in arrays of health.
    Call setUniformHealth(false) in order to set health per bond/chunk. You can then access directly values stored in NvBlastActorDesc,
    change them and they will be serialized/deserialized as withing asset itself.

    NOTE: do not change actual pointers in NvBlastActorDesc: initialBondHealths and initialSupportChunkHealths. You can change actual values
    in those arrays or if they are 'nullptr' call setUniformHealth(false) before. Or call setUniformHealth(true) to make them 'nullptr'.

    \return the default NvBlastActorDesc.
    */
    virtual NvBlastActorDesc&       getDefaultActorDesc() = 0;

    virtual const NvBlastActorDesc& getDefaultActorDesc() const = 0;

    /**
    Set if uniform health values should be used in NvBlastActorDesc or per bond/chunk ones. @see getDefaultActorDesc.
    */
    virtual void                    setUniformHealth(bool enabled) = 0;

    /**
    Set damage accelerator associated with this asset.
    */
    virtual void                    setAccelerator(NvBlastExtDamageAccelerator* accelerator) = 0;

    /**
    Set damage accelerator associated with this asset.
    */
    virtual NvBlastExtDamageAccelerator* getAccelerator() const = 0;

    /**
    Pointer field available to the user.
    */
    void*   userData;
};



} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXASSET_H
