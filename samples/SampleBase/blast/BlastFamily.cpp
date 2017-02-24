/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "BlastFamily.h"
#include "SampleProfiler.h"
#include "PhysXController.h"
#include "RenderUtils.h"
#include "SampleTime.h"
#include "UIHelpers.h"

#include "NvBlast.h"
#include "NvBlastTkFamily.h"
#include "NvBlastTkActor.h"
#include "NvBlastTkAsset.h"
#include "NvBlastTkJoint.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxActor.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxManager.h"

#include "PxRigidDynamic.h"
#include "PxScene.h"
#include "PxJoint.h"


const float RIGIDBODY_DENSITY = 2000.0f;

const float BlastFamily::BOND_HEALTH_MAX = 1.0f;

BlastFamily::BlastFamily(PhysXController& physXController, ExtPxManager& pxManager, const BlastAsset& blastAsset) 
	: m_physXController(physXController)
	, m_pxManager(pxManager)
	, m_blastAsset(blastAsset)
	, m_listener(this)
	, m_totalVisibleChunkCount(0)
	, m_stressSolver(nullptr)
{
	m_settings.stressSolverEnabled = false;
	m_settings.stressDamageEnabled = false;

	m_settings.material = { 3.0f, 0.1f, 0.2f, 1.5f + 1e-5f, 0.95f };;
}

BlastFamily::~BlastFamily()
{
	if (m_stressSolver)
	{
		m_stressSolver->release();
	}

	m_pxFamily->unsubscribe(m_listener);

	m_pxFamily->release();
}

void BlastFamily::initialize(const BlastAsset::ActorDesc& desc)
{
	ExtPxFamilyDesc familyDesc;
	familyDesc.actorDesc.initialBondHealths = nullptr;
	familyDesc.actorDesc.initialSupportChunkHealths = nullptr;
	familyDesc.actorDesc.uniformInitialBondHealth = BOND_HEALTH_MAX;
	familyDesc.actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
	familyDesc.group = desc.group;
	familyDesc.pxAsset = m_blastAsset.getPxAsset();
	m_pxFamily = m_pxManager.createFamily(familyDesc);

	m_tkFamily = &m_pxFamily->getTkFamily();
	m_tkFamily->setID(desc.id);
	m_tkFamily->setMaterial(&m_settings.material);
	
	m_familySize = NvBlastFamilyGetSize(m_tkFamily->getFamilyLL(), nullptr);

	m_pxFamily->subscribe(m_listener);

	ExtPxSpawnSettings spawnSettings = {
		&m_physXController.getPhysXScene(),
		m_physXController.getDefaultMaterial(),
		RIGIDBODY_DENSITY
	};
	m_pxFamily->spawn(desc.transform, PxVec3(1.0f), spawnSettings);

	reloadStressSolver();
}

void BlastFamily::updatePreSplit(float dt)
{
	PROFILER_BEGIN("Stress Solver");
	// update stress
	m_stressSolveTime = 0;
	if (m_stressSolver)
	{
		Time t;
		m_stressSolver->update(m_settings.stressDamageEnabled);
		m_stressSolveTime += t.getElapsedSeconds();
	}
	PROFILER_END();

	// collect potential actors to health update
	m_actorsToUpdateHealth.clear();
	for (const ExtPxActor* actor : m_actors)
	{
		if (actor->getTkActor().isPending())
		{
			m_actorsToUpdateHealth.emplace(actor);
		}
	}
}

void BlastFamily::updateAfterSplit(float dt)
{
	PROFILER_BEGIN("Actor Health Update");
	for (const ExtPxActor* actor : m_actors)
	{
		onActorUpdate(*actor);

		// update health if neccessary
		if (m_actorsToUpdateHealth.find(actor) != m_actorsToUpdateHealth.end())
		{
			onActorHealthUpdate(*actor);
		}
	}
	PROFILER_END();

	PROFILER_BEGIN("Actor Misc Update");
	onUpdate();
	PROFILER_END();

	m_pxFamily->postSplitUpdate();
}

void BlastFamily::processActorCreated(ExtPxFamily&, ExtPxActor& actor)
{
	m_totalVisibleChunkCount += actor.getChunkCount();
	m_actors.emplace(&actor);

	onActorCreated(actor);
	onActorHealthUpdate(actor);
}

