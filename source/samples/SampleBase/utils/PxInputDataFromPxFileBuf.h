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
// Copyright (c) 2008-2024 NVIDIA Corporation. All rights reserved.


#ifndef PXINPUTDATAFROMPXFILEBUF_H
#define PXINPUTDATAFROMPXFILEBUF_H

#include "foundation/PxIO.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>


// In-memory file input compatible with PhysX's public PxInputData API.
class PxInputDataFromPxFileBuf : public physx::PxInputData
{
public:
    explicit PxInputDataFromPxFileBuf(const char* path) : mReadPosition(0)
    {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream)
        {
            return;
        }

        const std::streamoff length = stream.tellg();
        if (length < 0 || static_cast<uint64_t>(length) > std::numeric_limits<uint32_t>::max())
        {
            return;
        }

        mData.resize(static_cast<size_t>(length));
        stream.seekg(0, std::ios::beg);
        if (!mData.empty() && !stream.read(reinterpret_cast<char*>(mData.data()), length))
        {
            mData.clear();
        }
    }

    bool isOpen() const
    {
        return !mData.empty();
    }

    // physx::PxInputData interface
    virtual uint32_t    getLength() const
    {
        return static_cast<uint32_t>(mData.size());
    }

    virtual void    seek(uint32_t offset)
    {
        mReadPosition = std::min(offset, getLength());
    }

    virtual uint32_t    tell() const
    {
        return mReadPosition;
    }

    // physx::PxInputStream interface
    virtual uint32_t read(void* dest, uint32_t count)
    {
        const uint32_t available = getLength() - mReadPosition;
        const uint32_t bytesToRead = std::min(count, available);
        if (bytesToRead != 0)
        {
            std::memcpy(dest, mData.data() + mReadPosition, bytesToRead);
            mReadPosition += bytesToRead;
        }
        return bytesToRead;
    }

    PX_NOCOPY(PxInputDataFromPxFileBuf)
private:
    std::vector<uint8_t> mData;
    uint32_t mReadPosition;
};


#endif //PXINPUTDATAFROMPXFILEBUF_H
