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
// Copyright (c) 2013-2015 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#pragma once
/*
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
*/
#include "windows.h"
#include "MathUtil.h"

class Mouse
{
public:
    Mouse();
    ~Mouse();

	/*
    enum Button
    {
        LEFT   = 0x00,
        RIGHT  = 0x01,
        MIDDLE = 0x02
    };

    bool IsButtonPressed(Button button) const
	{
		if (_mouseState.rgbButtons[button] & 0x80) 
			return true;
		return false;
	}
	*/
	void SetPosition(atcore_float2 position)
	{
		m_Position = position;
	}

    atcore_float2 GetDelta() const 
	{
		return m_Delta;
	}

	void SetDelta(atcore_float2 position)
	{
		m_Delta = gfsdk_makeFloat2(
			static_cast<float>(position.x - m_Position.x),
			static_cast<float>(position.y - m_Position.y));
		m_Position = position;
	}

    float GetDeltaWheel() const
	{
	    if (m_DeltaWheel > 0)
			return 1.0f;
		else if (m_DeltaWheel < 0)
			return -1.0f;
		else
			return 0.0f;
	}

	void SetDeltaWheel(float deltaWheel)
	{
		m_DeltaWheel = deltaWheel;
	}

    void Update();
	bool Initialize(HINSTANCE hInstance, HWND hWnd);
	void Free();         
	/*
    IDirectInput8* _pDirectInput;
    IDirectInputDevice8* _pDevice;
    DIMOUSESTATE _mouseState;
	*/
	atcore_float2 m_Position;
	atcore_float2 m_Delta;
	float m_DeltaWheel;
};

