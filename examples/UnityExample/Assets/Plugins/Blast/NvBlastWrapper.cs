using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Assertions;

public struct NvBlastChunkDesc
{
    public enum Flags
    {
        NoFlags = 0,
        SupportFlag = (1 << 0)
    };

	/** Central position in chunk. */
	public Single c0;
    public Single c1;
    public Single c2;


    /** Volume of chunk. */
    public Single volume;

    /** Index of this chunk's parent.  If this is a root chunk, then this value must be UINT32_MAX. */
    public UInt32 parentChunkIndex;

    /** See Flags enum for possible flags. */
    public UInt32 flags;

    /** User-supplied data which will be accessible to the user in chunk fracture events. */
    public UInt32 userData;
}

public struct NvBlastBond
{
    public Single n0;
    public Single n1;
    public Single n2;

    public Single area;

    public Single c0;
    public Single c1;
    public Single c2;

    UInt32 userData;
};

public struct NvBlastBondDesc
{
	/** Bond data (see NvBlastBond). */
	public NvBlastBond bond;

	/** The indices of the chunks linked by this bond.  They must be different support chunk indices. */
	public UInt32 chunk0;
    public UInt32 chunk1;
}

[StructLayout(LayoutKind.Sequential)]
public class NvBlastAssetDesc
{
    public UInt32 chunkCount;
    public NvBlastChunkDesc[] chunkDescs;
    public UInt32 bondCount;
    public NvBlastBondDesc[] bondDescs;
}

/**
Actor descriptor, used to create an instance of an NvBlastAsset with NvBlastActorCreate
    6
See NvBlastActorCreate.
*/
[StructLayout(LayoutKind.Sequential)]
public class NvBlastActorDesc
{
    /**
	Initial health of all bonds, if initialBondHealths is NULL (see initialBondHealths).
	*/
    public Single uniformInitialBondHealth;

    /**
	Initial bond healths.  If not NULL, this array must be of length NvBlastAssetGetChunkCount(asset, ... ).
	If NULL, uniformInitialBondHealth must be set.
	*/
    public Single[] initialBondHealths = null;

    /**
	Initial health of all lower-support chunks, if initialSupportChunkHealths is NULL (see initialSupportChunkHealths).
	*/
    public Single uniformInitialLowerSupportChunkHealth;

    /**
	Initial health of all support chunks.  If not NULL, this must be of length
	NvBlastAssetGetSupportGraph(asset, ... ).nodeCount. The elements in the initialSupportChunkHealth
	array will correspond to the chunk indices in the NvBlastAssetGetSupportGraph(asset, ... ).chunkIndices
	array.  Every descendent of a support chunk will have its health initialized to its ancestor support
	chunk's health, so this initializes all lower-support chunk healths.
	If NULL, uniformInitialLowerSupportChunkHealth must be set.
	*/
    public Single[] initialSupportChunkHealths = null;
};


public struct NvBlastChunk
{
    /**
	Central position for the chunk's volume
	*/
    public Single c0;
    public Single c1;
    public Single c2;

    /**
	Volume of the chunk
	*/
    public Single volume;

    /**
	Index of parent (UINT32_MAX denotes no parent)
	*/
    public UInt32 parentChunkIndex;

    /**
	Index of first child
	*/
    public UInt32 firstChildIndex;

    /**
	Stop for child indices
	*/
    public UInt32 childIndexStop;

    /**
	Field for user to associate with external data
	*/
    public UInt32 userData;
};


public struct NvBlastSupportGraph
{
    UInt32 nodeCount;
    public IntPtr chunkIndices;
    public IntPtr adjacencyPartition;
    public IntPtr adjacentNodeIndices;
    public IntPtr adjacentBondIndices;
};

[StructLayout(LayoutKind.Sequential)]
public class NvBlastTimers
{
    public Int64 material;       //!< Time spent in material function
    public Int64 fracture;       //!< Time spent applying damage
    public Int64 island;     //!< Time spent discovering islands
    public Int64 partition;      //!< Time spent partitioning the graph
    public Int64 visibility;	//!< Time spent updating visibility
};


