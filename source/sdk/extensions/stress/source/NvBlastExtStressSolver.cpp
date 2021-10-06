// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#include "NvBlastExtStressSolver.h"
#include "NvBlast.h"
#include "NvBlastGlobals.h"
#include "NvBlastArray.h"
#include "NvBlastHashMap.h"
#include "NvBlastHashSet.h"
#include "NvBlastAssert.h"
#include "NvBlastIndexFns.h"

#include <PsVecMath.h>
#include "PsFPU.h"
#include "NvBlastPxSharedHelpers.h"

#include <algorithm>

#define USE_SCALAR_IMPL 0
#define WARM_START 1
#define GRAPH_INTERGRIRY_CHECK 0

#if GRAPH_INTERGRIRY_CHECK
#include <set>
#endif


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

	const NodeData& getNodeData(uint32_t node) const
	{
		return m_nodesData[node];
	}

	const BondData& getBondData(uint32_t bond) const
	{
		return m_bondsData[bond];
	}

	uint32_t getBondCount() const
	{
		return m_bondsData.size();
	}

	uint32_t getNodeCount() const
	{
		return m_nodesData.size();;
	}

	void setNodeMassInfo(uint32_t node, float invMass, float invI)
	{
		m_nodesData[node].invMass = invMass;
		m_nodesData[node].invI = invI;
	}

	void initialize()
	{
		for (auto& node : m_nodesData)
		{
			node.velocityLinear = PxVec3(PxZero);
			node.velocityAngular = PxVec3(PxZero);
		}
	}

	void setNodeVelocities(uint32_t node, const PxVec3& velocityLinear, const PxVec3& velocityAngular)
	{
		m_nodesData[node].velocityLinear = velocityLinear;
		m_nodesData[node].velocityAngular = velocityAngular;
	}

	uint32_t addBond(uint32_t node0, uint32_t node1, const PxVec3& offset)
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

	void replaceWithLast(uint32_t bondIndex)
	{
		m_bondsData.replaceWithLast(bondIndex);
	}

	void reset(uint32_t nodeCount)
	{
		m_bondsData.clear();
		m_nodesData.resize(nodeCount);
	}

	void clearBonds()
	{
		m_bondsData.clear();
	}

	void solve(uint32_t iterationCount, bool warmStart = true)
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


	void iterate()
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

	Array<BondData>::type		m_bondsData;
	Array<NodeData>::type		m_nodesData;
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
		float	 stress;
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
		InlineArray<uint32_t, 8>::type blastBondIndices;
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

		resetImpulses();
	}

	const NodeData& getNodeData(uint32_t node) const
	{
		return m_nodesData[node];
	}

	const BondData& getBondData(uint32_t bond) const
	{
		return m_bondsData[bond];
	}

	const SolverNodeData& getSolverNodeData(uint32_t node) const
	{
		return m_solverNodesData[node];
	}

	const SolverBondData& getSolverBondData(uint32_t bond) const
	{
		return m_solverBondsData[bond];
	}

	const SequentialImpulseSolver::BondData& getSolverInternalBondData(uint32_t bond) const
	{
		return m_solver.getBondData(bond);
	}

	const SequentialImpulseSolver::NodeData& getSolverInternalNodeData(uint32_t node) const
	{
		return m_solver.getNodeData(node);
	}

	uint32_t getBondCount() const
	{
		return m_bondsData.size();
	}

	uint32_t getNodeCount() const
	{
		return m_nodesData.size();;
	}

	uint32_t getSolverBondCount() const
	{
		return m_solverBondsData.size();
	}

	uint32_t getSolverNodeCount() const
	{
		return m_solverNodesData.size();;
	}

	uint32_t getOverstressedBondCount() const
	{
		return m_overstressedBondCount;
	}

	float getSolverBondStressHealth(uint32_t bond, const ExtStressSolverSettings& settings) const
	{
		const auto& solverBond = getSolverInternalBondData(bond);
		const float impulse = solverBond.impulseLinear.magnitude() * settings.stressLinearFactor + solverBond.impulseAngular.magnitude() * settings.stressAngularFactor;
		// We then divide uniformly across bonds, which is obviously rough estimate.
		// Potentially we can add bond area there and norm across area sum
		const auto& blastBondIndices = m_solverBondsData[bond].blastBondIndices;
		return blastBondIndices.empty() ? 0.0f : impulse / (blastBondIndices.size() * settings.hardness);
	}

	void setNodeInfo(uint32_t node, float mass, float volume, PxVec3 localPos, bool isStatic)
	{
		m_nodesData[node].mass = mass;
		m_nodesData[node].volume = volume;
		m_nodesData[node].localPos = localPos;
		m_nodesData[node].isStatic = isStatic;
		m_nodesDirty = true;
	}

	void setNodeNeighborsCount(uint32_t node, uint32_t neighborsCount)
	{
		// neighbors count is expected to be the number of nodes on 1 island/actor.
		m_nodesData[node].neighborsCount = neighborsCount;

		// check for too huge aggregates (happens after island's split)
		if (!m_nodesDirty)
		{
			m_nodesDirty |= (m_solverNodesData[m_nodesData[node].solverNode].supportNodesCount > neighborsCount / 2);
		}
	}

	void addNodeForce(uint32_t node, const PxVec3& force, ExtForceMode::Enum mode)
	{
		const PxVec3 impuse = (mode == ExtForceMode::IMPULSE) ? force : force * m_nodesData[node].mass;
		m_nodesData[node].impulse += impuse;
	}

	void addNodeVelocity(uint32_t node, const PxVec3& velocity)
	{
		addNodeForce(node, velocity, ExtForceMode::VELOCITY);
	}

	void addNodeImpulse(uint32_t node, const PxVec3& impulse)
	{
		addNodeForce(node, impulse, ExtForceMode::IMPULSE);
	}

	void addBond(uint32_t node0, uint32_t node1, uint32_t blastBondIndex)
	{
		if (isInvalidIndex(m_blastBondIndexMap[blastBondIndex]))
		{
			const BondData data = {
				node0,
				node1,
				blastBondIndex,
				0.0f
			};
			m_bondsData.pushBack(data);
			m_blastBondIndexMap[blastBondIndex] = m_bondsData.size() - 1;
		}
	}

	void removeBondIfExists(uint32_t blastBondIndex)
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

	void setGraphReductionLevel(uint32_t level)
	{
		m_graphReductionLevel = level;
		m_nodesDirty = true;
	}

	uint32_t getGraphReductionLevel() const
	{
		return m_graphReductionLevel;
	}

	void solve(const ExtStressSolverSettings& settings, const float* bondHealth, bool warmStart = true)
	{
		sync();

		m_solver.initialize();

		for (const NodeData& node : m_nodesData)
		{
			const SequentialImpulseSolver::NodeData& solverNode = m_solver.getNodeData(node.solverNode);
			m_solver.setNodeVelocities(node.solverNode, solverNode.velocityLinear + node.impulse * solverNode.invMass, PxVec3(PxZero));
		}

		uint32_t iterationCount = ExtStressSolver::getIterationsPerFrame(settings, getSolverBondCount());
		m_solver.solve(iterationCount, warmStart);

		resetImpulses();

		updateBondStress(settings, bondHealth);
	}

	void calcError(float& linear, float& angular)
	{
		m_solver.calcError(linear, angular);
	}

	float getBondStress(uint32_t blastBondIndex)
	{
		const uint32_t bondIndex = m_blastBondIndexMap[blastBondIndex];
		return isInvalidIndex(bondIndex) ? 0.0f : m_bondsData[bondIndex].stress;
	}

