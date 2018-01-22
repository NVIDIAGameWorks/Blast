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
// Copyright (c) 2016-2017 NVIDIA Corporation. All rights reserved.


#include "BlastBaseTest.h"

#include <vector>
#include <algorithm>

#include "NvBlastExtAuthoringCutoutImpl.h"




template<int FailLevel, int Verbosity>
class AuthoringCutoutConvertToIncrementalTest : public BlastBaseTest<FailLevel, Verbosity>
{
public:
	AuthoringCutoutConvertToIncrementalTest()
	{

	}

	std::vector<Nv::Blast::POINT2D>* generateRectangleTrace(int32_t x1, int32_t x2, int32_t y1, int32_t y2, int32_t start, bool isCCW)
	{
		auto Ret = new std::vector<Nv::Blast::POINT2D>();
		auto& T = *Ret;
		T.reserve((x2 - x1) * 2 + (y2 - y1 - 1) * 2);
		for (int32_t x = x1; x <= x2; x++)
		{
			T.push_back({ x, y1 });
		}
		for (int32_t y = y1 + 1; y < y2; y++)
		{
			T.push_back({ x2, y });
		}
		for (int32_t x = x2; x >= x1; x--)
		{
			T.push_back({ x, y2 });
		}
		for (int32_t y = y2 - 1; y > y1; y--)
		{
			T.push_back({ x1, y });
		}
		if (isCCW)
		{
			start = (int32_t)T.size() - start + 1;
		}
		start = start % T.size();
		if (start > 0)
		{
			std::vector<Nv::Blast::POINT2D> buf; buf.resize(T.size());
			std::copy(T.begin() + start, T.end(), buf.begin());
			std::copy(T.begin(), T.begin() + start, buf.begin() + std::distance(T.begin() + start, T.end()));
			T = buf;
		}
		if (isCCW)
		{
			std::reverse(T.begin(), T.end());
		}
		//std::copy(buf.begin(), buf.end(), T.begin());
		return Ret;
	}

	void testTwoTracesWithOffset(uint32_t w, uint32_t h, bool isCCW1, bool isCCW2)
	{
		uint32_t mid = w / 2;
		uint32_t traceLenght = w + 1 + (h + 1) * 2;

		for (uint32_t start1 = 0; start1 < traceLenght; start1++)
		{
			for (uint32_t start2 = 0; start2 < traceLenght; start2++)
			{
				traces.push_back(generateRectangleTrace(-1, mid, -1, h, start1, isCCW1));
				traces.push_back(generateRectangleTrace(mid, w, -1, h, start2, isCCW2));
				expectedTraces.push_back(new std::vector<Nv::Blast::POINT2D>());
				*expectedTraces.front() = *traces.front();
				expectedTraces.push_back(generateRectangleTrace(-1, w, -1, h, 0, false));

				Nv::Blast::convertTracesToIncremental(traces);

				checkTraces();

				freeTraces();
			}
		}
	}

	void testThreeTracesWithOffset(uint32_t sz, bool isCCW1, bool isCCW2, bool isCCW3)
	{
		uint32_t mid = sz / 2;
		uint32_t traceLenght = sz * 2;
		uint32_t traceLenght3 = sz * 3;

		for (uint32_t start1 = 0; start1 < traceLenght; start1++)
		{
			for (uint32_t start2 = 0; start2 < traceLenght; start2++)
			{
				for (uint32_t start3 = 0; start3 < traceLenght3; start3++)
				{
					traces.push_back(generateRectangleTrace(0, mid, 0, mid, start1, isCCW1));
					traces.push_back(generateRectangleTrace(mid, sz, 0, mid, start2, isCCW2));
					traces.push_back(generateRectangleTrace(0, sz, mid, sz, start3, isCCW3));
					expectedTraces.resize(traces.size());
					expectedTraces.back() = generateRectangleTrace(0, sz, 0, sz, 0, false);

					Nv::Blast::convertTracesToIncremental(traces);

					checkTraces();

					freeTraces();
				}
			}
		}
	}

	void checkTraces()
	{
		ASSERT_EQ(traces.size(), expectedTraces.size());
		for (uint32_t i = 0; i < traces.size(); i++)
		{
			if (expectedTraces[i] != nullptr)
			{
				ASSERT_TRUE(traces[i] != nullptr && traces[i]->size() > 3);
				ASSERT_EQ(traces[i]->size(), expectedTraces[i]->size());
				auto it = std::find(expectedTraces[i]->begin(), expectedTraces[i]->end(), (*traces[i])[0]);
				EXPECT_TRUE(it != expectedTraces[i]->end());

				bool codirectional;
				if (it + 1 != expectedTraces[i]->end())
				{
					codirectional = *(it + 1) == (*traces[i])[1];
				}
				else
				{
					codirectional = expectedTraces[i]->front() == (*traces[i])[1];
				}
				for (uint32_t j = 0; j < traces[i]->size(); j++)
				{
					EXPECT_TRUE((*traces[i])[j] == *it);
					if (codirectional)
					{
						it++;
						if (it == expectedTraces[i]->end())
						{
							it = expectedTraces[i]->begin();
						}
					}
					else
					{
						if (it == expectedTraces[i]->begin())
						{
							it = expectedTraces[i]->end();
						}
						it--;
					}
				}
			}
		}
	}