///////////////////////////////////////////////////////////////////////////////
//	Types used for damage and fracturing
///////////////////////////////////////////////////////////////////////////////

public struct NvBlastChunkFractureData
{
    public UInt32 userdata;  //!<	chunk's user data
    public UInt32 chunkIndex;    //!<	asset chunk index
    public Single health;		//!<	health value (damage or remains)
};

public struct NvBlastBondFractureData
{
    public UInt32 userdata;  //!<	bond's user data
    public UInt32 nodeIndex0;    //!<	graph node index of bond
    public UInt32 nodeIndex1;    //!<	pair graph node index of bond
    public Single health;		//!<	health value (damage or remains)
};

[StructLayout(LayoutKind.Sequential)]
public class NvBlastFractureBuffers
{
    public UInt32 bondFractureCount;
    public UInt32 chunkFractureCount;
    public IntPtr bondFractures;        // NvBlastBondFractureData[]
    public IntPtr chunkFractures;       // NvBlastChunkFractureData[]
};

[StructLayout(LayoutKind.Sequential)]
public class NvBlastActorSplitEvent
{
    public IntPtr deletedActor; //!<	deleted actor or nullptr if actor has not changed
    public IntPtr newActors;		//!<	list of created actors ( NvBlastActor** )
};

[StructLayout(LayoutKind.Sequential)]
public class NvBlastGraphShaderActor
{
	UInt32 actorIndex;						//!<	Actor's index.
	UInt32 graphNodeCount;					//!<	Actor's graph node count.
	UInt32 assetNodeCount;					//!<	Asset node count.
	UInt32 firstGraphNodeIndex;				//<!	Entry index for graphNodeIndexLinks
    public UInt32[] graphNodeIndexLinks;    //<!	Linked index list of connected nodes.  Traversable with nextIndex = graphNodeIndexLinks[currentIndex], terminates with 0xFFFFFFFF.
    public UInt32[] chunkIndices;           //<!	Graph's map from node index to support chunk index.
    public UInt32[] adjacencyPartition;     //<!	See NvBlastChunkGraph::adjacencyPartition.
    public UInt32[] adjacentNodeIndices;    //<!	See NvBlastChunkGraph::adjacentNodeIndices.
    public UInt32[] adjacentBondIndices;    //<!	See NvBlastChunkGraph::adjacentBondIndices.
    public NvBlastBond[] assetBonds;        //<!	NvBlastBonds geometry in the NvBlastAsset.
	public NvBlastChunk[] assetChunks;      //!<	NvBlastChunks geometry in the NvBlastAsset.
	public Single[] familyBondHealths;      //<!	Actual bond health values for broken bond detection.
	public Single[] supportChunkHealths;    //!<	Actual chunk health values for dead chunk detection.
	public UInt32[] nodeActorIndices;       //!<	Family's map from node index to actor index.
};

[StructLayout(LayoutKind.Sequential)]
public struct NvBlastExtProgramParams
{
	public IntPtr damageDescBuffer;                     //!<	array of damage descriptions	// (NvBlastExtRadialDamageDesc)
	public IntPtr material;                             //!<	pointer to material
	public IntPtr accelerator;                          //!<	//NvBlastExtDamageAccelerator* accelerator;
};

[StructLayout(LayoutKind.Sequential)]
public class NvBlastSubgraphShaderActor
{
    public UInt32 chunkIndex;        //<!	Index of chunk represented by this actor.
    public NvBlastChunk[] assetChunks;	//<!	NvBlastChunks geometry in the NvBlastAsset.
};

public struct NvBlastDamageProgram
{
    public delegate void NvBlastGraphShaderFunction( NvBlastFractureBuffers commandBuffers, NvBlastGraphShaderActor actor, NvBlastExtProgramParams p );  // System.IntPtr p
	public delegate void NvBlastSubgraphShaderFunction( NvBlastFractureBuffers commandBuffers, NvBlastSubgraphShaderActor actor, NvBlastExtProgramParams p);

