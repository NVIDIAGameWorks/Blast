using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;


public class CubeFamily : MonoBehaviour
{
    public void Initialize(CubeAsset asset)
    {
        // Blast asset creation
        _cubeAsset = asset;

        NvBlastAssetDesc desc = _cubeAsset.solverAssetDesc;
        _blastAsset = new NvBlastAsset(desc);
//        Debug.Log(_blastAsset.leafChunkCount);

        // Actual Cubes
        var cubePrefab = Resources.Load<GameObject>("CubePrefab");
        _cubes = new GameObject[desc.chunkCount];
        for (int i = 0; i < desc.chunkCount; ++i)
        {
            GameObject cube = GameObject.Instantiate<GameObject>(cubePrefab);
            cube.transform.parent = transform;
            cube.transform.localScale = _cubeAsset.chunks[i].extents;
            cube.transform.localPosition = _cubeAsset.chunks[i].position;
            cube.transform.localRotation = Quaternion.identity;
            cube.SetActive(false);
            Color color = Color.HSVToRGB(UnityEngine.Random.Range(0f, 1f), 0.42f, 1.0f);
            cube.GetComponent<Renderer>().material.color = color;
            _cubes[i] = cube;
        }

        // First actor
        _blastFamily = new NvBlastFamily(_blastAsset);

        NvBlastActorDesc actorDesc = new NvBlastActorDesc();
        actorDesc.uniformInitialBondHealth = 1.0f;
        actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
        var actor = new NvBlastActor(_blastFamily, actorDesc);
//        Debug.Log(actor.visibleChunkCount);

        OnActorCreated(actor, Vector3.zero, Quaternion.identity);

        // Reserved buffers
        _fractureBuffers = new NvBlastFractureBuffers();
        _fractureBuffers.chunkFractures = Marshal.AllocHGlobal((int)desc.chunkCount * Marshal.SizeOf(typeof(NvBlastChunkFractureData)));
        _fractureBuffers.bondFractures = Marshal.AllocHGlobal((int)desc.bondCount * Marshal.SizeOf(typeof(NvBlastBondFractureData)));
        _leafChunkCount = (uint)_blastAsset.leafChunkCount;
        _newActorsBuffer = Marshal.AllocHGlobal((int)_leafChunkCount * Marshal.SizeOf(typeof(IntPtr)));
    }

    private void OnActorCreated(NvBlastActor actor, Vector3 localPosition, Quaternion localRotation)
    {
        var rigidBodyGO = new GameObject("RigidActor");
        rigidBodyGO.transform.SetParent(this.transform, false);
        var rigidbody = rigidBodyGO.AddComponent<Rigidbody>();
        rigidbody.transform.localPosition = localPosition;
        rigidbody.transform.localRotation = localRotation;

        // chunks
        var chunkIndices = actor.visibleChunkIndices;
        foreach (var chunkIndex in chunkIndices)
        {
            var chunkCube = _cubes[chunkIndex];
            chunkCube.transform.SetParent(rigidbody.transform, false);
            chunkCube.SetActive(true);
        }

        // search for static chunks
        var graphNodeIndices = actor.graphNodeIndices;
        var chunkGraph = _blastAsset.chunkGraph;
        foreach(var node in graphNodeIndices)
        {
            var chunkIndex = Marshal.ReadInt32(chunkGraph.chunkIndices, Marshal.SizeOf(typeof(UInt32)) * (int)node);
            var chunkCube = _cubeAsset.chunks[chunkIndex];
            if(chunkCube.isStatic)
            {
                rigidbody.isKinematic = true;
                break;
            }
        }

        actor.userData = rigidbody;
        _actors.Add(rigidbody, actor);
    }

    private void OnActorDestroyed(NvBlastActor actor)
    {
        var chunkIndices = actor.visibleChunkIndices;
        foreach (var chunkIndex in chunkIndices)
        {
            var chunkCube = _cubes[chunkIndex];
            chunkCube.transform.SetParent(transform, false);
            chunkCube.SetActive(false);
        }

        var rigidbody = (actor.userData as Rigidbody);
        _actors.Remove(rigidbody);
        Destroy(rigidbody.gameObject);
        actor.userData = null;
    }

