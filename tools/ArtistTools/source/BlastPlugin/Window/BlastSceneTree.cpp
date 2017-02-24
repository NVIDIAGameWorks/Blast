#include "BlastSceneTree.h"
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMenu>
#include <QtCore/QFileInfo>
#include <assert.h>
#include "ProjectParams.h"
#include <SimpleScene.h>
#include <BlastController.h>
#include <SceneController.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastTkAsset.h>
#include <NvBlastAsset.h>
#include <BlastFamilyModelSimple.h>
#include "GlobalSettings.h"

bool isChunkVisible(std::vector<BlastFamily*>& fs, uint32_t chunkIndex)
{
	int fsSize = fs.size();
	if (fsSize == 0)
	{
		return false;
	}

	bool visible = false;
	for (int i = 0; i < fsSize; i++)
	{
		if (fs[i]->isChunkVisible(chunkIndex))
		{
			visible = true;
			break;
		}
	}
	return visible;
}

void setChunkVisible(std::vector<BlastFamily*>& fs, uint32_t chunkIndex, bool visible)
{
	int fsSize = fs.size();
	if (fsSize == 0)
	{
		return;
	}

	for (int i = 0; i < fsSize; i++)
	{
		fs[i]->setChunkVisible(chunkIndex, visible);
	}
}

void setChunkSelected(std::vector<BlastFamily*>& fs, uint32_t chunkIndex, bool selected)
{
	int fsSize = fs.size();
	if (fsSize == 0)
	{
		return;
	}

	for (int i = 0; i < fsSize; i++)
	{
		fs[i]->setChunkSelected(chunkIndex, selected);
	}
}

void BlastChunkNode::setVisible(bool val)
{
	BPPChunk* pBPPChunk = (BPPChunk*)_data;
	pBPPChunk->visible = val;

	BlastAsset* pBlastAsset = (BlastAsset*)_assetPtr;

	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = sampleManager.getAssetFamiliesMap();
	std::vector<BlastFamily*>& fs = AssetFamiliesMap[pBlastAsset];

	setChunkVisible(fs, pBPPChunk->ID, val);
}

void BlastChunkNode::setSelected(bool val)
{
	BPPChunk* pBPPChunk = (BPPChunk*)_data;
	pBPPChunk->visible = val;

	BlastAsset* pBlastAsset = (BlastAsset*)_assetPtr;

	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = sampleManager.getAssetFamiliesMap();
	std::vector<BlastFamily*>& fs = AssetFamiliesMap[pBlastAsset];

	setChunkSelected(fs, pBPPChunk->ID, val);
}

void BlastAssetInstanceNode::setSelected(bool val)
{
	BPPAssetInstance* pBPPAssetInstance = (BPPAssetInstance*)_data;	
	std::string name = pBPPAssetInstance->name;

	std::string strAsset = name.substr(0, name.find_first_of("_"));
	BlastAsset* pBlastAsset = nullptr;
	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = sampleManager.getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itAssetDescMap;
	for (itAssetDescMap = AssetDescMap.begin();
		itAssetDescMap != AssetDescMap.end(); itAssetDescMap++)
	{
		AssetList::ModelAsset& model = itAssetDescMap->second;
		if (model.name == strAsset)
		{
			pBlastAsset = itAssetDescMap->first;
			break;
		}
	}

	std::string strIndex = name.substr(name.find_last_of("_") + 1);
	int nIndex = atoi(strIndex.c_str());	

	sampleManager.setCurrentSelectedInstance(pBlastAsset, nIndex);
}

BlastTreeData& BlastTreeData::ins()
{
	static BlastTreeData _ins;
	return _ins;
}

bool BlastTreeData::isChild(BlastChunkNode* parent, BlastChunkNode* child)
{
	if (parent == nullptr || child == nullptr)
		return false;

	BlastNode* curParent = child->getParent();
	while (eChunk == curParent->getType() && curParent != nullptr)
	{
		if (curParent == parent)
			return true;
		curParent = curParent->getParent();
	}

	return false;
}

std::vector<BlastChunkNode*> BlastTreeData::getTopChunkNodes(std::vector<BlastChunkNode*>& nodes)
{
	std::vector<BlastChunkNode*> result;

	for (size_t i = 0; i < nodes.size(); ++i)
	{
		bool isCurNodeTop = true;
		for (size_t j = 0; j < nodes.size(); ++j)
		{
			if (i != j && isChild(nodes[i], nodes[j]))
			{
				isCurNodeTop = false;
				break;
			}
		}

		if (isCurNodeTop)
		{
			result.push_back(nodes[i]);
		}
	}

	return result;
}

bool BlastTreeData::isRoot(BlastChunkNode* node)
{
	if (node == nullptr || node->getParent() == nullptr)
		return false;

	return eAsset == node->getParent()->getType();
}

bool BlastTreeData::isLeaf(BlastChunkNode* node)
{
	if (node == nullptr)
		return false;

	for (BlastNode* curNode : node->children)
	{
		if (eChunk == curNode->getType())
			return false;
	}

	return true;
}

void removeChunkNodeSupport(BlastChunkNode* node)
{
	if (node == nullptr)
		return;

	BPPChunk* chunk = static_cast<BPPChunk*>(node->getData());
	chunk->support = false;

	for (BlastNode* curNode : node->children)
	{
		if (eChunk == curNode->getType())
		{
			removeChunkNodeSupport((BlastChunkNode*)curNode);
		}
	}
}

void BlastTreeData::makeSupport(BlastChunkNode* node)
{
	if (node == nullptr)
		return;
	BPPChunk* chunk = static_cast<BPPChunk*>(node->getData());
	chunk->staticFlag = false;
	chunk->support = true;

	for (BlastNode* curNode : node->children)
	{
		if (eChunk == curNode->getType())
		{
			removeChunkNodeSupport((BlastChunkNode*)curNode);
		}
	}
}

void BlastTreeData::makeStaticSupport(BlastChunkNode* node)
{
	if (node == nullptr)
		return;
	BPPChunk* chunk = static_cast<BPPChunk*>(node->getData());
	chunk->staticFlag = true;
	chunk->support = true;

	for (BlastNode* curNode : node->children)
	{
		if (eChunk == curNode->getType())
		{
			removeChunkNodeSupport((BlastChunkNode*)curNode);
		}
	}
}

void BlastTreeData::removeSupport(BlastChunkNode* node)
{
	if (node == nullptr)
		return;

	if (isLeaf(node))
		return;

	BPPChunk* chunk = static_cast<BPPChunk*>(node->getData());
	chunk->support = false;
	chunk->staticFlag = false;

	for (BlastNode* curNode : node->children)
	{
		if (eChunk == curNode->getType())
		{
			BPPChunk* curChunk = static_cast<BPPChunk*>(curNode->getData());
			curChunk->support = true;
		}
	}
}

BlastNode* BlastTreeData::getBlastNodeByProjectData(void* data)
{
	std::map<void*, BlastNode*>::iterator itr = _blastProjectDataToNodeMap.find(data);
	if (itr != _blastProjectDataToNodeMap.end())
		return itr->second;

	return nullptr;
}

struct ChunkSupport
{
	ChunkSupport()
	{
		m_bSupport = false;
	}

	bool m_bSupport;
};

struct BondChunkIndices
{
	BondChunkIndices()
	{
		chunkIndices[0] = -1;
		chunkIndices[1] = -1;
	}

	void SetIndices(uint32_t chunkIndex0, uint32_t chunkIndex1)
	{
		if (chunkIndex0 < chunkIndex1)
		{
			chunkIndices[0] = chunkIndex0;
			chunkIndices[1] = chunkIndex1;
		}
		else
		{
			chunkIndices[0] = chunkIndex1;
			chunkIndices[1] = chunkIndex0;
		}
	}

	uint32_t chunkIndices[2];
};

BlastAssetNode* BlastTreeData::addAsset(const BlastAsset* asset)
{
	//to do
	return nullptr;
}
BlastAssetInstanceNode* BlastTreeData::addAssetInstance(const BlastAsset* asset)
{
	//to do
	return nullptr;
}

void BlastTreeData::remove(const BlastAssetNode* node)
{
	//to do
}