	public NvBlastGraphShaderFunction graphShaderFunction;
    public NvBlastSubgraphShaderFunction subgraphShaderFunction;
};


public class NvBlastWrapper
{
    //////// DLL ////////

//	public const string DLL_POSTFIX = "DEBUG";
	public const string DLL_POSTFIX = "";
	public const string DLL_PLATFORM = "x64";
    public const string DLL_NAME = "NvBlast" + DLL_POSTFIX + "_" + DLL_PLATFORM;


    //////// Internal Types ////////

    public delegate IntPtr NvBlastAlloc(Int64 size);
    public delegate void NvBlastFree(IntPtr ptr);
    public delegate void NvBlastLog(Int32 type, string msg, string file, Int32 line);


    //////// Public Types ////////



    //////// Helpers ////////

    public static IntPtr Alloc(Int64 size)
    {
        return Marshal.AllocHGlobal((Int32)size);
    }

    public static void Free(IntPtr ptr)
    {
        Marshal.FreeHGlobal(ptr);
    }

    public static void Log(Int32 type, string msg, string file, Int32 line)
    {
        Debug.Log(DLL_NAME + ": [" + type + "] " + msg + "(" + file + ":" + line + ")");
    }

    public static IntPtr GetScratch(int size)
    {
        return _greedyScratch.GetScratch(size);
    }

    private static GreedyScratch _greedyScratch = new GreedyScratch();
}


public class GreedyScratch : IDisposable
{
    public IntPtr GetScratch(int size)
    {
        if (_size < size)
        {
            releaseScratch();
            _scratch = Marshal.AllocHGlobal(size);
            _size = size;
        }

        return _scratch;
    }

    private void releaseScratch()
    {
        if (_size > 0)
        {
            Marshal.FreeHGlobal(_scratch);
            _scratch = IntPtr.Zero;
        }
    }

    public void Dispose()
    {
        Dispose(true);
    }

    protected virtual void Dispose(bool bDisposing)
    {
        releaseScratch();

        if (bDisposing)
        {
            GC.SuppressFinalize(this);
        }
    }

    ~GreedyScratch()
    {
        Dispose(false);
    }

    private int _size = 0;
    private IntPtr _scratch = IntPtr.Zero;
}


public abstract class DisposablePtr : IDisposable
{
    protected void Initialize(IntPtr ptr)
    {
        Assert.IsTrue(this._ptr == IntPtr.Zero);
        this._ptr = ptr;
    }

    protected void ResetPtr()
    {
        this._ptr = IntPtr.Zero;
    }

    protected abstract void Release();

    public IntPtr ptr
    {
        get { return _ptr; }
    }


    public void Dispose()
    {
        Dispose(true);
    }

    protected virtual void Dispose(bool bDisposing)
    {
        if (_ptr != IntPtr.Zero)
        {
            Release();
            _ptr = IntPtr.Zero;
        }

        if (bDisposing)
        {
            GC.SuppressFinalize(this);
        }
    }

    ~DisposablePtr()
    {
        Dispose(false);
    }

    private IntPtr _ptr = IntPtr.Zero;
}


public class NvBlastAsset : DisposablePtr
{
    #region Dll
    [DllImport(NvBlastWrapper.DLL_NAME)]
	static extern UInt64 NvBlastGetRequiredScratchForCreateAsset(NvBlastAssetDesc desc, NvBlastWrapper.NvBlastLog logFn);

	[DllImport(NvBlastWrapper.DLL_NAME)]
	static extern UInt64 NvBlastGetAssetMemorySize( NvBlastAssetDesc desc, NvBlastWrapper.NvBlastLog logFn );

