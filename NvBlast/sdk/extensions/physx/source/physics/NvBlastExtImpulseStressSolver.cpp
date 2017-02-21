/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtImpulseStressSolver.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastAssert.h"
#include "NvBlastIndexFns.h"
#include "NvBlastExtDefs.h"

#include "NvBlastTkAsset.h"
#include "NvBlastTkActor.h"
#include "NvBlastTkFamily.h"

#include "PxScene.h"
#include "PxRigidDynamic.h"

#include <PsVecMath.h>
#include "PsFPU.h"

#include <algorithm>
#include <set>

#define USE_SCALAR_IMPL 0
#define WARM_START 1
#define USE_PHYSX_CONVEX_DATA 1
#define GRAPH_INTERGRIRY_CHECK 0


namespace Nv
{
namespace Blast
{

using namespace physx;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													 Solver
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SequentialImpulseSolver
{
public:
	PX_ALIGN_PREFIX(16)
	struct BondData
	{
		physx::PxVec3 impulseLinear;
		uint32_t node0;
		physx::PxVec3 impulseAngular;
		uint32_t node1;
		physx::PxVec3 offset0;
		float invOffsetSqrLength;

		float getStressHealth(const ExtStressSolverSettings& settings) const
		{
			return (impulseLinear.magnitude() * settings.stressLinearFactor + impulseAngular.magnitude() * settings.stressAngularFactor);
		}
	}
	PX_ALIGN_SUFFIX(16);

	PX_ALIGN_PREFIX(16)
	struct NodeData
	{
		physx::PxVec3 velocityLinear;
		float invI;
		physx::PxVec3 velocityAngular;
		float invMass;
	}
	PX_ALIGN_SUFFIX(16);

	SequentialImpulseSolver(uint32_t nodeCount, uint32_t maxBondCount)
	{
		m_nodesData.resize(nodeCount);
		m_bondsData.reserve(maxBondCount);
	}

	NV_INLINE const NodeData& getNodeData(uint32_t node) const
	{
		return m_nodesData[node];
	}

	NV_INLINE const BondData& getBondData(uint32_t bond) const
	{
		return m_bondsData[bond];
	}

	NV_INLINE uint32_t getBondCount() const
	{
		return m_bondsData.size();
	}

	NV_INLINE uint32_t getNodeCount() const
	{
		return m_nodesData.size();;
	}

	NV_INLINE void setNodeMassInfo(uint32_t node, float invMass, float invI)
	{
		m_nodesData[node].invMass = invMass;
		m_nodesData[node].invI = invI;
	}

	NV_INLINE void initialize()
	{
		for (auto& node : m_nodesData)
		{
			node.velocityLinear = PxVec3(PxZero);
			node.velocityAngular = PxVec3(PxZero);
		}
	}

	NV_INLINE void setNodeVelocities(uint32_t node, const PxVec3& velocityLinear, const PxVec3& velocityAngular)
	{
		m_nodesData[node].velocityLinear = velocityLinear;
		m_nodesData[node].velocityAngular = velocityAngular;
	}

	NV_INLINE uint32_t addBond(uint32_t node0, uint32_t node1, const PxVec3& offset)
	{
		const BondData data = {
			PxVec3(PxZero), 
			node0, 
			PxVec3(PxZero), 
			node1, 
			offset, 
			1.0f / offset.magnitudeSquared() 
		};
		m_bondsData.pushBack(data);
		return m_bondsData.size() - 1;
	}

	NV_INLINE void replaceWithLast(uint32_t bondIndex)
	{
		m_bondsData.replaceWithLast(bondIndex);
	}

	NV_INLINE void reset(uint32_t nodeCount)
	{
		m_bondsData.clear();
		m_nodesData.resize(nodeCount);
	}

	NV_INLINE void clearBonds()
	{
		m_bondsData.clear();
	}

	void solve(uint32_t iterationCount, bool warmStart = false)
	{
		solveInit(warmStart);

		for (uint32_t i = 0; i < iterationCount; ++i)
		{
			iterate();
		}
	}

	void calcError(float& linear, float& angular)
	{
		linear = 0.0f;
		angular = 0.0f;
		for (BondData& bond : m_bondsData)
		{
			NodeData* node0 = &m_nodesData[bond.node0];
			NodeData* node1 = &m_nodesData[bond.node1];

			const PxVec3 vA = node0->velocityLinear - node0->velocityAngular.cross(bond.offset0);
			const PxVec3 vB = node1->velocityLinear + node1->velocityAngular.cross(bond.offset0);

			const PxVec3 vErrorLinear = vA - vB;
			const PxVec3 vErrorAngular = node0->velocityAngular - node1->velocityAngular;

			linear += vErrorLinear.magnitude();
			angular += vErrorAngular.magnitude();
		}
	}

private:
	void solveInit(bool warmStart = false)
	{
		if (warmStart)
		{
			for (BondData& bond : m_bondsData)
			{
				NodeData* node0 = &m_nodesData[bond.node0];
				NodeData* node1 = &m_nodesData[bond.node1];

				const PxVec3 velocityLinearCorr0 = bond.impulseLinear * node0->invMass;
				const PxVec3 velocityLinearCorr1 = bond.impulseLinear * node1->invMass;

				const PxVec3 velocityAngularCorr0 = bond.impulseAngular * node0->invI - bond.offset0.cross(velocityLinearCorr0) * bond.invOffsetSqrLength;
				const PxVec3 velocityAngularCorr1 = bond.impulseAngular * node1->invI + bond.offset0.cross(velocityLinearCorr1) * bond.invOffsetSqrLength;

				node0->velocityLinear += velocityLinearCorr0;
				node1->velocityLinear -= velocityLinearCorr1;

				node0->velocityAngular += velocityAngularCorr0;
				node1->velocityAngular -= velocityAngularCorr1;
			}
		}
		else
		{
			for (BondData& bond : m_bondsData)
			{
				bond.impulseLinear = PxVec3(PxZero);
				bond.impulseAngular = PxVec3(PxZero);
			}
		}
	}


	NV_INLINE void iterate()
	{
		using namespace physx::shdfnd::aos;

		for (BondData& bond : m_bondsData)
		{
			NodeData* node0 = &m_nodesData[bond.node0];
			NodeData* node1 = &m_nodesData[bond.node1];

#if USE_SCALAR_IMPL
			const PxVec3 vA = node0->velocityLinear - node0->velocityAngular.cross(bond.offset0);
			const PxVec3 vB = node1->velocityLinear + node1->velocityAngular.cross(bond.offset0);

			const PxVec3 vErrorLinear = vA - vB;
			const PxVec3 vErrorAngular = node0->velocityAngular - node1->velocityAngular;

			const float weightedMass = 1.0f / (node0->invMass + node1->invMass);
			const float weightedInertia = 1.0f / (node0->invI + node1->invI);

			const PxVec3 outImpulseLinear = -vErrorLinear * weightedMass * 0.5f;
			const PxVec3 outImpulseAngular = -vErrorAngular * weightedInertia * 0.5f;

			bond.impulseLinear += outImpulseLinear;
			bond.impulseAngular += outImpulseAngular;

			const PxVec3 velocityLinearCorr0 = outImpulseLinear * node0->invMass;
			const PxVec3 velocityLinearCorr1 = outImpulseLinear * node1->invMass;

			const PxVec3 velocityAngularCorr0 = outImpulseAngular * node0->invI - bond.offset0.cross(velocityLinearCorr0) * bond.invOffsetSqrLength;
			const PxVec3 velocityAngularCorr1 = outImpulseAngular * node1->invI + bond.offset0.cross(velocityLinearCorr1) * bond.invOffsetSqrLength;

			node0->velocityLinear += velocityLinearCorr0;
			node1->velocityLinear -= velocityLinearCorr1;

			node0->velocityAngular += velocityAngularCorr0;
			node1->velocityAngular -= velocityAngularCorr1;
#else
			const Vec3V velocityLinear0 = V3LoadUnsafeA(node0->velocityLinear);
			const Vec3V velocityLinear1 = V3LoadUnsafeA(node1->velocityLinear);
			const Vec3V velocityAngular0 = V3LoadUnsafeA(node0->velocityAngular);
			const Vec3V velocityAngular1 = V3LoadUnsafeA(node1->velocityAngular);

			const Vec3V offset = V3LoadUnsafeA(bond.offset0);
			const Vec3V vA = V3Add(velocityLinear0, V3Neg(V3Cross(velocityAngular0, offset)));
			const Vec3V vB = V3Add(velocityLinear1, V3Cross(velocityAngular1, offset));

			const Vec3V vErrorLinear = V3Sub(vA, vB);
			const Vec3V vErrorAngular = V3Sub(velocityAngular0, velocityAngular1);

			const FloatV invM0 = FLoad(node0->invMass);
			const FloatV invM1 = FLoad(node1->invMass);
			const FloatV invI0 = FLoad(node0->invI);
			const FloatV invI1 = FLoad(node1->invI);
			const FloatV invOffsetSqrLength = FLoad(bond.invOffsetSqrLength);

			const FloatV weightedMass = FLoad(-0.5f / (node0->invMass + node1->invMass));
			const FloatV weightedInertia = FLoad(-0.5f / (node0->invI + node1->invI));

			const Vec3V outImpulseLinear = V3Scale(vErrorLinear, weightedMass);
			const Vec3V outImpulseAngular = V3Scale(vErrorAngular, weightedInertia);

			V3StoreA(V3Add(V3LoadUnsafeA(bond.impulseLinear), outImpulseLinear), bond.impulseLinear);
			V3StoreA(V3Add(V3LoadUnsafeA(bond.impulseAngular), outImpulseAngular), bond.impulseAngular);

			const Vec3V velocityLinearCorr0 = V3Scale(outImpulseLinear, invM0);
			const Vec3V velocityLinearCorr1 = V3Scale(outImpulseLinear, invM1);

			const Vec3V velocityAngularCorr0 = V3Sub(V3Scale(outImpulseAngular, invI0), V3Scale(V3Cross(offset, velocityLinearCorr0), invOffsetSqrLength));
			const Vec3V velocityAngularCorr1 = V3Add(V3Scale(outImpulseAngular, invI1),	V3Scale(V3Cross(offset, velocityLinearCorr1), invOffsetSqrLength));

			V3StoreA(V3Add(velocityLinear0, velocityLinearCorr0), node0->velocityLinear);
			V3StoreA(V3Sub(velocityLinear1, velocityLinearCorr1), node1->velocityLinear);

			V3StoreA(V3Add(velocityAngular0, velocityAngularCorr0), node0->velocityAngular);
			V3StoreA(V3Sub(velocityAngular1, velocityAngularCorr1), node1->velocityAngular);
#endif
		}
	}

	shdfnd::Array<BondData, ExtAlignedAllocator<16>>		m_bondsData;
	shdfnd::Array<NodeData, ExtAlignedAllocator<16>>		m_nodesData;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													 Graph Processor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GRAPH_INTERGRIRY_CHECK
#define CHECK_GRAPH_INTEGRITY checkGraphIntegrity()
#else
#define CHECK_GRAPH_INTEGRITY ((void)0)
#endif

class SupportGraphProcessor
{

public:
	struct BondData
	{
		uint32_t node0;
		uint32_t node1;
		uint32_t blastBondIndex;
	};

	struct NodeData
	{
		float mass;
		float volume;
		PxVec3 localPos;
		bool isStatic;
		uint32_t solverNode;
		uint32_t neighborsCount;
		PxVec3 impulse;
	};

	struct SolverNodeData
	{
		uint32_t supportNodesCount;
		PxVec3 localPos;
		union
		{
			float mass;
			int32_t indexShift;
		};
		float volume;
		bool isStatic;
	};

	struct SolverBondData
	{
		ExtInlineArray<uint32_t, 8>::type blastBondIndices;
	};

	SupportGraphProcessor(uint32_t nodeCount, uint32_t maxBondCount) : m_solver(nodeCount, maxBondCount), m_nodesDirty(true)
	{
		m_nodesData.resize(nodeCount);
		m_bondsData.reserve(maxBondCount);

		m_solverNodesData.resize(nodeCount);
		m_solverBondsData.reserve(maxBondCount);

		m_solverBondsMap.reserve(maxBondCount);

		m_blastBondIndexMap.resize(maxBondCount);
		memset(m_blastBondIndexMap.begin(), 0xFF, m_blastBondIndexMap.size() * sizeof(uint32_t));
	}

	NV_INLINE const NodeData& getNodeData(uint32_t node) const
	{
		return m_nodesData[node];
	}

	NV_INLINE const BondData& getBondData(uint32_t bond) const
	{
		return m_bondsData[bond];
	}

	NV_INLINE const SolverNodeData& getSolverNodeData(uint32_t node) const
	{
		return m_solverNodesData[node];
	}

	NV_INLINE const SolverBondData& getSolverBondData(uint32_t bond) const
	{
		return m_solverBondsData[bond];
	}

	NV_INLINE const SequentialImpulseSolver::BondData& getSolverInternalBondData(uint32_t bond) const
	{
		return m_solver.getBondData(bond);
	}

	NV_INLINE const SequentialImpulseSolver::NodeData& getSolverInternalNodeData(uint32_t node) const
	{
		return m_solver.getNodeData(node);
	}

	NV_INLINE uint32_t getBondCount() const
	{
		return m_bondsData.size();
	}

	NV_INLINE uint32_t getNodeCount() const
	{
		return m_nodesData.size();;
	}

	NV_INLINE uint32_t getSolverBondCount() const
	{
		return m_solverBondsData.size();
	}

	NV_INLINE uint32_t getSolverNodeCount() const
	{
		return m_solverNodesData.size();;
	}

	NV_INLINE void setNodeInfo(uint32_t node, float mass, float volume, PxVec3 localPos,	bool isStatic)
	{
		m_nodesData[node].mass = mass;
		m_nodesData[node].volume = volume;
		m_nodesData[node].localPos = localPos;
		m_nodesData[node].isStatic = isStatic;
	}

	NV_INLINE void setNodeNeighborsCount(uint32_t node, uint32_t neighborsCount)
	{
		// neighbors count is expected to be the number of nodes on 1 island/actor.
		m_nodesData[node].neighborsCount = neighborsCount;

		// check for too huge aggregates (happens after island's split)
		if (!m_nodesDirty)
		{
			m_nodesDirty |= (m_solverNodesData[m_nodesData[node].solverNode].supportNodesCount > neighborsCount / 2);
		}
	}

	NV_INLINE void initialize()
	{
		sync();
		
		m_solver.initialize();

		for (auto& node : m_nodesData)
		{
			node.impulse = PxVec3(PxZero);
		}
	}

	NV_INLINE void addNodeImpulse(uint32_t node, const PxVec3& impulse)
	{
		m_nodesData[node].impulse += impulse;
	}

	NV_INLINE void addNodeVelocity(uint32_t node, const PxVec3& velocity)
	{
		PxVec3 impulse = velocity * m_nodesData[node].mass;
		addNodeImpulse(node, impulse);
	}

	NV_INLINE void addBond(uint32_t node0, uint32_t node1, uint32_t blastBondIndex)
	{
		if (isInvalidIndex(m_blastBondIndexMap[blastBondIndex]))
		{
			const BondData data = {
				node0,
				node1,
				blastBondIndex
			};
			m_bondsData.pushBack(data);
			m_blastBondIndexMap[blastBondIndex] = m_bondsData.size() - 1;
		}
	}

	NV_INLINE void removeBondIfExists(uint32_t blastBondIndex)
	{
		const uint32_t bondIndex = m_blastBondIndexMap[blastBondIndex];

		if (!isInvalidIndex(bondIndex))
		{
			const BondData& bond = m_bondsData[bondIndex];
			const uint32_t solverNode0 = m_nodesData[bond.node0].solverNode;
			const uint32_t solverNode1 = m_nodesData[bond.node1].solverNode;
			bool isBondInternal = (solverNode0 == solverNode1);

			if (isBondInternal)
			{
				// internal bond sadly requires graph resync (it never happens on reduction level '0')
				m_nodesDirty = true;
			}
			else if (!m_nodesDirty)
			{
				// otherwise it's external bond, we can remove it manually and keep graph synced
				// we don't need to spend time there if (m_nodesDirty == true), graph will be resynced anyways

				BondKey solverBondKey(solverNode0, solverNode1);
				auto entry = m_solverBondsMap.find(solverBondKey);
				if (entry)
				{
					const uint32_t solverBondIndex = entry->second;
					auto& blastBondIndices = m_solverBondsData[solverBondIndex].blastBondIndices;
					blastBondIndices.findAndReplaceWithLast(blastBondIndex);
					if (blastBondIndices.empty())
					{
						// all bonds associated with this solver bond were removed, so let's remove solver bond

						m_solverBondsData.replaceWithLast(solverBondIndex);
						m_solver.replaceWithLast(solverBondIndex);
						if (m_solver.getBondCount() > 0)
						{
							// update 'previously last' solver bond mapping
							const auto& solverBond = m_solver.getBondData(solverBondIndex);
							m_solverBondsMap[BondKey(solverBond.node0, solverBond.node1)] = solverBondIndex;
						}

						m_solverBondsMap.erase(solverBondKey);
					}
				}

				CHECK_GRAPH_INTEGRITY;
			}

			// remove bond from graph processor's list
			m_blastBondIndexMap[blastBondIndex] = invalidIndex<uint32_t>();
			m_bondsData.replaceWithLast(bondIndex);
			m_blastBondIndexMap[m_bondsData[bondIndex].blastBondIndex] = m_bondsData.size() > bondIndex ? bondIndex : invalidIndex<uint32_t>();
		}
	}

	NV_INLINE void setGraphReductionLevel(uint32_t level)
	{
		m_graphReductionLevel = level;
		m_nodesDirty = true;
	}

	uint32_t getGraphReductionLevel() const
	{
		return m_graphReductionLevel;
	}

	void solve(uint32_t iterationCount, bool warmStart = false)
	{
		CHECK_GRAPH_INTEGRITY;

		for (const NodeData& node : m_nodesData)
		{
			const SequentialImpulseSolver::NodeData& solverNode = m_solver.getNodeData(node.solverNode);
			m_solver.setNodeVelocities(node.solverNode, solverNode.velocityLinear + node.impulse * solverNode.invMass, PxVec3(PxZero));
		}

		m_solver.solve(iterationCount, warmStart);
	}

	void calcError(float& linear, float& angular)
	{
		m_solver.calcError(linear, angular);
	}

	void generateFracture(ExtArray<NvBlastBondFractureData>::type& bondFractureBuffer, const ExtStressSolverSettings& settings, const float* blastBondHealths)
	{
		CHECK_GRAPH_INTEGRITY;

		for (uint32_t i = 0; i < m_solverBondsData.size(); ++i)
		{
			const SequentialImpulseSolver::BondData& solverInternalBond = m_solver.getBondData(i);
			if (solverInternalBond.getStressHealth(settings) > 1.0f)
			{
				const auto& blastBondIndices = m_solverBondsData[i].blastBondIndices;
				for (auto blastBondIndex : blastBondIndices)
				{
					const uint32_t bondIndex = m_blastBondIndexMap[blastBondIndex];
					if (!isInvalidIndex(bondIndex))
					{
						const BondData& bond = m_bondsData[bondIndex];

						NVBLAST_ASSERT(getNodeData(bond.node0).solverNode != getNodeData(bond.node1).solverNode);
						NVBLAST_ASSERT(bond.blastBondIndex == blastBondIndex);

						NvBlastBondFractureData data;
						data.health = blastBondHealths[blastBondIndex];
						data.nodeIndex0 = bond.node0;
						data.nodeIndex1 = bond.node1;
						bondFractureBuffer.pushBack(data);
					}
				}
			}
		}
	}

private:

	NV_INLINE void sync()
	{
		if (m_nodesDirty)
		{
			syncNodes();
		}
		if (m_bondsDirty)
		{
			syncBonds();
		}

		CHECK_GRAPH_INTEGRITY;
	}

	void syncNodes()
	{
		// init with 1<->1 blast nodes to solver nodes mapping
		m_solverNodesData.resize(m_nodesData.size());
		for (uint32_t i = 0; i < m_nodesData.size(); ++i)
		{
			m_nodesData[i].solverNode = i;
			m_solverNodesData[i].supportNodesCount = 1;
			m_solverNodesData[i].indexShift = 0;
		}

		// for static nodes aggregate size per graph reduction level is lower, it
		// falls behind on few levels. (can be made as parameter)
		const uint32_t STATIC_NODES_COUNT_PENALTY = 2 << 2;

		// reducing graph by aggregating nodes level by level
		for (uint32_t k = 0; k < m_graphReductionLevel; k++)
		{
			const uint32_t maxAggregateSize = 1 << (k + 1);

			for (const BondData& bond : m_bondsData)
			{
				NodeData& node0 = m_nodesData[bond.node0];
				NodeData& node1 = m_nodesData[bond.node1];

				if (node0.isStatic != node1.isStatic)
					continue;

				if (node0.solverNode == node1.solverNode)
					continue;

				SolverNodeData& solverNode0 = m_solverNodesData[node0.solverNode];
				SolverNodeData& solverNode1 = m_solverNodesData[node1.solverNode];
				
				const int countPenalty = node0.isStatic ? STATIC_NODES_COUNT_PENALTY : 1;
				const uint32_t aggregateSize = std::min<uint32_t>(maxAggregateSize, node0.neighborsCount / 2);

				if (solverNode0.supportNodesCount * countPenalty >= aggregateSize)
					continue;
				if (solverNode1.supportNodesCount * countPenalty >= aggregateSize)
					continue;

				if (solverNode0.supportNodesCount >= solverNode1.supportNodesCount)
				{
					solverNode1.supportNodesCount--;
					solverNode0.supportNodesCount++;
					node1.solverNode = node0.solverNode;
				}
				else if (solverNode1.supportNodesCount >= solverNode0.supportNodesCount)
				{
					solverNode1.supportNodesCount++;
					solverNode0.supportNodesCount--;
					node0.solverNode = node1.solverNode;
				}
			}
		}

		// Solver Nodes now sparse, a lot of empty ones. Rearrange them by moving all non-empty to the front
		// 2 passes used for that
		{
			uint32_t currentNode = 0;
			for (; currentNode < m_solverNodesData.size(); ++currentNode)
			{
				if (m_solverNodesData[currentNode].supportNodesCount > 0)
					continue;

				// 'currentNode' is free

				// search next occupied node
				uint32_t k = currentNode + 1;
				for (; k < m_solverNodesData.size(); ++k)
				{
					if (m_solverNodesData[k].supportNodesCount > 0)
					{
						// replace currentNode and keep indexShift
						m_solverNodesData[currentNode].supportNodesCount = m_solverNodesData[k].supportNodesCount;
						m_solverNodesData[k].indexShift = k - currentNode;
						m_solverNodesData[k].supportNodesCount = 0;
						break;
					}
				}

				if (k == m_solverNodesData.size())
				{
					break;
				}
			}
			for (auto& node : m_nodesData)
			{
				node.solverNode -= m_solverNodesData[node.solverNode].indexShift;
			}

			// now, we know total solver nodes count and which nodes are aggregated into them
			m_solverNodesData.resize(currentNode);
		}


		// calculate all needed data
		for (SolverNodeData& solverNode : m_solverNodesData)
		{
			solverNode.supportNodesCount = 0;
			solverNode.localPos = PxVec3(PxZero);
			solverNode.mass = 0.0f;
			solverNode.volume = 0.0f;
			solverNode.isStatic = false;
		}

		for (NodeData& node : m_nodesData)
		{
			SolverNodeData& solverNode = m_solverNodesData[node.solverNode];
			solverNode.supportNodesCount++;
			solverNode.localPos += node.localPos;
			solverNode.mass += node.mass;
			solverNode.volume += node.volume;
			solverNode.isStatic |= node.isStatic;
		}

		for (SolverNodeData& solverNode : m_solverNodesData)
		{
			solverNode.localPos /= (float)solverNode.supportNodesCount;
		}

		m_solver.reset(m_solverNodesData.size());
		for (uint32_t nodeIndex = 0; nodeIndex < m_solverNodesData.size(); ++nodeIndex)
		{
			const SolverNodeData& solverNode = m_solverNodesData[nodeIndex];

			const float invMass = solverNode.isStatic ? 0.0f : 1.0f / solverNode.mass;
			const float R = PxPow(solverNode.volume * 3.0f * PxInvPi / 4.0f, 1.0f / 3.0f); // sphere volume approximation
			const float invI = invMass / (R * R * 0.4f); // sphere inertia tensor approximation: I = 2/5 * M * R^2 ; invI = 1 / I;
			m_solver.setNodeMassInfo(nodeIndex, invMass, invI);
		}

		m_nodesDirty = false;

		syncBonds();
	}

	void syncBonds()
	{
		// traverse all blast bonds and aggregate
		m_solver.clearBonds();
		m_solverBondsMap.clear();
		m_solverBondsData.clear();
		for (const BondData& bond : m_bondsData)
		{
			const NodeData& node0 = m_nodesData[bond.node0];
			const NodeData& node1 = m_nodesData[bond.node1];

			if (node0.solverNode == node1.solverNode)
				continue; // skip (internal)

			if (node0.isStatic && node1.isStatic)
				continue;

			BondKey key(node0.solverNode, node1.solverNode);
			auto entry = m_solverBondsMap.find(key);
			SolverBondData* data;
			if (!entry)
			{
				m_solverBondsData.pushBack(SolverBondData());
				data = &m_solverBondsData.back();
				m_solverBondsMap[key] = m_solverBondsData.size() - 1;

				SolverNodeData& solverNode0 = m_solverNodesData[node0.solverNode];
				SolverNodeData& solverNode1 = m_solverNodesData[node1.solverNode];
				m_solver.addBond(node0.solverNode, node1.solverNode, (solverNode1.localPos - solverNode0.localPos) * 0.5f);
			}
			else
			{
				data = &m_solverBondsData[entry->second];
			}
			data->blastBondIndices.pushBack(bond.blastBondIndex);
		}

		m_bondsDirty = false;
	}

#if GRAPH_INTERGRIRY_CHECK
	void checkGraphIntegrity()
	{
		NVBLAST_ASSERT(m_solver.getBondCount() == m_solverBondsData.size());
		NVBLAST_ASSERT(m_solver.getNodeCount() == m_solverNodesData.size());

		std::set<uint64_t> solverBonds;
		for (uint32_t i = 0; i < m_solverBondsData.size(); ++i)
		{
			const auto& bondData = m_solver.getBondData(i);
			BondKey key(bondData.node0, bondData.node1);
			NVBLAST_ASSERT(solverBonds.find(key) == solverBonds.end());
			solverBonds.emplace(key);
			auto entry = m_solverBondsMap.find(key);
			NVBLAST_ASSERT(entry != nullptr);
			const auto& solverBond = m_solverBondsData[entry->second];
			for (auto& blastBondIndex : solverBond.blastBondIndices)
			{
				if (!isInvalidIndex(m_blastBondIndexMap[blastBondIndex]))
				{
					auto& b = m_bondsData[m_blastBondIndexMap[blastBondIndex]];
					BondKey key2(m_nodesData[b.node0].solverNode, m_nodesData[b.node1].solverNode);
					NVBLAST_ASSERT(key2 == key);
				}
			}
		}

		for (auto& solverBond : m_solverBondsData)
		{
			for (auto& blastBondIndex : solverBond.blastBondIndices)
			{
				if (!isInvalidIndex(m_blastBondIndexMap[blastBondIndex]))
				{
					auto& b = m_bondsData[m_blastBondIndexMap[blastBondIndex]];
					NVBLAST_ASSERT(m_nodesData[b.node0].solverNode != m_nodesData[b.node1].solverNode);
				}
			}
		}
		uint32_t mappedBondCount = 0;
		for (uint32_t i = 0; i < m_blastBondIndexMap.size(); i++)
		{
			const auto& bondIndex = m_blastBondIndexMap[i];
			if (!isInvalidIndex(bondIndex))
			{
				mappedBondCount++;
				NVBLAST_ASSERT(m_bondsData[bondIndex].blastBondIndex == i);
			}
		}
		NVBLAST_ASSERT(m_bondsData.size() == mappedBondCount);
	}
#endif

	struct BondKey
	{
		uint32_t node0;
		uint32_t node1;

		BondKey(uint32_t n0, uint32_t n1)
		{
			node0 = n0 < n1 ? n0 : n1;
			node1 = n0 < n1 ? n1 : n0;
		}

		operator uint64_t() const
		{
			return static_cast<uint64_t>(node0) + (static_cast<uint64_t>(node1) << 32);
		}
	};

	SequentialImpulseSolver			    m_solver;
	ExtArray<SolverNodeData>::type	    m_solverNodesData;
	ExtArray<SolverBondData>::type	    m_solverBondsData;

	uint32_t						    m_graphReductionLevel;

	bool	                            m_nodesDirty;
	bool	                            m_bondsDirty;

	ExtHashMap<BondKey, uint32_t>::type m_solverBondsMap;
	ExtArray<uint32_t>::type		    m_blastBondIndexMap;

	ExtArray<BondData>::type		    m_bondsData;
	ExtArray<NodeData>::type		    m_nodesData;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											 ExtImpulseStressSolver
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Creation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExtImpulseStressSolver::ExtImpulseStressSolver(ExtPxFamily& family, ExtStressSolverSettings settings)
	: m_family(family), m_settings(settings), m_isDirty(false), m_reset(false), 
	m_errorAngular(std::numeric_limits<float>::max()), m_errorLinear(std::numeric_limits<float>::max()), m_framesCount(0)
{

	const TkAsset* tkAsset = m_family.getTkFamily().getAsset();
	const ExtPxAsset& asset = m_family.getPxAsset();
	const ExtPxChunk* chunks = asset.getChunks();
	const ExtPxSubchunk* subChunks = asset.getSubchunks();
	m_graph = tkAsset->getGraph();
	const uint32_t bondCount = tkAsset->getBondCount();
	
	TkActor* tkActor;
	m_family.getTkFamily().getActors(&tkActor, 1);
	m_bondHealths = tkActor->getBondHealths();

	m_graphProcessor = NVBLASTEXT_NEW(SupportGraphProcessor)(m_graph.nodeCount, bondCount);

	// traverse graph and fill node info
	for (uint32_t i = 0; i < m_graph.nodeCount; ++i)
	{
		uint32_t node0 = i;
		uint32_t chunkIndex0 = m_graph.chunkIndices[node0];
		const ExtPxChunk& chunk0 = chunks[chunkIndex0];

		bool isChunkStatic = chunk0.isStatic;

		for (uint32_t adjacencyIndex = m_graph.adjacencyPartition[node0]; adjacencyIndex < m_graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
		{
			uint32_t bondIndex = m_graph.adjacentBondIndices[adjacencyIndex];
			if (m_bondHealths[bondIndex] <= 0.0f)
				continue;
			uint32_t node1 = m_graph.adjacentNodeIndices[adjacencyIndex];
			uint32_t chunkIndex1 = m_graph.chunkIndices[node1];
			const ExtPxChunk& chunk1 = chunks[chunkIndex1];

			if (chunk1.subchunkCount == 0 || chunk1.isStatic)
			{
				isChunkStatic |= chunk1.isStatic;
				continue;
			}
		}

		// fill node info

		float mass;
		float volume;
		PxVec3 localPos;
		if (chunk0.subchunkCount > 0)
		{
#if USE_PHYSX_CONVEX_DATA
			const ExtPxSubchunk& subChunk = subChunks[chunk0.firstSubchunkIndex];
			PxVec3 localCenterOfMass;
			PxMat33 intertia;
			PxVec3 scale = subChunk.geometry.scale.scale;
			subChunk.geometry.convexMesh->getMassInformation(mass, intertia, localCenterOfMass);
			mass *= scale.x * scale.y * scale.z;
			const PxTransform& chunk0LocalTransform = subChunk.transform;
			localPos = chunk0LocalTransform.transform(localCenterOfMass);
			volume = mass / 1.0f; // unit density
#else
			volume = solverChunk0.volume;
			mass = volume * 1.0f; // density
			localPos = *reinterpret_cast<const PxVec3*>(solverChunk0.centroid);
#endif
		}
		else
		{
			mass = 0.0f;
			volume = 0.0f;
			localPos = PxVec3(PxZero);
			isChunkStatic = true;
		}
		m_graphProcessor->setNodeInfo(node0, mass, volume, localPos, isChunkStatic);
	}

	// traverse graph and fill bond info
	for (uint32_t node0 = 0; node0 < m_graph.nodeCount; ++node0)
	{
		for (uint32_t adjacencyIndex = m_graph.adjacencyPartition[node0]; adjacencyIndex < m_graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
		{
			uint32_t bondIndex = m_graph.adjacentBondIndices[adjacencyIndex];
			if (m_bondHealths[bondIndex] <= 0.0f)
				continue;
			uint32_t node1 = m_graph.adjacentNodeIndices[adjacencyIndex];

			if (node0 < node1)
			{
				m_graphProcessor->addBond(node0, node1, bondIndex);
			}
		}
	}

	// fire initial actor's created
	ExtInlineArray<ExtPxActor*, 4>::type actors;;
	actors.resize((uint32_t)m_family.getActorCount());
	m_family.getActors(actors.begin(), actors.size());
	for (const auto actor : actors)
	{
		onActorCreated(m_family, *actor);
	}

	m_family.subscribe(*this);
}

ExtImpulseStressSolver::~ExtImpulseStressSolver()
{
	NVBLASTEXT_DELETE(m_graphProcessor, SupportGraphProcessor);
	m_family.unsubscribe(*this);
}

ExtStressSolver* ExtStressSolver::create(ExtPxFamily& family, ExtStressSolverSettings settings)
{
	return NVBLASTEXT_NEW(ExtImpulseStressSolver) (family, settings);
}

void ExtImpulseStressSolver::release()
{
	NVBLASTEXT_DELETE(this, ExtImpulseStressSolver);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Actors
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtImpulseStressSolver::onActorCreated(ExtPxFamily& /*family*/, ExtPxActor& actor)
{
	if (actor.getTkActor().getGraphNodeCount() > 1)
	{
		// update neighbors
		{
			const uint32_t graphNodeCount = actor.getTkActor().getGraphNodeCount();
			uint32_t* graphNodeIndices = getScratchArray<uint32_t>(graphNodeCount);
			actor.getTkActor().getGraphNodeIndices(graphNodeIndices, graphNodeCount);
			for (uint32_t i = 0; i < graphNodeCount; ++i)
			{
				m_graphProcessor->setNodeNeighborsCount(graphNodeIndices[i], graphNodeCount);
			}
		}

		m_actors.insert(&actor);
		m_isDirty = true;
	}
}

void ExtImpulseStressSolver::onActorDestroyed(ExtPxFamily& /*family*/, ExtPxActor& actor)
{
	if (m_actors.erase(&actor))
	{
		m_isDirty = true;
	}
}

void ExtImpulseStressSolver::syncSolver()
{
	// traverse graph and remove dead bonds
	for (uint32_t node0 = 0; node0 < m_graph.nodeCount; ++node0)
	{
		for (uint32_t adjacencyIndex = m_graph.adjacencyPartition[node0]; adjacencyIndex < m_graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
		{
			uint32_t node1 = m_graph.adjacentNodeIndices[adjacencyIndex];
			if (node0 < node1)
			{
				uint32_t bondIndex = m_graph.adjacentBondIndices[adjacencyIndex];

				if (m_bondHealths[bondIndex] <= 0.0f)
				{
					m_graphProcessor->removeBondIfExists(bondIndex);
				}
			}
		}
	}

	m_isDirty = false;
}


void ExtImpulseStressSolver::initialize()
{
	if (m_reset)
	{
		m_framesCount = 0;
	}

	if (m_isDirty)
	{
		syncSolver();
	}

	if (m_settings.graphReductionLevel != m_graphProcessor->getGraphReductionLevel())
	{
		m_graphProcessor->setGraphReductionLevel(m_settings.graphReductionLevel);
	}

	m_graphProcessor->initialize();

	for (auto it = m_actors.getIterator(); !it.done(); ++it)
	{
		const ExtPxActor* actor = *it;
		const uint32_t graphNodeCount = actor->getTkActor().getGraphNodeCount();
		uint32_t* graphNodeIndices = getScratchArray<uint32_t>(graphNodeCount);
		actor->getTkActor().getGraphNodeIndices(graphNodeIndices, graphNodeCount);

		PxRigidDynamic& rigidDynamic = actor->getPhysXActor();
		const bool isStatic = rigidDynamic.getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
		if (isStatic)
		{
			PxVec3 gravity = rigidDynamic.getScene()->getGravity();
			gravity = rigidDynamic.getGlobalPose().rotateInv(gravity);

			for (uint32_t i = 0; i < graphNodeCount; ++i)
			{
				const uint32_t node = graphNodeIndices[i];
				m_graphProcessor->addNodeVelocity(node, gravity);
			}
		}
		else
		{
			PxVec3 cMassPose = rigidDynamic.getCMassLocalPose().p;

			PxVec3 angularVelocity = rigidDynamic.getGlobalPose().rotateInv(rigidDynamic.getAngularVelocity());
			//PxVec3 linearVelocity = rigidDynamic.getGlobalPose().rotateInv(rigidDynamic.getLinearVelocity());

			// Apply centrifugal force
			for (uint32_t i = 0; i < graphNodeCount; ++i)
			{
				const uint32_t node = graphNodeIndices[i];
				const auto& localPos = m_graphProcessor->getNodeData(node).localPos;
				// a = w x (w x r)
				const PxVec3 centrifugalAcceleration = angularVelocity.cross(angularVelocity.cross(localPos - cMassPose));
				m_graphProcessor->addNodeVelocity(node, centrifugalAcceleration);
			}
		}

		const auto entry = m_impulseBuffer.find(actor);
		if (entry)
		{
			for (const ImpulseData& data : entry->second)
			{
				float bestDist = FLT_MAX;
				uint32_t bestNode = invalidIndex<uint32_t>();

				for (uint32_t i = 0; i < graphNodeCount; ++i)
				{
					const uint32_t node = graphNodeIndices[i];
					const float sqrDist = (data.position - m_graphProcessor->getNodeData(node).localPos).magnitudeSquared();
					if (sqrDist < bestDist)
					{
						bestDist = sqrDist;
						bestNode = node;
					}
				}

				if (!isInvalidIndex(bestNode))
				{
					m_graphProcessor->addNodeImpulse(bestNode, data.impulse);
				}
			}
			m_impulseBuffer[actor].clear();
		}
	}
}

void ExtImpulseStressSolver::applyImpulse(ExtPxActor& actor, physx::PxVec3 position, physx::PxVec3 force)
{
	ImpulseData data = { position, force };

	m_impulseBuffer[&actor].pushBack(data);
}

uint32_t ExtImpulseStressSolver::getBondCount() const
{
	return m_graphProcessor->getSolverBondCount();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Update
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtImpulseStressSolver::update(bool doDamage)
{
	initialize();

	solve();

	if (doDamage)
	{
		applyDamage();
	}

	m_framesCount++;
}

void ExtImpulseStressSolver::solve()
{
	PX_SIMD_GUARD;

	const uint32_t iterations = getIterationsPerFrame();
	m_graphProcessor->solve(iterations, WARM_START && !m_reset);
	m_reset = false;

	m_graphProcessor->calcError(m_errorLinear, m_errorAngular);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Damage
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtImpulseStressSolver::applyDamage()
{
	m_bondFractureBuffer.clear();
	m_graphProcessor->generateFracture(m_bondFractureBuffer, m_settings, m_bondHealths);

	if (m_bondFractureBuffer.size() > 0)
	{
		NvBlastFractureBuffers fractureCommands;
		fractureCommands.chunkFractureCount = 0;
		fractureCommands.bondFractureCount = m_bondFractureBuffer.size();
		fractureCommands.bondFractures = m_bondFractureBuffer.begin();

		m_family.getTkFamily().applyFracture(&fractureCommands);
	}
}

uint32_t ExtImpulseStressSolver::getIterationCount() const
{
	return getFrameCount() * getIterationsPerFrame();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Debug Render
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PxU32 PxVec4ToU32Color(const PxVec4& color)
{
	PxU32 c = 0;
	c |= (int)(color.w * 255); c <<= 8;
	c |= (int)(color.z * 255); c <<= 8;
	c |= (int)(color.y * 255); c <<= 8;
	c |= (int)(color.x * 255);
	return c;
}

static PxVec4 PxVec4Lerp(const PxVec4 v0, const PxVec4 v1, float val)
{
	PxVec4 v(
		v0.x * (1 - val) + v1.x * val,
		v0.y * (1 - val) + v1.y * val,
		v0.z * (1 - val) + v1.z * val,
		v0.w * (1 - val) + v1.w * val
		);
	return v;
}

inline float clamp01(float v)
{
	return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

inline PxVec4 bondHealthColor(float healthFraction)
{
	healthFraction = clamp01(healthFraction);

	const PxVec4 BOND_HEALTHY_COLOR(0.0f, 1.0f, 1.0f, 1.0f);
	const PxVec4 BOND_MID_COLOR(1.0f, 1.0f, 0.0f, 1.0f);
	const PxVec4 BOND_BROKEN_COLOR(1.0f, 0.0f, 0.0f, 1.0f);

	return healthFraction < 0.5 ? PxVec4Lerp(BOND_BROKEN_COLOR, BOND_MID_COLOR, 2.0f * healthFraction) : PxVec4Lerp(BOND_MID_COLOR, BOND_HEALTHY_COLOR, 2.0f * healthFraction - 1.0f);
}

void ExtImpulseStressSolver::fillDebugRender(const std::vector<uint32_t>& nodes, std::vector<PxDebugLine>& lines, DebugRenderMode mode, float scale)
{
	const PxVec4 BOND_IMPULSE_LINEAR_COLOR(0.0f, 1.0f, 0.0f, 1.0f);
	const PxVec4 BOND_IMPULSE_ANGULAR_COLOR(1.0f, 0.0f, 0.0f, 1.0f);

	if (m_isDirty)
		return;

	ExtArray<uint8_t>::type& nodesSet = m_scratch;

	nodesSet.resize(m_graphProcessor->getSolverNodeCount());
	memset(nodesSet.begin(), 0, nodesSet.size() * sizeof(uint8_t));
	for (auto& nodeIndex : nodes)
	{
		nodesSet[m_graphProcessor->getNodeData(nodeIndex).solverNode] = 1;
	}

	const uint32_t bondCount = m_graphProcessor->getSolverBondCount();
	for (uint32_t i = 0; i < bondCount; ++i)
	{
		const auto& solverInternalBondData = m_graphProcessor->getSolverInternalBondData(i);
		if (nodesSet[solverInternalBondData.node0] != 0)
		{
			NVBLAST_ASSERT(nodesSet[solverInternalBondData.node1] != 0);

			const auto& solverInternalNode0 = m_graphProcessor->getSolverInternalNodeData(solverInternalBondData.node0);
			const auto& solverInternalNode1 = m_graphProcessor->getSolverInternalNodeData(solverInternalBondData.node1);
			const auto& solverNode0 = m_graphProcessor->getSolverNodeData(solverInternalBondData.node0);
			const auto& solverNode1 = m_graphProcessor->getSolverNodeData(solverInternalBondData.node1);

			PxVec3 p0 = solverNode0.localPos;
			PxVec3 p1 = solverNode1.localPos;
			PxVec3 center = (p0 + p1) * 0.5f;

			const float stress = std::min<float>(solverInternalBondData.getStressHealth(m_settings), 1.0f);
			PxVec4 color = bondHealthColor(1.0f - stress);

			lines.push_back(PxDebugLine(p0, p1, PxVec4ToU32Color(color)));

			float impulseScale = scale;

			if (mode == DebugRenderMode::STRESS_GRAPH_NODES_IMPULSES)
			{
				lines.push_back(PxDebugLine(p0, p0 + solverInternalNode0.velocityLinear * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_LINEAR_COLOR)));
				lines.push_back(PxDebugLine(p0, p0 + solverInternalNode0.velocityAngular * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_ANGULAR_COLOR)));
				lines.push_back(PxDebugLine(p1, p1 + solverInternalNode1.velocityLinear * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_LINEAR_COLOR)));
				lines.push_back(PxDebugLine(p1, p1 + solverInternalNode1.velocityAngular * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_ANGULAR_COLOR)));
			}
			else if (mode == DebugRenderMode::STRESS_GRAPH_BONDS_IMPULSES)
			{
				lines.push_back(PxDebugLine(center, center + solverInternalBondData.impulseLinear * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_LINEAR_COLOR)));
				lines.push_back(PxDebugLine(center, center + solverInternalBondData.impulseAngular * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_ANGULAR_COLOR)));
			}
		}
	}
}


} // namespace Blast
} // namespace Nv