void BlastTreeData::remove(const BlastAssetInstanceNode* node)
{
	//to do
}

void BlastTreeData::update()
{
	_freeBlastNode();

	BPPBlast& blast = BlastProject::ins().getParams().blast;
	std::vector<BlastAsset*> BlastAssetVec;
	{
		SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();

		std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = sampleManager.getAssetFamiliesMap();
		std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = sampleManager.getAssetDescMap();
		
		BlastController& blastController = sampleManager.getBlastController();
		SceneController& sceneController = sampleManager.getSceneController();

		std::vector<BlastFamilyPtr>& families = blastController.getFamilies();
		int familiesSize = families.size();

		std::map<BlastAsset*, AssetList::ModelAsset>::iterator it;
		for (it = AssetDescMap.begin(); it != AssetDescMap.end(); it++)
		{
			BlastAssetVec.push_back(it->first);
		}
		int modelAssetsSize = AssetDescMap.size();
		
		std::vector<std::string> projectilesNames;
		sceneController.GetProjectilesNames(projectilesNames);

		// compoistie
		{
			BPPComposite& composite = blast.composite;
			composite.composite.buf = nullptr;
			composite.visible = true;

			copy(composite.composite, "BlastComposite");

			// asset instance array
			{
				BPPAssetInstanceArray& instanceArray = composite.blastAssetInstances;
				instanceArray.arraySizes[0] = familiesSize;
				if (familiesSize > 0)
				{
					instanceArray.buf = new BPPAssetInstance[familiesSize];					
					int curInstanceIndex = 0;
					char instancename[MAX_PATH];
					std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAssetFamiliesMap;
					for (itAssetFamiliesMap = AssetFamiliesMap.begin();
						itAssetFamiliesMap != AssetFamiliesMap.end(); itAssetFamiliesMap++)
					{
						BlastAsset* pBlastAsset = itAssetFamiliesMap->first;
						std::vector<BlastFamily*>& fs = itAssetFamiliesMap->second;
						int fsSize = fs.size();
						for (int i = 0; i < fsSize; i++)
						{
							BPPAssetInstance& instance = instanceArray.buf[curInstanceIndex];
							instance.name.buf = nullptr;
							instance.source.buf = nullptr;
							instance.visible = true;

							AssetList::ModelAsset desc = AssetDescMap[pBlastAsset];
							sprintf(instancename, "%s_Instance_%d", desc.name.c_str(), i);
							copy(instance.name, instancename);

							std::string assetFilePath = GlobalSettings::MakeFileName(GlobalSettings::Inst().m_projectFileDir.c_str(), std::string(desc.file + ".bpxa").c_str());
							sprintf(instancename, "%s", assetFilePath.c_str());
							copy(instance.source, instancename);

							PxVec3 p = desc.transform.p;
							PxQuat q = desc.transform.q;
							instanceArray.buf[curInstanceIndex].transform.position = nvidia::NvVec3(p.x, p.y, p.z);
							instanceArray.buf[curInstanceIndex].transform.rotation = nvidia::NvVec4(q.x, q.y, q.z, q.w);

							curInstanceIndex++;
						}
					}
				}
			}

			// landmark array
			if (0)
			{
				BPPLandmarkArray& landmarkArray = composite.landmarks;
				landmarkArray.buf = new BPPLandmark[2];
				landmarkArray.arraySizes[0] = 2;
				landmarkArray.buf[0].name.buf = nullptr;
				landmarkArray.buf[1].name.buf = nullptr;

				copy(landmarkArray.buf[0].name, "Landmark_1");
				copy(landmarkArray.buf[1].name, "Landmark_2");
			}
		}

		// asset array
		{
			BPPAssetArray& assetArray = blast.blastAssets;
			assetArray.arraySizes[0] = modelAssetsSize;
			if (modelAssetsSize > 0)
			{
				assetArray.buf = new BPPAsset[modelAssetsSize];
				int curAssetIndex = 0;

				blast.chunks.buf = nullptr;
				blast.chunks.arraySizes[0] = 0;
				blast.bonds.buf = nullptr;
				blast.bonds.arraySizes[0] = 0;

				std::map<BlastAsset*, AssetList::ModelAsset>::iterator itAssetDescMap;
				for (itAssetDescMap = AssetDescMap.begin();
					itAssetDescMap != AssetDescMap.end(); itAssetDescMap++)
				{
					BlastAsset* pBlastAsset = itAssetDescMap->first;
					AssetList::ModelAsset& desc = itAssetDescMap->second;
					std::vector<BlastFamily*>& fs = AssetFamiliesMap[pBlastAsset];

					BPPAsset& asset = assetArray.buf[curAssetIndex];
					asset.path.buf = nullptr;
					asset.activePreset.buf = nullptr;
					std::string assetFilePath = GlobalSettings::MakeFileName(GlobalSettings::Inst().m_projectFileDir.c_str(), std::string(desc.file + ".bpxa").c_str());
					copy(asset.path, assetFilePath.c_str());

					const ExtPxAsset* pExtPxAsset = pBlastAsset->getPxAsset();
					const ExtPxChunk* pExtPxChunk = pExtPxAsset->getChunks();

					const TkAsset& tkAsset = pExtPxAsset->getTkAsset();
					uint32_t chunkCount = tkAsset.getChunkCount();
					const NvBlastChunk* pNvBlastChunk = tkAsset.getChunks();
					uint32_t bondCount = tkAsset.getBondCount();
					const NvBlastBond* pNvBlastBond = tkAsset.getBonds();

					const NvBlastSupportGraph supportGraph = tkAsset.getGraph();
					uint32_t* chunkIndices = supportGraph.chunkIndices;
					uint32_t* adjacencyPartition = supportGraph.adjacencyPartition;
					uint32_t* adjacentNodeIndices = supportGraph.adjacentNodeIndices;
					uint32_t* adjacentBondIndices = supportGraph.adjacentBondIndices;

					ChunkSupport* pSupport = new ChunkSupport[chunkCount];
					BondChunkIndices* pBCIndices = new BondChunkIndices[bondCount];

					for (uint32_t node0 = 0; node0 < supportGraph.nodeCount; ++node0)
					{
						const uint32_t chunkIndex0 = supportGraph.chunkIndices[node0];

						pSupport[chunkIndex0].m_bSupport = true;

						for (uint32_t adjacencyIndex = adjacencyPartition[node0]; adjacencyIndex < adjacencyPartition[node0 + 1]; adjacencyIndex++)
						{
							uint32_t node1 = supportGraph.adjacentNodeIndices[adjacencyIndex];

							// add this condition if you don't want to iterate all bonds twice
							if (node0 > node1)
								continue;

							const uint32_t chunkIndex1 = supportGraph.chunkIndices[node1];

							uint32_t bondIndex = supportGraph.adjacentBondIndices[adjacencyIndex];

							pBCIndices[bondIndex].SetIndices(chunkIndex0, chunkIndex1);
						}
					}

					// chunks
					{
						BPPChunkArray curArray;
						curArray.buf = new BPPChunk[chunkCount];
						curArray.arraySizes[0] = chunkCount;
						char chunkname[10];
						for (int cc = 0; cc < chunkCount; ++cc)
						{
							BPPChunk& chunk = curArray.buf[cc];
							chunk.name.buf = nullptr;
							chunk.asset.buf = nullptr;

							std::vector<uint32_t> parentChunkIndexes;
							parentChunkIndexes.push_back(cc);
							uint32_t parentChunkIndex = cc;
							while ((parentChunkIndex = pNvBlastChunk[parentChunkIndex].parentChunkIndex) != -1)
							{
								parentChunkIndexes.push_back(parentChunkIndex);
							}

							std::string strChunkName = "Chunk";
							for (int pcIndex = parentChunkIndexes.size() - 1; pcIndex >= 0; pcIndex--)
							{
								sprintf(chunkname, "_%d", parentChunkIndexes[pcIndex]);
								strChunkName += chunkname;
							}
							copy(chunk.name, strChunkName.c_str());

							copy(chunk.asset, asset.path);
							chunk.ID = cc;
							chunk.parentID = pNvBlastChunk[cc].parentChunkIndex;
							chunk.staticFlag = pExtPxChunk[cc].isStatic;
							chunk.visible = isChunkVisible(fs, cc);
							chunk.support = pSupport[cc].m_bSupport;
						}

						merge(blast.chunks, curArray);
						freeBlast(curArray);
					}

					// bonds
					{
						BPPBondArray curArray;
						curArray.buf = new BPPBond[bondCount];
						curArray.arraySizes[0] = bondCount;
						char bondname[10];
						bool visible;
						for (int bc = 0; bc < bondCount; ++bc)
						{
							BPPBond& bond = curArray.buf[bc];
							bond.name.buf = nullptr;
							bond.asset.buf = nullptr;

							visible = isChunkVisible(fs, pBCIndices[bc].chunkIndices[0])
								|| isChunkVisible(fs, pBCIndices[bc].chunkIndices[1]);
							bond.visible = visible;
							bond.fromChunk = pBCIndices[bc].chunkIndices[0];
							bond.toChunk = pBCIndices[bc].chunkIndices[1];

							sprintf(bondname, "Bond_%d_%d", bond.fromChunk, bond.toChunk);
							copy(bond.name, bondname);
							copy(bond.asset, asset.path);

							bond.support.healthMask.buf = nullptr;
							bond.support.bondStrength = 1.0;
							bond.support.enableJoint = false;
						}

						merge(blast.bonds, curArray);
						freeBlast(curArray);
					}

					delete[] pSupport;
					pSupport = nullptr;
					delete[] pBCIndices;
					pBCIndices = nullptr;

					curAssetIndex++;
				}
			}
		}

		// projectile
		{
			BPPProjectileArray& projectileArray = blast.projectiles;
			int BPPProjectileSize = projectilesNames.size();
			projectileArray.arraySizes[0] = BPPProjectileSize;
			if (BPPProjectileSize > 0)
			{
				projectileArray.buf = new BPPProjectile[BPPProjectileSize];
				for (int i = 0; i < BPPProjectileSize; i++)
				{
					projectileArray.buf[i].name.buf = nullptr;
					copy(projectileArray.buf[i].name, projectilesNames[i].c_str());
					projectileArray.buf[i].visible = true;
				}
			}
		}

		// graphics meshes
		if (0)
		{
			BPPGraphicsMeshArray& graphicsMeshArray = blast.graphicsMeshes;
			graphicsMeshArray.buf = new BPPGraphicsMesh[3];
			graphicsMeshArray.arraySizes[0] = 3;
			graphicsMeshArray.buf[0].name.buf = nullptr;
			copy(graphicsMeshArray.buf[0].name, "SurfaceMesh1");
			graphicsMeshArray.buf[0].visible = true;

			graphicsMeshArray.buf[1].name.buf = nullptr;
			copy(graphicsMeshArray.buf[1].name, "SurfaceMesh2");
			graphicsMeshArray.buf[1].visible = true;

			graphicsMeshArray.buf[2].name.buf = nullptr;
			copy(graphicsMeshArray.buf[2].name, "DisplayMesh1");
			graphicsMeshArray.buf[2].visible = true;
		}
	}

	BPPAssetArray& assetArray = blast.blastAssets;

	int count = assetArray.arraySizes[0];
	for (int c = 0; c < count; ++c)
	{
		BPPAsset& asset = assetArray.buf[c];
		QFileInfo fileInfo(asset.path.buf);
		BlastAssetNode* assetNode = new BlastAssetNode(fileInfo.baseName().toUtf8().data(), asset);
		_assets.push_back(assetNode);
		_blastProjectDataToNodeMap.insert(std::make_pair((void*)&asset, assetNode));

		// get the firlst level chunks whose parentID is -1
		std::vector<BPPChunk*> childChunks = BlastProject::ins().getChildrenChunks(asset, -1);

		for (size_t i = 0; i < childChunks.size(); ++i)
		{
			BPPChunk& chunk = *(childChunks[i]);
			BlastChunkNode* chunkNode = new BlastChunkNode(chunk.name.buf, chunk, BlastAssetVec[c]);
			assetNode->children.push_back(chunkNode);
			chunkNode->setParent(assetNode);
			_blastProjectDataToNodeMap.insert(std::make_pair((void*)&chunk, chunkNode));
			_addChunkNode(chunk, asset, chunkNode, BlastAssetVec[c]);
		}
	}

	BPPComposite& composite = blast.composite;
	_composite = new BlastCompositeNode(composite.composite.buf, composite);
	_blastProjectDataToNodeMap.insert(std::make_pair((void*)&composite, _composite));
	BPPAssetInstanceArray& assetInstanceArray = composite.blastAssetInstances;
	count = assetInstanceArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPAssetInstance& blastAssetInstance = composite.blastAssetInstances.buf[i];
		BlastAssetInstanceNode* blastAssetInstanceNode = new BlastAssetInstanceNode(blastAssetInstance.name.buf, blastAssetInstance);
		_composite->children.push_back(blastAssetInstanceNode);
		blastAssetInstanceNode->setParent(_composite);
		_blastProjectDataToNodeMap.insert(std::make_pair((void*)&blastAssetInstance, blastAssetInstanceNode));
	}

	BPPLandmarkArray& landmarkArray = composite.landmarks;
	count = landmarkArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPLandmark& landmark = composite.landmarks.buf[i];
		BlastLandmarkNode* landmarkNode = new BlastLandmarkNode(landmark.name.buf, landmark);
		_composite->children.push_back(landmarkNode);
		landmarkNode->setParent(_composite);
		_blastProjectDataToNodeMap.insert(std::make_pair((void*)&landmark, landmarkNode));
	}

	BPPProjectileArray& projectileArray = blast.projectiles;
	count = projectileArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPProjectile& projectile = projectileArray.buf[i];
		BlastProjectileNode* projectileNode = new BlastProjectileNode(projectile.name.buf, projectile);
		_projectiles.push_back(projectileNode);
		_blastProjectDataToNodeMap.insert(std::make_pair((void*)&projectile, projectileNode));
	}

	BPPGraphicsMeshArray& graphicsMeshArray = blast.graphicsMeshes;
	count = graphicsMeshArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPGraphicsMesh& graphicsMesh = graphicsMeshArray.buf[i];
		BlastGraphicsMeshNode* projectileNode = new BlastGraphicsMeshNode(graphicsMesh.name.buf, graphicsMesh);
		_graphicsMeshes.push_back(projectileNode);
	}
}

