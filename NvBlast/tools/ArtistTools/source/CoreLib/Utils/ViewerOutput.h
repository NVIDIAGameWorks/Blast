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

#include <set>

#include "corelib_global.h"

/*
 * !! CAUSION: 
 *		Not thread safe, should only output messages in the MAIN thread!
 *		Otherwise, cannot guarantee the Qt Gui behavior.
 */

// DON'T TRY TO WRITE CHARS MORE THAN THE FURVIEWER_MAX_OUTPUT_CHAR
#define FURVIEWER_MAX_OUTPUT_CHAR 2048

// c-style print
#define viewer_msg(fmt, ...)	ViewerOutput::Inst().print(FrontPrinter::NONE,	0,				fmt, ##__VA_ARGS__)
#define viewer_warn(fmt, ...)	ViewerOutput::Inst().print(FrontPrinter::NONE,	RGB(255,200,020), fmt, ##__VA_ARGS__)
#define viewer_info(fmt, ...)	ViewerOutput::Inst().print(FrontPrinter::NONE,	RGB(118,185,000), fmt, ##__VA_ARGS__)
#define viewer_err(fmt, ...)	ViewerOutput::Inst().print(FrontPrinter::NONE,	RGB(255,000,000), fmt, ##__VA_ARGS__)
#define viewer_fatal(fmt, ...)	ViewerOutput::Inst().print(FrontPrinter::BOLD,	RGB(255,000,000), fmt, ##__VA_ARGS__)

// c++ style output stream
#define viewer_stream_msg	ViewerStream(FrontPrinter::NONE, 0)
#define viewer_stream_warn	ViewerStream(FrontPrinter::NONE, RGB(255,200,020))
#define viewer_stream_info	ViewerStream(FrontPrinter::NONE, RGB(118,185,000))
#define viewer_stream_err	ViewerStream(FrontPrinter::NONE, RGB(255,000,000))
#define viewer_stream_fatal ViewerStream(FrontPrinter::NONE, RGB(255,000,000))


//////////////////////////////////////////////////////////////////////////
// implementation
//////////////////////////////////////////////////////////////////////////
class FrontPrinter
{
public:
	enum Effect
	{
		NONE = 0x0,
		BOLD = 0x1,
		ITALIC = 0x2,
		UNDERLINE = 0x4
	};
		
	virtual void print(const char* txt, unsigned long color = 0, Effect e = NONE) = 0;
};

class CORELIB_EXPORT ViewerOutput
{
public:
	static ViewerOutput& Inst();

	void RegisterPrinter(FrontPrinter* printer);
	void UnRegisterPrinter(FrontPrinter* printer);

	void print(FrontPrinter::Effect e, unsigned long color, const char* fmt, ...);

private:
	enum {MAX_BUFFER_SIZE = FURVIEWER_MAX_OUTPUT_CHAR + 1};

	char _buf[MAX_BUFFER_SIZE];

	std::set<FrontPrinter*> _printers;
};


//////////////////////////////////////////////////////////////////////////
// stream to support c++ style output

#include <streambuf>
#include <sstream>

class ViewerStream : public std::ostream
{
public:
	explicit ViewerStream(FrontPrinter::Effect e, unsigned long color)
		: std::ostream(&_buffer)
		, _effect(e)
		, _color(color)
	{}

	~ViewerStream();

	ViewerStream& flush();

private:
	
	class ViewerStreamBuf : public std::streambuf
	{
	public:
		enum
		{
			STATIC_BUFFER_SIZE = FURVIEWER_MAX_OUTPUT_CHAR + 1,
		};

		ViewerStreamBuf();

		void reset();
		std::streamsize size();

		// fixed stack buffer
		char buf[STATIC_BUFFER_SIZE];
	};

	ViewerStreamBuf _buffer;
	FrontPrinter::Effect _effect;
	unsigned long _color;
};