private:

	void resetImpulses()
	{
		for (auto& node : m_nodesData)
		{
			node.impulse = PxVec3(PxZero);
		}
	}

	void updateBondStress(const ExtStressSolverSettings& settings, const float* bondHealth)
	{
		m_overstressedBondCount = 0;

		for (uint32_t i = 0; i < m_solverBondsData.size(); ++i)
		{
			const float stress = getSolverBondStressHealth(i, settings);
			const auto& blastBondIndices = m_solverBondsData[i].blastBondIndices;
			const float stressPerBond = blastBondIndices.size() > 0 ? stress / blastBondIndices.size() : 0.0f;
			for (auto blastBondIndex : blastBondIndices)
			{
				const uint32_t bondIndex = m_blastBondIndexMap[blastBondIndex];
				if (!isInvalidIndex(bondIndex))
				{
					BondData& bond = m_bondsData[bondIndex];

					NVBLAST_ASSERT(getNodeData(bond.node0).solverNode != getNodeData(bond.node1).solverNode);
					NVBLAST_ASSERT(bond.blastBondIndex == blastBondIndex);
					
					bond.stress = stressPerBond;

					if (stress > bondHealth[blastBondIndex])
					{
						m_overstressedBondCount++;
					}
				}
			}
		}
	}

	void sync()
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
		// NOTE (@anovoselov):  Recently, I found a flow in the algorithm below. In very rare situations aggregate (solver node)
		// can contain more then one connected component. I didn't notice it to produce any visual artifacts and it's 
		// unlikely to influence stress solvement a lot. Possible solution is to merge *whole* solver nodes, that
		// will raise complexity a bit (at least will add another loop on nodes for every reduction level. 
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
		for (BondData& bond : m_bondsData)
		{
			const NodeData& node0 = m_nodesData[bond.node0];
			const NodeData& node1 = m_nodesData[bond.node1];

			// reset stress, bond structure changed and internal bonds stress won't be updated during updateBondStress()
			bond.stress = 0.0f;

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
	Array<SolverNodeData>::type			m_solverNodesData;
	Array<SolverBondData>::type			m_solverBondsData;

	uint32_t							m_graphReductionLevel;

	bool	                            m_nodesDirty;
	bool	                            m_bondsDirty;

	uint32_t							m_overstressedBondCount;

	HashMap<BondKey, uint32_t>::type	m_solverBondsMap;
	Array<uint32_t>::type				m_blastBondIndexMap;

	Array<BondData>::type				m_bondsData;
	Array<NodeData>::type				m_nodesData;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											 ExtStressSolver
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ExtStressNodeCachedData
{
	physx::PxVec3 localPos;
	bool isStatic;
};


struct ExtStressBondCachedData
{
	uint32_t bondIndex;
};

/**
*/
class ExtStressSolverImpl final : public ExtStressSolver
{
	NV_NOCOPY(ExtStressSolverImpl)

public:
	ExtStressSolverImpl(NvBlastFamily& family, ExtStressSolverSettings settings);
	virtual void							release() override;


	//////// ExtStressSolverImpl interface ////////

	virtual void							setAllNodesInfoFromLL(float density = 1.0f) override;

	virtual void							setNodeInfo(uint32_t graphNode, float mass, float volume, NvcVec3 localPos, bool isStatic) override;

	virtual void							setSettings(const ExtStressSolverSettings& settings) override
	{
		m_settings = settings;
	}

	virtual const ExtStressSolverSettings&	getSettings() const override
	{
		return m_settings;
	}

	virtual bool
	addForce(const NvBlastActor& actor, NvcVec3 localPosition, NvcVec3 localForce,
	                      ExtForceMode::Enum mode) override;

	virtual void addForce(uint32_t graphNode, NvcVec3 localForce, ExtForceMode::Enum mode) override;

	virtual bool							addGravityForce(const NvBlastActor& actor, NvcVec3 localGravity) override;
	virtual bool							addAngularVelocity(const NvBlastActor& actor, NvcVec3 localCenterMass, NvcVec3 localAngularVelocity) override;

	virtual void							update() override;

	virtual uint32_t						getOverstressedBondCount() const override
	{
		return m_graphProcessor->getOverstressedBondCount();
	}

	virtual void							generateFractureCommands(const NvBlastActor& actor, NvBlastFractureBuffers& commands) override;
	virtual void							generateFractureCommands(NvBlastFractureBuffers& commands) override;
	virtual uint32_t						generateFractureCommandsPerActor(const NvBlastActor** actorBuffer, NvBlastFractureBuffers* commandsBuffer, uint32_t bufferSize) override;


	void									reset() override
	{
		m_reset = true;
	}

	virtual float							getStressErrorLinear() const override
	{
		return m_errorLinear;
	}

	virtual float							getStressErrorAngular() const override
	{
		return m_errorAngular;
	}

	virtual uint32_t						getFrameCount() const override
	{
		return m_framesCount;
	}

	virtual uint32_t						getBondCount() const override
	{
		return m_graphProcessor->getSolverBondCount();
	}

	virtual bool							notifyActorCreated(const NvBlastActor& actor) override;

	virtual void							notifyActorDestroyed(const NvBlastActor& actor) override;

	virtual const DebugBuffer				fillDebugRender(const uint32_t* nodes, uint32_t nodeCount, DebugRenderMode mode, float scale) override;


private:
	~ExtStressSolverImpl();


	//////// private methods ////////

	void									solve();

	void									fillFractureCommands(const NvBlastActor& actor, NvBlastFractureBuffers& commands);

	void									initialize();

	void									iterate();

	void									syncSolver();

	template<class T>
	T*										getScratchArray(uint32_t size);


	//////// data ////////

	struct ImpulseData
	{
		physx::PxVec3 position;
		physx::PxVec3 impulse;
	};

	NvBlastFamily&														m_family;
	HashSet<const NvBlastActor*>::type									m_activeActors;
	ExtStressSolverSettings												m_settings;
	NvBlastSupportGraph													m_graph;
	bool																m_isDirty;
	bool																m_reset;
	const float*														m_bondHealths;
	SupportGraphProcessor*												m_graphProcessor;
	float																m_errorAngular;
	float																m_errorLinear;
	uint32_t															m_framesCount;
	Array<NvBlastBondFractureData>::type								m_bondFractureBuffer;
	Array<uint8_t>::type												m_scratch;
	Array<DebugLine>::type												m_debugLineBuffer;
};


template<class T>
NV_INLINE T* ExtStressSolverImpl::getScratchArray(uint32_t size)
{
	const uint32_t scratchSize = sizeof(T) * size;
	if (m_scratch.size() < scratchSize)
	{
		m_scratch.resize(scratchSize);
	}
	return reinterpret_cast<T*>(m_scratch.begin());
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Creation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ExtStressSolverImpl::ExtStressSolverImpl(NvBlastFamily& family, ExtStressSolverSettings settings)
	: m_family(family), m_settings(settings), m_isDirty(false), m_reset(false), 
	m_errorAngular(std::numeric_limits<float>::max()), m_errorLinear(std::numeric_limits<float>::max()), m_framesCount(0)
{
	const NvBlastAsset* asset = NvBlastFamilyGetAsset(&m_family, logLL);
	NVBLAST_ASSERT(asset);

	m_graph = NvBlastAssetGetSupportGraph(asset, logLL);
	const uint32_t bondCount = NvBlastAssetGetBondCount(asset, logLL);

	m_bondFractureBuffer.reserve(bondCount);

	{
		NvBlastActor* actor;
		NvBlastFamilyGetActors(&actor, 1, &family, logLL);
		m_bondHealths = NvBlastActorGetBondHealths(actor, logLL);
	}

	m_graphProcessor = NVBLAST_NEW(SupportGraphProcessor)(m_graph.nodeCount, bondCount);

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
}

ExtStressSolverImpl::~ExtStressSolverImpl()
{
	NVBLAST_DELETE(m_graphProcessor, SupportGraphProcessor);
}

ExtStressSolver* ExtStressSolver::create(NvBlastFamily& family, ExtStressSolverSettings settings)
{
	return NVBLAST_NEW(ExtStressSolverImpl) (family, settings);
}

void ExtStressSolverImpl::release()
{
	NVBLAST_DELETE(this, ExtStressSolverImpl);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											Actors & Graph Data
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtStressSolverImpl::setAllNodesInfoFromLL(float density)
{
	const NvBlastAsset* asset = NvBlastFamilyGetAsset(&m_family, logLL);
	NVBLAST_ASSERT(asset);

	const uint32_t chunkCount = NvBlastAssetGetChunkCount(asset, logLL);
	const NvBlastChunk* chunks = NvBlastAssetGetChunks(asset, logLL);

	// traverse graph and fill node info
	for (uint32_t node0 = 0; node0 < m_graph.nodeCount; ++node0)
	{
		const uint32_t chunkIndex0 = m_graph.chunkIndices[node0];
		if (chunkIndex0 >= chunkCount)
		{
			// chunkIndex is invalid means it is static node (represents world)
			m_graphProcessor->setNodeInfo(node0, 0.0f, 0.0f, PxVec3(), true);
		}
		else
		{
			// Check if node is static. There is at maximum only one static node in LL that represents world, but we consider all nodes 
			// connected to it directly to be static too. It's better for general stress solver quality to have more then 1 static node.
			bool isNodeConnectedToStatic = false;
			for (uint32_t adjacencyIndex = m_graph.adjacencyPartition[node0]; adjacencyIndex < m_graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
			{
				uint32_t bondIndex = m_graph.adjacentBondIndices[adjacencyIndex];
				if (m_bondHealths[bondIndex] <= 0.0f)
					continue;
				uint32_t node1 = m_graph.adjacentNodeIndices[adjacencyIndex];
				uint32_t chunkIndex1 = m_graph.chunkIndices[node1];
				if (chunkIndex1 >= chunkCount)
				{
					isNodeConnectedToStatic = true;
					break;
				}
			}

			// fill node info
			const NvBlastChunk& chunk = chunks[chunkIndex0];
			const float volume = chunk.volume;
			const float mass = volume * density;
			const PxVec3 localPos = *reinterpret_cast<const PxVec3*>(chunk.centroid);
			m_graphProcessor->setNodeInfo(node0, mass, volume, localPos, isNodeConnectedToStatic);
		}
	}
}

void ExtStressSolverImpl::setNodeInfo(uint32_t graphNode, float mass, float volume, NvcVec3 localPos, bool isStatic)
{
	m_graphProcessor->setNodeInfo(graphNode, mass, volume, toPxShared(localPos), isStatic);
}

bool ExtStressSolverImpl::notifyActorCreated(const NvBlastActor& actor)
{
	const uint32_t graphNodeCount = NvBlastActorGetGraphNodeCount(&actor, logLL);
	if (graphNodeCount > 1)
	{
		// update neighbors
		{
			uint32_t* graphNodeIndices = getScratchArray<uint32_t>(graphNodeCount);
			const uint32_t nodeCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices, graphNodeCount, &actor, logLL);
			for (uint32_t i = 0; i < nodeCount; ++i)
			{
				m_graphProcessor->setNodeNeighborsCount(graphNodeIndices[i], nodeCount);
			}
		}

		m_activeActors.insert(&actor);
		m_isDirty = true;
		return true;
	}
	return false;
}

void ExtStressSolverImpl::notifyActorDestroyed(const NvBlastActor& actor)
{
	if (m_activeActors.erase(&actor))
	{
		m_isDirty = true;
	}
}

void ExtStressSolverImpl::syncSolver()
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

void ExtStressSolverImpl::initialize()
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
}

bool ExtStressSolverImpl::addForce(const NvBlastActor& actor, NvcVec3 localPosition, NvcVec3 localForce, ExtForceMode::Enum mode)
{
	float bestDist = FLT_MAX;
	uint32_t bestNode = invalidIndex<uint32_t>();

	const uint32_t graphNodeCount = NvBlastActorGetGraphNodeCount(&actor, logLL);
	if (graphNodeCount > 1)
	{
		uint32_t* graphNodeIndices = getScratchArray<uint32_t>(graphNodeCount);
		const uint32_t nodeCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices, graphNodeCount, &actor, logLL);

		for (uint32_t i = 0; i < nodeCount; ++i)
		{
			const uint32_t node = graphNodeIndices[i];
			const float sqrDist = (toPxShared(localPosition) - m_graphProcessor->getNodeData(node).localPos).magnitudeSquared();
			if (sqrDist < bestDist)
			{
				bestDist = sqrDist;
				bestNode = node;
			}
		}

		if (!isInvalidIndex(bestNode))
		{
			m_graphProcessor->addNodeForce(bestNode, toPxShared(localForce), mode);
			return true;
		}
	}
	return false;
}

void ExtStressSolverImpl::addForce(uint32_t graphNode, NvcVec3 localForce, ExtForceMode::Enum mode)
{
	m_graphProcessor->addNodeForce(graphNode, toPxShared(localForce), mode);
}

bool ExtStressSolverImpl::addGravityForce(const NvBlastActor& actor, NvcVec3 localGravity)
{
	const uint32_t graphNodeCount = NvBlastActorGetGraphNodeCount(&actor, logLL);
	if (graphNodeCount > 1)
	{
		uint32_t* graphNodeIndices = getScratchArray<uint32_t>(graphNodeCount);
		const uint32_t nodeCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices, graphNodeCount, &actor, logLL);

		for (uint32_t i = 0; i < nodeCount; ++i)
		{
			const uint32_t node = graphNodeIndices[i];
			m_graphProcessor->addNodeVelocity(node, toPxShared(localGravity));
		}
		return true;
	}
	return false;
}