void BlastTreeData::updateVisible(uint32_t assetIndex, uint32_t chunkIndex, bool visible)
{
	BPPBlast& blast = BlastProject::ins().getParams().blast;
	BPPAssetArray& assetArray = blast.blastAssets;
	if (assetIndex < assetArray.arraySizes[0])
	{
		BPPAsset& asset = assetArray.buf[assetIndex];

		std::vector<BPPChunk*> childChunks = BlastProject::ins().getChildrenChunks(asset);
		if (chunkIndex < childChunks.size())
		{
			BPPChunk& chunk = *(childChunks[chunkIndex]);
			chunk.visible = visible;
		}
	}
}


void BlastTreeData::update(const BlastAsset* asset)
{
	//to do
}

BlastChunkNode* findChunkNode(BlastChunkNode* chunkNode, uint32_t chunkIndex)
{
	if (chunkNode == nullptr)
		return nullptr;

	if (((BPPChunk*)chunkNode->getData())->ID == chunkIndex)
		return chunkNode;

	std::vector<BlastNode*>& children = chunkNode->children;
	for (size_t i = 0; i < children.size(); ++i)
	{
		BlastNode* node = children[i];
		if (node->getType() == eChunk)
		{
			BlastChunkNode* chunkNode = findChunkNode(static_cast<BlastChunkNode*>(node), chunkIndex);
			if (chunkNode)
			{
				return chunkNode;
			}
			else
				continue;
		}
			
		else
			continue;
	}

	return nullptr;
}