	void freeTraces()
	{
		for (auto t : traces)
		{
			delete t;
		}
		traces.clear();

		for (auto t : expectedTraces)
		{
			delete t;
		}
		expectedTraces.clear();
	}

	static void messageLog(int type, const char* msg, const char* file, int line)
	{
		BlastBaseTest<FailLevel, Verbosity>::messageLog(type, msg, file, line);
	}

	static void* alloc(size_t size)
	{
		return BlastBaseTest<FailLevel, Verbosity>::alignedZeroedAlloc(size);
	}

	static void free(void* mem)
	{

		BlastBaseTest<FailLevel, Verbosity>::alignedFree(mem);
	}

	std::vector< std::vector<Nv::Blast::POINT2D>* > traces;
	std::vector< std::vector<Nv::Blast::POINT2D>* > expectedTraces;

	static std::vector<char>		s_storage;

	static size_t					s_curr;
};

// Static values
template<int FailLevel, int Verbosity>
std::vector<char>	AuthoringCutoutConvertToIncrementalTest<FailLevel, Verbosity>::s_storage;

template<int FailLevel, int Verbosity>
size_t				AuthoringCutoutConvertToIncrementalTest<FailLevel, Verbosity>::s_curr;

// Specializations
typedef AuthoringCutoutConvertToIncrementalTest<NvBlastMessage::Error, 1> AuthoringCutoutConvertToIncrementalTestAllowWarnings;
typedef AuthoringCutoutConvertToIncrementalTest<NvBlastMessage::Warning, 1> AuthoringCutoutConvertToIncrementalTestStrict;

// Two traces with common segment
// -----
// |0|1|
// -----
//--gtest_filter=AuthoringCutoutConvertToIncrementalTestStrict.TwoTraces
TEST_F(AuthoringCutoutConvertToIncrementalTestStrict, TwoTraces)
{
	testTwoTracesWithOffset(3, 2, false, false);
	testTwoTracesWithOffset(7, 4, true, true);
	testTwoTracesWithOffset(7, 4, false, true);
	testTwoTracesWithOffset(7, 4, true, false);
}

// Three traces
// -----
// |0|1|
// -----
// | 2 |
// -----
//--gtest_filter=AuthoringCutoutConvertToIncrementalTestStrict.ThreeTraces
TEST_F(AuthoringCutoutConvertToIncrementalTestStrict, ThreeTraces)
{
	testThreeTracesWithOffset(4, false, false, false);
	testThreeTracesWithOffset(4, false, false, true);
	testThreeTracesWithOffset(4, false, true, false);
	testThreeTracesWithOffset(4, false, true, true);
	testThreeTracesWithOffset(4, true, false, false);
	testThreeTracesWithOffset(4, true, false, true);
	testThreeTracesWithOffset(4, true, true, false);
	testThreeTracesWithOffset(4, true, true, true);
}

// Four traces
// -----
// |0|1|
// -----
// |2|3|
// -----
//--gtest_filter=AuthoringCutoutConvertToIncrementalTestStrict.FourTraces
TEST_F(AuthoringCutoutConvertToIncrementalTestStrict, FourTraces)
{
	traces.push_back(generateRectangleTrace(0, 10, 0, 10, 0, false));
	traces.push_back(generateRectangleTrace(10,20, 0, 10, 0, false));
	traces.push_back(generateRectangleTrace(0, 10,10, 20, 0, false));
	traces.push_back(generateRectangleTrace(10,20,10, 20, 0, false));
	expectedTraces.resize(traces.size());
	expectedTraces.back() = generateRectangleTrace(0, 20, 0, 20, 0, false);

	Nv::Blast::convertTracesToIncremental(traces);

	checkTraces();

	freeTraces();
}