bool ExtStressSolverImpl::addAngularVelocity(const NvBlastActor& actor, NvcVec3 localCenterMass, NvcVec3 localAngularVelocity)
{
	const uint32_t graphNodeCount = NvBlastActorGetGraphNodeCount(&actor, logLL);
	if (graphNodeCount > 1)
	{
		uint32_t* graphNodeIndices = getScratchArray<uint32_t>(graphNodeCount);
		const uint32_t nodeCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices, graphNodeCount, &actor, logLL);

		// Apply centrifugal force
		for (uint32_t i = 0; i < nodeCount; ++i)
		{
			const uint32_t node = graphNodeIndices[i];
			const auto& localPos = m_graphProcessor->getNodeData(node).localPos;
			// a = w x (w x r)
			const PxVec3 centrifugalAcceleration =
			    toPxShared(localAngularVelocity)
			        .cross(toPxShared(localAngularVelocity).cross(localPos - toPxShared(localCenterMass)));
			m_graphProcessor->addNodeVelocity(node, centrifugalAcceleration);
		}
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Update
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtStressSolverImpl::update()
{
	initialize();

	solve();

	m_framesCount++;
}

void ExtStressSolverImpl::solve()
{
	PX_SIMD_GUARD;

	m_graphProcessor->solve(m_settings, m_bondHealths, WARM_START && !m_reset);
	m_reset = false;

	m_graphProcessor->calcError(m_errorLinear, m_errorAngular);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Damage
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ExtStressSolverImpl::fillFractureCommands(const NvBlastActor& actor, NvBlastFractureBuffers& commands)
{
	const uint32_t graphNodeCount = NvBlastActorGetGraphNodeCount(&actor, logLL);
	uint32_t commandCount = 0;

	if (graphNodeCount > 1 && m_graphProcessor->getOverstressedBondCount() > 0)
	{
		uint32_t* graphNodeIndices = getScratchArray<uint32_t>(graphNodeCount);
		const uint32_t nodeCount = NvBlastActorGetGraphNodeIndices(graphNodeIndices, graphNodeCount, &actor, logLL);

		for (uint32_t i = 0; i < nodeCount; ++i)
		{
			const uint32_t node0 = graphNodeIndices[i];
			for (uint32_t adjacencyIndex = m_graph.adjacencyPartition[node0]; adjacencyIndex < m_graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
			{
				uint32_t node1 = m_graph.adjacentNodeIndices[adjacencyIndex];
				if (node0 < node1)
				{
					uint32_t bondIndex = m_graph.adjacentBondIndices[adjacencyIndex];
					const float bondHealth = m_bondHealths[bondIndex];
					const float bondStress = m_graphProcessor->getBondStress(bondIndex);

					if (bondHealth > 0.0f && bondStress > bondHealth)
					{
						const NvBlastBondFractureData data = {
							0,
							node0,
							node1,
							bondHealth
						};
						m_bondFractureBuffer.pushBack(data);
						commandCount++;
					}
				}
			}
		}
	}

	commands.chunkFractureCount = 0;
	commands.chunkFractures = nullptr;
	commands.bondFractureCount = commandCount;
	commands.bondFractures = commandCount > 0 ? m_bondFractureBuffer.end() - commandCount : nullptr;
}

void ExtStressSolverImpl::generateFractureCommands(const NvBlastActor& actor, NvBlastFractureBuffers& commands)
{
	m_bondFractureBuffer.clear();
	fillFractureCommands(actor, commands);
}

void ExtStressSolverImpl::generateFractureCommands(NvBlastFractureBuffers& commands)
{
	m_bondFractureBuffer.clear();

	const uint32_t bondCount = m_graphProcessor->getBondCount();
	const uint32_t overstressedBondCount = m_graphProcessor->getOverstressedBondCount();
	for (uint32_t i = 0; i < bondCount && m_bondFractureBuffer.size() < overstressedBondCount; i++)
	{
		const auto& bondData = m_graphProcessor->getBondData(i);
		const float bondHealth = m_bondHealths[bondData.blastBondIndex];
		if (bondHealth > 0.0f && bondData.stress > bondHealth)
		{
			const NvBlastBondFractureData data = {
				0,
				bondData.node0,
				bondData.node1,
				bondHealth
			};
			m_bondFractureBuffer.pushBack(data);
		}
	}

	commands.chunkFractureCount = 0;
	commands.chunkFractures = nullptr;
	commands.bondFractureCount = m_bondFractureBuffer.size();
	commands.bondFractures = m_bondFractureBuffer.size() > 0 ? m_bondFractureBuffer.begin() : nullptr;
}

uint32_t ExtStressSolverImpl::generateFractureCommandsPerActor(const NvBlastActor**  actorBuffer, NvBlastFractureBuffers* commandsBuffer, uint32_t bufferSize)
{
	if (m_graphProcessor->getOverstressedBondCount() == 0)
		return 0;

	m_bondFractureBuffer.clear();
	uint32_t index = 0;
	for (auto it = m_activeActors.getIterator(); !it.done() && index < bufferSize; ++it)
	{
		const NvBlastActor* actor = *it;
		NvBlastFractureBuffers& nextCommand = commandsBuffer[index];
		fillFractureCommands(*actor, nextCommand);
		if (nextCommand.bondFractureCount > 0)
		{
			actorBuffer[index] = actor;
			index++;
		}
	}
	return index;
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

const ExtStressSolver::DebugBuffer ExtStressSolverImpl::fillDebugRender(const uint32_t* nodes, uint32_t nodeCount, DebugRenderMode mode, float scale)
{
	const PxVec4 BOND_IMPULSE_LINEAR_COLOR(0.0f, 1.0f, 0.0f, 1.0f);
	const PxVec4 BOND_IMPULSE_ANGULAR_COLOR(1.0f, 0.0f, 0.0f, 1.0f);

	ExtStressSolver::DebugBuffer debugBuffer = { nullptr, 0 };

	if (m_isDirty)
		return debugBuffer;

	m_debugLineBuffer.clear();

	Array<uint8_t>::type& nodesSet = m_scratch;

	nodesSet.resize(m_graphProcessor->getSolverNodeCount());
	memset(nodesSet.begin(), 0, nodesSet.size() * sizeof(uint8_t));
	for (uint32_t i = 0; i < nodeCount; ++i)
	{
		NVBLAST_ASSERT(m_graphProcessor->getNodeData(nodes[i]).solverNode < nodesSet.size());
		nodesSet[m_graphProcessor->getNodeData(nodes[i]).solverNode] = 1;
	}

	const uint32_t bondCount = m_graphProcessor->getSolverBondCount();
	for (uint32_t i = 0; i < bondCount; ++i)
	{
		const auto& solverInternalBondData = m_graphProcessor->getSolverInternalBondData(i);
		if (nodesSet[solverInternalBondData.node0] != 0)
		{
			//NVBLAST_ASSERT(nodesSet[solverInternalBondData.node1] != 0);
			const auto& solverInternalNode0 = m_graphProcessor->getSolverInternalNodeData(solverInternalBondData.node0);
			const auto& solverInternalNode1 = m_graphProcessor->getSolverInternalNodeData(solverInternalBondData.node1);
			const auto& solverNode0 = m_graphProcessor->getSolverNodeData(solverInternalBondData.node0);
			const auto& solverNode1 = m_graphProcessor->getSolverNodeData(solverInternalBondData.node1);

			NvcVec3 p0 = fromPxShared(solverNode0.localPos);
			NvcVec3 p1 = fromPxShared(solverNode1.localPos);
			NvcVec3 center = (p0 + p1) * 0.5f;

			const float stress = std::min<float>(m_graphProcessor->getSolverBondStressHealth(i, m_settings), 1.0f);
			PxVec4 color = bondHealthColor(1.0f - stress);

			m_debugLineBuffer.pushBack(DebugLine(p0, p1, PxVec4ToU32Color(color)));

			float impulseScale = scale;

			if (mode == DebugRenderMode::STRESS_GRAPH_NODES_IMPULSES)
			{
				m_debugLineBuffer.pushBack(DebugLine(p0, p0 + fromPxShared(solverInternalNode0.velocityLinear) * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_LINEAR_COLOR)));
				m_debugLineBuffer.pushBack(DebugLine(p0, p0 + fromPxShared(solverInternalNode0.velocityAngular) * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_ANGULAR_COLOR)));
				m_debugLineBuffer.pushBack(DebugLine(p1, p1 + fromPxShared(solverInternalNode1.velocityLinear) * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_LINEAR_COLOR)));
				m_debugLineBuffer.pushBack(DebugLine(p1, p1 + fromPxShared(solverInternalNode1.velocityAngular) * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_ANGULAR_COLOR)));
			}
			else if (mode == DebugRenderMode::STRESS_GRAPH_BONDS_IMPULSES)
			{
				m_debugLineBuffer.pushBack(DebugLine(center, center + fromPxShared(solverInternalBondData.impulseLinear) * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_LINEAR_COLOR)));
				m_debugLineBuffer.pushBack(DebugLine(center, center + fromPxShared(solverInternalBondData.impulseAngular) * impulseScale, PxVec4ToU32Color(BOND_IMPULSE_ANGULAR_COLOR)));
			}
		}
	}

	debugBuffer.lines = m_debugLineBuffer.begin();
	debugBuffer.lineCount = m_debugLineBuffer.size();

	return debugBuffer;
}


} // namespace Blast
} // namespace Nv
