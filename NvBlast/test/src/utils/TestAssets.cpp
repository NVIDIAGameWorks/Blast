#include "TestAssets.h"
#include "AssetGenerator.h"

const NvBlastChunkDesc g_cube1ChunkDescs[9] =
{
//    centroid              volume  parent idx  flags                           ID
	{ { 0.0f, 0.0f, 0.0f }, 8.0f,   UINT32_MAX, NvBlastChunkDesc::NoFlags,      0 },
	{ {-0.5f,-0.5f,-0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  1 },
	{ { 0.5f,-0.5f,-0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  2 },
	{ {-0.5f, 0.5f,-0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  3 },
	{ { 0.5f, 0.5f,-0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  4 },
	{ {-0.5f,-0.5f, 0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  5 },
	{ { 0.5f,-0.5f, 0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  6 },
	{ {-0.5f, 0.5f, 0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  7 },
	{ { 0.5f, 0.5f, 0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  8 },
};

const NvBlastBondDesc g_cube1BondDescs[12] =
{
//    chunks      normal                area  centroid              userData
	{ { 1, 2 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 0.0f,-0.5f,-0.5f }, 0 } },
	{ { 3, 4 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 0.0f, 0.5f,-0.5f }, 0 } },
	{ { 5, 6 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 0.0f,-0.5f, 0.5f }, 0 } },
	{ { 7, 8 }, { { 1.0f, 0.0f, 0.0f }, 1.0f, { 0.0f, 0.5f, 0.5f }, 0 } },

	{ { 1, 3 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, {-0.5f, 0.0f,-0.5f }, 0 } },
	{ { 2, 4 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 0.5f, 0.0f,-0.5f }, 0 } },
	{ { 5, 7 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, {-0.5f, 0.0f, 0.5f }, 0 } },
	{ { 6, 8 }, { { 0.0f, 1.0f, 0.0f }, 1.0f, { 0.5f, 0.0f, 0.5f }, 0 } },

	{ { 1, 5 }, { { 0.0f, 0.0f, 1.0f }, 1.0f, {-0.5f,-0.5f, 0.0f }, 0 } },
	{ { 2, 6 }, { { 0.0f, 0.0f, 1.0f }, 1.0f, { 0.5f,-0.5f, 0.0f }, 0 } },
	{ { 3, 7 }, { { 0.0f, 0.0f, 1.0f }, 1.0f, {-0.5f, 0.5f, 0.0f }, 0 } },
	{ { 4, 8 }, { { 0.0f, 0.0f, 1.0f }, 1.0f, { 0.5f, 0.5f, 0.0f }, 0 } },
};

const NvBlastChunkDesc g_cube2ChunkDescs[73] =
{
//    centroid                                volume  parent idx  flags                           ID
	{ { 0.0f,  0.0f,  0.0f },                 8.0f,   UINT32_MAX, NvBlastChunkDesc::NoFlags,      0  },
	{ {-0.5f, -0.5f, -0.5f },                 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  1  },
	{ { 0.5f, -0.5f, -0.5f },                 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  2  },
	{ {-0.5f,  0.5f, -0.5f },                 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  3  },
	{ { 0.5f,  0.5f, -0.5f },                 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  4  },
	{ {-0.5f, -0.5f,  0.5f },                 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  5  },
	{ { 0.5f, -0.5f,  0.5f },                 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  6  },
	{ {-0.5f,  0.5f,  0.5f },                 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  7  },
	{ { 0.5f,  0.5f,  0.5f },                 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  8  },
	{ {-0.25f-0.5f,-0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,          NvBlastChunkDesc::NoFlags,      9  },
	{ { 0.25f-0.5f,-0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,          NvBlastChunkDesc::NoFlags,      10 },
	{ {-0.25f-0.5f, 0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,          NvBlastChunkDesc::NoFlags,      11 },
	{ { 0.25f-0.5f, 0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,          NvBlastChunkDesc::NoFlags,      12 },
	{ {-0.25f-0.5f,-0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,          NvBlastChunkDesc::NoFlags,      13 },
	{ { 0.25f-0.5f,-0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,          NvBlastChunkDesc::NoFlags,      14 },
	{ {-0.25f-0.5f, 0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,          NvBlastChunkDesc::NoFlags,      15 },
	{ { 0.25f-0.5f, 0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,          NvBlastChunkDesc::NoFlags,      16 },
	{ {-0.25f+0.5f,-0.25f-0.5f,-0.25f-0.5f }, 0.125f, 2,          NvBlastChunkDesc::NoFlags,      17 },
	{ { 0.25f+0.5f,-0.25f-0.5f,-0.25f-0.5f }, 0.125f, 2,          NvBlastChunkDesc::NoFlags,      18 },
	{ {-0.25f+0.5f, 0.25f-0.5f,-0.25f-0.5f }, 0.125f, 2,          NvBlastChunkDesc::NoFlags,      19 },
	{ { 0.25f+0.5f, 0.25f-0.5f,-0.25f-0.5f }, 0.125f, 2,          NvBlastChunkDesc::NoFlags,      20 },
	{ {-0.25f+0.5f,-0.25f-0.5f, 0.25f-0.5f }, 0.125f, 2,          NvBlastChunkDesc::NoFlags,      21 },
	{ { 0.25f+0.5f,-0.25f-0.5f, 0.25f-0.5f }, 0.125f, 2,          NvBlastChunkDesc::NoFlags,      22 },
	{ {-0.25f+0.5f, 0.25f-0.5f, 0.25f-0.5f }, 0.125f, 2,          NvBlastChunkDesc::NoFlags,      23 },
	{ { 0.25f+0.5f, 0.25f-0.5f, 0.25f-0.5f }, 0.125f, 2,          NvBlastChunkDesc::NoFlags,      24 },
	{ {-0.25f-0.5f,-0.25f+0.5f,-0.25f-0.5f }, 0.125f, 3,          NvBlastChunkDesc::NoFlags,      25 },
	{ { 0.25f-0.5f,-0.25f+0.5f,-0.25f-0.5f }, 0.125f, 3,          NvBlastChunkDesc::NoFlags,      26 },
	{ {-0.25f-0.5f, 0.25f+0.5f,-0.25f-0.5f }, 0.125f, 3,          NvBlastChunkDesc::NoFlags,      27 },
	{ { 0.25f-0.5f, 0.25f+0.5f,-0.25f-0.5f }, 0.125f, 3,          NvBlastChunkDesc::NoFlags,      28 },
	{ {-0.25f-0.5f,-0.25f+0.5f, 0.25f-0.5f }, 0.125f, 3,          NvBlastChunkDesc::NoFlags,      29 },
	{ { 0.25f-0.5f,-0.25f+0.5f, 0.25f-0.5f }, 0.125f, 3,          NvBlastChunkDesc::NoFlags,      30 },
	{ {-0.25f-0.5f, 0.25f+0.5f, 0.25f-0.5f }, 0.125f, 3,          NvBlastChunkDesc::NoFlags,      31 },
	{ { 0.25f-0.5f, 0.25f+0.5f, 0.25f-0.5f }, 0.125f, 3,          NvBlastChunkDesc::NoFlags,      32 },
	{ {-0.25f+0.5f,-0.25f+0.5f,-0.25f-0.5f }, 0.125f, 4,          NvBlastChunkDesc::NoFlags,      33 },
	{ { 0.25f+0.5f,-0.25f+0.5f,-0.25f-0.5f }, 0.125f, 4,          NvBlastChunkDesc::NoFlags,      34 },
	{ {-0.25f+0.5f, 0.25f+0.5f,-0.25f-0.5f }, 0.125f, 4,          NvBlastChunkDesc::NoFlags,      35 },
	{ { 0.25f+0.5f, 0.25f+0.5f,-0.25f-0.5f }, 0.125f, 4,          NvBlastChunkDesc::NoFlags,      36 },
	{ {-0.25f+0.5f,-0.25f+0.5f, 0.25f-0.5f }, 0.125f, 4,          NvBlastChunkDesc::NoFlags,      37 },
	{ { 0.25f+0.5f,-0.25f+0.5f, 0.25f-0.5f }, 0.125f, 4,          NvBlastChunkDesc::NoFlags,      38 },
	{ {-0.25f+0.5f, 0.25f+0.5f, 0.25f-0.5f }, 0.125f, 4,          NvBlastChunkDesc::NoFlags,      39 },
	{ { 0.25f+0.5f, 0.25f+0.5f, 0.25f-0.5f }, 0.125f, 4,          NvBlastChunkDesc::NoFlags,      40 },
	{ {-0.25f-0.5f,-0.25f-0.5f,-0.25f+0.5f }, 0.125f, 5,          NvBlastChunkDesc::NoFlags,      41 },
	{ { 0.25f-0.5f,-0.25f-0.5f,-0.25f+0.5f }, 0.125f, 5,          NvBlastChunkDesc::NoFlags,      42 },
	{ {-0.25f-0.5f, 0.25f-0.5f,-0.25f+0.5f }, 0.125f, 5,          NvBlastChunkDesc::NoFlags,      43 },
	{ { 0.25f-0.5f, 0.25f-0.5f,-0.25f+0.5f }, 0.125f, 5,          NvBlastChunkDesc::NoFlags,      44 },
	{ {-0.25f-0.5f,-0.25f-0.5f, 0.25f+0.5f }, 0.125f, 5,          NvBlastChunkDesc::NoFlags,      45 },
	{ { 0.25f-0.5f,-0.25f-0.5f, 0.25f+0.5f }, 0.125f, 5,          NvBlastChunkDesc::NoFlags,      46 },
	{ {-0.25f-0.5f, 0.25f-0.5f, 0.25f+0.5f }, 0.125f, 5,          NvBlastChunkDesc::NoFlags,      47 },
	{ { 0.25f-0.5f, 0.25f-0.5f, 0.25f+0.5f }, 0.125f, 5,          NvBlastChunkDesc::NoFlags,      48 },
	{ {-0.25f+0.5f,-0.25f-0.5f,-0.25f+0.5f }, 0.125f, 6,          NvBlastChunkDesc::NoFlags,      49 },
	{ { 0.25f+0.5f,-0.25f-0.5f,-0.25f+0.5f }, 0.125f, 6,          NvBlastChunkDesc::NoFlags,      50 },
	{ {-0.25f+0.5f, 0.25f-0.5f,-0.25f+0.5f }, 0.125f, 6,          NvBlastChunkDesc::NoFlags,      51 },
	{ { 0.25f+0.5f, 0.25f-0.5f,-0.25f+0.5f }, 0.125f, 6,          NvBlastChunkDesc::NoFlags,      52 },
	{ {-0.25f+0.5f,-0.25f-0.5f, 0.25f+0.5f }, 0.125f, 6,          NvBlastChunkDesc::NoFlags,      53 },
	{ { 0.25f+0.5f,-0.25f-0.5f, 0.25f+0.5f }, 0.125f, 6,          NvBlastChunkDesc::NoFlags,      54 },
	{ {-0.25f+0.5f, 0.25f-0.5f, 0.25f+0.5f }, 0.125f, 6,          NvBlastChunkDesc::NoFlags,      55 },
	{ { 0.25f+0.5f, 0.25f-0.5f, 0.25f+0.5f }, 0.125f, 6,          NvBlastChunkDesc::NoFlags,      56 },
	{ {-0.25f-0.5f,-0.25f+0.5f,-0.25f+0.5f }, 0.125f, 7,          NvBlastChunkDesc::NoFlags,      57 },
	{ { 0.25f-0.5f,-0.25f+0.5f,-0.25f+0.5f }, 0.125f, 7,          NvBlastChunkDesc::NoFlags,      58 },
	{ {-0.25f-0.5f, 0.25f+0.5f,-0.25f+0.5f }, 0.125f, 7,          NvBlastChunkDesc::NoFlags,      59 },
	{ { 0.25f-0.5f, 0.25f+0.5f,-0.25f+0.5f }, 0.125f, 7,          NvBlastChunkDesc::NoFlags,      60 },
	{ {-0.25f-0.5f,-0.25f+0.5f, 0.25f+0.5f }, 0.125f, 7,          NvBlastChunkDesc::NoFlags,      61 },
	{ { 0.25f-0.5f,-0.25f+0.5f, 0.25f+0.5f }, 0.125f, 7,          NvBlastChunkDesc::NoFlags,      62 },
	{ {-0.25f-0.5f, 0.25f+0.5f, 0.25f+0.5f }, 0.125f, 7,          NvBlastChunkDesc::NoFlags,      63 },
	{ { 0.25f-0.5f, 0.25f+0.5f, 0.25f+0.5f }, 0.125f, 7,          NvBlastChunkDesc::NoFlags,      64 },
	{ {-0.25f+0.5f,-0.25f+0.5f,-0.25f+0.5f }, 0.125f, 8,          NvBlastChunkDesc::NoFlags,      65 },
	{ { 0.25f+0.5f,-0.25f+0.5f,-0.25f+0.5f }, 0.125f, 8,          NvBlastChunkDesc::NoFlags,      66 },
	{ {-0.25f+0.5f, 0.25f+0.5f,-0.25f+0.5f }, 0.125f, 8,          NvBlastChunkDesc::NoFlags,      67 },
	{ { 0.25f+0.5f, 0.25f+0.5f,-0.25f+0.5f }, 0.125f, 8,          NvBlastChunkDesc::NoFlags,      68 },
	{ {-0.25f+0.5f,-0.25f+0.5f, 0.25f+0.5f }, 0.125f, 8,          NvBlastChunkDesc::NoFlags,      69 },
	{ { 0.25f+0.5f,-0.25f+0.5f, 0.25f+0.5f }, 0.125f, 8,          NvBlastChunkDesc::NoFlags,      70 },
	{ {-0.25f+0.5f, 0.25f+0.5f, 0.25f+0.5f }, 0.125f, 8,          NvBlastChunkDesc::NoFlags,      71 },
	{ { 0.25f+0.5f, 0.25f+0.5f, 0.25f+0.5f }, 0.125f, 8,          NvBlastChunkDesc::NoFlags,      72 },
};

const NvBlastChunkDesc g_cube3ChunkDescs[11] =
{
//    centroid              volume  parent idx  flags                           ID
	{ { 0.0f, 0.0f, 0.0f }, 4.0f,   UINT32_MAX, NvBlastChunkDesc::NoFlags,      0 },
	{ { 0.0f, 0.0f, 0.0f }, 3.0f,   UINT32_MAX, NvBlastChunkDesc::NoFlags,      1 },
	{ { 0.0f, 0.0f, 0.0f }, 1.0f,   UINT32_MAX, NvBlastChunkDesc::NoFlags,      2 },
	{ {-0.5f,-0.5f,-0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  3 },
	{ { 0.5f,-0.5f,-0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  4 },
	{ {-0.5f, 0.5f,-0.5f }, 1.0f,   0,          NvBlastChunkDesc::SupportFlag,  5 },
	{ { 0.5f, 0.5f,-0.5f }, 1.0f,   1,          NvBlastChunkDesc::SupportFlag,  6 },
	{ {-0.5f,-0.5f, 0.5f }, 1.0f,   1,          NvBlastChunkDesc::SupportFlag,  7 },
	{ { 0.5f,-0.5f, 0.5f }, 1.0f,   1,          NvBlastChunkDesc::SupportFlag,  8 },
	{ {-0.5f, 0.5f, 0.5f }, 1.0f,   2,          NvBlastChunkDesc::SupportFlag,  9 },
	{ { 0.5f, 0.5f, 0.5f }, 1.0f,   2,          NvBlastChunkDesc::SupportFlag,   10 },
};

const NvBlastBondDesc g_cube3BondDescs[12] =
{
	//    chunks      normal                area  centroid              userData
	{ { 3, 4 },{ { 1.0f, 0.0f, 0.0f }, 1.0f,{ 0.0f,-0.5f,-0.5f }, 0 } },
	{ { 5, 6 },{ { 1.0f, 0.0f, 0.0f }, 1.0f,{ 0.0f, 0.5f,-0.5f }, 0 } },
	{ { 7, 8 },{ { 1.0f, 0.0f, 0.0f }, 1.0f,{ 0.0f,-0.5f, 0.5f }, 0 } },
	{ { 9, 10},{ { 1.0f, 0.0f, 0.0f }, 1.0f,{ 0.0f, 0.5f, 0.5f }, 0 } },

	{ { 3, 5 },{ { 0.0f, 1.0f, 0.0f }, 1.0f,{-0.5f, 0.0f,-0.5f }, 0 } },
	{ { 4, 6 },{ { 0.0f, 1.0f, 0.0f }, 1.0f,{ 0.5f, 0.0f,-0.5f }, 0 } },
	{ { 7, 9 },{ { 0.0f, 1.0f, 0.0f }, 1.0f,{-0.5f, 0.0f, 0.5f }, 0 } },
	{ { 8, 10},{ { 0.0f, 1.0f, 0.0f }, 1.0f,{ 0.5f, 0.0f, 0.5f }, 0 } },

	{ { 3, 7 },{ { 0.0f, 0.0f, 1.0f }, 1.0f,{-0.5f,-0.5f, 0.0f }, 0 } },
	{ { 4, 8 },{ { 0.0f, 0.0f, 1.0f }, 1.0f,{ 0.5f,-0.5f, 0.0f }, 0 } },
	{ { 5, 9 },{ { 0.0f, 0.0f, 1.0f }, 1.0f,{-0.5f, 0.5f, 0.0f }, 0 } },
	{ { 6, 10},{ { 0.0f, 0.0f, 1.0f }, 1.0f,{ 0.5f, 0.5f, 0.0f }, 0 } },
};

const NvBlastAssetDesc g_assetDescs[3] =
{
	// 2x2x2 axis-aligned cube centered at the origin, split into 8 depth-1 1x1x1 child chunks
	{ sizeof(g_cube1ChunkDescs) / sizeof(g_cube1ChunkDescs[0]), g_cube1ChunkDescs, sizeof(g_cube1BondDescs) / sizeof(g_cube1BondDescs[0]), g_cube1BondDescs },

	// 2x2x2 axis-aligned cube centered at the origin, split into 8 depth-1 1x1x1 child chunks which are then split into 8 depth-2 (1/2)x(1/2)x(1/2) child chunks each
	// Support is at depth-1, so the g_cube1BondDescs are used
	{ sizeof(g_cube2ChunkDescs) / sizeof(g_cube2ChunkDescs[0]), g_cube2ChunkDescs, sizeof(g_cube1BondDescs) / sizeof(g_cube1BondDescs[0]), g_cube1BondDescs },

	// 2x2x2 axis-aligned cube centered at the origin, split into 8 depth-1 1x1x1 child chunks with multiple roots
	{ sizeof(g_cube3ChunkDescs) / sizeof(g_cube3ChunkDescs[0]), g_cube3ChunkDescs, sizeof(g_cube3BondDescs) / sizeof(g_cube3BondDescs[0]), g_cube3BondDescs },
};



struct ExpectedValues
{
	uint32_t	totalChunkCount;
	uint32_t	graphNodeCount;
	uint32_t	leafChunkCount;
	uint32_t	bondCount;
	uint32_t	subsupportChunkCount;
};

const ExpectedAssetValues g_assetExpectedValues[3] =
{
//	  total	graph	leaves	bonds	sub
	{ 9,	8,		8,		12,		0	},
	{ 73,	8,		64,		12,		64	},
	{ 11,	8,		8,		12,		0	}
};


///////////// Badly-formed asset descs below //////////////

const NvBlastChunkDesc g_cube1ChunkDescsMissingCoverage[9] =
{
//    centroid              volume  parent idx    flags                             ID
	{ { 0.0f, 0.0f, 0.0f }, 8.0f,   UINT32_MAX,   NvBlastChunkDesc::NoFlags,        0 },
	{ {-0.5f,-0.5f,-0.5f }, 1.0f,   0,            NvBlastChunkDesc::SupportFlag,    1 },
	{ { 0.5f,-0.5f,-0.5f }, 1.0f,   0,            NvBlastChunkDesc::SupportFlag,    2 },
	{ {-0.5f, 0.5f,-0.5f }, 1.0f,   0,            NvBlastChunkDesc::SupportFlag,    3 },
	{ { 0.5f, 0.5f,-0.5f }, 1.0f,   0,            NvBlastChunkDesc::SupportFlag,    4 },
	{ {-0.5f,-0.5f, 0.5f }, 1.0f,   0,            NvBlastChunkDesc::SupportFlag,    5 },
	{ { 0.5f,-0.5f, 0.5f }, 1.0f,   0,            NvBlastChunkDesc::SupportFlag,    6 },
	{ {-0.5f, 0.5f, 0.5f }, 1.0f,   0,            NvBlastChunkDesc::SupportFlag,    7 },
	{ { 0.5f, 0.5f, 0.5f }, 1.0f,   0,            NvBlastChunkDesc::NoFlags,        8 },
};


const NvBlastChunkDesc g_cube2ChunkDescsMissingCoverage1[17] =
{
//    centroid                                volume  parent idx    flags                             ID
	{ { 0.0f, 0.0f, 0.0f },                   8.0f,   UINT32_MAX,   NvBlastChunkDesc::NoFlags,		  0  },
	{ {-0.5f,-0.5f,-0.5f },                   1.0f,   0,            NvBlastChunkDesc::NoFlags,		  1  },
	{ { 0.5f,-0.5f,-0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    2  },
	{ {-0.5f, 0.5f,-0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    3  },
	{ { 0.5f, 0.5f,-0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    4  },
	{ {-0.5f,-0.5f, 0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    5  },
	{ { 0.5f,-0.5f, 0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    6  },
	{ {-0.5f, 0.5f, 0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    7  },
	{ { 0.5f, 0.5f, 0.5f },                   1.0f,   0,            NvBlastChunkDesc::NoFlags,        8  },
	{ {-0.25f-0.5f,-0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        9  },
	{ { 0.25f-0.5f,-0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        10 },
	{ {-0.25f-0.5f, 0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        11 },
	{ { 0.25f-0.5f, 0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        12 },
	{ {-0.25f-0.5f,-0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        13 },
	{ { 0.25f-0.5f,-0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        14 },
	{ {-0.25f-0.5f, 0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        15 },
	{ { 0.25f-0.5f, 0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        16 },
};


const NvBlastChunkDesc g_cube2ChunkDescsMissingCoverage2[17] =
{
//    centroid                                volume  parent idx    flags                             ID
	{ { 0.0f, 0.0f, 0.0f },                   8.0f,   UINT32_MAX,   NvBlastChunkDesc::NoFlags,        0  },
	{ {-0.5f,-0.5f,-0.5f },                   1.0f,   0,            NvBlastChunkDesc::NoFlags,        1  },
	{ { 0.5f,-0.5f,-0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    2  },
	{ {-0.5f, 0.5f,-0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    3  },
	{ { 0.5f, 0.5f,-0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    4  },
	{ {-0.5f,-0.5f, 0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    5  },
	{ { 0.5f,-0.5f, 0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    6  },
	{ {-0.5f, 0.5f, 0.5f },                   1.0f,   0,            NvBlastChunkDesc::SupportFlag,    7  },
	{ { 0.5f, 0.5f, 0.5f },                   1.0f,   0,            NvBlastChunkDesc::NoFlags,        8  },
	{ {-0.25f-0.5f,-0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        9  },
	{ { 0.25f-0.5f,-0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        10 },
	{ {-0.25f-0.5f, 0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        11 },
	{ { 0.25f-0.5f, 0.25f-0.5f,-0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        12 },
	{ {-0.25f-0.5f,-0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        13 },
	{ { 0.25f-0.5f,-0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        14 },
	{ {-0.25f-0.5f, 0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::NoFlags,        15 },
	{ { 0.25f-0.5f, 0.25f-0.5f, 0.25f-0.5f }, 0.125f, 1,            NvBlastChunkDesc::SupportFlag,    16 },
};


const NvBlastAssetDesc g_assetDescsMissingCoverage[3] =
{
	{ sizeof(g_cube1ChunkDescsMissingCoverage) / sizeof(g_cube1ChunkDescsMissingCoverage[0]), g_cube1ChunkDescsMissingCoverage, sizeof(g_cube1BondDescs) / sizeof(g_cube1BondDescs[0]), g_cube1BondDescs },
	{ sizeof(g_cube2ChunkDescsMissingCoverage1) / sizeof(g_cube2ChunkDescsMissingCoverage1[0]), g_cube2ChunkDescsMissingCoverage1, sizeof(g_cube1BondDescs) / sizeof(g_cube1BondDescs[0]), g_cube1BondDescs },
	{ sizeof(g_cube2ChunkDescsMissingCoverage2) / sizeof(g_cube2ChunkDescsMissingCoverage2[0]), g_cube2ChunkDescsMissingCoverage2, sizeof(g_cube1BondDescs) / sizeof(g_cube1BondDescs[0]), g_cube1BondDescs },
};

extern const ExpectedAssetValues g_assetsFromMissingCoverageExpectedValues[3] =
{
//	  total	graph	leaves	bonds	sub
	{ 9,	8,		8,		12,		0	},
	{ 17,	8,		15,		12,		8	},
	{ 17,	15,		15,		9,		0	},
};


void generateCube(GeneratorAsset& cubeAsset, size_t maxDepth, size_t width, int32_t supportDepth)
{
	CubeAssetGenerator::Settings settings;
	settings.extents = GeneratorAsset::Vec3(1, 1, 1);
	CubeAssetGenerator::DepthInfo depthInfo;
	depthInfo.slicesPerAxis = GeneratorAsset::Vec3(1, 1, 1);
	depthInfo.flag = NvBlastChunkDesc::Flags::NoFlags;
	settings.depths.push_back(depthInfo);

	for (uint32_t depth = 1; depth < maxDepth; ++depth)
	{
		depthInfo.slicesPerAxis = GeneratorAsset::Vec3((float)width, (float)width, (float)width);
		settings.depths.push_back(depthInfo);
	}
	settings.depths[(supportDepth > 0 ? supportDepth : maxDepth) - 1].flag = NvBlastChunkDesc::SupportFlag;	// Leaves are support

	CubeAssetGenerator::generate(cubeAsset, settings);
}

void generateCube(GeneratorAsset& cubeAsset, NvBlastAssetDesc& assetDesc, size_t maxDepth, size_t width, int32_t supportDepth)
{
	generateCube(cubeAsset, maxDepth, width, supportDepth);
	assetDesc.bondCount = (uint32_t)cubeAsset.solverBonds.size();
	assetDesc.bondDescs = cubeAsset.solverBonds.data();
	assetDesc.chunkCount = (uint32_t)cubeAsset.chunks.size();
	assetDesc.chunkDescs = cubeAsset.solverChunks.data();
}