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
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


#include "SceneController.h"
#include "RenderUtils.h"
#include "Utils.h"

#include "BlastAssetBoxes.h"
#include "BlastAssetModelSimple.h"
#include "BlastAssetModelSkinned.h"
#include "NvBlastExtPxAsset.h"
#include "NvBlastExtPxFamily.h"
#include "NvBlastExtPxManager.h"

#include "SampleAssetListParser.h"
#include "BlastReplay.h"
#include "Renderer.h"

#include "BlastController.h"
#include "CommonUIController.h"
#include "PhysXController.h"

#include "PxRigidDynamic.h"
#include <PsFastXml.h>
#include "PxInputDataFromPxFileBuf.h"

#include <algorithm>
#include <imgui.h>
#include <sstream>
#include <tuple>



//////// Simple hash function ////////
static NvBlastID generateIDFromString(const char* str)
{
	uint32_t h[4] = { 5381, 5381, 5381, 5381 };
	int i = 0;
	for (const char* ptr = str; *ptr; i = ((i + 1) & 3), ++ptr)
	{
		h[i] = ((h[i] << 5) + h[i]) ^ static_cast<uint32_t>(*ptr);
	}
	return *reinterpret_cast<NvBlastID*>(h);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Scenes Setup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const DirectX::XMFLOAT4 PICK_POINTER_ACTIVE_COLOR(1.0f, 0.f, 0.f, 0.6f);
const float RIGIDBODY_DENSITY = 2000.0f;


class SingleSceneAsset;

class SceneAsset
{
public:
	SceneAsset() : spawnCount(0) {}
	virtual ~SceneAsset() {}

	void initialize(Scene* scene)
	{
		m_scene = scene;
	}

	virtual const char* getID() const = 0;
	virtual const char* getName() const = 0;

	virtual void load() = 0;
	virtual void unload() = 0;
	virtual bool isLoaded() const = 0;
	virtual void spawn(PxVec3 shift) = 0;

	virtual ImVec4 getUIColor() const
	{
		return ImGui::GetStyle().Colors[ImGuiCol_Text];
	}

	uint32_t spawnCount;
protected:
	Scene*	m_scene;
};


class SceneActor
{
public:
	SceneActor() : removeOnReload(false) {}

	virtual ~SceneActor() {}
	virtual const char* getName() const = 0;
	virtual const char* getSubname(int subindex) const { return nullptr; }
	virtual ImVec4 getUIColor() const = 0;
	virtual void drawUI(int subindex) {}
	virtual void drawStatsUI(int subindex) {}
	virtual uint32_t getSubactorCount() const { return 0; }
	virtual PxVec3 getSpawnShift() const { return PxVec3(PxZero); }
	virtual void reload() {}
	virtual void removeSubactor(int subindex) {}

	bool	removeOnReload;
};


class Scene
{
public:
	struct ActorIndex
	{
		int		index;
		int		subindex;

		ActorIndex() { reset(); }
		ActorIndex(int i, int s = -1) : index(i), subindex(s) {}

		bool operator==(const ActorIndex& other) const
		{
			return index == other.index && subindex == other.subindex;
		}

		void reset()
		{
			index = -1;
			subindex = -1;
		}
	};

	Scene(Renderer& renderer, PhysXController& physXController, BlastController& blastController, CommonUIController& commonUIController) :
		m_renderer(renderer), m_physXController(physXController), m_blastController(blastController), m_commonUIController(commonUIController)
	{
	}

	~Scene()
	{
		removeAllSceneActors();

		for (uint32_t i = 0; i < m_assets.size(); i++)
		{
			SAFE_DELETE(m_assets[i]);
		}
		m_assets.clear();
		m_assetsByID.clear();
		m_tkAssetMap.clear();
	}

	void addAsset(SceneAsset* asset)
	{
		m_assets.push_back(asset);
		asset->initialize(this);
		m_assetsByID[asset->getID()] = asset;
	}

	void drawUI()
	{
		///////////////////////////////////////////////////////////////////////////////////////////
		// Assets Selection
		///////////////////////////////////////////////////////////////////////////////////////////
		{
			static int mode = 0;
			ImGui::RadioButton("Replace", &mode, 0); ImGui::SameLine();
			ImGui::RadioButton("Append", &mode, 1);

			ImGui::ListBoxHeader("Assets", (int)m_assets.size());
			for (uint32_t i = 0; i < m_assets.size(); ++i)
			{
				ImVec4 color = m_assets[i]->getUIColor();
				color.w = color.w * (m_assets[i]->isLoaded() ? 1.0f : 0.5f);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				if (ImGui::Selectable(m_assets[i]->getName(), m_lastSpawnedAsset == i))
				{
					m_lastSpawnedAsset = i;
					if (mode == 0)
					{
						removeAllSceneActors();
					}
					m_commonUIController.addDelayedCall([=]() { spawnAsset(m_lastSpawnedAsset); }, "Loading Asset");
				}
				ImGui::PopStyleColor();
			}
			ImGui::ListBoxFooter();
		}

		ImGui::Spacing();
		ImGui::Separator();

		///////////////////////////////////////////////////////////////////////////////////////////
		// Actors Selection
		///////////////////////////////////////////////////////////////////////////////////////////
		{
			// actor's list
			{
				int itemCount = 0;
				for (size_t i = 0; i < m_sceneActors.size(); ++i)
				{
					itemCount += 1 + m_sceneActors[i]->getSubactorCount();
				}

				ImGui::ListBoxHeader("Scene Actors", itemCount);
				for (int i = 0; i < (int)m_sceneActors.size(); ++i)
				{
					ImVec4 color = m_sceneActors[i]->getUIColor();
					ImGui::PushStyleColor(ImGuiCol_Text, color);

					const bool isSelected = (m_selectedActor.index == i);

					ImGui::PushID(i);
					if (ImGui::Selectable(m_sceneActors[i]->getName(), isSelected && m_selectedActor.subindex == -1))
					{
						setSelectedActor(i);
					}

					for (int s = 0; s < (int)m_sceneActors[i]->getSubactorCount(); ++s)
					{
						ImGui::PushID(s);
						if (ImGui::Selectable(m_sceneActors[i]->getSubname(s), isSelected && m_selectedActor.subindex == s))
						{
							setSelectedActor(i, s);
						}
						ImGui::PopID();
					}

					ImGui::PopID();
					ImGui::PopStyleColor();
				}
				ImGui::ListBoxFooter();
			}

			SceneActor* selectedActor = getSelectedActor();
			if (selectedActor)
			{
				if (ImGui::Button("Remove"))
				{
					removeSceneActor(m_selectedActor);
				}

				ImGui::SameLine();

				if (ImGui::Button("Reload"))
				{
					selectedActor->reload();
				}

				ImGui::SameLine();
			}

			if (ImGui::Button("Remove All"))
			{
				removeAllSceneActors();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload All (R)"))
			{
				reloadAllActors();
			}
		}

		ImGui::Spacing();
		ImGui::Spacing();

		///////////////////////////////////////////////////////////////////////////////////////////
		// Selected Actor
		///////////////////////////////////////////////////////////////////////////////////////////
		{
			SceneActor* selectedActor = getSelectedActor();
			if (selectedActor)
			{
				ImGui::Text("Selected Actor: ");
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, ImColor(40, 200, 80, 255));
				ImGui::Text(m_selectedActor.subindex >= 0 ? selectedActor->getSubname(m_selectedActor.subindex) : selectedActor->getName());
				ImGui::PopStyleColor();

				ImGui::Spacing();

				selectedActor->drawUI(m_selectedActor.subindex);
			}
			else
			{
				ImGui::Text("No Selected Actor");
			}
		}
	}

	void drawStatsUI()
	{
		SceneActor* selectedActor = getSelectedActor();
		if (selectedActor)
		{
			selectedActor->drawStatsUI(m_selectedActor.subindex);
		}
	}

	void spawnAsset(int32_t num)
	{
		m_lastSpawnedAsset = physx::PxClamp<int32_t>(num, -1, (uint32_t)m_assets.size() - 1);

		if (m_lastSpawnedAsset < 0)
		{
			return;
		}

		PxVec3 shift(PxZero);
		for (SceneActor* a : m_sceneActors)
		{
			shift += a->getSpawnShift();
		}

		SceneAsset* asset = m_assets[m_lastSpawnedAsset];
		asset->spawn(shift);
	}

	void addSceneActor(SceneActor* actor)
	{
		m_sceneActors.push_back(actor);
		if (!getSelectedActor())
		{
			setSelectedActor((uint32_t)m_sceneActors.size() - 1);
		}
	}

	void removeSceneActor(ActorIndex actorIndex)
	{
		SceneActor* actor = getActorByIndex(actorIndex.index);
		if (actorIndex.subindex < 0)
		{
			delete actor;
			m_sceneActors.erase(std::remove(m_sceneActors.begin(), m_sceneActors.end(), actor));
		}
		else
		{
			actor->removeSubactor(actorIndex.subindex);

			if (actor->getSubactorCount() == 0)
			{
				removeSceneActor(ActorIndex(actorIndex.index, -1));
				return;
			}
		}

		SceneActor* selectedActor = getActorByIndex(m_selectedActor.index);
		if (selectedActor == nullptr)
		{
			if (!m_sceneActors.empty())
			{
				setSelectedActor((uint32_t)m_sceneActors.size() - 1);
			}
		}
		else
		{
			int subactorCount = selectedActor->getSubactorCount();
			if (m_selectedActor.subindex >= subactorCount || (m_selectedActor.subindex < 0 && subactorCount > 0))
			{
				setSelectedActor(m_selectedActor.index, subactorCount - 1);
			}
		}
	}

	void removeAllSceneActors()
	{
		for (SceneActor* a : m_sceneActors)
		{
			delete a;
		}
		m_sceneActors.clear();
		setSelectedActor(-1);
	}

	void setSelectedActor(int index, int subindex = -1)
	{
		m_selectedActor.index = physx::PxClamp<int32_t>(index, -1, (uint32_t)m_sceneActors.size() - 1);
		m_selectedActor.subindex = subindex;
	}

	SceneActor* getSelectedActor() const
	{
		return getActorByIndex(m_selectedActor.index);
	}

	SceneActor* getActorByIndex(int index) const
	{
		return (index >= 0 && index < (int)m_sceneActors.size()) ? m_sceneActors[index] : nullptr;
	}

	int releaseAll()
	{
		removeAllSceneActors();

		for (size_t i = 0; i < m_assets.size(); ++i)
		{
			m_assets[i]->unload();
		}

		const int currentAsset = m_lastSpawnedAsset;
		m_lastSpawnedAsset = -1;
		return currentAsset;
	}

	void reloadAllActors()
	{
		SceneActor* selectedActor = getSelectedActor();
		ActorIndex selectIndex(0);

		for (uint32_t i = 0; i < m_sceneActors.size(); i++)
		{
			if (m_sceneActors[i]->removeOnReload)
			{
				removeSceneActor(ActorIndex(i));
				i--;
			}
		}

		for (uint32_t i = 0; i < m_sceneActors.size(); i++)
		{
			if (m_sceneActors[i] == selectedActor)
			{
				selectIndex.index = i;
			}
			m_sceneActors[i]->reload();
		}

		setSelectedActor(selectIndex.index, selectIndex.subindex);
	}

	void registerTkAsset(const TkAsset& tkAsset, SingleSceneAsset* asset)
	{
		m_tkAssetMap[&tkAsset] = asset;
	}

	void unregisterTkAsset(const TkAsset& tkAsset)
	{
		m_tkAssetMap.erase(&tkAsset);
	}

	SingleSceneAsset* findSingleSceneAsset(const TkAsset& tkAsset)
	{
		auto entry = m_tkAssetMap.find(&tkAsset);
		return entry != m_tkAssetMap.end() ? entry->second : nullptr;
	}

	SceneAsset* findSceneAsset(const std::string& id)
	{
		auto entry = m_assetsByID.find(id);
		return entry != m_assetsByID.end() ? entry->second : nullptr;
	}


	//////// used controllers ////////

	Renderer& getRenderer() const
	{
		return m_renderer;
	}

	PhysXController& getPhysXController() const
	{
		return m_physXController;
	}

	BlastController& getBlastController() const
	{
		return m_blastController;
	}

	CommonUIController& getCommonUIController() const
	{
		return m_commonUIController;
	}

private:

	Renderer&			                              m_renderer;
	PhysXController&	                              m_physXController;
	BlastController&	                              m_blastController;
	CommonUIController&                               m_commonUIController;

	std::vector<SceneAsset*>                          m_assets;
	std::vector<SceneActor*>                          m_sceneActors;
	std::map<const TkAsset*, SingleSceneAsset*>		  m_tkAssetMap;
	std::map<std::string, SceneAsset*>				  m_assetsByID;

	int                                               m_lastSpawnedAsset;

	ActorIndex										  m_selectedActor;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Assets
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SingleSceneActor;

class SingleSceneAsset : public SceneAsset
{
public:
	SingleSceneAsset() : m_asset(nullptr) {}
	virtual ~SingleSceneAsset() { unload(); }

	virtual void spawn(PxVec3 shift) override;

	virtual void load() override
	{
		if (!m_asset)
		{
			m_asset = createAsset();
			m_scene->registerTkAsset(m_asset->getPxAsset()->getTkAsset(), this);
		}
	}

	virtual void unload() override
	{
		if (m_asset)
		{
			m_scene->unregisterTkAsset(m_asset->getPxAsset()->getTkAsset());
			delete m_asset;
			m_asset = nullptr;
		}
	}

	virtual bool isLoaded() const override
	{
		return m_asset != nullptr;
	}

	BlastAsset* getAsset() const
	{
		return m_asset;
	}

	virtual PxTransform getInitialTransform() = 0;

protected:
	virtual BlastAsset* createAsset() = 0;

private:
	BlastAsset* m_asset;
};


class ModelSceneAsset : public SingleSceneAsset
{
public:
	ModelSceneAsset() {}

	virtual const char* getID() const override{ return desc.id.c_str(); }
	virtual const char* getName() const override { return desc.name.c_str(); }

	AssetList::ModelAsset desc;

	virtual PxTransform getInitialTransform() { return desc.transform; }
};


class SimpleModelSceneAsset : public ModelSceneAsset
{
public:
	virtual BlastAsset* createAsset() 
	{ 
		return new BlastAssetModelSimple(m_scene->getBlastController().getTkFramework(), m_scene->getPhysXController().getPhysics(),
			m_scene->getPhysXController().getCooking(), *m_scene->getBlastController().getExtSerialization(), m_scene->getRenderer(), desc.file.c_str());
	}

	virtual ImVec4 getUIColor() const override
	{
		return ImColor(255, 255, 200, 255);
	}
};


class SkinnedModelSceneAsset : public ModelSceneAsset
{
public:
	virtual BlastAsset* createAsset()
	{ 
		return new BlastAssetModelSkinned(m_scene->getBlastController().getTkFramework(), m_scene->getPhysXController().getPhysics(),
			m_scene->getPhysXController().getCooking(), *m_scene->getBlastController().getExtSerialization(), m_scene->getRenderer(), desc.file.c_str());
	}

	virtual ImVec4 getUIColor() const override
	{
		return ImColor(255, 200, 255, 255);
	}
};


class BoxesSceneAsset : public SingleSceneAsset
{
public:
	BoxesSceneAsset(const AssetList::BoxAsset& d) : desc(d)
	{
		for (uint32_t lv = 0; lv < desc.levels.size(); ++lv)
		{
			const AssetList::BoxAsset::Level& level = desc.levels[lv];
			NvBlastChunkDesc::Flags fl = (level.isSupport) ? NvBlastChunkDesc::Flags::SupportFlag : NvBlastChunkDesc::Flags::NoFlags;
			assetDesc.generatorSettings.depths.push_back({ GeneratorAsset::Vec3(level.x, level.y, level.z), fl });
		}
		assetDesc.generatorSettings.extents = GeneratorAsset::Vec3(desc.extents.x, desc.extents.y, desc.extents.z);
		assetDesc.staticHeight = desc.staticHeight;
		assetDesc.jointAllBonds = desc.jointAllBonds;
		assetDesc.generatorSettings.bondFlags = (CubeAssetGenerator::BondFlags)desc.bondFlags;
	}

	virtual ImVec4 getUIColor() const override
	{
		return ImColor(255, 200, 200, 255);
	}

	virtual const char* getID() const override { return desc.id.c_str(); }
	virtual const char* getName() const override { return desc.name.c_str(); }


	AssetList::BoxAsset desc;
	BlastAssetBoxes::Desc assetDesc;

	virtual BlastAsset* createAsset() 
	{ 
		return new BlastAssetBoxes(m_scene->getBlastController().getTkFramework(), m_scene->getPhysXController().getPhysics(),
			m_scene->getPhysXController().getCooking(), m_scene->getRenderer(), assetDesc);
	}

	virtual PxTransform getInitialTransform() { return PxTransform(PxVec3(0, assetDesc.generatorSettings.extents.y / 2, 0)); }
};


class CompositeSceneAsset : public SceneAsset
{
public:
	CompositeSceneAsset(const AssetList::CompositeAsset& desc) 
		: m_desc(desc)
	{
	}

	virtual ~CompositeSceneAsset() { unload(); }

	virtual ImVec4 getUIColor() const override
	{
		return ImColor(200, 255, 255, 255);
	}

	virtual const char* getID() const override { return m_desc.id.c_str(); }
	virtual const char* getName() const override { return m_desc.name.c_str(); }

	virtual void spawn(PxVec3 shift) override;

	virtual void load() override
	{
		if (!isLoaded())
		{
			// load dependent assets
			for (const auto& assetRef : m_desc.assetRefs)
			{
				SceneAsset* asset = m_scene->findSceneAsset(assetRef.id);
				if (asset)
				{
					asset->load();
					m_sceneAssets.push_back(static_cast<SingleSceneAsset*>(asset));
				}
				else
				{
					m_scene->getCommonUIController().addPopupMessage("Error", "Wrong asset dependency on composite");
					m_sceneAssets.clear();
					return;
				}
			}

			// check joints
			for (const auto& joint : m_desc.joints)
			{
				bool ok = (joint.assetIndices[0] >= 0 || joint.assetIndices[1] >= 0);
				for (char k = 0; k < 2 && ok; ++k)
				{
					if (joint.assetIndices[k] < 0)
						continue;
					ok &= (joint.assetIndices[k] < (int32_t)m_sceneAssets.size());
					if (!ok)
						break;
					ok &= joint.chunkIndices[k] < m_sceneAssets[joint.assetIndices[k]]->getAsset()->getPxAsset()->getTkAsset().getChunkCount();
				}
				if (!ok)
				{
					m_scene->getCommonUIController().addPopupMessage("Error", "Wrong joint on composite");
					m_sceneAssets.clear();
					return;
				}
			}
		}
	}

	virtual void unload() override
	{
		m_sceneAssets.clear();
	}

	virtual bool isLoaded() const override
	{
		return !m_sceneAssets.empty();
	}

	virtual PxTransform getInitialTransform()
	{
		return m_desc.transform;
	}

	const AssetList::CompositeAsset& getDesc() const
	{
		return m_desc;
	}

	const std::vector<SingleSceneAsset*>& getSceneAssets() const
	{
		return m_sceneAssets;
	}

private:
	AssetList::CompositeAsset		m_desc;
	std::vector<SingleSceneAsset*>	m_sceneAssets;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Scene Actors
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SingleSceneActor : public SceneActor
{
public:
	SingleSceneActor(Scene& scene, SingleSceneAsset* asset, PxVec3 shift)
		: m_scene(scene)
		, m_asset(asset)
		, m_shift(shift)
	{
		m_index = m_asset->spawnCount++;
		spawn();
	}

	SingleSceneActor::~SingleSceneActor()
	{
		remove();
	}

	virtual const char* getName() const override
	{
		return m_name.c_str();
	}

	virtual const char* getSubname(int) const override
	{
		return nullptr;
	}

	virtual ImVec4 getUIColor() const override
	{
		return m_asset->getUIColor();
	}

	virtual void drawUI(int) override
	{
		m_actor->drawUI();
	}

	virtual void drawStatsUI(int) override
	{
		m_actor->drawStatsUI();
	}

	virtual PxVec3 getSpawnShift() const override
	{ 
		return PxVec3(-20, 0, 0); 
	}

	virtual void reload() override
	{
		auto settings = m_actor->getSettings();
		remove();
		spawn();
		m_actor->setSettings(settings);
	}

private:
	void remove()
	{
		m_scene.getBlastController().removeFamily(m_actor);
		m_actor = nullptr;
	}

	void spawn()
	{
		std::ostringstream str;
		str << m_asset->getName();
		if (m_index)
			str << " (" << m_index << ")";
		m_name = str.str();

		PxTransform pose = m_asset->getInitialTransform();
		pose.p += m_shift;

		BlastAsset::ActorDesc actorDesc = {
			actorDesc.id = generateIDFromString(m_name.c_str()),
			pose,
			m_scene.getBlastController().getTkGroup()
		};

		m_actor = m_scene.getBlastController().spawnFamily(m_asset->getAsset(), actorDesc);
	}

	Scene&					m_scene;
	BlastFamilyPtr			m_actor;
	SingleSceneAsset*		m_asset;
	PxVec3					m_shift;
	uint32_t				m_index;
	std::string				m_name;
};

class CompositeSceneActor : public SceneActor
{
public:
	CompositeSceneActor(Scene& scene, CompositeSceneAsset* asset, PxVec3 shift)
		: m_scene(scene)
		, m_asset(asset)
		, m_shift(shift)
	{
		m_index = m_asset->spawnCount++;
		spawn();
	}

	CompositeSceneActor::~CompositeSceneActor()
	{
		remove();
	}

	virtual uint32_t getSubactorCount() const
	{
		return (uint32_t)m_actors.size();
	}

	virtual const char* getName() const override
	{
		return m_name.c_str();
	}

	virtual const char* getSubname(int subindex) const override
	{
		return m_actors[subindex].name.c_str();
	}

	virtual ImVec4 getUIColor() const override
	{
		return m_asset->getUIColor();
	}

	virtual void drawUI(int subindex) override
	{
		if (subindex >= 0)
		{
			m_actors[subindex].actor->drawUI();
		}
		else
		{
			ImGui::Text("Select subactor to edit settings.");
		}
	}

	virtual void drawStatsUI(int subindex) override
	{
		if (subindex >= 0)
		{
			m_actors[subindex].actor->drawStatsUI();
		}
	}

	virtual PxVec3 getSpawnShift() const override
	{
		return PxVec3(-20, 0, 0);
	}

	virtual void reload() override
	{
		std::map<uint32_t, BlastFamily::Settings> settings;
		for (uint32_t i = 0; i < m_actors.size(); ++i)
		{
			settings[m_actors[i].initialIndex] = m_actors[i].actor->getSettings();
		}
		remove();
		spawn();
		for (uint32_t i = 0; i < m_actors.size(); ++i)
		{
			if (settings.find(i) != settings.end())
			{
				m_actors[i].actor->setSettings(settings[i]);
			}
		}
	}

	virtual void removeSubactor(int subindex)
	{
		if (subindex >= 0 && subindex < (int)m_actors.size())
		{
			m_scene.getBlastController().removeFamily(m_actors[subindex].actor);
			m_actors[subindex] = m_actors.back();
			m_actors.resize(m_actors.size() - 1);
		}
	}

private:
	void remove()
	{
		for (uint32_t i = 0; i < m_actors.size(); ++i)
		{
			m_scene.getBlastController().removeFamily(m_actors[i].actor);
		}
		m_actors.clear();
	}

	void spawn()
	{
		std::ostringstream str;
		str << m_asset->getName();
		if (m_index)
			str << " (" << m_index << ")";
		m_name = str.str();

		const AssetList::CompositeAsset& assetDesc = m_asset->getDesc();
		const std::vector<SingleSceneAsset*>& sceneAssets = m_asset->getSceneAssets();
		
		const uint32_t actorCount = (uint32_t)sceneAssets.size();
		m_actors.resize(actorCount);

		for (uint32_t i = 0; i < actorCount; ++i)
		{
			std::ostringstream str;
			str << "  -> " << i << "." << sceneAssets[i]->getName();
			m_actors[i].name = str.str();
		}

		ExtPxManager& pxManager = m_scene.getBlastController().getExtPxManager();
		for (uint32_t i = 0; i < actorCount; ++i)
		{
			PxTransform pose = m_asset->getInitialTransform();
			pose.p += m_shift;
			pose = assetDesc.assetRefs[i].transform.transform(pose);

			BlastAsset::ActorDesc actorDesc = {
				generateIDFromString(m_actors[i].name.c_str()),
				pose,
				m_scene.getBlastController().getTkGroup()
			};
			m_actors[i].actor = m_scene.getBlastController().spawnFamily(sceneAssets[i]->getAsset(), actorDesc);
			m_actors[i].initialIndex = i;
		}

		for (const auto& joint : assetDesc.joints)
		{
			TkJointDesc jointDesc;
			for (char k = 0; k < 2; ++k)
			{
				jointDesc.attachPositions[k] = joint.attachPositions[k];
				jointDesc.chunkIndices[k] = joint.chunkIndices[k];
				jointDesc.families[k] = (joint.assetIndices[k] < 0) ? nullptr :  &m_actors[joint.assetIndices[k]].actor->getFamily()->getTkFamily();
			}
			TkJoint* joint = pxManager.getFramework().createJoint(jointDesc);
			if (joint)
			{
				pxManager.createJoint(*joint);
			}
			else
			{
				m_scene.getCommonUIController().addPopupMessage("Error", "Some joints can't be created");
			}
		}
	}

	struct Subactor
	{
		BlastFamilyPtr			actor;
		uint32_t				initialIndex;
		std::string				name;
	};

	Scene&                             m_scene;
	std::vector<Subactor>			   m_actors;
	CompositeSceneAsset*               m_asset;
	PxVec3                             m_shift;
	uint32_t                           m_index;
	std::string                        m_name;
};

class PhysXSceneActor : public SceneActor
{
public:
	PhysXSceneActor(PhysXController& physXController, PhysXController::Actor* actor, const char* name)
	: m_physXController(physXController)
	, m_actor(actor)
	, m_name(name)
	{
	}

	PhysXSceneActor::~PhysXSceneActor()
	{
		m_physXController.removePhysXPrimitive(m_actor);
	}

	virtual const char* getName() const override
	{
		return m_name;
	}

	virtual ImVec4 getUIColor() const override
	{
		return ImColor(255, 100, 100, 255);
	}


private:
	PhysXController& m_physXController;
	PhysXController::Actor* m_actor;
	const char* m_name;
	
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Assets Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SingleSceneAsset::spawn(PxVec3 shift)
{
	load();
	SingleSceneActor* actor = new SingleSceneActor(*m_scene, this, shift);
	m_scene->addSceneActor(actor);
}

void CompositeSceneAsset::spawn(PxVec3 shift)
{
	load();
	if (isLoaded())
	{
		CompositeSceneActor* actor = new CompositeSceneActor(*m_scene, this, shift);
		m_scene->addSceneActor(actor);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												PackmanConfigParser
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PackmanConfigParser : public physx::shdfnd::FastXml::Callback
{
public:
	std::vector<std::pair<std::string, std::string>> dependencies;

protected:

	// encountered a comment in the XML
	virtual bool processComment(const char* /*comment*/)
	{
		return true;
	}

	virtual bool processClose(const char* elementName, unsigned int /*depth*/, bool& /*isError*/)
	{
		return true;
	}

	// return true to continue processing the XML document, false to skip.
	virtual bool processElement(const char* elementName, // name of the element
		const char* elementData, // element data, null if none
		const physx::shdfnd::FastXml::AttributePairs& attr,
		int /*lineno*/) // line number in the source XML file
	{
		if (::strcmp(elementName, "dependency") == 0)
		{
			dependencies.resize(dependencies.size() + 1);
			for (int i = 0; i < attr.getNbAttr(); ++i)
			{
				if (::strcmp(attr.getKey(i), "name") == 0)
				{
					dependencies.back().first = std::string(attr.getValue(i));
				}
				else if (::strcmp(attr.getKey(i), "version") == 0)
				{
					dependencies.back().second = std::string(attr.getValue(i));
				}
			}
		}
		return true;
	}
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Controller
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SceneController::SceneController() : m_cubeScale(1.0f), m_cubeThrowDownTime(-1.f)
{
	m_scene = NULL;
}

SceneController::~SceneController()
{
}

void SceneController::onSampleStart()
{
	// setup camera
	CFirstPersonCamera* camera = &getRenderer().getCamera();
	DirectX::XMVECTORF32 lookAtPt = { 0, 10, 0, 0 };
	DirectX::XMVECTORF32 eyePt = { 0, 20, 60, 0 };
	camera->SetViewParams(eyePt, lookAtPt);
	camera->SetRotateButtons(false, false, true, false);
	camera->SetEnablePositionMovement(true);

	// setup scene
	m_scene = new Scene(getRenderer(), getPhysXController(), getBlastController(), getCommonUIController());

	const SampleConfig& config = getManager()->getConfig();

	// add packman repo to search dirs
	bool packmanResourcesAdded = false;
	if (const char* packmanPath = std::getenv("PM_PACKAGES_ROOT"))
	{
		const char* RESOURCES_CONFIG_FILE = "resources.xml";

		std::string path;
		if (getRenderer().getResourceManager().findFile(RESOURCES_CONFIG_FILE, path))
		{
			physx::PsFileBuffer fileBuffer(path.c_str(), physx::general_PxIOStream2::PxFileBuf::OPEN_READ_ONLY);
			if (fileBuffer.isOpen())
			{
				PxInputDataFromPxFileBuf inputData(fileBuffer);
				PackmanConfigParser parser;
				physx::shdfnd::FastXml* xml = physx::shdfnd::createFastXml(&parser);
				xml->processXml(inputData, false);
				xml->release();
				for (auto& dep : parser.dependencies)
				{
					std::stringstream ss;
					ss << packmanPath << "\\" << dep.first << "\\" << dep.second;
					if (getRenderer().getResourceManager().addSearchDir(ss.str().c_str()))
					{
						packmanResourcesAdded = true;
					}
				}
			}
		}
	}
	if (!packmanResourcesAdded)
	{
		getManager()->getCommonUIController().addPopupMessage("Error", "BlastSampleResources package wasn't found. Consider running download_sample_resources.bat in root folder.", 5.0f);
	}

	// parse asset file
	AssetList assetList;
	if (!config.assetsFile.empty())
	{
		std::string path;
		if (getRenderer().getResourceManager().findFile(config.assetsFile, path))
		{
			parseAssetList(assetList, path);
		}
	}

	// add both asset file and asset list from config
	addAssets(config.additionalAssetList, true); // only used for command line assets
	addAssets(assetList, packmanResourcesAdded);

	// prepare scene
	spawnAsset(0);
}

void SceneController::addAssets(const AssetList& assetList, bool loadModels)
{
	if (loadModels)
	{
		for (const auto& model : assetList.models)
		{
			ModelSceneAsset* asset;
			if (!model.isSkinned)
			{
				asset = new SimpleModelSceneAsset();
			}
			else
			{
				asset = new SkinnedModelSceneAsset();
			}
			asset->desc = model;
			m_scene->addAsset(asset);
		}

		for (const auto& composite : assetList.composites)
		{
			m_scene->addAsset(new CompositeSceneAsset(composite));
		}
	}

	for (const auto& box : assetList.boxes)
	{
		BoxesSceneAsset* asset = new BoxesSceneAsset(box);
		m_scene->addAsset(asset);
	}
}

void SceneController::onInitialize()
{

}

void SceneController::onSampleStop()
{
	if (NULL != m_scene)
	{
		delete m_scene;
		m_scene = nullptr;
	}
}

void SceneController::onTerminate() 
{
}

void SceneController::Animate(double dt)
{
}

LRESULT SceneController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
	{
		int iKeyPressed = static_cast<int>(wParam);
		switch (iKeyPressed)
		{
		case 'R':
			m_scene->reloadAllActors();
			return 0;
		case 'F':
			if (m_cubeThrowDownTime == -1.f)
			{
				m_cubeThrowDownTime = ImGui::GetTime();
			}
			return 0;
		default:
			break; 
		}
	}
	else if (uMsg == WM_KEYUP)
	{
		int iKeyPressed = static_cast<int>(wParam);
		switch (iKeyPressed)
		{
		case 'F':
			throwCube();
			m_cubeThrowDownTime = -1.f;
			return 0;
		default:
			break;
		}
	}

	return 1;
}

void SceneController::drawUI()
{
	///////////////////////////////////////////////////////////////////////////////////////////
	// Scene UI
	///////////////////////////////////////////////////////////////////////////////////////////

	m_scene->drawUI();


	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();

	///////////////////////////////////////////////////////////////////////////////////////////
	// Replay
	///////////////////////////////////////////////////////////////////////////////////////////
	{
		ImGui::Text("Replay Control:");

		BlastReplay* replay = getBlastController().getReplay();
		if (replay->isRecording())
		{
			auto getAnimStr = []()
			{
				const uint32_t count = 5;
				const uint64_t periodMS = 150;
				static char str[count + 1] = "";
				for (uint32_t i = 0; i < count; i++)
				{
					uint64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
					str[i] = (i == (ts % (periodMS * count)) / periodMS) ? '*' : ' ';
				}
				return str;
			};
			ImGui::Text("State: Recording [%s] | Events: %d", getAnimStr(), replay->getEventCount());

			if (ImGui::Button("Stop Recording"))
			{
				replay->stopRecording();
			}
		}
		else if (replay->isPlaying())
		{
			ImGui::Text("State: Playing | Events: %d / %d", replay->getCurrentEventIndex(), replay->getEventCount());

			if (ImGui::Button("Stop Playing"))
			{
				replay->stopPlayback();
			}
		}
		else
		{
			ImGui::Text("State: Idle | Events: %d", replay->getEventCount());

			static bool syncFamilies = true;
			static bool syncPhysics = true;
			
			ImGui::Checkbox("Sync Initial Actors", &syncFamilies);
			if (ImGui::Checkbox("Sync Initial Transforms", &syncPhysics))
			{
				syncFamilies = syncPhysics;
			}

			if (ImGui::Button("Start Recording"))
			{
				replay->startRecording(getBlastController().getExtPxManager(), syncFamilies, syncPhysics);
			}

			if (replay->hasRecord())
			{
				static bool reload = false;
				if (ImGui::Button("Start Playback"))
				{
					if (reload)
						m_scene->reloadAllActors();
					replay->startPlayback(getBlastController().getExtPxManager(), getBlastController().getTkGroup());
				}
				ImGui::SameLine();
				ImGui::Checkbox("Reload Scene On Playback", &reload);
			}
		}
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();

	///////////////////////////////////////////////////////////////////////////////////////////
	// Cube
	///////////////////////////////////////////////////////////////////////////////////////////
	{
		ImGui::Text("Thrown Cube Params (F)");
		ImGui::DragFloat("Cube Size", &m_cubeScale, 1.0f, 0.0f, 100.0f);
		ImGui::Text("Cube Speed (hold F): %1.f", getCubeSpeed());
	}

}

void SceneController::drawStatsUI()
{
	m_scene->drawStatsUI();
}

float SceneController::getCubeSpeed()
{
	const float CUBE_VELOCITY_SPEED_MIN = 70;
	const float CUBE_VELOCITY_CHARGE_PER_SECOND = 300;
	return m_cubeThrowDownTime > 0 ? CUBE_VELOCITY_SPEED_MIN + (ImGui::GetTime() - m_cubeThrowDownTime) * CUBE_VELOCITY_CHARGE_PER_SECOND : 0.f;
}

void SceneController::throwCube()
{
	const float CUBE_DENSITY = 20000.0f;

	CFirstPersonCamera* camera = &getRenderer().getCamera();
	PxVec3 eyePos = XMVECTORToPxVec4(camera->GetEyePt()).getXYZ();
	PxVec3 lookAtPos = XMVECTORToPxVec4(camera->GetLookAtPt()).getXYZ();
	PhysXController::Actor* cube = getPhysXController().spawnPhysXPrimitiveBox(PxTransform(eyePos), PxVec3(m_cubeScale, m_cubeScale, m_cubeScale), CUBE_DENSITY);
	PxRigidDynamic* rigidDynamic = cube->getActor()->is<PxRigidDynamic>();
	cube->setColor(DirectX::XMFLOAT4(1, 0, 0, 1));

	PxVec3 dir = (lookAtPos - eyePos).getNormalized();
	rigidDynamic->setLinearVelocity(dir * getCubeSpeed());

	PhysXSceneActor* actor = new PhysXSceneActor(getPhysXController(), cube, "Cube");
	actor->removeOnReload = true;
	m_scene->addSceneActor(actor);
}

void SceneController::spawnAsset(int32_t num)
{
	m_scene->spawnAsset(num);
}

int SceneController::releaseAll()
{
	return m_scene->releaseAll();
}
