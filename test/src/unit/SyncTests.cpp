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
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#include "TkBaseTest.h"

#include "NvBlastExtSync.h"
#include "NvBlastTkEvent.h"

#include <map>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													ExtSync Tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Base
{
public:
	Base(TkTestStrict* test) : m_test(test)
	{

	}

	void run(std::stringstream& finalState)
	{
		//////// initial setup ////////

		m_test->createTestAssets();

		TkFramework* fwk = NvBlastTkFrameworkGet();

		TkGroupDesc gdesc;
		gdesc.workerCount = m_test->m_taskman->getCpuDispatcher()->getWorkerCount();
		m_group = fwk->createGroup(gdesc);
		EXPECT_TRUE(m_group != nullptr);

		TkActorDesc adesc(m_test->testAssets[0]);

		NvBlastID id;

		TkActor* actor0 = fwk->createActor(adesc);
		EXPECT_TRUE(actor0 != nullptr);
		families[0] = &actor0->getFamily();
		memcpy(id.data, "Mumble Jumble Bumble", sizeof(NvBlastID));	// Stuffing an arbitrary 16 bytes (The prefix of the given string)
		families[0]->setID(id);
		m_group->addActor(*actor0);

		TkActor* actor1 = fwk->createActor(adesc);
		EXPECT_TRUE(actor1 != nullptr);
		families[1] = &actor1->getFamily();
		memcpy(id.data, "buzzkillerdiller", sizeof(NvBlastID));	// Stuffing an arbitrary 16 bytes (The prefix of the given string)
		families[1]->setID(id);
		m_group->addActor(*actor1);

		m_test->m_groupTM->setGroup(m_group);

		//////// server/client specific impl ////////

		impl();


		//////// write out framework final state  ////////

		finalState.clear();
		for (auto family : families)
		{
			std::vector<TkActor*> actors(family->getActorCount());
			family->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
			for (auto actor : actors)
			{
				finalState << actor->getVisibleChunkCount();
				finalState << actor->getGraphNodeCount();
				std::vector<uint32_t> chunkIndices(actor->getGraphNodeCount());
				actor->getVisibleChunkIndices(chunkIndices.data(), (uint32_t)chunkIndices.size());

				for (uint32_t chunkIndex : chunkIndices)
					finalState << chunkIndex;
				const float* bondHealths = actor->getBondHealths();
				for (uint32_t i = 0; i < actor->getAsset()->getBondCount(); ++i)
					finalState << bondHealths[i];
			}
		}


		//////// release ////////

		m_group->release();

		for (auto family : families)
		{
			family->release();
		}

		m_test->releaseTestAssets();
	}

protected:
	virtual void impl() = 0;

	TkTestStrict*	m_test;
	TkGroup*		m_group;
	TkFamily*	families[2];
};


class Server : public Base
{
public:
	Server(TkTestStrict* test, std::vector<ExtSyncEvent*>& syncBuffer) : Base(test), m_syncBuffer(syncBuffer) {}

protected:
	virtual void impl() override
	{
		// create sync ext
		ExtSync* sync = ExtSync::create();

		// add sync as listener to family #1
		families[1]->addListener(*sync);

		// damage params
		CSParams cs0(1, 0.0f);
		NvBlastExtProgramParams csParams0 = { &cs0, nullptr };
		NvBlastExtRadialDamageDesc radialDamage0 = m_test->getRadialDamageDesc(0, 0, 0);
		NvBlastExtProgramParams radialParams0 = { &radialDamage0, nullptr };
		NvBlastExtRadialDamageDesc radialDamage1 = m_test->getRadialDamageDesc(0, 0, 0, 10.0f, 10.0f, 0.1f);
		NvBlastExtProgramParams radialParams1 = { &radialDamage1, nullptr };

		// damage family #0 (make it split)
		{
			TkActor* actor;
			families[0]->getActors(&actor, 1);
			actor->damage(m_test->getCubeSlicerProgram(), &csParams0);
		}

		// process
		m_test->m_groupTM->process();
		m_test->m_groupTM->wait();
		EXPECT_EQ(families[0]->getActorCount(), 2);

		// sync family #0
		sync->syncFamily(*families[0]);

		// add sync as listener to family #0
		families[0]->addListener(*sync);

		// damage family #0 (make it split fully)
		{
			TkActor* actor;
			families[0]->getActors(&actor, 1, 1);
			actor->damage(m_test->getFalloffProgram(), &radialParams0);
		}


		// damage family 1 (just damage bonds health)
		{
			TkActor* actor;
			families[1]->getActors(&actor, 1);
			NvBlastExtRadialDamageDesc radialDamage = m_test->getRadialDamageDesc(0, 0, 0, 10.0f, 10.0f, 0.1f);
			actor->damage(m_test->getFalloffProgram(), &radialParams1);
		}

		// process
		m_test->m_groupTM->process();
		m_test->m_groupTM->wait();
		EXPECT_EQ(families[0]->getActorCount(), 5);
		EXPECT_EQ(families[1]->getActorCount(), 1);

		// take sync buffer from sync
		{
			const ExtSyncEvent*const* buffer;
			uint32_t size;
			sync->acquireSyncBuffer(buffer, size);

			m_syncBuffer.resize(size);
			for (size_t i = 0; i < size; ++i)
			{
				m_syncBuffer[i] = buffer[i]->clone();
			}

			sync->releaseSyncBuffer();
		}

		// 
		families[0]->removeListener(*sync);
		families[1]->removeListener(*sync);

		// 
		sync->release();
	}

private:
	std::vector<ExtSyncEvent*>& m_syncBuffer;
};


