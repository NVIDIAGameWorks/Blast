
// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.

#include "Mouse.h"

//#include "windows.h"

Mouse::Mouse()
{
}

Mouse::~Mouse()
{
    Free();
}

bool Mouse::Initialize(HINSTANCE hInstance, HWND hWnd)
{
	/*
    if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION,
            IID_IDirectInput8, reinterpret_cast<void**>(&_pDirectInput), 0)))
        return false;

    if (FAILED(_pDirectInput->CreateDevice(GUID_SysMouse, &_pDevice, 0)))
        return false;

    if (FAILED(_pDevice->SetDataFormat(&c_dfDIMouse)))
        return false;

    if (FAILED(_pDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
        return false;

    if (FAILED(_pDevice->Acquire()))
        return false;
	*/
    return true;
}

void Mouse::Free()
{
	/*
    if (_pDevice)
    {
        _pDevice->Unacquire();
        _pDevice = NULL;
    }

	_pDirectInput = NULL;
	*/
}

void Mouse::Update()
{
	/*
    if (!_pDirectInput || !_pDevice)
    {
		return;
    }

    HRESULT hr;

    while (true)
    {
        hr = _pDevice->GetDeviceState(sizeof(DIMOUSESTATE), &_mouseState);

        if (FAILED(hr))
        {
            if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
            {
                if (FAILED(_pDevice->Acquire()))
                    return;
            }
        }
        else
        {
            break;
        }
    }
	*/
}
