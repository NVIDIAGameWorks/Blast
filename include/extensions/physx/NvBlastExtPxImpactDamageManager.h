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
//! @brief Defines the API for the NvBlastExtPxDamageManager class, which processes impact damage from physx

#ifndef NVBLASTEXTIMPACTDAMAGEMANAGER_H
#define NVBLASTEXTIMPACTDAMAGEMANAGER_H

#include "PxFiltering.h"
#include "NvPreprocessor.h"

// Forward declarations
namespace physx
{
struct PxContactPair;
struct PxContactPairHeader;
}


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxActor;
class ExtPxManager;


/**
Custom Damage Function
*/
typedef bool(*ExtImpactDamageFunction)(void* data, ExtPxActor* actor, physx::PxShape* shape, physx::PxVec3 worldPos, physx::PxVec3 worldForce);


/**
Impact Damage Manager Settings.
*/
struct ExtImpactSettings
{
    bool                    isSelfCollissionEnabled;    //!<    family's self collision enabled.
    bool                    shearDamage;                //!<    use shear damage program (otherwise simple radial damage is used)
    float                   hardness;                   //!<    hardness of material for impact damage. Damage = impulse / hardness . This damage is capped by the material's health.
    float                   damageRadiusMax;            //!<    the maximum radius in which full damage is applied.
    float                   damageThresholdMin;         //!<    minimum damage fraction threshold to be applied. Range [0, 1]. For example 0.1 filters all damage below 10% of health.
    float                   damageThresholdMax;         //!<    maximum damage fraction threshold to be applied. Range [0, 1]. For example 0.8 won't allow more then 80% of health damage to be applied.
    float                   damageFalloffRadiusFactor;  //!<    damage attenuation radius factor. Given a radius R for full damage, for [R, R * damageFalloffRadiusFactor] radius interval damage attenuates down to zero at the outer radius.
    ExtImpactDamageFunction damageFunction;             //!<    custom damage function, can be nullptr, default internal one will be used in that case.
    void*                   damageFunctionData;         //!<    data to be passed in custom damage function.


    ExtImpactSettings() :
        isSelfCollissionEnabled(false),
        shearDamage(true),
        hardness(10.0f),
        damageRadiusMax(2.0f),
        damageThresholdMin(0.1f), // to filter small damage events
        damageThresholdMax(1.0f),
        damageFalloffRadiusFactor(2.0f),
        damageFunction(nullptr),
        damageFunctionData(nullptr)
    {}
};


/**
Impact Damage Manager.

Requires ExtPxManager.
Call onContact from PxSimulationEventCallback onContact to accumulate damage.
Call applyDamage to apply accumulated damage.
*/
class NV_DLL_EXPORT ExtImpactDamageManager
{
public:
    //////// manager creation ////////

    /**
    Create a new ExtImpactDamageManager.

    \param[in]  pxManager   The ExtPxManager instance to be used by impact damage manager.
    \param[in]  settings        The settings to be set on ExtImpactDamageManager.

    \return the new ExtImpactDamageManager if successful, NULL otherwise.
    */
    static ExtImpactDamageManager*  create(ExtPxManager* pxManager, ExtImpactSettings settings = ExtImpactSettings());

    /**
    Release this manager.
    */
    virtual void                    release() = 0;


    //////// interface ////////

    /**
    Set ExtImpactDamageManager settings.

    \param[in]  settings        The settings to be set on ExtImpactDamageManager.
    */
    virtual void                    setSettings(const ExtImpactSettings& settings) = 0;

    /**
    This method is equal to PxSimulationEventCallback::onContact.

    User should implement own PxSimulationEventCallback onContact and call this method in order ExtImpactDamageManager to work correctly.
    
    Contacts will be processed and impact damage will be accumulated.

    \param[in] pairHeader Information on the two actors whose shapes triggered a contact report.
    \param[in] pairs The contact pairs of two actors for which contact reports have been requested. @see PxContactPair.
    \param[in] nbPairs The number of provided contact pairs.

    @see PxSimulationEventCallback
    */
    virtual void                    onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, uint32_t nbPairs) = 0;


    /**
    Apply accumulated impact damage.
    */
    virtual void                    applyDamage() = 0;


    //////// filter shader ////////

    /**
    Custom implementation of PxSimulationFilterShader, enables necessary information to be passed in onContact().
    Set it in your PxScene PxSceneDesc in order to impact damage to work correctly or implement your own.

    @see PxSimulationFilterShader
    */
    static physx::PxFilterFlags     FilterShader(
        physx::PxFilterObjectAttributes attributes0,
        physx::PxFilterData filterData0,
        physx::PxFilterObjectAttributes attributes1,
        physx::PxFilterData filterData1,
        physx::PxPairFlags& pairFlags,
        const void* constantBlock,
        uint32_t constantBlockSize);

};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTIMPACTDAMAGEMANAGER_H
