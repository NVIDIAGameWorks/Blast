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
// Copyright (c) 2022 NVIDIA Corporation. All rights reserved.

#pragma once

#include "NvPreprocessor.h"
#include <assert.h>
#include <stdio.h>


template<typename T, int Alignment = sizeof(T)>
class POD_Buffer
{
public:
    POD_Buffer() : _size(0), _capacity(0), _data(nullptr) {}
    ~POD_Buffer() { deallocate(); }

    size_t  size() const { return _size; }

    void
    resize(size_t new_size)
    {
        if (new_size > _capacity)
        {
            reserve(new_size);
        }
        _size = new_size;
    }

    void
    reserve(size_t min_capacity)
    {
        if (min_capacity > _capacity)
        {
            void* new_data = allocate(min_capacity);
            if (!!_size)
            {
                memcpy(new_data, _data, _size*sizeof(T));
            }
            deallocate();
            _capacity = min_capacity;
            _data = reinterpret_cast<T*>(new_data);
        }
    }

    void
    push_back(const T& e)
    {
        if (_size >= _capacity)
        {
            reserve(!!_size ? 2*_size : (size_t)16);
        }
        _data[_size++] = e;
    }

    void
    pop_back()
    {
        if (!!_size) --_size;
    }

    T* data() { return _data; }
    const T* data() const { return _data; }

    T& operator [] (size_t index) { assert(_size > index); return _data[index]; }
    const T& operator [] (size_t index) const { assert(_size > index); return _data[index]; }

    T& back() { return (*this)[_size-1]; }
    const T& back() const { return (*this)[_size-1]; }

private:
    void*
    allocate(size_t buffer_size)
    {
        const size_t mem_size = sizeof(T)*buffer_size;
        unsigned char* mem = (unsigned char*)malloc(mem_size + Alignment);
        const unsigned char offset = (unsigned char)((uintptr_t)Alignment - (uintptr_t)mem % Alignment - 1);
        mem += offset;
        *mem++ = offset;
        return mem;
    }

    void
    deallocate()
    {
        if (!!_data)
        {
            unsigned char* cmem = (unsigned char*)_data;
            const unsigned char offset = *--cmem;
            ::free(cmem - offset);
        }
        _size = 0;
        _capacity = 0;
        _data = nullptr;
    }

    size_t  _size;
    size_t  _capacity;
    T*      _data;
};