    [DllImport(NvBlastWrapper.DLL_NAME)]
	static extern IntPtr NvBlastCreateAsset( System.IntPtr mem, NvBlastAssetDesc desc, IntPtr scratch, NvBlastWrapper.NvBlastLog logFn);

//	[DllImport(NvBlastWrapper.DLL_NAME)]
//    static extern void NvBlastAssetRelease(IntPtr asset, NvBlastWrapper.NvBlastFree freeFn, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern UInt32 NvBlastAssetGetLeafChunkCount(IntPtr asset, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern NvBlastSupportGraph NvBlastAssetGetSupportGraph(IntPtr asset, NvBlastWrapper.NvBlastLog logFn);
    #endregion

    public NvBlastAsset( NvBlastAssetDesc desc )
    {
        var scratchSize = NvBlastGetRequiredScratchForCreateAsset( desc, NvBlastWrapper.Log );
		var assetSize = NvBlastGetAssetMemorySize(desc, NvBlastWrapper.Log);
		IntPtr mem = NvBlastWrapper.Alloc( (long)assetSize);
		var asset = NvBlastCreateAsset( mem, desc, NvBlastWrapper.GetScratch( (int)scratchSize ), NvBlastWrapper.Log );
        Initialize( asset );
    }
	// asset = mem, or NULL on failure

	protected override void Release()
    {
        //NvBlastAssetRelease( ptr, NvBlastWrapper.Free, NvBlastWrapper.Log );
    }

    public UInt32 leafChunkCount 
    {
        get 
        {
            return NvBlastAssetGetLeafChunkCount(ptr, NvBlastWrapper.Log);
        }
    }

    public NvBlastSupportGraph chunkGraph
    {
        get
        {
            if (!_graph.HasValue)
            {
                _graph = NvBlastAssetGetSupportGraph(ptr, NvBlastWrapper.Log);
            }
            return _graph.Value;
        }
    }

    private NvBlastSupportGraph? _graph = null;
}


public class NvBlastFamily : DisposablePtr
{
    #region Dll
    [DllImport(NvBlastWrapper.DLL_NAME)]
	static extern IntPtr NvBlastAssetCreateFamily( IntPtr mem, IntPtr asset, NvBlastWrapper.NvBlastLog logFn );

//    [DllImport(NvBlastWrapper.DLL_NAME)]
//    static extern void NvBlastFamilyRelease(IntPtr family, NvBlastWrapper.NvBlastFree freeFn, NvBlastWrapper.NvBlastLog logFn);

	[DllImport(NvBlastWrapper.DLL_NAME)]
	static extern UInt64 NvBlastAssetGetFamilyMemorySize( IntPtr asset, NvBlastWrapper.NvBlastLog logFn );
	#endregion

	public NvBlastAsset asset
    {
        get;
        private set;
    }

    public NvBlastFamily(NvBlastAsset asset_)
    {
		var memSize = NvBlastAssetGetFamilyMemorySize(asset_.ptr, NvBlastWrapper.Log);
		IntPtr mem = NvBlastWrapper.Alloc((long)memSize );
		asset = asset_;
        var family = NvBlastAssetCreateFamily( mem, asset.ptr, NvBlastWrapper.Log );
		Initialize( family );
    }

	protected override void Release()
    {
        //NvBlastFamilyRelease( ptr, NvBlastWrapper.Free, NvBlastWrapper.Log );
    }
}