// chess order segments (below for t = 4)
// -------------
// | 0|10| 4|14|
// -------------
// | 8| 2|12| 6|
// -------------
// | 1|11| 5|15|
// -------------
// | 9| 3|13| 7|
// -------------
//--gtest_filter=AuthoringCutoutConvertToIncrementalTestStrict.ChessTraces
TEST_F(AuthoringCutoutConvertToIncrementalTestStrict, ChessTraces)
{
	for (int32_t t = 2; t < 9; t++)
	{
		int32_t sz = t * 2;
		for (int32_t i = 0; i < sz; i += 2)
		{
			int32_t s = (i % 4);
			for (int32_t j = s; j < sz; j += 4)
			{
				traces.push_back(generateRectangleTrace(i - 1, i + 1, j - 1, j + 1, 0, false));
			}
		}
		for (int32_t i = 0; i < sz; i += 2)
		{
			int32_t s = ((i + 2) % 4);
			for (int32_t j = s; j < sz; j += 4)
			{	
				traces.push_back(generateRectangleTrace(i - 1, i + 1, j - 1, j + 1, 0, false));
			}
		}

		Nv::Blast::convertTracesToIncremental(traces);

		expectedTraces.resize(traces.size());
		expectedTraces.back() = generateRectangleTrace(-1, sz - 1, -1, sz - 1, 0, false);

		checkTraces();

		freeTraces();
	}
}


// if convertTracesToIncremental tries to unite 0 and 1 traces the resulting trace breaks into outer and inner traces (hole).
// Currently we don't support it. So need to shaffle traces in a way that inner traces don't appear.
// -------
// |0|1|2|
// |7|8|3|
// |6|5|4|
// -------
//--gtest_filter=AuthoringCutoutConvertToIncrementalTestStrict.AvoidHoles
TEST_F(AuthoringCutoutConvertToIncrementalTestStrict, DISABLED_AvoidHoles)
{
	traces.push_back(generateRectangleTrace(0, 2, 0, 2, 0, false));
	traces.push_back(generateRectangleTrace(2, 4, 0, 2, 0, false));
	traces.push_back(generateRectangleTrace(4, 6, 0, 2, 0, false));
	traces.push_back(generateRectangleTrace(4, 6, 2, 4, 0, false));
	traces.push_back(generateRectangleTrace(4, 6, 4, 6, 0, false));
	traces.push_back(generateRectangleTrace(2, 4, 4, 6, 0, false));
	traces.push_back(generateRectangleTrace(0, 2, 4, 6, 0, false));
	traces.push_back(generateRectangleTrace(0, 2, 2, 4, 0, false));
	traces.push_back(generateRectangleTrace(2, 4, 2, 4, 0, false));

	Nv::Blast::convertTracesToIncremental(traces);

	expectedTraces.resize(traces.size());
	expectedTraces.back() = generateRectangleTrace(0, 6, 0, 6, 0, false);

	checkTraces();

	freeTraces();
}

// if convertTracesToIncremental tries to unite 0 and 1 traces the resulting trace breaks into outer and inner traces (hole).
// Currently we don't support it. So need to shaffle traces in a way that inner traces don't appear.
// ---------
// |   |   |
// |  ---  |
// | 0|2|1 |
// |  ---  |
// |   |   |
// ---------
//--gtest_filter=AuthoringCutoutConvertToIncrementalTestStrict.AvoidHoles2
TEST_F(AuthoringCutoutConvertToIncrementalTestStrict, DISABLED_AvoidHoles2)
{
	std::vector<std::vector<Nv::Blast::POINT2D>*> rightTraces, leftTraces, expectedMid;

	leftTraces.push_back(generateRectangleTrace(-1, 2, -1, 1, 0, false));
	leftTraces.push_back(generateRectangleTrace(-1, 1, 1, 3, 0, false));
	leftTraces.push_back(generateRectangleTrace(-1, 2, 3, 5, 0, false));
	Nv::Blast::convertTracesToIncremental(leftTraces);
	delete leftTraces[0]; delete leftTraces[1];
	traces.push_back(leftTraces.back());

	rightTraces.push_back(generateRectangleTrace(2, 5, -1, 1, 0, false));
	rightTraces.push_back(generateRectangleTrace(3, 5, 1, 3, 0, false));
	rightTraces.push_back(generateRectangleTrace(2, 5, 3, 5, 0, false));
	Nv::Blast::convertTracesToIncremental(rightTraces);
	delete rightTraces[0]; delete rightTraces[1];
	traces.push_back(rightTraces.back());

	traces.push_back(generateRectangleTrace(1, 3, 1, 3, 0, false));

	expectedTraces.push_back(new std::vector<Nv::Blast::POINT2D>());
	*expectedTraces.front() = *traces.front();

	expectedMid.push_back(generateRectangleTrace(-1, 2, -1, 5, 0, false));
	expectedMid.push_back(generateRectangleTrace(2, 3, 1, 3, 0, false));
	delete expectedMid[0];
	expectedTraces.push_back(expectedMid.back());
	expectedMid.push_back(generateRectangleTrace(-1, 5, -1, 5, 0, false));

	Nv::Blast::convertTracesToIncremental(traces);

	checkTraces();

	freeTraces();
}
