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
// Copyright (c) 2016-2023 NVIDIA Corporation. All rights reserved.

//! @file
//!
//! @brief Defines the API for the NvBlastExtPxFamily class

#ifndef NVBLASTEXTPXFAMILY_H
#define NVBLASTEXTPXFAMILY_H

#include "PxFiltering.h"


// Forward declarations
namespace physx
{
class PxRigidDynamic;
class PxMaterial;
class PxScene;
class PxTransform;
}

struct NvBlastExtMaterial;


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxActor;
class ExtPxAsset;
class ExtPxListener;
class TkFamily;


/**
PxShape Desc.

Used to set settings for newly created PxShapes.

@see PxShape
*/
struct ExtPxShapeDescTemplate
{
    uint8_t             flags;                  //!<    PxShapeFlags flags
    physx::PxFilterData simulationFilterData;   //!<    user definable collision filter data
    physx::PxFilterData queryFilterData;        //!<    user definable query filter data.
    float               contactOffset;          //!<    contact offset
    float               restOffset;             //!<    rest offset
};


/**
PxActor Desc.

Used to set settings for newly created PxActors.
*/
struct ExtPxActorDescTemplate
{
    uint8_t             flags;                      //!<    actor flags
};


/**
Physics Spawn Settings.

This Struct unifies setting to be used when PhysX actors are created.
*/
struct ExtPxSpawnSettings
{
    physx::PxScene*     scene;          //!< PxScene for PxActors to be spawned
    physx::PxMaterial*  material;       //!< default PxMaterial
    float               density;        //!< default density for PhysX
};


/**
PxFamily.

A collection of actors. Maps 1 to 1 with TkFamily.
*/
class ExtPxFamily
{
public:
    /**
    Spawn ExtPxFamily. Can be called only once. Actual PhysX actors will created and placed in PxScene

    \param[in]  pose            World transform.
    \param[in]  scale           Scale applied to spawned actors.
    \param[in]  settings        Spawn settings.

    \return true if spawn was successful, false otherwise.
    */
    virtual bool                            spawn(const physx::PxTransform& pose, const physx::PxVec3& scale, const ExtPxSpawnSettings& settings) = 0;

    
    /**
    Despawn this ExtPxFamily. This removes the PhysX actors from PxScene and deletes them, as well as
    deleting the created ExtPxActors

    This does not call release() on the family.

    \returns true if successful.
    */
    virtual bool                            despawn() = 0;


    /**
    The number of actors currently in this family.

    \return the number of ExtPxActor that currently exist in this family.
    */
    virtual uint32_t                        getActorCount() const = 0;

    /**
    Retrieve an array of pointers (into the user-supplied buffer) to actors.

    \param[out] buffer          A user-supplied array of ExtPxActor pointers.
    \param[in]  bufferSize      The number of elements available to write into buffer.

    \return the number of ExtPxActor pointers written to the buffer.
    */
    virtual uint32_t                        getActors(ExtPxActor** buffer, uint32_t bufferSize) const = 0;

    /**
    Every family has corresponding TkFamily.

    /return a pointer to TkFamily actor.
    */
    virtual TkFamily&                       getTkFamily() const = 0;

    /**
    Access an array of shapes of subchunks. The size of array is equal getPxAsset()->getSubchunkCount().
    For every corresponding subchunk it contains pointer to created PxShape or nullptr.

    \return the pointer to subchunk shapes array.
    */
    virtual const physx::PxShape* const*    getSubchunkShapes() const = 0;

    /**
    Every family has an associated asset.

    \return a pointer to the (const) ExtPxAsset object.
    */
    virtual ExtPxAsset&                     getPxAsset() const = 0;

    /**
    Set the default material to be used for PxRigidDynamic creation.

    \param[in] material         The material to be the new default.
    */
    virtual void                            setMaterial(physx::PxMaterial& material) = 0;

    /*
    Set ExtPxPxShapeDesc to be used on all newly created PxShapes.

    NOTE: Using it will override marking LEAF_CHUNK in simulationFilterData.word3 now.

    \param[in] pxShapeDesc      The PxShape desc to be the new default. Can be nullptr.
    */
    virtual void                            setPxShapeDescTemplate(const ExtPxShapeDescTemplate* pxShapeDesc) = 0;

    /**
    Get the default ExtPxPxShapeDesc to be used on all newly created PxShapes.

    \return a pointer to the default PxShape desc. Can be nullptr.
    */
    virtual const ExtPxShapeDescTemplate*   getPxShapeDescTemplate() const = 0;

    /*
    Set ExtPxPxActorDesc to be used on all newly created PxActors.
    
    \param[in] pxActorDesc      The PxActor desc to be the new default. Can be nullptr.
    */
    virtual void                            setPxActorDesc(const ExtPxActorDescTemplate* pxActorDesc) = 0;

    /**
    Get the default ExtPxPxActorDesc to be used on all newly created PxActors.

    \return a pointer to the default PxActor desc. Can be nullptr.
    */
    virtual const ExtPxActorDescTemplate*   getPxActorDesc() const = 0;

    /**
    The default material associated with this actor family.

    \return a pointer to the default material.
    */
    virtual const NvBlastExtMaterial*       getMaterial() const = 0;

    /**
    Set the default material associated with this actor family.

    \param[in] material         The material to be the new default.
    */
    virtual void                            setMaterial(const NvBlastExtMaterial* material) = 0;

    /**
    Add a user implementation of ExtPxListener to this family's list of listeners.  

    \param[in]  listener        The event listener to add.
    */
    virtual void                            subscribe(ExtPxListener& listener) = 0;

    /**
    Remove a user implementation of ExtPxListener from this family's list of listeners.

    \param[in]  listener        The event listener to remove.
    */
    virtual void                            unsubscribe(ExtPxListener& listener) = 0;

    /**
    Call after split.
    */
    virtual void                            postSplitUpdate() = 0;

    /**
    Release this family.
    */
    virtual void                            release() = 0;

    /**
    UserData pointer. Free to be used by user in any way.
    */
    void* userData;
};



} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXFAMILY_H
