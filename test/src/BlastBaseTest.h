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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef BLASTBASETEST_H
#define BLASTBASETEST_H


#include "NvBlastTkFramework.h"

#include "gtest/gtest.h"

#include "NvBlast.h"

#include "TestAssets.h"

#include "NvBlastGlobals.h"

#include <ostream>


template<int FailLevel, int Verbosity>
class BlastBaseTest : public testing::Test, public Nv::Blast::ErrorCallback
{
public:
	BlastBaseTest()
	{
		NvBlastGlobalSetErrorCallback(this);
	}

	// A zeroing alloc with the same signature as malloc
	static void* alignedZeroedAlloc(size_t size)
	{
		return memset(NVBLAST_ALLOC(size), 0, size);
	}

	static void alignedFree(void* mem)
	{
		NVBLAST_FREE(mem);
	}

	// Message log for blast functions
	static void messageLog(int type, const char* msg, const char* file, int line)
	{
		if (FailLevel >= type)
		{
			switch (type)
			{
			case NvBlastMessage::Error:		EXPECT_TRUE(false) << "NvBlast Error message in " << file << "(" << line << "): " << msg << "\n";	break;
			case NvBlastMessage::Warning:	EXPECT_TRUE(false) << "NvBlast Warning message in " << file << "(" << line << "): " << msg << "\n";	break;
			case NvBlastMessage::Info:		EXPECT_TRUE(false) << "NvBlast Info message in " << file << "(" << line << "): " << msg << "\n";	break;
			case NvBlastMessage::Debug:		EXPECT_TRUE(false) << "NvBlast Debug message in " << file << "(" << line << "): " << msg << "\n";	break;
			}
		}
		else
		if (Verbosity > 0)
		{
			switch (type)
			{
			case NvBlastMessage::Error:		std::cout << "NvBlast Error message in " << file << "(" << line << "): " << msg << "\n";	break;
			case NvBlastMessage::Warning:	std::cout << "NvBlast Warning message in " << file << "(" << line << "): " << msg << "\n";	break;
			case NvBlastMessage::Info:		std::cout << "NvBlast Info message in " << file << "(" << line << "): " << msg << "\n";		break;
			case NvBlastMessage::Debug:		std::cout << "NvBlast Debug message in " << file << "(" << line << "): " << msg << "\n";	break;
			}
		}
	}

	// ErrorCallback interface
	virtual void reportError(Nv::Blast::ErrorCode::Enum code, const char* message, const char* file, int line) override
	{
		uint32_t failMask = 0;
		switch (FailLevel)
		{
		case NvBlastMessage::Debug:
		case NvBlastMessage::Info:		failMask |= Nv::Blast::ErrorCode::eDEBUG_INFO;
		case NvBlastMessage::Warning:	failMask |= Nv::Blast::ErrorCode::eDEBUG_WARNING;
		case NvBlastMessage::Error:		failMask |= Nv::Blast::ErrorCode::eABORT | Nv::Blast::ErrorCode::eABORT | Nv::Blast::ErrorCode::eINTERNAL_ERROR | Nv::Blast::ErrorCode::eOUT_OF_MEMORY | Nv::Blast::ErrorCode::eINVALID_OPERATION | Nv::Blast::ErrorCode::eINVALID_PARAMETER;
		default: break;
		}

		if (!(failMask & code) && Verbosity <= 0)
		{
			return;
		}

		std::string output = "NvBlast Test ";
		switch (code)
		{
		case Nv::Blast::ErrorCode::eNO_ERROR:											break;
		case Nv::Blast::ErrorCode::eDEBUG_INFO:			output += "Debug Info";			break;
		case Nv::Blast::ErrorCode::eDEBUG_WARNING:		output += "Debug Warning";		break;
		case Nv::Blast::ErrorCode::eINVALID_PARAMETER:	output += "Invalid Parameter";	break;
		case Nv::Blast::ErrorCode::eINVALID_OPERATION:	output += "Invalid Operation";	break;
		case Nv::Blast::ErrorCode::eOUT_OF_MEMORY:		output += "Out of Memory";		break;
		case Nv::Blast::ErrorCode::eINTERNAL_ERROR:		output += "Internal Error";		break;
		case Nv::Blast::ErrorCode::eABORT:				output += "Abort";				break;
		case Nv::Blast::ErrorCode::ePERF_WARNING:		output += "Perf Warning";	break;
		default:										FAIL();
		}
		output += std::string(" message in ") + file + "(" + std::to_string(line) + "): " + message + "\n";

		if (failMask & code)
		{
			EXPECT_TRUE(false) << output;
		}
		else
		{
			std::cout << output;
		}
	}
};


#endif // #ifndef BLASTBASETEST_H