BlastAsset* BlastTreeData::getAsset(BlastNode* node)
{
	if (nullptr == node)
		return nullptr;

	BlastNode* parent = node->getParent();
	while (nullptr != parent)
	{
		node = parent;
		parent = node->getParent();
	}

	if (eAsset == node->getType())
	{
		std::map<BlastAsset*, AssetList::ModelAsset>& assetDescMap = SampleManager::ins()->getAssetDescMap();
		std::map<BlastAsset*, AssetList::ModelAsset>::iterator itrAssetDesc = assetDescMap.begin();
		for (; itrAssetDesc != assetDescMap.end(); ++itrAssetDesc)
		{
			if (itrAssetDesc->second.name == node->name)
			{
				return itrAssetDesc->first;
			}
		}
	}

	return nullptr;
}

std::vector<BlastChunkNode*> BlastTreeData::getChunkNodeByBlastChunk(const BlastAsset* asset, const std::vector<uint32_t>& chunkIndexes)
{
	std::vector<BlastChunkNode*> chunkNodes;
	if (asset == nullptr || chunkIndexes.size() == 0)
	{
		return chunkNodes;
	}

	BlastAssetNode* assetNode = _getAssetNode(asset);
	if (assetNode)
	{
		std::vector<BlastNode*>& children = assetNode->children;
		for (BlastNode* node : children)
		{
			if (node->getType() == eChunk)
			{
				for (uint32_t chunkId : chunkIndexes)
				{
					BlastChunkNode* chunkNode = findChunkNode(static_cast<BlastChunkNode*>(node), chunkId);
					if (chunkNode)
					{
						chunkNodes.push_back(chunkNode);
					}
				}
			}
		}
	}

	return chunkNodes;
}

bool isCompleteSupport(BlastChunkNode* node)
{
	if (node == nullptr)
		return false;

	if (node->isSupport())
		return true;

	const std::vector<BlastNode*>& children = node->children;
	for (BlastNode* curNode : children)
	{
		if (eChunk == curNode->getType())
		{
			BlastChunkNode* chunkNode = (BlastChunkNode*)curNode;
			if (0 == chunkNode->children.size())
			{
				if (!chunkNode->isSupport())
					return false;
			}

			if (!isCompleteSupport(chunkNode))
				return false;
		}
	}

	return true;
}

bool BlastTreeData::isCompleteSupportAsset(const BlastAsset* asset)
{
	BlastAssetNode* assetNode = _getAssetNode(asset);
	return isCompleteSupportAsset(assetNode);
}

bool BlastTreeData::isCompleteSupportAsset(const BlastAssetNode* node)
{
	if (node == nullptr)
		return false;

	const std::vector<BlastNode*>& children = node->children;
	for (BlastNode* curNode : children)
	{
		if (eChunk == curNode->getType())
		{
			BlastChunkNode* chunkNode = (BlastChunkNode*)curNode;
			if (!isCompleteSupport(chunkNode))
				return false;
		}
	}

	return true;
}

bool BlastTreeData::isOverlapSupportAsset(const BlastAsset* asset)
{
	BlastAssetNode* assetNode = _getAssetNode(asset);
	return isOverlapSupportAsset(assetNode);
}

bool isOverlapSupport(BlastChunkNode* node)
{
	if (node == nullptr)
		return false;

	bool isParentSupport = node->isSupport();

	const std::vector<BlastNode*>& children = node->children;
	for (BlastNode* curNode : children)
	{
		if (eChunk == curNode->getType())
		{
			BlastChunkNode* chunkNode = (BlastChunkNode*)curNode;
			if (0 == chunkNode->children.size())
			{
				if (isParentSupport && chunkNode->isSupport())
					return true;
			}

			if (isParentSupport && isOverlapSupport(chunkNode))
				return true;
		}
	}

	return false;
}

bool BlastTreeData::isOverlapSupportAsset(const BlastAssetNode* node)
{
	if (node == nullptr)
		return false;

	const std::vector<BlastNode*>& children = node->children;
	for (BlastNode* curNode : children)
	{
		if (eChunk == curNode->getType())
		{
			BlastChunkNode* chunkNode = (BlastChunkNode*)curNode;
			if (isOverlapSupport(chunkNode))
				return true;
		}
	}

	return false;
}

BlastTreeData::BlastTreeData()
{

}

void BlastTreeData::_addChunkNode(const BPPChunk& parentData, BPPAsset& asset, BlastChunkNode* parentNode, void* assetPtr)
{
	if (parentNode != nullptr)
	{
		std::vector<BPPBond*> bonds = BlastProject::ins().getBondsByChunk(asset, parentData.ID);
		for (size_t i = 0; i < bonds.size(); ++i)
		{
			BPPBond* bond = bonds[i];
			BlastBondNode* bondNode = new BlastBondNode(bond->name.buf, *bond);
			parentNode->children.push_back(bondNode);
			bondNode->setParent(parentNode);
			_blastProjectDataToNodeMap.insert(std::make_pair((void*)bond, bondNode));
		}

		std::vector<BPPChunk*> childChunks = BlastProject::ins().getChildrenChunks(asset, parentData.ID);

		for (size_t i = 0; i < childChunks.size(); ++i)
		{
			BPPChunk& chunk = *(childChunks[i]);
			BlastChunkNode* chunkNode = new BlastChunkNode(chunk.name.buf, chunk, assetPtr);
			parentNode->children.push_back(chunkNode);
			chunkNode->setParent(parentNode);
			_blastProjectDataToNodeMap.insert(std::make_pair((void*)&chunk, chunkNode));
			_addChunkNode(chunk, asset, chunkNode, assetPtr);
		}
	}
}

void freeChunkNode(BlastChunkNode* chunkNode)
{
	if (chunkNode == nullptr)
		return;

	std::vector<BlastNode*>& children = chunkNode->children;
	for (size_t i = 0; i < children.size(); ++i)
	{
		BlastNode* node = children[i];
		if (node->getType() == eChunk)
			freeChunkNode(static_cast<BlastChunkNode*>(node));
		else
		{
			delete node;
			node = nullptr;
		}
	}

	delete chunkNode;
	chunkNode = nullptr;
}

void BlastTreeData::_freeBlastNode()
{
	if (_composite)
	{
		size_t count = _composite->children.size();
		for (size_t i = 0; i < count; ++i)
		{
			delete _composite->children[i];
		}
		delete _composite;
		_composite = nullptr;
	}

	size_t count = _assets.size();
	for (size_t i = 0; i < count; ++i)
	{
		std::vector<BlastNode*>& children = _assets[i]->children;
		for (size_t j = 0; j < children.size(); ++j)
		{
			freeChunkNode(static_cast<BlastChunkNode*>(children[j]));
		}
		delete _assets[i];
	}
	_assets.clear();

	count = _projectiles.size();
	for (size_t i = 0; i < count; ++i)
	{
		delete _projectiles[i];
	}
	_projectiles.clear();

	count = _graphicsMeshes.size();
	for (size_t i = 0; i < count; ++i)
	{
		delete _graphicsMeshes[i];
	}
	_graphicsMeshes.clear();

	_blastProjectDataToNodeMap.clear();
}

BlastAssetNode* BlastTreeData::_getAssetNode(const BlastAsset* asset)
{
	std::map<BlastAsset*, AssetList::ModelAsset>& assetDescMap = SampleManager::ins()->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itrAssetDesc = assetDescMap.find(const_cast<BlastAsset*>(asset));

	BlastAssetNode* foundAssetNode = nullptr;
	for (BlastAssetNode* assetNode : _assets)
	{
		if (itrAssetDesc->second.name == assetNode->name)
		{
			foundAssetNode = assetNode;
			break;
		}
	}

	return foundAssetNode;
}

VisualButton::VisualButton(QWidget* parent, BlastNode* blastItem)
	: QWidget(parent)
	, _button(new QPushButton(parent))
	, _blastItem(blastItem)
{
	connect(_button, SIGNAL(toggled(bool)), this, SLOT(on_visualbility_toggled(bool)));
	_button->setCheckable(true);
	_button->setChecked(blastItem->getVisible());
	if (blastItem->getVisible())
		_button->setIcon(QIcon(":/AppMainWindow/images/visibilityToggle_visible.png"));
	else
		_button->setIcon(QIcon(":/AppMainWindow/images/visibilityToggle_notVisible.png"));
	_button->setFixedSize(20, 20);
	this->setLayoutDirection(Qt::RightToLeft);
	this->setLayout(new QHBoxLayout);
	this->layout()->setMargin(0);
	this->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
	this->layout()->addWidget(_button);
}