    public void ApplyRadialDamage(Vector3 position, float minRadius, float maxRadius, float compressive)
    {
        var hits = Physics.OverlapSphere(position, maxRadius);
        foreach (var hit in hits)
        {
            var rb = hit.GetComponentInParent<Rigidbody>();
            if (rb != null)
            {
                ApplyRadialDamage(rb, position, minRadius, maxRadius, compressive);
            }
        }
    }

    public bool ApplyRadialDamage(Rigidbody rb, Vector3 position, float minRadius, float maxRadius, float compressive)
    {
        if (_actors.ContainsKey(rb))
        {
            Vector3 localPosition = rb.transform.InverseTransformPoint(position);
            ApplyRadialDamage(_actors[rb], localPosition, minRadius, maxRadius, compressive);
            return true;
        }
        return false;
    }

    private void ApplyRadialDamage( NvBlastActor actor, Vector3 localPosition, float minRadius, float maxRadius, float compressive )
    {
        _fractureBuffers.chunkFractureCount = _cubeAsset.solverAssetDesc.chunkCount;
        _fractureBuffers.bondFractureCount = _cubeAsset.solverAssetDesc.bondCount;

        NvBlastExtRadialDamageDesc desc = new NvBlastExtRadialDamageDesc();
        desc.minRadius = minRadius;
        desc.maxRadius = maxRadius;
        desc.damage = compressive;
        desc.p0 = localPosition.x;
        desc.p1 = localPosition.y;
        desc.p2 = localPosition.z;

		IntPtr dam = Marshal.AllocHGlobal( Marshal.SizeOf( typeof(NvBlastExtRadialDamageDesc) ) );
		Marshal.StructureToPtr( desc, dam, false );

		var damP = new NvBlastDamageProgram() {
			graphShaderFunction = NvBlastExtShadersWrapper.NvBlastExtFalloffGraphShader,
			subgraphShaderFunction = NvBlastExtShadersWrapper.NvBlastExtFalloffSubgraphShader
			};
		var programParams = new NvBlastExtProgramParams() {
			damageDescBuffer = dam,
			material = IntPtr.Zero,
			accelerator = IntPtr.Zero
		};

		actor.GenerateFracture( _fractureBuffers, damP, programParams );
		actor.ApplyFracture( _fractureBuffers );
		if ( _fractureBuffers.bondFractureCount + _fractureBuffers.chunkFractureCount > 0 )
		{
			Split( actor );
		}

		Marshal.FreeHGlobal(dam);
    }

    private void Split( NvBlastActor actor )
    {
        NvBlastActorSplitEvent split = new NvBlastActorSplitEvent();
        split.newActors = _newActorsBuffer;
        var count = actor.Split(split, _leafChunkCount);

        Vector3 localPosition = Vector3.zero;
        Quaternion localRotation = Quaternion.identity;

        if (split.deletedActor != IntPtr.Zero)
        {
            if (actor.userData != null)
            {
                var parentRigidbody = (actor.userData as Rigidbody);
                localPosition = parentRigidbody.transform.localPosition;
                localRotation = parentRigidbody.transform.localRotation;
            }
            OnActorDestroyed(actor);
        }
        for (int i = 0; i < count; i++)
        {
            int elementSize = Marshal.SizeOf(typeof(IntPtr));
            var ptr = Marshal.ReadIntPtr(split.newActors, elementSize * i);
            OnActorCreated(new NvBlastActor(_blastFamily, ptr), localPosition, localRotation);
        }
    }

    private void OnDestroy()
    {
        if (_fractureBuffers != null)
        {
            Marshal.FreeHGlobal(_fractureBuffers.chunkFractures);
            Marshal.FreeHGlobal(_fractureBuffers.bondFractures);
            _fractureBuffers = null;
        }

        if (_newActorsBuffer != IntPtr.Zero)
        {
            Marshal.FreeHGlobal(_newActorsBuffer);
        }

        _actors.Clear();
        _blastFamily = null;
        _blastAsset = null;
    }

    private CubeAsset _cubeAsset;
    private NvBlastAsset _blastAsset;
    private NvBlastFamily _blastFamily;
    private Dictionary<Rigidbody, NvBlastActor> _actors = new Dictionary<Rigidbody, NvBlastActor>();
    private GameObject[] _cubes;
    private NvBlastFractureBuffers _fractureBuffers;
    private IntPtr _newActorsBuffer;
    private uint _leafChunkCount;
}