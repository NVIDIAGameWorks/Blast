using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

[StructLayout(LayoutKind.Sequential)]
public class NvBlastExtRadialDamageDesc
{
    public float compressive;  //!<	compressive (radial) damage component
    public float p0;
    public float p1;
    public float p2;
    public float minRadius;        //!<	inner radius of damage action
    public float maxRadius;		//!<	outer radius of damage action
};

[StructLayout(LayoutKind.Sequential)]
public class NvBlastExtMaterial
{
    public float singleChunkThreshold;                     //!<	subsupport chunks only take damage surpassing this value
    public float graphChunkThreshold;                      //!<	support chunks only take damage surpassing this value
    public float bondTangentialThreshold;                  //!<	bond only take damage surpassing this value
    public float bondNormalThreshold;                      //!<	currently unused - forward damage propagation
    public float damageAttenuation;						   //!<	factor of damage attenuation while forwarding
};


public static class NvBlastExtShadersWrapper
{
    public const string DLL_NAME = "NvBlastExtShaders" + NvBlastWrapper.DLL_POSTFIX + "_" + NvBlastWrapper.DLL_PLATFORM;

    #region Dll
    [DllImport(DLL_NAME)]
    private static extern bool NvBlastExtDamageActorRadialFalloff(IntPtr actor, NvBlastFractureBuffers buffers, NvBlastExtRadialDamageDesc damageDescBuffer, UInt32 damageDescCount, NvBlastExtMaterial material, NvBlastWrapper.NvBlastLog logFn, NvBlastTimers timers);
    #endregion

    public static bool DamageRadialFalloff(this NvBlastActor actor, NvBlastFractureBuffers buffers, NvBlastExtRadialDamageDesc damageDescBuffer, UInt32 damageDescCount, NvBlastExtMaterial material)
    {
        return NvBlastExtDamageActorRadialFalloff(actor.ptr, buffers, damageDescBuffer, damageDescCount, material, NvBlastWrapper.Log, null);
    }
}