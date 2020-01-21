using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;


public static class NvBlastExtUtilsWrapper
{
	//!AJB 20180809	Function was moved to a different plug in
	public const string DLL_NAME = "NvBlast" + NvBlastWrapper.DLL_POSTFIX + "_" + NvBlastWrapper.DLL_PLATFORM;      // NvBlastExtAssetUtils

	#region Dll
	[DllImport(DLL_NAME)]
	private static extern void NvBlastReorderAssetDescChunks
	(
		[In, Out] NvBlastChunkDesc[] chunkDescs,
		UInt32 chunkCount,
		[In, Out] NvBlastBondDesc[] bondDescs,
		UInt32 bondCount,
		UInt32[] chunkReorderMap,
		bool keepBondNormalChunkOrder,
		System.IntPtr scratch,
		System.UIntPtr logFn        // NvBlastLog, may be null
	);
	#endregion

	public static void ReorderAssetDescChunks(NvBlastAssetDesc assetDesc, uint[] chunkReorderMap)
    {
		System.IntPtr scratchPtr = NvBlastWrapper.GetScratch( (int)( assetDesc.chunkCount * Marshal.SizeOf( typeof(NvBlastChunkDesc) ) ) );
		NvBlastReorderAssetDescChunks(assetDesc.chunkDescs, assetDesc.chunkCount, assetDesc.bondDescs, assetDesc.bondCount, chunkReorderMap, true, scratchPtr, System.UIntPtr.Zero);
	}
}