void VisualButton::on_visualbility_toggled(bool checked)
{
	if (checked)
	{
		_button->setIcon(QIcon(":/AppMainWindow/images/visibilityToggle_visible.png"));
	}
	else
	{
		_button->setIcon(QIcon(":/AppMainWindow/images/visibilityToggle_notVisible.png"));
	}

	if (_blastItem)
	{
		_blastItem->setVisible(checked);
	}
}

void VisualButton::_updateBlast(bool visible)
{
	EBlastNodeType type = _blastItem->getType();

	switch (type)
	{
	case eBond:
		((BPPBond*)_blastItem->getData())->visible = visible;
		break;
	case eChunk:
		((BPPChunk*)_blastItem->getData())->visible = visible;
		break;
	case eAsset:
		((BPPAsset*)_blastItem->getData())->visible = visible;
		break;
	case eProjectile:
		((BPPBond*)_blastItem->getData())->visible = visible;
		break;
	case eGraphicsMesh:
		((BPPGraphicsMesh*)_blastItem->getData())->visible = visible;
		break;
	case eAssetInstance:
		((BPPAssetInstance*)_blastItem->getData())->visible = visible;
		break;
	case eLandmark:
		((BPPLandmark*)_blastItem->getData())->visible = visible;
		break;
	case eComposite:
		((BPPComposite*)_blastItem->getData())->visible = visible;
		break;
	default:
		break;
	}
}

static BlastSceneTree* sBlastSceneTree = nullptr;
BlastSceneTree* BlastSceneTree::ins()
{
	return sBlastSceneTree;
}

BlastSceneTree::BlastSceneTree(QWidget *parent)
	: QDockWidget(parent)
{
	ui.setupUi(this);
	_updateData = true;
	sBlastSceneTree = this;

	ui.blastSceneTree->setStyleSheet("QTreeWidget::item{height:24px}");
	ui.blastSceneTree->setColumnWidth(0, 260);
	ui.blastSceneTree->setColumnWidth(1, 20);

	ui.blastSceneTree->setContextMenuPolicy(Qt::CustomContextMenu);

	_treeChunkContextMenu = new QMenu(this);
	_makeSupportAction = new QAction(tr("Make Support"), this);
	_treeChunkContextMenu->addAction(_makeSupportAction);
	connect(_makeSupportAction, SIGNAL(triggered()), this, SLOT(onMakeSupportMenuItemClicked()));

	_makeStaticSupportAction = new QAction(tr("Make Static Support"), this);
	_treeChunkContextMenu->addAction(_makeStaticSupportAction);
	connect(_makeStaticSupportAction, SIGNAL(triggered()), this, SLOT(onMakeStaticSupportMenuItemClicked()));

	_removeSupportAction = new QAction(tr("Remove Support"), this);
	_treeChunkContextMenu->addAction(_removeSupportAction);
	connect(_removeSupportAction, SIGNAL(triggered()), this, SLOT(onRemoveSupportMenuItemClicked()));

	_treeBondContextMenu = new QMenu(this);
	_bondChunksAction = new QAction(tr("Bond Chunks"), this);
	_treeBondContextMenu->addAction(_bondChunksAction);
	connect(_bondChunksAction, SIGNAL(triggered()), this, SLOT(onBondChunksMenuItemClicked()));

	_bondChunksWithJointsAction = new QAction(tr("Bond Chunks With Joints"), this);
	_treeBondContextMenu->addAction(_bondChunksWithJointsAction);
	connect(_bondChunksWithJointsAction, SIGNAL(triggered()), this, SLOT(onBondChunksWithJointsMenuItemClicked()));

	_removeAllBondsAction = new QAction(tr("Remove All Bonds"), this);
	_treeBondContextMenu->addAction(_removeAllBondsAction);
	connect(_removeAllBondsAction, SIGNAL(triggered()), this, SLOT(onRemoveAllBondsMenuItemClicked()));
}

BlastSceneTree::~BlastSceneTree()
{

}

void BlastSceneTree::updateValues(bool updataData)
{
	if (updataData)
	{
		BlastTreeData::ins().update();
	}

	_updateTreeUIs();
}

void BlastSceneTree::dataSelected(std::vector<BlastNode*> selections)
{
	for (size_t i = 0; i < selections.size(); ++i)
	{
		_selectTreeItem(selections[i]);
	}
}

void BlastSceneTree::addObserver(ISceneObserver* observer)
{
	std::vector<ISceneObserver*>::iterator itr = std::find(_observers.begin(), _observers.end(), observer);
	if (itr == _observers.end())
	{
		_observers.push_back(observer);
	}
}

void BlastSceneTree::removeObserver(ISceneObserver* observer)
{
	std::vector<ISceneObserver*>::iterator itr = std::find(_observers.begin(), _observers.end(), observer);
	_observers.erase(itr);
}

void BlastSceneTree::updateVisible(uint32_t assetIndex, uint32_t chunkIndex, bool visible)
{
	BlastTreeData::ins().updateVisible(assetIndex, chunkIndex, visible);
}

void BlastSceneTree::updateChunkItemSelection()
{
	_updateData = false;

	ui.blastSceneTree->clearSelection();
	BlastTreeData& treeData = BlastTreeData::ins();
	std::vector<BlastChunkNode*> chunkNodes;

	std::map<BlastAsset*, std::vector<uint32_t>> selectedChunks = SampleManager::ins()->getSelectedChunks();
	std::map<BlastAsset*, std::vector<uint32_t>>::iterator itrAssetSelectedChunks = selectedChunks.begin();
	for (; itrAssetSelectedChunks != selectedChunks.end(); ++itrAssetSelectedChunks)
	{
		std::vector<BlastChunkNode*> aseetNodes = treeData.getChunkNodeByBlastChunk(itrAssetSelectedChunks->first, itrAssetSelectedChunks->second);
		chunkNodes.insert(chunkNodes.end(), aseetNodes.begin(), aseetNodes.end());
	}

	for (BlastChunkNode* node : chunkNodes)
	{
		_selectTreeItem(node);
	}

	_updateData = true;
}

void BlastSceneTree::makeSupport()
{
	std::vector<BlastChunkNode*> selectedChunkNodes;
	QList<QTreeWidgetItem*> selectedItems = ui.blastSceneTree->selectedItems();
	for (int i = 0; i < selectedItems.size(); ++i)
	{
		QMap<QTreeWidgetItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(selectedItems.at(i));

		if (eChunk == itr.value()->getType())
		{
			selectedChunkNodes.push_back((BlastChunkNode*)itr.value());
		}
	}

	std::vector<BlastChunkNode*> topChunkNodes = BlastTreeData::getTopChunkNodes(selectedChunkNodes);
	for (size_t i = 0; i < topChunkNodes.size(); ++i)
	{
		BlastChunkNode* chunkNode = topChunkNodes[i];
		BlastTreeData::makeSupport(chunkNode);
	}

	_updateChunkTreeItems();
}

void BlastSceneTree::makeStaticSupport()
{
	std::vector<BlastChunkNode*> selectedChunkNodes;
	QList<QTreeWidgetItem*> selectedItems = ui.blastSceneTree->selectedItems();
	for (int i = 0; i < selectedItems.size(); ++i)
	{
		QMap<QTreeWidgetItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(selectedItems.at(i));

		if (eChunk == itr.value()->getType())
		{
			selectedChunkNodes.push_back((BlastChunkNode*)itr.value());
		}
	}

	std::vector<BlastChunkNode*> topChunkNodes = BlastTreeData::getTopChunkNodes(selectedChunkNodes);
	for (size_t i = 0; i < topChunkNodes.size(); ++i)
	{
		BlastChunkNode* chunkNode = topChunkNodes[i];
		BlastTreeData::makeStaticSupport(chunkNode);
	}

	_updateChunkTreeItems();
}

