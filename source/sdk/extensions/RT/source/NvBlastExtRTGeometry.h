#pragma once
#include "NvBlastExtAuthoringTypes.h"
#include "NvBlastExtAuthoringInternalCommon.h"
#include "NvBlastExtAuthoringAccelerator.h"
#include "NvBlastExtRT.h"
#include <map>

using physx::PxVec3;
using physx::PxVec2;


namespace Nv
{
	namespace Blast
	{
		class Mesh;

		class VertexWelding
		{
		public:

			typedef int32_t (VertexWelding::*LOCATE_CALLBACK)(const Vertex& v, uint32_t bucket, bool& isAllDataTheSame);

			VertexWelding(uint32_t maxVertexCount, uint32_t maxBucketCount, float gridCellSize, float weldEpsilon, float auxEpsilon, LOCATE_CALLBACK clb);
		
			const Vertex* getVertices() const
			{
				return vertex.data();
			}
			uint32_t getVerticesCount() const
			{
				return vertex.size();
			}
			void reset();
			int32_t WeldVertex(const Vertex *v);

			int32_t LocateVertexInBucket(const Vertex& v, uint32_t bucket, bool& isAllDataTheSame);

			int32_t LocateVertexInBucketOnlyPosition(const Vertex& v, uint32_t bucket, bool& isAllDataTheSame);


		private:
			// Computes hash bucket index in range [0, NUM_BUCKETS-1]
			//int32_t ComputeHashBucketIndex(const Vertex& v)
			int32_t ComputeHashBucketIndex(int32_t x, int32_t y, int32_t z);

			void AddVertexToBucket(const Vertex& v, uint32_t bucket);

			std::vector<int32_t> first; // start of linked list for each bucket
			std::vector<int32_t> next; // links each vertex to next in linked list
			std::vector<Vertex> vertex; // unique vertices within tolerance

			const uint32_t maxVertexCount; // max number of vertices that can be welded at once
			const uint32_t maxBucketCount; // number of hash buckets to map grid cells into
			const float gridCellSizeInv; // grid cell size; must be at least 2*WELD_EPSILON
			const float weldEpsilon; // radius around vertex defining welding neighborhood
			const float auxEpsilon; // epsilon for normal and uv of vertex

			LOCATE_CALLBACK locateCallback;
		};
		
		class BooleanToolV2 : public Fracturer
		{
		public:

			struct Mode
			{
				int32_t ca, cb, ci;
				Mode() { ca = 0; cb = 0; ci = -1; };
				Mode(int32_t a, int32_t b, int32_t c) : ca(a), cb(b), ci(c)
				{
				}

				static Mode Intersection()
				{
					return Mode(0, 0, 1);
				}

				/**
				Creates boolean tool configuration to perform union of meshes A and B.
				*/
				static Mode Union()
				{
					return Mode(1, 1, -1);
				}
				/**
				Creates boolean tool configuration to perform difference of meshes(A - B).
				*/
				static Mode Difference()
				{
					return Mode(1, 0, -1);
				}
			};

			BooleanToolV2();

			void release();

			/**
				Set up this pointers before call evaluate();
			*/

			const Mesh* mMeshA;
			const Mesh* mMeshB;
			SpatialAccelerator* mAccelA;
			SpatialAccelerator* mAccelB;
			//const PreparedMesh* mPreparedA;
			//const PreparedMesh* mPreparedB;


			/**
				Computes result of setted boolean operation.
			*/
			void makeFacetFacetTests(BooleanToolOutputData* outputData, int32_t threadId, int32_t threadCount);
			void retain(bool isA, BooleanToolOutputData* outputData, int32_t threadId, int32_t threadCount,
				const DamagePattern* pattern = nullptr, int32_t chunk = -1);

			/**
				Get result mesh.
			*/
			//Nv::Blast::Mesh* getMesh();


		private:

			int32_t computeV03(const PxVec3& point);
			int32_t computeV30(const PxVec3& point);


			/**
				Boolean sub-operations.
			*/
			void computeRetained(const Mesh* mesh, const physx::PxBounds3& bMeshBoudning,
				int32_t(BooleanToolV2::*computeV3)(const physx::PxVec3&), int32_t btC, int32_t btCI, int32_t parentFacetOffset,
				BooleanToolOutputData* outputData, int32_t threadId, int32_t threadCount,
				struct FaceOrientation* fo = nullptr, const std::vector<bool>* validAdjacentFacet = nullptr);


			/////////////////////////////////////////////////////////////////////////
			/**
				SIMD buffers
			*/
			float* sx1;
			float* sy1;
			float* ex1;
			float* ey1;
			float* px1;
			float* py1;
			float* pt1;
			float* resy1;
			int32_t* winding1;
			int32_t* projectedWinding1;


			float* sx2;
			float* sy2;
			float* ex2;
			float* ey2;
			float* px2;
			float* py2;
			float* pt2;
			float* resy2;
			int32_t* winding2;
			int32_t* projectedWinding2;

			uint32_t* edgeFacetTestA;
			uint32_t* edgeFacetTestB;


			/**
				Other buffers.
			*/
			int32_t* edgeCrossCheckTest;
			Vertex* edgeCrossA;
			Vertex* edgeCrossB;

			Mode mToolMode;
		};

		class TriangulatorV2 : public MeshGenerator
		{
		public:
			TriangulatorV2();
			
			void release();

			uint32_t build(const BooleanResultEdge* edges, uint32_t inEdgeCount, const Vertex* inVertices, Vertex* outWeldedVrts, uint32_t& vcount, uint32_t* outTriangles, PerTriangleAdditionalData* adata, uint32_t maxTcount, const Mesh* ma, const Mesh* mb);

			struct LinkedListElement
			{
				uint32_t point;
				uint32_t nextPoint;
				uint32_t prevPoint;
			};

			Vertex* weldedVertices;
			uint32_t	weldedCount;
			uint32_t*	triangleIndices;
			uint32_t	triangleCount;

		private:

			void triangulatePolygonWithEarClipping(ProjectionDirections dir);

			LinkedListElement facetList[1024];
			uint32_t facetListSize;
			uint32_t pointIndicesList[1024];
			uint32_t pointCount;

			physx::PxVec2 projectedPointList[1024];
			uint32_t projectedPointCount;

			uint32_t visitedFlagValue[1024];
			uint32_t currentFlagValue;



			Edge* weldedEdges;


			const BooleanResultEdge*	mInpEdges;
			uint32_t					mInpEdgeCount;
			uint32_t					maxTriangleCount;

			const Mesh*					meshA;
			const Mesh*					meshB;

			VertexWelding wldg;
		};

	}
}