class Client : public Base, public TkEventListener
{
public:
	Client(TkTestStrict* test, std::vector<ExtSyncEvent*>& syncBuffer) : Base(test), m_syncBuffer(syncBuffer) {}

protected:

	virtual void impl() override
	{
		ExtSync* sync = ExtSync::create();

		// fill map
		for (auto& family : families)
		{
			std::vector<TkActor*> actors(family->getActorCount());
			family->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
			auto& actorsSet = m_actorsPerFamily[family];
			for (auto actor : actors)
				EXPECT_TRUE(actorsSet.insert(actor->getIndex()).second);
		}

		// subscribe
		for (auto& family : families)
		{
			family->addListener(*this);
		}

		// apply sync buffer
		sync->applySyncBuffer(*NvBlastTkFrameworkGet(), (const Nv::Blast::ExtSyncEvent**)m_syncBuffer.data(), static_cast<uint32_t>(m_syncBuffer.size()), m_group);

		// check map
		for (auto& family : families)
		{
			std::vector<TkActor*> actors(family->getActorCount());
			family->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
			std::set<uint32_t> actorsSet;
			for (auto actor : actors)
				EXPECT_TRUE(actorsSet.insert(actor->getIndex()).second);
			EXPECT_TRUE(m_actorsPerFamily[family] == actorsSet);
		}

		// unsubscribe
		for (auto& family : families)
		{
			family->removeListener(*this);
		}

		m_test->m_groupTM->process();
		m_test->m_groupTM->wait();

		sync->release();
	}

	// listen for Split event and update actors map
	virtual void receive(const TkEvent* events, uint32_t eventCount) override
	{
		for (size_t i = 0; i < eventCount; ++i)
		{
			const TkEvent& e = events[i];
			switch (e.type)
			{
			case (TkEvent::Split) :
			{
				const TkSplitEvent* splitEvent = e.getPayload<TkSplitEvent>();
				auto& actorsSet = m_actorsPerFamily[splitEvent->parentData.family];
				if (!isInvalidIndex(splitEvent->parentData.index))
				{
					EXPECT_EQ((size_t)1, actorsSet.erase(splitEvent->parentData.index));
				}
				for (size_t i = 0; i < splitEvent->numChildren; ++i)
				{
					TkActor* a = splitEvent->children[i];
					EXPECT_TRUE(actorsSet.insert(a->getIndex()).second);
				}
				break;
			}
			case (TkEvent::FractureCommand) :
			{
				break;
			}
			case (TkEvent::JointUpdate) :
			{
				FAIL();
				break;
			}
			default:
				break;
			}
		}
	}

private:
	std::map<TkFamily*, std::set<uint32_t>> m_actorsPerFamily;
	std::vector<ExtSyncEvent*>& m_syncBuffer;
};

TEST_F(TkTestStrict, SyncTest1)
{
	this->createFramework();

	std::vector<ExtSyncEvent*> syncBuffer;

	std::stringstream serverFinalState;
	{
		Server s(this, syncBuffer);
		s.run(serverFinalState);
	}
	EXPECT_TRUE(syncBuffer.size() > 0);

	std::stringstream clientFinalState;
	{
		Client c(this, syncBuffer);
		c.run(clientFinalState);
	}

	for (auto e : syncBuffer)
	{
		e->release();
	}
	syncBuffer.clear();

	EXPECT_EQ(serverFinalState.str(), clientFinalState.str());

	this->releaseFramework();
}
