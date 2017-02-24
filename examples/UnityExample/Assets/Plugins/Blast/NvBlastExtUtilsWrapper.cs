using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;


public static class NvBlastExtUtilsWrapper
{
    public const string DLL_NAME = "NvBlastExtUtils" + NvBlastWrapper.DLL_POSTFIX + "_" + NvBlastWrapper.DLL_PLATFORM;

    #region Dll
    [DllImport(DLL_NAME)]
    private static extern void NvBlastReorderAssetDescChunks([In, Out] NvBlastChunkDesc[] chunkDescs, uint chunkCount, [In, Out] NvBlastBondDesc[] bondDescs, uint bondCount, [In, Out] uint[] chunkReorderMap);
    #endregion

    public static void ReorderAssetDescChunks(NvBlastAssetDesc assetDesc, uint[] chunkReorderMap)
    {
        NvBlastReorderAssetDescChunks(assetDesc.chunkDescs, assetDesc.chunkCount, assetDesc.bondDescs, assetDesc.bondCount, chunkReorderMap);
    }
}