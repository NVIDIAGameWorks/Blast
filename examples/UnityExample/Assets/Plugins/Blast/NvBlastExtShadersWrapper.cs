using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

[StructLayout(LayoutKind.Sequential)]
public class NvBlastExtRadialDamageDesc
{
	public float damage;       //!<	normalized damage amount, range: [0, 1] (maximum health value to be reduced)
    public float p0;
    public float p1;
    public float p2;
    public float minRadius;		//!<	inner radius of damage action
    public float maxRadius;		//!<	outer radius of damage action
};

[StructLayout(LayoutKind.Sequential)]
public class NvBlastExtMaterial
{
	public float health;                   //!<	health
	public float minDamageThreshold;       //!<	min damage fraction threshold to be applied. Range [0, 1]. For example 0.1 filters all damage below 10% of health.
	public float maxDamageThreshold;       //!<	max damage fraction threshold to be applied. Range [0, 1]. For example 0.8 won't allow more then 80% of health damage to be applied.
};

public static class NvBlastExtShadersWrapper
{
    public const string DLL_NAME = "NvBlastExtShaders" + NvBlastWrapper.DLL_POSTFIX + "_" + NvBlastWrapper.DLL_PLATFORM;

    #region Dll
    [DllImport(DLL_NAME)]
	public static extern void NvBlastExtFalloffGraphShader( NvBlastFractureBuffers buffers, NvBlastGraphShaderActor actor, NvBlastExtProgramParams p );    // System.IntPtr xparams

	[DllImport(DLL_NAME)]
	public static extern void NvBlastExtFalloffSubgraphShader( NvBlastFractureBuffers buffers, NvBlastSubgraphShaderActor actor, NvBlastExtProgramParams p );    // NvBlastExtProgramParams
	#endregion

}