void BlastSceneTree::removeSupport()
{
	std::vector<BlastChunkNode*> selectedChunkNodes;
	QList<QTreeWidgetItem*> selectedItems = ui.blastSceneTree->selectedItems();
	for (int i = 0; i < selectedItems.size(); ++i)
	{
		QMap<QTreeWidgetItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(selectedItems.at(i));

		if (eChunk == itr.value()->getType())
		{
			selectedChunkNodes.push_back((BlastChunkNode*)itr.value());
		}
	}

	std::vector<BlastChunkNode*> topChunkNodes = BlastTreeData::getTopChunkNodes(selectedChunkNodes);
	for (size_t i = 0; i < topChunkNodes.size(); ++i)
	{
		BlastChunkNode* chunkNode = topChunkNodes[i];
		BlastTreeData::removeSupport(chunkNode);
	}

	_updateChunkTreeItems();
}

void BlastSceneTree::bondChunks()
{

}

void BlastSceneTree::bondChunksWithJoints()
{

}

void BlastSceneTree::removeAllBonds()
{

}

void BlastSceneTree::on_btnAsset_clicked()
{
    QMessageBox::information(NULL, "test", "on_btnAsset_clicked");
}

void BlastSceneTree::on_assetComposite_clicked()
{
    QMessageBox::information(NULL, "test", "on_assetComposite_clicked");
}

void BlastSceneTree::on_btnChunk_clicked()
{
    QMessageBox::information(NULL, "test", "on_btnChunk_clicked");
}

void BlastSceneTree::on_btnBond_clicked()
{
    QMessageBox::information(NULL, "test", "on_btnBond_clicked");
}

void BlastSceneTree::on_btnProjectile_clicked()
{
    QMessageBox::information(NULL, "test", "on_btnProjectile_clicked");
}

void BlastSceneTree::on_blastSceneTree_customContextMenuRequested(const QPoint &pos)
{
	QList<QTreeWidgetItem*> items = ui.blastSceneTree->selectedItems();

	if (items.count() == 1)
	{
		QTreeWidgetItem* curItem = items.at(0);

		QMap<QTreeWidgetItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(curItem);
		if (itr != _treeItemDataMap.end())
		{
			if (eChunk == itr.value()->getType() || eBond == itr.value()->getType())
			{
				_treeChunkContextMenu->exec(QCursor::pos());
			}
		}
	}
	else if (items.count() > 1)
	{
		bool allSupportChunk = true;
		for (int i = 0; i < items.count(); ++i)
		{
			QMap<QTreeWidgetItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(items.at(i));
			if (itr != _treeItemDataMap.end())
			{
				if (eChunk != itr.value()->getType())
				{
					allSupportChunk = false;
					break;
				}
			}
		}

		if (allSupportChunk)
		{
			_treeBondContextMenu->exec(QCursor::pos());
		}
	}

}

void BlastSceneTree::on_blastSceneTree_itemSelectionChanged()
{
	if (!_updateData)
		return;

	SampleManager::ins()->clearChunksSelected();

	QList<QTreeWidgetItem*> selectedItems = ui.blastSceneTree->selectedItems();
	std::vector<BlastNode*> nodes;
	for (int i = 0; i < selectedItems.count(); ++i)
	{
		QMap<QTreeWidgetItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(selectedItems.at(i));
		if (itr != _treeItemDataMap.end())
		{
			nodes.push_back(itr.value());

			BlastNode* node = itr.value();
			if (eChunk == node->getType())
			{
				((BlastChunkNode*)node)->setSelected(true);
			}
			else if (eAssetInstance == node->getType())
			{
				((BlastAssetInstanceNode*)node)->setSelected(true);
			}
		}
	}

	for (size_t i = 0; i < _observers.size(); ++i)
	{
		_observers[i]->dataSelected(nodes);
	}
}

void BlastSceneTree::onMakeSupportMenuItemClicked()
{
	makeSupport();
}

void BlastSceneTree::onMakeStaticSupportMenuItemClicked()
{
	makeStaticSupport();
}

void BlastSceneTree::onRemoveSupportMenuItemClicked()
{
	removeSupport();
}

void BlastSceneTree::onBondChunksMenuItemClicked()
{
	bondChunks();
}

void BlastSceneTree::onBondChunksWithJointsMenuItemClicked()
{
	bondChunksWithJoints();
}

void BlastSceneTree::onRemoveAllBondsMenuItemClicked()
{
	removeAllBonds();
}

void BlastSceneTree::_updateTreeUIs()
{
	ui.blastSceneTree->clear();
	_treeItemDataMap.clear();
	_treeDataItemMap.clear();

	BlastCompositeNode* compositeNode = BlastTreeData::ins().getCompsiteNode();
	if (compositeNode != nullptr)
	{
		QTreeWidgetItem* compositeTreeWidgetItem = new QTreeWidgetItem(ui.blastSceneTree);
		compositeTreeWidgetItem->setText(0, compositeNode->name.c_str());
		compositeTreeWidgetItem->setIcon(0, QIcon(":/AppMainWindow/images/AssetComposite.png"));
		VisualButton* btn = new VisualButton(this, compositeNode);
		ui.blastSceneTree->setItemWidget(compositeTreeWidgetItem, 1, btn);
		_treeItemDataMap.insert(compositeTreeWidgetItem, compositeNode);
		_treeDataItemMap.insert(compositeNode, compositeTreeWidgetItem);

		size_t count = compositeNode->children.size();
		for (size_t i = 0; i < count; ++i)
		{
			BlastNode* assetInstanceNode = compositeNode->children[i];
			QTreeWidgetItem* assetInstanceWidgetItem = new QTreeWidgetItem(compositeTreeWidgetItem);
			assetInstanceWidgetItem->setText(0, assetInstanceNode->name.c_str());
			if (assetInstanceNode->getType() == eAssetInstance)
				assetInstanceWidgetItem->setIcon(0, QIcon(":/AppMainWindow/images/Asset.png"));
			VisualButton* btn = new VisualButton(this, assetInstanceNode);
			ui.blastSceneTree->setItemWidget(assetInstanceWidgetItem, 1, btn);
			_treeItemDataMap.insert(assetInstanceWidgetItem, assetInstanceNode);
			_treeDataItemMap.insert(assetInstanceNode, assetInstanceWidgetItem);
		}
	}

	std::vector<BlastAssetNode*>& assets = BlastTreeData::ins().getAssetNodes();
	size_t count = assets.size();
	for (size_t i = 0; i < count; ++i)
	{
		BlastAssetNode* assetNode = assets[i];

		QTreeWidgetItem* assetTreeWidgetItem = new QTreeWidgetItem(ui.blastSceneTree);
		assetTreeWidgetItem->setText(0, assetNode->name.c_str());
		assetTreeWidgetItem->setIcon(0, QIcon(":/AppMainWindow/images/Asset.png"));
		VisualButton* btn = new VisualButton(this, assetNode);
		ui.blastSceneTree->setItemWidget(assetTreeWidgetItem, 1, btn);
		_treeItemDataMap.insert(assetTreeWidgetItem, assetNode);
		_treeDataItemMap.insert(assetNode, assetTreeWidgetItem);

		_addChunkUI(assetNode, assetTreeWidgetItem);
	}

	std::vector<BlastProjectileNode*>& projectiles = BlastTreeData::ins().getProjectileNodes();
	count = projectiles.size();
	for (int i = 0; i < count; ++i)
	{
		BlastProjectileNode* projectileNode = projectiles[i];

		QTreeWidgetItem* projectileTreeWidgetItem = new QTreeWidgetItem(ui.blastSceneTree);
		projectileTreeWidgetItem->setText(0, projectileNode->name.c_str());
		projectileTreeWidgetItem->setIcon(0, QIcon(":/AppMainWindow/images/Projectile.png"));
		VisualButton* btn = new VisualButton(this, projectileNode);
		ui.blastSceneTree->setItemWidget(projectileTreeWidgetItem, 1, btn);
		_treeItemDataMap.insert(projectileTreeWidgetItem, projectileNode);
		_treeDataItemMap.insert(projectileNode, projectileTreeWidgetItem);
	}

	std::vector<BlastGraphicsMeshNode*>& graphicsMeshes = BlastTreeData::ins().getGraphicsMeshNodes();
	count = graphicsMeshes.size();
	for (int i = 0; i < count; ++i)
	{
		BlastGraphicsMeshNode* graphicsMesheNode = graphicsMeshes[i];

		QTreeWidgetItem* graphicsMesheTreeWidgetItem = new QTreeWidgetItem(ui.blastSceneTree);
		graphicsMesheTreeWidgetItem->setText(0, graphicsMesheNode->name.c_str());
		VisualButton* btn = new VisualButton(this, graphicsMesheNode);
		ui.blastSceneTree->setItemWidget(graphicsMesheTreeWidgetItem, 1, btn);
		_treeItemDataMap.insert(graphicsMesheTreeWidgetItem, graphicsMesheNode);
		_treeDataItemMap.insert(graphicsMesheNode, graphicsMesheTreeWidgetItem);
	}

	//for (int j = 0; j < ui.blastSceneTree->topLevelItemCount(); ++j)
	//{
	//	QTreeWidgetItem* topLevelItem = ui.blastSceneTree->topLevelItem(j);
	//	ui.blastSceneTree->expandItem(topLevelItem);
	//}
	ui.blastSceneTree->expandAll();
}