void BlastFamily::processActorDestroyed(ExtPxFamily&, ExtPxActor& actor)
{
	m_totalVisibleChunkCount -= actor.getChunkCount();
	m_physXController.notifyRigidDynamicDestroyed(&actor.getPhysXActor());

	onActorDestroyed(actor);

	m_actors.erase(m_actors.find(&actor));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Data Helpers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t BlastFamily::getActorCount() const
{
	return (uint32_t)m_tkFamily->getActorCount();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													  UI
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BlastFamily::drawUI()
{
	// Blast Material
	ImGui::Spacing();
	ImGui::Text("Blast Material:");
	ImGui::DragFloat("singleChunkThreshold", &m_settings.material.singleChunkThreshold);
	ImGui::DragFloat("graphChunkThreshold", &m_settings.material.graphChunkThreshold);
	ImGui::DragFloat("bondNormalThreshold", &m_settings.material.bondNormalThreshold);
	ImGui::DragFloat("bondTangentialThreshold", &m_settings.material.bondTangentialThreshold);
	ImGui::DragFloat("damageAttenuation", &m_settings.material.damageAttenuation);

	ImGui::Spacing();

	// Stress Solver Settings
	if (ImGui::Checkbox("Stress Solver Enabled", &m_settings.stressSolverEnabled))
	{
		reloadStressSolver();
	}

	if (m_settings.stressSolverEnabled)
	{
		// Settings
		bool changed = false;
		
		changed |= ImGui::DragInt("Bond Iterations Per Frame", (int*)&m_settings.stressSolverSettings.bondIterationsPerFrame, 100, 0, 500000);
		changed |= ImGui::DragFloat("Stress Linear Factor", &m_settings.stressSolverSettings.stressLinearFactor, 0.00001f, 0.0f, 10.0f, "%.6f");
		changed |= ImGui::DragFloat("Stress Angular Factor", &m_settings.stressSolverSettings.stressAngularFactor, 0.00001f, 0.0f, 10.0f, "%.6f");
		changed |= ImGui::SliderInt("Graph Reduction Level", (int*)&m_settings.stressSolverSettings.graphReductionLevel, 0, 32);
		if (changed)
		{
			refreshStressSolverSettings();
		}

		ImGui::Checkbox("Stress Damage Enabled", &m_settings.stressDamageEnabled);

		if (ImGui::Button("Recalculate Stress"))
		{
			resetStress();
		}
	}
}

void BlastFamily::drawStatsUI()
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(10, 255, 10, 255));
	const ExtStressSolver* stressSolver = m_stressSolver;
	if (stressSolver)
	{
		const float errorLinear = stressSolver->getStressErrorLinear();
		const float errorAngular = stressSolver->getStressErrorAngular();

		ImGui::Text("Stress Bond Count:               %d", stressSolver->getBondCount());
		ImGui::Text("Stress Iterations:	           %d (+%d)", stressSolver->getIterationCount(), stressSolver->getIterationsPerFrame());
		ImGui::Text("Stress Frames:                   %d", stressSolver->getFrameCount());
		ImGui::Text("Stress Error Lin / Ang:          %.4f / %.4f", errorLinear, errorAngular);
		ImGui::Text("Stress Solve Time:               %.3f ms", m_stressSolveTime * 1000);

		// plot errors
		{
			static float scale = 1.0f;
			scale = stressSolver->getFrameCount() <= 1 ? 1.0f : scale;
			scale = std::max<float>(scale, errorLinear);
			scale = std::max<float>(scale, errorAngular);

			static PlotLinesInstance<> linearErrorPlot;
			linearErrorPlot.plot("Stress Linear Error", errorLinear, "error/frame", 0.0f, 1.0f * scale);
			static PlotLinesInstance<> angularErrorPlot;
			angularErrorPlot.plot("Stress Angular Error", errorAngular, "error/frame", 0.0f, 1.0f * scale);
		}
	}
	else
	{
		ImGui::Text("No Stress Solver");
	}
	ImGui::PopStyleColor();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												  Stress Solver
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BlastFamily::setSettings(const Settings& settings)
{
	bool reloadStressSolverNeeded = (m_settings.stressSolverEnabled != settings.stressSolverEnabled);

	m_settings = settings;
	refreshStressSolverSettings();

	if (reloadStressSolverNeeded)
	{
		reloadStressSolver();
	}

	m_tkFamily->setMaterial(&m_settings.material);
}

void BlastFamily::refreshStressSolverSettings()
{
	if (m_stressSolver)
	{
		m_stressSolver->setSettings(m_settings.stressSolverSettings);
	}
}

void BlastFamily::resetStress()
{
	m_stressSolver->reset();
}

void BlastFamily::reloadStressSolver()
{
	if (m_stressSolver)
	{
		m_stressSolver->release();
		m_stressSolver = nullptr;
	}

	if (m_settings.stressSolverEnabled)
	{
		m_stressSolver = ExtStressSolver::create(*m_pxFamily, m_settings.stressSolverSettings);
		m_pxFamily->userData = m_stressSolver;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												  debug render
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const DirectX::XMFLOAT4 BOND_NORMAL_COLOR(0.0f, 0.8f, 1.0f, 1.0f);
const DirectX::XMFLOAT4 BOND_INVISIBLE_COLOR(0.65f, 0.16f, 0.16f, 1.0f);
const DirectX::XMFLOAT4 BOND_IMPULSE_LINEAR_COLOR(0.0f, 1.0f, 0.0f, 1.0f);
const DirectX::XMFLOAT4 BOND_IMPULSE_ANGULAR_COLOR(1.0f, 0.0f, 0.0f, 1.0f);
const DirectX::XMFLOAT4 JOINT_COLOR(0.5f, 0.6f, 7.0f, 1.0f);


inline void pushCentroid(std::vector<PxDebugLine>& lines, PxVec3 pos, PxU32 color, const float& area, const PxVec3& normal)
{
	// draw square of area 'area' rotated by normal
	{
		// build world rotation
		PxVec3 n0(0, 0, 1);
		PxVec3 n1 = normal;
		PxVec3 axis = n0.cross(n1);
		float d = n0.dot(n1);
		PxQuat q(axis.x, axis.y, axis.z, 1.f + d);
		q.normalize();
		float e = PxSqrt(1.0f / 2.0f);
		float r = PxSqrt(area);

		// transform all 4 square points
		PxTransform t(pos, q);
		PxVec3 p0 = t.transform(PxVec3(-e,  e, 0) * r);
		PxVec3 p1 = t.transform(PxVec3( e,  e, 0) * r);
		PxVec3 p2 = t.transform(PxVec3( e, -e, 0) * r);
		PxVec3 p3 = t.transform(PxVec3(-e, -e, 0) * r);

		// push square edges
		lines.push_back(PxDebugLine(p0, p1, color));
		lines.push_back(PxDebugLine(p3, p2, color));
		lines.push_back(PxDebugLine(p1, p2, color));
		lines.push_back(PxDebugLine(p0, p3, color));
	}

	// draw normal
	lines.push_back(PxDebugLine(pos, pos + normal * 0.5f, XMFLOAT4ToU32Color(BOND_NORMAL_COLOR)));
}

inline DirectX::XMFLOAT4 bondHealthColor(float healthFraction)
{
	const DirectX::XMFLOAT4 BOND_HEALTHY_COLOR(0.0f, 1.0f, 0.0f, 1.0f);
	const DirectX::XMFLOAT4 BOND_MID_COLOR(1.0f, 1.0f, 0.0f, 1.0f);
	const DirectX::XMFLOAT4 BOND_BROKEN_COLOR(1.0f, 0.0f, 0.0f, 1.0f);

	return healthFraction < 0.5 ? XMFLOAT4Lerp(BOND_BROKEN_COLOR, BOND_MID_COLOR, 2.0f * healthFraction) : XMFLOAT4Lerp(BOND_MID_COLOR, BOND_HEALTHY_COLOR, 2.0f * healthFraction - 1.0f);
}

void BlastFamily::fillDebugRender(DebugRenderBuffer& debugRenderBuffer, DebugRenderMode mode, float renderScale)
{
	const NvBlastChunk* chunks = m_tkFamily->getAsset()->getChunks();
	const NvBlastBond* bonds = m_tkFamily->getAsset()->getBonds();
	const NvBlastSupportGraph graph = m_tkFamily->getAsset()->getGraph();

	for (const ExtPxActor* pxActor : m_actors)
	{
		TkActor& actor = pxActor->getTkActor();
		uint32_t lineStartIndex = (uint32_t)debugRenderBuffer.m_lines.size();

		uint32_t nodeCount = actor.getGraphNodeCount();
		if (nodeCount == 0) // subsupport chunks don't have graph nodes
			continue;

		std::vector<uint32_t> nodes(actor.getGraphNodeCount());
		actor.getGraphNodeIndices(nodes.data(), static_cast<uint32_t>(nodes.size()));

		if (DEBUG_RENDER_HEALTH_GRAPH <= mode && mode <= DEBUG_RENDER_HEALTH_GRAPH_CENTROIDS)
		{
			const float* bondHealths = actor.getBondHealths();

			const ExtPxChunk* pxChunks = m_blastAsset.getPxAsset()->getChunks();

			for (uint32_t node0 : nodes)
			{
				const uint32_t chunkIndex0 = graph.chunkIndices[node0];
				const NvBlastChunk& blastChunk0 = chunks[chunkIndex0];
				const ExtPxChunk& assetChunk0 = pxChunks[chunkIndex0];

				for (uint32_t adjacencyIndex = graph.adjacencyPartition[node0]; adjacencyIndex < graph.adjacencyPartition[node0 + 1]; adjacencyIndex++)
				{
					uint32_t node1 = graph.adjacentNodeIndices[adjacencyIndex];
					const uint32_t chunkIndex1 = graph.chunkIndices[node1];
					const NvBlastChunk& blastChunk1 = chunks[chunkIndex1];
					const ExtPxChunk& assetChunk1 = pxChunks[chunkIndex1];
					if (node0 > node1)
						continue;

					bool invisibleBond = assetChunk0.subchunkCount == 0 || assetChunk1.subchunkCount == 0;

					// health
					uint32_t bondIndex = graph.adjacentBondIndices[adjacencyIndex];
					float healthVal = PxClamp(bondHealths[bondIndex] / BOND_HEALTH_MAX, 0.0f, 1.0f);

					DirectX::XMFLOAT4 color = bondHealthColor(healthVal);

					const NvBlastBond& solverBond = bonds[bondIndex];
					const PxVec3& centroid = reinterpret_cast<const PxVec3&>(solverBond.centroid);

					// centroid
					if (mode == DEBUG_RENDER_HEALTH_GRAPH_CENTROIDS || mode == DEBUG_RENDER_CENTROIDS)
					{
						const PxVec3& normal = reinterpret_cast<const PxVec3&>(solverBond.normal);
						pushCentroid(debugRenderBuffer.m_lines, centroid, XMFLOAT4ToU32Color(invisibleBond ? BOND_INVISIBLE_COLOR : color), solverBond.area, normal.getNormalized());
					}

					// chunk connection (bond)
					if ((mode == DEBUG_RENDER_HEALTH_GRAPH || mode == DEBUG_RENDER_HEALTH_GRAPH_CENTROIDS) && !invisibleBond)
					{
						const PxVec3& c0 = reinterpret_cast<const PxVec3&>(blastChunk0.centroid);
						const PxVec3& c1 = reinterpret_cast<const PxVec3&>(blastChunk1.centroid);
						debugRenderBuffer.m_lines.push_back(PxDebugLine(c0, c1, XMFLOAT4ToU32Color(color)));
					}
				}
			}
		}

		// stress
		if (DEBUG_RENDER_STRESS_GRAPH <= mode && mode <= DEBUG_RENDER_STRESS_GRAPH_BONDS_IMPULSES)
		{
			if (m_stressSolver)
			{
				m_stressSolver->fillDebugRender(nodes, debugRenderBuffer.m_lines, (ExtStressSolver::DebugRenderMode)(mode - DEBUG_RENDER_STRESS_GRAPH), renderScale);
			}
		}

		// transform all added lines from local to global
		PxTransform localToGlobal = pxActor->getPhysXActor().getGlobalPose();
		for (uint32_t i = lineStartIndex; i < debugRenderBuffer.m_lines.size(); i++)
		{
			PxDebugLine& line = debugRenderBuffer.m_lines[i];
			line.pos0 = localToGlobal.transform(line.pos0);
			line.pos1 = localToGlobal.transform(line.pos1);
		}
	}

	// joints debug render
	if (mode == DEBUG_RENDER_JOINTS)
	{
		for (const ExtPxActor* pxActor : m_actors)
		{
			TkActor& actor = pxActor->getTkActor();
			const uint32_t jointCount = actor.getJointCount();
			if (jointCount > 0)
			{
				std::vector<TkJoint*> joints(jointCount);
				actor.getJoints(joints.data(), jointCount);
				for (auto joint : joints)
				{
					PxJoint* pxJoint = reinterpret_cast<PxJoint*>(joint->userData);
					if (pxJoint)
					{
						PxRigidActor *actor0, *actor1;
						pxJoint->getActors(actor0, actor1);
						auto lp0 = pxJoint->getLocalPose(PxJointActorIndex::eACTOR0);
						auto lp1 = pxJoint->getLocalPose(PxJointActorIndex::eACTOR1);
						PxVec3 p0 = actor0 ? actor0->getGlobalPose().transform(lp0).p : lp0.p;
						PxVec3 p1 = actor1 ? actor1->getGlobalPose().transform(lp1).p : lp1.p;
						debugRenderBuffer.m_lines.push_back(PxDebugLine(p0, p1, XMFLOAT4ToU32Color(JOINT_COLOR)));
					}
				}
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														action!!!
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class BlastOverlapCallback : public PxOverlapCallback
{
public:
	BlastOverlapCallback(ExtPxManager& pxManager, std::set<ExtPxActor*>& actorBuffer)
		: m_pxManager(pxManager), m_actorBuffer(actorBuffer), PxOverlapCallback(m_hitBuffer, sizeof(m_hitBuffer) / sizeof(m_hitBuffer[0])) {}

	PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
	{
		for (PxU32 i = 0; i < nbHits; ++i)
		{
			PxRigidDynamic* rigidDynamic = buffer[i].actor->is<PxRigidDynamic>();
			if (rigidDynamic)
			{
				ExtPxActor* actor = m_pxManager.getActorFromPhysXActor(*rigidDynamic);
				if (actor != nullptr)
				{
					m_actorBuffer.insert(actor);
				}
			}
		}
		return true;
	}

private:
	ExtPxManager&						m_pxManager;
	std::set<ExtPxActor*>&				m_actorBuffer;
	PxOverlapHit						m_hitBuffer[1000];
};

void BlastFamily::blast(PxVec3 worldPos, float damageRadius, float explosiveImpulse, std::function<void(ExtPxActor*)> damageFunction)
{
	std::set<ExtPxActor*> actorsToDamage;
#if 1
	BlastOverlapCallback overlapCallback(m_pxManager, actorsToDamage);
	m_physXController.getPhysXScene().overlap(PxSphereGeometry(damageRadius), PxTransform(worldPos), overlapCallback);
#else
	for (std::map<NvBlastActor*, PhysXController::Actor*>::iterator it = m_actorsMap.begin(); it != m_actorsMap.end(); it++)
	{
		actorsToDamage.insert(it->first);
	}
#endif

	for (auto actor : actorsToDamage)
	{
		damageFunction(actor);
	}

	if (explosiveImpulse > 0.0f)
	{
		explode(worldPos, damageRadius, explosiveImpulse);
	}
}


class ExplodeOverlapCallback : public PxOverlapCallback
{
public:
	ExplodeOverlapCallback(PxVec3 worldPos, float radius, float explosiveImpulse)
		: m_worldPos(worldPos)
		, m_radius(radius)
		, m_explosiveImpulse(explosiveImpulse)
		, PxOverlapCallback(m_hitBuffer, sizeof(m_hitBuffer) / sizeof(m_hitBuffer[0])) {}

	PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
	{
		for (PxU32 i = 0; i < nbHits; ++i)
		{
			PxRigidActor* actor = buffer[i].actor;
			PxRigidDynamic* rigidDynamic = actor->is<PxRigidDynamic>();
			if (rigidDynamic && !(rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
			{
				if (m_actorBuffer.find(rigidDynamic) == m_actorBuffer.end())
				{
					m_actorBuffer.insert(rigidDynamic);
					PxVec3 dr = rigidDynamic->getGlobalPose().transform(rigidDynamic->getCMassLocalPose()).p - m_worldPos;
					float distance = dr.magnitude();
					float factor = PxClamp(1.0f - (distance * distance) / (m_radius * m_radius), 0.0f, 1.0f);
					float impulse = factor * m_explosiveImpulse * RIGIDBODY_DENSITY;
					PxVec3 vel = dr.getNormalized() * impulse / rigidDynamic->getMass();
					rigidDynamic->setLinearVelocity(rigidDynamic->getLinearVelocity() + vel);
				}
			}
		}
		return true;
	}

private:
	PxOverlapHit					m_hitBuffer[1000];
	float							m_explosiveImpulse;
	std::set<PxRigidDynamic*>		m_actorBuffer;
	PxVec3							m_worldPos;
	float							m_radius;
};

void BlastFamily::explode(PxVec3 worldPos, float damageRadius, float explosiveImpulse)
{
	ExplodeOverlapCallback overlapCallback(worldPos, damageRadius, explosiveImpulse);
	m_physXController.getPhysXScene().overlap(PxSphereGeometry(damageRadius), PxTransform(worldPos), overlapCallback);

}
