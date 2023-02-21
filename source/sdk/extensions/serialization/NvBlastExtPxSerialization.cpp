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
// Copyright (c) 2023 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtSerializationInternal.h"
#include "NvBlastExtPxSerialization.h"
#include "NvBlastExtPxSerializerCAPN.h"
#include "NvBlastExtPxSerializerRAW.h"


namespace Nv
{
namespace Blast
{

TkFramework*        sExtPxSerializerFramework = nullptr;
physx::PxPhysics*   sExtPxSerializerPhysics = nullptr;
physx::PxCooking*   sExtPxSerializerCooking = nullptr;


class ExtPxSerializerAsset_CPNB : public ExtSerializer
{
public:
    ExtSerializerBoilerplate("ExtPxAsset_CPNB", "Blast PhysX extension asset (Nv::Blast::ExtPxAsset) serialization using Cap'n Proto binary format.", ExtPxObjectTypeID::Asset, ExtSerialization::EncodingID::CapnProtoBinary);
    ExtSerializerDefaultFactoryAndRelease(ExtPxSerializerAsset_CPNB);

    virtual void* deserializeFromBuffer(const void* buffer, uint64_t size) override
    {
        return ExtSerializationCAPN<ExtPxAsset, Serialization::ExtPxAsset::Reader, Serialization::ExtPxAsset::Builder>::deserializeFromBuffer(reinterpret_cast<const unsigned char*>(buffer), size);
    }

    virtual uint64_t serializeIntoBuffer(void*& buffer, ExtSerialization::BufferProvider& bufferProvider, const void* object, uint64_t offset = 0) override
    {
        uint64_t usedSize;
        if (!ExtSerializationCAPN<ExtPxAsset, Serialization::ExtPxAsset::Reader, Serialization::ExtPxAsset::Builder>::serializeIntoBuffer(reinterpret_cast<const ExtPxAsset*>(object),
            reinterpret_cast<unsigned char*&>(buffer), usedSize, &bufferProvider, offset))
        {
            return 0;
        }
        return usedSize;
    }
};


class ExtPxSerializerAsset_RAW : public ExtSerializer
{
public:
    ExtSerializerBoilerplate("ExtPxAsset_RAW", "Blast PhysX extension asset (Nv::Blast::TkAsset) serialization using raw memory format.", ExtPxObjectTypeID::Asset, ExtSerialization::EncodingID::RawBinary);
    ExtSerializerDefaultFactoryAndRelease(ExtPxSerializerAsset_RAW);
    ExtSerializerReadOnly(ExtPxSerializerAsset_RAW);

    virtual void* deserializeFromBuffer(const void* buffer, uint64_t size) override
    {
        ExtIStream stream(buffer, size);
        return deserializeExtPxAsset(stream, *sExtPxSerializerFramework, *sExtPxSerializerPhysics);
    }
};

}   // namespace Blast
}   // namespace Nv


///////////////////////////////////////


size_t NvBlastExtPxSerializerLoadSet(Nv::Blast::TkFramework& framework, physx::PxPhysics& physics, physx::PxCooking& cooking, Nv::Blast::ExtSerialization& serialization)
{
    Nv::Blast::sExtPxSerializerFramework = &framework;
    Nv::Blast::sExtPxSerializerPhysics = &physics;
    Nv::Blast::sExtPxSerializerCooking = &cooking;

    Nv::Blast::ExtSerializer* (*factories[])() =
    {
        Nv::Blast::ExtPxSerializerAsset_CPNB::create,
        Nv::Blast::ExtPxSerializerAsset_RAW::create
    };

    return Nv::Blast::ExtSerializationLoadSet(static_cast<Nv::Blast::ExtSerializationInternal&>(serialization), factories);
}


uint64_t NvBlastExtSerializationSerializeExtPxAssetIntoBuffer(void*& buffer, Nv::Blast::ExtSerialization& serialization, const Nv::Blast::ExtPxAsset* asset)
{
    return serialization.serializeIntoBuffer(buffer, asset, Nv::Blast::ExtPxObjectTypeID::Asset);
}