public class NvBlastActor : DisposablePtr
{
    #region Dll
    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern UInt64 NvBlastFamilyGetRequiredScratchForCreateFirstActor(IntPtr asset, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern IntPtr NvBlastFamilyCreateFirstActor(IntPtr family, NvBlastActorDesc desc, IntPtr scratch, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern void NvBlastActorRelease(IntPtr actor);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern UInt32 NvBlastActorGetVisibleChunkCount(IntPtr actor, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern UInt32 NvBlastActorGetVisibleChunkIndices([In, Out] UInt32[] visibleChunkIndices, UInt32 visibleChunkIndicesSize, IntPtr actor, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern UInt32 NvBlastActorGetGraphNodeCount(IntPtr actor, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern UInt32 NvBlastActorGetGraphNodeIndices([In, Out] UInt32[] graphNodeIndices, UInt32 graphNodeIndicesSize, IntPtr actor, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern void NvBlastActorGenerateFracture(NvBlastFractureBuffers commandBuffers, IntPtr actor, NvBlastDamageProgram program, NvBlastExtProgramParams programParams, NvBlastWrapper.NvBlastLog logFn, NvBlastTimers timers);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern void NvBlastActorApplyFracture(IntPtr eventBuffers, IntPtr actor, NvBlastFractureBuffers commands, NvBlastWrapper.NvBlastLog logFn, NvBlastTimers timers);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern UInt64 NvBlastActorGetRequiredScratchForSplit(IntPtr actor, NvBlastWrapper.NvBlastLog logFn);

    [DllImport(NvBlastWrapper.DLL_NAME)]
    static extern UInt32 NvBlastActorSplit([In, Out] NvBlastActorSplitEvent result, IntPtr actor, UInt32 newActorsMaxCount, IntPtr scratch, NvBlastWrapper.NvBlastLog logFn, NvBlastTimers timers);
    #endregion

    public NvBlastFamily family
    {
        get;
        private set;
    }

    public object userData = null;

    public NvBlastActor(NvBlastFamily family_, NvBlastActorDesc desc)
    {
        family = family_;

        var scratchSize = NvBlastFamilyGetRequiredScratchForCreateFirstActor( family.ptr, NvBlastWrapper.Log);
        var actor = NvBlastFamilyCreateFirstActor( family.ptr, desc, NvBlastWrapper.GetScratch((int)scratchSize), NvBlastWrapper.Log );
		Initialize(actor);
    }

    public NvBlastActor(NvBlastFamily family_, IntPtr ptr)
    {
        family = family_;

        Initialize(ptr);
    }

    protected override void Release()
    {
        NvBlastActorRelease(ptr);
    }

    public UInt32 visibleChunkCount
    {
        get { return NvBlastActorGetVisibleChunkCount(ptr, NvBlastWrapper.Log); }
    }

    public UInt32[] visibleChunkIndices
    {
        get
        {
            if(_visibleChunkIndices == null)
            {
                _visibleChunkIndices = new UInt32[visibleChunkCount];
                NvBlastActorGetVisibleChunkIndices(_visibleChunkIndices, visibleChunkCount, ptr, NvBlastWrapper.Log);
            }
            return _visibleChunkIndices;
        }
    }

    public UInt32 graphNodeCount
    {
        get { return NvBlastActorGetGraphNodeCount(ptr, NvBlastWrapper.Log); }
    }

    public UInt32[] graphNodeIndices
    {
        get
        {
            if (_graphNodeIndices == null)
            {
                _graphNodeIndices = new UInt32[graphNodeCount];
                NvBlastActorGetGraphNodeIndices(_graphNodeIndices, graphNodeCount, ptr, NvBlastWrapper.Log);
            }
            return _graphNodeIndices;
        }
    }

    public void GenerateFracture( NvBlastFractureBuffers buffers, NvBlastDamageProgram program, NvBlastExtProgramParams programParams )
    {
        NvBlastActorGenerateFracture( buffers, ptr, program, programParams, NvBlastWrapper.Log, null );
    }

    public void ApplyFracture(NvBlastFractureBuffers commands)
    {
        NvBlastActorApplyFracture(IntPtr.Zero, ptr, commands, NvBlastWrapper.Log, null);
    }

    public UInt32 Split( NvBlastActorSplitEvent result, UInt32 newActorsMaxCount )
    {
        var scratchSize = NvBlastActorGetRequiredScratchForSplit(ptr, NvBlastWrapper.Log);
        UInt32 newActorsCount = NvBlastActorSplit( result, ptr, newActorsMaxCount, NvBlastWrapper.GetScratch((int)scratchSize), NvBlastWrapper.Log, null);
        if( result.deletedActor != IntPtr.Zero )
        {
            ResetPtr();
        }
        return newActorsCount;
    }

    private UInt32[] _visibleChunkIndices = null;
    private UInt32[] _graphNodeIndices = null;
}