void BlastSceneTree::_addChunkUI(const BlastNode* parentNode, QTreeWidgetItem* parentTreeItem)
{
	if (parentNode != nullptr && parentTreeItem != nullptr)
	{
		for (size_t i = 0; i < parentNode->children.size(); ++i)
		{
			BlastNode* node = parentNode->children[i];
			if (node == nullptr)
				continue;

			QTreeWidgetItem* treeWidgetItem = nullptr;

			if (node->getType() == eChunk)
			{
				BlastChunkNode* chunk = static_cast<BlastChunkNode*>(node);
				treeWidgetItem = new QTreeWidgetItem(parentTreeItem);
				treeWidgetItem->setText(0, chunk->name.c_str());
				if (chunk->isSupport())
				{
					treeWidgetItem->setIcon(0, QIcon(":/AppMainWindow/images/Chunk_Support_Unstatic.png"));
				}
				else
				{
					treeWidgetItem->setIcon(0, QIcon(":/AppMainWindow/images/Chunk_Unsupport_Unstatic.png"));
				}

				_addChunkUI(chunk, treeWidgetItem);
			}
			else if (node->getType() == eBond)
			{
				BlastBondNode* bond = static_cast<BlastBondNode*>(node);
				treeWidgetItem = new QTreeWidgetItem(parentTreeItem);
				treeWidgetItem->setIcon(0, QIcon(":/AppMainWindow/images/Bond.png"));
				treeWidgetItem->setText(0, bond->name.c_str());
			}

			if (treeWidgetItem == nullptr)
				continue;
			VisualButton* btn = new VisualButton(this, node);
			ui.blastSceneTree->setItemWidget(treeWidgetItem, 1, btn);
			_treeItemDataMap.insert(treeWidgetItem, node);
			_treeDataItemMap.insert(node, treeWidgetItem);
		}
	}
}

void BlastSceneTree::_updateChunkTreeItemAndMenu(BPPChunk* chunk, QTreeWidgetItem* chunkItem)
{
	assert(chunk != nullptr);

	_removeSupportAction->setEnabled(true);
	_makeSupportAction->setEnabled(true);
	_makeStaticSupportAction->setEnabled(true);

	if (!chunk->support && !chunk->staticFlag)
	{
		_removeSupportAction->setEnabled(false);
		chunkItem->setIcon(0, QIcon(":/AppMainWindow/images/Chunk_Unsupport_Unstatic.png"));
	}
	else if (chunk->support && !chunk->staticFlag)
	{
		_makeSupportAction->setEnabled(false);
		chunkItem->setIcon(0, QIcon(":/AppMainWindow/images/Chunk_Support_Unstatic.png"));
	}
	else if (chunk->support && chunk->staticFlag)
	{
		_makeStaticSupportAction->setEnabled(false);
		chunkItem->setIcon(0, QIcon(":/AppMainWindow/images/Chunk_Support_Static.png"));
	}
}

void BlastSceneTree::_updateChunkTreeItems()
{
	for (QMap<BlastNode*, QTreeWidgetItem*>::iterator itr = _treeDataItemMap.begin(); itr != _treeDataItemMap.end(); ++itr)
	{
		BlastNode* node = itr.key();
		QTreeWidgetItem* treeItem = itr.value();
		if (eChunk == node->getType())
		{
			BPPChunk* chunk = static_cast<BPPChunk*>(node->getData());
			if (!chunk->support && !chunk->staticFlag)
			{
				treeItem->setIcon(0, QIcon(":/AppMainWindow/images/Chunk_Unsupport_Unstatic.png"));
			}
			else if (chunk->support && !chunk->staticFlag)
			{
				treeItem->setIcon(0, QIcon(":/AppMainWindow/images/Chunk_Support_Unstatic.png"));
			}
			else if (chunk->support && chunk->staticFlag)
			{
				treeItem->setIcon(0, QIcon(":/AppMainWindow/images/Chunk_Support_Static.png"));
			}
		}
	}
}

void BlastSceneTree::_selectTreeItem(BlastNode* node)
{
	QMap<BlastNode*, QTreeWidgetItem*>::iterator itr = _treeDataItemMap.find(node);
	if (itr != _treeDataItemMap.end())
	{
		ui.blastSceneTree->setItemSelected(itr.value(), true);
	}
}

