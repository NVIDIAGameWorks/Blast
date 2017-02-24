using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CubeAsset
{
    public struct DepthInfo
    {
        public DepthInfo(Vector3 slices, NvBlastChunkDesc.Flags flag_ = NvBlastChunkDesc.Flags.NoFlags)
        {
            this.slicesPerAxis = slices;
            this.flag = flag_;
        }

        public Vector3 slicesPerAxis;
        public NvBlastChunkDesc.Flags flag;
    };

    public enum BondFlags
    {
        NO_BONDS = 0,
        X_BONDS = 1,
        Y_BONDS = 2,
        Z_BONDS = 4,
        ALL_BONDS = X_BONDS | Y_BONDS | Z_BONDS
    };

    public class Settings
    {
        public List<DepthInfo> depths = new List<DepthInfo>();
        public Vector3 extents;
        public BondFlags bondFlags = BondFlags.ALL_BONDS;
        public float staticHeight = float.NegativeInfinity;
    };

    public struct BlastChunkCube
	{
		public BlastChunkCube(Vector3 position_, Vector3 extents_, bool isStatic_)
		{
			this.position = position_;
            this.extents = extents_;
            this.isStatic = isStatic_;
        }

        public Vector3 position;
        public Vector3 extents;
        public bool isStatic;
	};

    public List<BlastChunkCube> chunks = new List<BlastChunkCube>();
    public NvBlastAssetDesc solverAssetDesc = new NvBlastAssetDesc();
    public Vector3 extents { get; private set; }

    public static CubeAsset generate(Settings settings)
    {
        CubeAsset asset = new CubeAsset();
        asset.extents = settings.extents;

        List<NvBlastChunkDesc> solverChunks = new List<NvBlastChunkDesc>();
        List<NvBlastBondDesc> solverBonds = new List<NvBlastBondDesc>();

	    // initial params
	    List<uint> depthStartIDs = new List<uint>();
        List<Vector3> depthSlicesPerAxisTotal = new List<Vector3>();
        uint currentID = 0;
        Vector3 extents = settings.extents;

	    // Iterate over depths and create children
	    for (int depth = 0; depth<settings.depths.Count; depth++)
	    {
		    Vector3 slicesPerAxis = settings.depths[depth].slicesPerAxis;
            Vector3 slicesPerAxisTotal = (depth == 0) ? slicesPerAxis : Vector3.Scale(slicesPerAxis, (depthSlicesPerAxisTotal[depth - 1]));
            depthSlicesPerAxisTotal.Add(slicesPerAxisTotal);

		    depthStartIDs.Add(currentID);

		    extents.x /= slicesPerAxis.x;
		    extents.y /= slicesPerAxis.y;
		    extents.z /= slicesPerAxis.z;

		    for (uint z = 0; z< (uint)slicesPerAxisTotal.z; ++z)
		    {
			    uint parent_z = z / (uint)slicesPerAxis.z;
			    for (uint y = 0; y< (uint)slicesPerAxisTotal.y; ++y)
			    {
				    uint parent_y = y / (uint)slicesPerAxis.y;
				    for (uint x = 0; x< (uint)slicesPerAxisTotal.x; ++x)
				    {
					    uint parent_x = x / (uint)slicesPerAxis.x;
                        uint parentID = depth == 0 ? uint.MaxValue :
                        depthStartIDs[depth - 1] + parent_x + (uint)depthSlicesPerAxisTotal[depth - 1].x * (parent_y + (uint)depthSlicesPerAxisTotal[depth - 1].y * parent_z);

                        Vector3 position;
                        position.x = ((float)x - (slicesPerAxisTotal.x / 2) + 0.5f) * extents.x;
					    position.y = ((float)y - (slicesPerAxisTotal.y / 2) + 0.5f) * extents.y;
					    position.z = ((float)z - (slicesPerAxisTotal.z / 2) + 0.5f) * extents.z;

					    NvBlastChunkDesc chunkDesc;

                        chunkDesc.c0 = position.x;
                        chunkDesc.c1 = position.y;
                        chunkDesc.c2 = position.z;
					    chunkDesc.volume = extents.x * extents.y * extents.z;
                        chunkDesc.flags = (uint)settings.depths[depth].flag;
					    chunkDesc.userData = currentID++;
					    chunkDesc.parentChunkIndex = parentID;
					    solverChunks.Add(chunkDesc);

					    bool isStatic = false;

					    if (settings.depths[depth].flag == NvBlastChunkDesc.Flags.SupportFlag)
					    {
						    isStatic = position.y - (extents.y - asset.extents.y) / 2 <= settings.staticHeight;

						    // x-neighbor
						    if (x > 0 && (settings.bondFlags & BondFlags.X_BONDS) != 0)
						    {
							    Vector3 xNeighborPosition = position - new Vector3(extents.x, 0, 0);
                                uint neighborID = chunkDesc.userData - 1;

                                fillBondDesc(solverBonds, chunkDesc.userData, neighborID, position, xNeighborPosition, extents, extents.y* extents.z);
                            }

						    // y-neighbor
						    if (y > 0 && (settings.bondFlags & BondFlags.Y_BONDS) != 0)
						    {
							    Vector3 yNeighborPosition = position - new Vector3(0, extents.y, 0);
                                uint neighborID = chunkDesc.userData - (uint)slicesPerAxisTotal.x;

                                fillBondDesc(solverBonds, chunkDesc.userData, neighborID, position, yNeighborPosition, extents, extents.z* extents.x);
						    }

						    // z-neighbor
						    if (z > 0 && (settings.bondFlags & BondFlags.Z_BONDS) != 0)
						    {
							    Vector3 zNeighborPosition = position - new Vector3(0, 0, extents.z);
                                uint neighborID = chunkDesc.userData - (uint)slicesPerAxisTotal.x * (uint)slicesPerAxisTotal.y;

                                fillBondDesc(solverBonds, chunkDesc.userData, neighborID, position, zNeighborPosition, extents, extents.x* extents.y);
						    }
					    }

					    asset.chunks.Add(new BlastChunkCube(position, extents, isStatic));
				    }
			    }
		    }
	    }

        // Prepare solver asset desc
        asset.solverAssetDesc.chunkCount = (uint)solverChunks.Count;
        asset.solverAssetDesc.chunkDescs = solverChunks.ToArray();
        asset.solverAssetDesc.bondCount = (uint)solverBonds.Count;
        asset.solverAssetDesc.bondDescs = solverBonds.ToArray();

        // Reorder chunks
        uint[] chunkReorderMap = new uint[asset.solverAssetDesc.chunkCount];
        NvBlastExtUtilsWrapper.ReorderAssetDescChunks(asset.solverAssetDesc, chunkReorderMap);
	    BlastChunkCube[] chunksTemp = asset.chunks.ToArray();
	    for (uint i = 0; i < chunkReorderMap.Length; ++i)
	    {
		    asset.chunks[(int)chunkReorderMap[i]] = chunksTemp[i];
        }

        return asset;
    }

    static void fillBondDesc(List<NvBlastBondDesc> bondDescs, uint id0, uint id1, Vector3 pos0, Vector3 pos1, Vector3 size, float area)
    {
        NvBlastBondDesc bondDesc = new NvBlastBondDesc();
        bondDesc.chunk0 = id0;
        bondDesc.chunk1 = id1;
        bondDesc.bond.area = area;
        Vector3 centroid = (pos0 + pos1) * 0.5f;
        bondDesc.bond.c0 = centroid.x;
        bondDesc.bond.c1 = centroid.y;
        bondDesc.bond.c2 = centroid.z;
        Vector3 normal = (pos0 - pos1).normalized;
        bondDesc.bond.n0 = normal.x;
        bondDesc.bond.n1= normal.y;
        bondDesc.bond.n2 = normal.z;
        bondDescs.Add(bondDesc);
    }
}
