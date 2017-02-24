/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef BLASTBASETEST_H
#define BLASTBASETEST_H


#include "NvBlastTkFramework.h"

#include "gtest/gtest.h"

#include "NvBlast.h"

#include "AlignedAllocator.h"

#include "TestAssets.h"

#include "PxErrorCallback.h"
#include "PxAllocatorCallback.h"


#include <ostream>


template<int FailLevel, int Verbosity>
class BlastBaseTest : public testing::Test, public physx::PxAllocatorCallback, public physx::PxErrorCallback
{
public:
	static void* tkAlloc(size_t size)
	{
		Nv::Blast::TkFramework* fw = NvBlastTkFrameworkGet();
		if (fw != nullptr)
		{
			return fw->getAllocatorCallback().allocate(size, nullptr, __FILE__, __LINE__);
		}
		else
		{
			return std::malloc(size);
		}
	}

	static void tkFree(void* mem)
	{
		Nv::Blast::TkFramework* fw = NvBlastTkFrameworkGet();
		if (fw != nullptr)
		{
			fw->getAllocatorCallback().deallocate(mem);
		}
		else
		{
			std::free(mem);
		}
	}

	// A zeroing alloc with the same signature as malloc
	static void* alloc(size_t size)
	{
		return memset(alignedAlloc<tkAlloc>(size), 0, size);
	}

	static void free(void* mem)
	{
		alignedFree<tkFree>(mem);
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

	// PxAllocatorCallback interface
	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override
	{
		NV_UNUSED(typeName);
		NV_UNUSED(filename);
		NV_UNUSED(line);
		return alignedAlloc<std::malloc>(size);
	}

	// PxAllocatorCallback interface
	virtual void deallocate(void* ptr) override
	{
		alignedFree<std::free>(ptr);
	}

	// PxErrorCallback interface
	virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
	{
		uint32_t failMask = 0;
		switch (FailLevel)
		{
		case NvBlastMessage::Debug:
		case NvBlastMessage::Info:		failMask |= physx::PxErrorCode::eDEBUG_INFO;
		case NvBlastMessage::Warning:	failMask |= physx::PxErrorCode::eDEBUG_WARNING;
		case NvBlastMessage::Error:		failMask |= physx::PxErrorCode::eABORT | physx::PxErrorCode::eABORT | physx::PxErrorCode::eINTERNAL_ERROR | physx::PxErrorCode::eOUT_OF_MEMORY | physx::PxErrorCode::eINVALID_OPERATION | physx::PxErrorCode::eINVALID_PARAMETER;
		}

		if (!(failMask & code) && Verbosity <= 0)
		{
			return;
		}

		std::string output = "NvBlast Test ";
		switch (code)
		{
		case physx::PxErrorCode::eNO_ERROR:												break;
		case physx::PxErrorCode::eDEBUG_INFO:			output += "Debug Info";			break;
		case physx::PxErrorCode::eDEBUG_WARNING:		output += "Debug Warning";		break;
		case physx::PxErrorCode::eINVALID_PARAMETER:	output += "Invalid Parameter";	break;
		case physx::PxErrorCode::eINVALID_OPERATION:	output += "Invalid Operation";	break;
		case physx::PxErrorCode::eOUT_OF_MEMORY:		output += "Out of Memory";		break;
		case physx::PxErrorCode::eINTERNAL_ERROR:		output += "Internal Error";		break;
		case physx::PxErrorCode::eABORT:				output += "Abort";				break;
		case physx::PxErrorCode::ePERF_WARNING:			output += "Perf Warning";		break;
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