//void BlastSceneTree::_createTestData()
//{
//	BPPBlast& blast = BlastProject::ins().getParams().blast;
//
//	// compoistie
//	{
//		BPPComposite& composite = blast.composite;
//		composite.composite.buf = nullptr;
//		composite.visible = true;
//
//		copy(composite.composite, "BlastComposite");
//
//		// asset instance array
//		{
//			BPPAssetInstanceArray& instanceArray = composite.blastAssetInstances;
//			instanceArray.buf = new BPPAssetInstance[4];
//			instanceArray.arraySizes[0] = 4;
//			instanceArray.buf[0].name.buf = nullptr;
//			instanceArray.buf[1].name.buf = nullptr;
//			instanceArray.buf[2].name.buf = nullptr;
//			instanceArray.buf[3].name.buf = nullptr;
//			instanceArray.buf[0].source.buf = nullptr;
//			instanceArray.buf[1].source.buf = nullptr;
//			instanceArray.buf[2].source.buf = nullptr;
//			instanceArray.buf[3].source.buf = nullptr;
//
//			copy(instanceArray.buf[0].name, "BlastAsset_instance1");
//			instanceArray.buf[0].visible = true;
//
//			copy(instanceArray.buf[1].name, "BlastAsset_instance2");
//			instanceArray.buf[1].visible = true;
//
//			copy(instanceArray.buf[2].name, "BlastAsset1_instance1");
//			instanceArray.buf[2].visible = true;
//
//			copy(instanceArray.buf[3].name, "BlastAsset1_instance2");
//			instanceArray.buf[3].visible = true;
//		}
//
//		// landmark array
//		{
//			BPPLandmarkArray& landmarkArray = composite.landmarks;
//			landmarkArray.buf = new BPPLandmark[2];
//			landmarkArray.arraySizes[0] = 2;
//			landmarkArray.buf[0].name.buf = nullptr;
//			landmarkArray.buf[1].name.buf = nullptr;
//
//			copy(landmarkArray.buf[0].name, "Landmark_1");
//			copy(landmarkArray.buf[1].name, "Landmark_2");
//		}
//	}
//
//	// asset array
//	{
//		BPPAssetArray& assetArray = blast.blastAssets;
//		assetArray.buf = new BPPAsset[2];
//		assetArray.arraySizes[0] = 2;
//
//		// asset 0
//		{
//			BPPAsset& asset = assetArray.buf[0];
//			asset.path.buf = nullptr;
//			asset.activePreset.buf = nullptr;
//			asset.bonds.buf = nullptr;
//			asset.chunks.buf = nullptr;
//
//			copy(asset.path, "c:/temp/BlastAsset.asset");
//
//			// chunks
//			{
//				asset.chunks.buf = new BPPChunk[10];
//				asset.chunks.arraySizes[0] = 10;
//				for (int i = 0; i < 10; ++i)
//				{
//					asset.chunks.buf[i].name.buf = nullptr;
//					asset.chunks.buf[i].visible = true;
//					asset.chunks.buf[i].staticFlag = false;
//				}
//
//				copy(asset.chunks.buf[0].name, "Chunk_L0_00");
//				asset.chunks.buf[0].ID = 0;
//				asset.chunks.buf[0].parentID = -1;
//				asset.chunks.buf[0].support = false;
//
//				copy(asset.chunks.buf[1].name, "Chunk_L0_01");
//				asset.chunks.buf[1].ID = 1;
//				asset.chunks.buf[1].parentID = -1;
//				asset.chunks.buf[1].support = false;
//
//				copy(asset.chunks.buf[2].name, "Chunk_L1_02");
//				asset.chunks.buf[2].ID = 2;
//				asset.chunks.buf[2].parentID = 0;
//				asset.chunks.buf[2].support = false;
//
//				copy(asset.chunks.buf[3].name, "Chunk_L1_03");
//				asset.chunks.buf[3].ID = 3;
//				asset.chunks.buf[3].parentID = 0;
//				asset.chunks.buf[3].support = false;
//
//				copy(asset.chunks.buf[4].name, "Chunk_L1_04");
//				asset.chunks.buf[4].ID = 4;
//				asset.chunks.buf[4].parentID = 1;
//				asset.chunks.buf[4].support = false;
//
//				copy(asset.chunks.buf[5].name, "Chunk_L1_05");
//				asset.chunks.buf[5].ID = 5;
//				asset.chunks.buf[5].parentID = 1;
//				asset.chunks.buf[5].support = false;
//
//				copy(asset.chunks.buf[6].name, "Chunk_L2_06");
//				asset.chunks.buf[6].ID = 6;
//				asset.chunks.buf[6].parentID = 2;
//				asset.chunks.buf[6].support = true;
//
//				copy(asset.chunks.buf[7].name, "Chunk_L2_07");
//				asset.chunks.buf[7].ID = 7;
//				asset.chunks.buf[7].parentID = 2;
//				asset.chunks.buf[7].support = true;
//
//				copy(asset.chunks.buf[8].name, "Chunk_L2_08");
//				asset.chunks.buf[8].ID = 8;
//				asset.chunks.buf[8].parentID = 3;
//				asset.chunks.buf[8].support = true;
//
//				copy(asset.chunks.buf[9].name, "Chunk_L2_09");
//				asset.chunks.buf[9].ID = 9;
//				asset.chunks.buf[9].parentID = 3;
//				asset.chunks.buf[9].support = true;
//			}
//
//			// bonds
//			{
//				asset.bonds.buf = new BPPBond[4];
//				asset.bonds.arraySizes[0] = 4;
//				for (int i = 0; i < 4; ++i)
//				{
//					asset.bonds.buf[i].name.buf = nullptr;
//					asset.bonds.buf[i].visible = true;
//					asset.bonds.buf[i].support.healthMask.buf = nullptr;
//				}
//
//				copy(asset.bonds.buf[0].name, "Chunk_L2_08");
//				asset.bonds.buf[0].fromChunk = 6;
//				asset.bonds.buf[0].toChunk = 8;
//
//				copy(asset.bonds.buf[1].name, "Chunk_L2_06");
//				asset.bonds.buf[1].fromChunk = 6;
//				asset.bonds.buf[1].toChunk = 8;
//
//				copy(asset.bonds.buf[2].name, "Chunk_L2_09");
//				asset.bonds.buf[2].fromChunk = 7;
//				asset.bonds.buf[2].toChunk = 9;
//
//				copy(asset.bonds.buf[3].name, "Chunk_L2_07");
//				asset.bonds.buf[3].fromChunk = 7;
//				asset.bonds.buf[3].toChunk = 9;
//			}
//		}
//
//		// asset 1
//		{
//			BPPAsset& asset = assetArray.buf[1];
//			asset.path.buf = nullptr;
//			asset.activePreset.buf = nullptr;
//			asset.bonds.buf = nullptr;
//			asset.chunks.buf = nullptr;
//
//			copy(asset.path, "c:/temp/BlastAsset1.asset");
//			{
//				asset.chunks.buf = new BPPChunk[10];
//				asset.chunks.arraySizes[0] = 10;
//				for (int i = 0; i < 10; ++i)
//				{
//					asset.chunks.buf[i].name.buf = nullptr;
//					asset.chunks.buf[i].visible = true;
//				}
//
//				copy(asset.chunks.buf[0].name, "Chunk_L0_00");
//				asset.chunks.buf[0].ID = 0;
//				asset.chunks.buf[0].parentID = -1;
//				asset.chunks.buf[0].support = false;
//
//				copy(asset.chunks.buf[1].name, "Chunk_L0_01");
//				asset.chunks.buf[1].ID = 1;
//				asset.chunks.buf[1].parentID = -1;
//				asset.chunks.buf[1].support = false;
//
//				copy(asset.chunks.buf[2].name, "Chunk_L1_02");
//				asset.chunks.buf[2].ID = 2;
//				asset.chunks.buf[2].parentID = 0;
//				asset.chunks.buf[2].support = false;
//
//				copy(asset.chunks.buf[3].name, "Chunk_L1_03");
//				asset.chunks.buf[3].ID = 3;
//				asset.chunks.buf[3].parentID = 0;
//				asset.chunks.buf[3].support = false;
//
//				copy(asset.chunks.buf[4].name, "Chunk_L1_04");
//				asset.chunks.buf[4].ID = 4;
//				asset.chunks.buf[4].parentID = 1;
//				asset.chunks.buf[4].support = false;
//
//				copy(asset.chunks.buf[5].name, "Chunk_L1_05");
//				asset.chunks.buf[5].ID = 5;
//				asset.chunks.buf[5].parentID = 1;
//				asset.chunks.buf[5].support = false;
//
//				copy(asset.chunks.buf[6].name, "Chunk_L2_06");
//				asset.chunks.buf[6].ID = 6;
//				asset.chunks.buf[6].parentID = 2;
//				asset.chunks.buf[6].support = true;
//
//				copy(asset.chunks.buf[7].name, "Chunk_L2_07");
//				asset.chunks.buf[7].ID = 7;
//				asset.chunks.buf[7].parentID = 2;
//				asset.chunks.buf[7].support = true;
//
//				copy(asset.chunks.buf[8].name, "Chunk_L2_08");
//				asset.chunks.buf[8].ID = 8;
//				asset.chunks.buf[8].parentID = 3;
//				asset.chunks.buf[8].support = true;
//
//				copy(asset.chunks.buf[9].name, "Chunk_L2_09");
//				asset.chunks.buf[9].ID = 9;
//				asset.chunks.buf[9].parentID = 3;
//				asset.chunks.buf[9].support = true;
//			}
//		}
//	}
//
//	// projectile
//	{
//		BPPProjectileArray& projectileArray = blast.projectiles;
//		projectileArray.buf = new BPPProjectile[1];
//		projectileArray.arraySizes[0] = 1;
//		projectileArray.buf[0].name.buf = nullptr;
//		copy(projectileArray.buf[0].name, "Projectile");
//		projectileArray.buf[0].visible = true;
//	}
//
//	// graphics meshes
//	{
//		BPPGraphicsMeshArray& graphicsMeshArray = blast.graphicsMeshes;
//		graphicsMeshArray.buf = new BPPGraphicsMesh[3];
//		graphicsMeshArray.arraySizes[0] = 3;
//		graphicsMeshArray.buf[0].name.buf = nullptr;
//		copy(graphicsMeshArray.buf[0].name, "SurfaceMesh1");
//		graphicsMeshArray.buf[0].visible = true;
//
//		graphicsMeshArray.buf[1].name.buf = nullptr;
//		copy(graphicsMeshArray.buf[1].name, "SurfaceMesh2");
//		graphicsMeshArray.buf[1].visible = true;
//
//		graphicsMeshArray.buf[2].name.buf = nullptr;
//		copy(graphicsMeshArray.buf[2].name, "DisplayMesh1");
//		graphicsMeshArray.buf[2].visible = true;
//	}
//}
