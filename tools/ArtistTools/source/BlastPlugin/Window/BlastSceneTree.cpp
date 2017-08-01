#include "BlastSceneTree.h"
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QShortcut>
#include <QtCore/QFileInfo>
#include <QtGui/qevent.h>
#include <QtGui/QPainter>
#include <assert.h>
#include "ProjectParams.h"
#include <SimpleScene.h>
#include <BlastController.h>
#include "SelectionToolController.h"
#include "GizmoToolController.h"
#include <SceneController.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastTkAsset.h>
#include <NvBlastAsset.h>
#include <BlastFamilyModelSimple.h>
#include "GlobalSettings.h"
#include <deque>
#include "ViewerOutput.h"

static QIcon sCompositeIcon;
static QIcon sAssetIcon;
static QIcon sChunkUUIcon;
static QIcon sChunkSUIcon;
static QIcon sChunkSSIcon;
static QIcon sBondIcon;
static QIcon sProjectileIcon;

static QPixmap sVisibleIcon		= QPixmap(":/AppMainWindow/images/visibilityToggle_visible.png").scaled(QSize(24, 24), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
static QPixmap sInVisibleIcon	= QPixmap(":/AppMainWindow/images/visibilityToggle_notVisible.png").scaled(QSize(24, 24), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

class BlastSceneTreeDataLock
{
public:
	BlastSceneTreeDataLock()
	{
		BlastSceneTree::ins()->_updateData = false;
	}
	~BlastSceneTreeDataLock()
	{
		BlastSceneTree::ins()->_updateData = true;
	}
};

#if 0
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
#endif

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
		BlastFamily* bf = fs[i];
		if(bf)
			bf->setChunkSelected(chunkIndex, selected);
	}
}

void BlastNode::traverse(BlastVisitorBase& visitor)
{
	visitor.visit(this);

	for (BlastNode* node : children)
	{
		if (!visitor.continueTraversing())
			break;

		node->traverse(visitor);
	}
}

void BlastChunkNode::setVisible(bool val)
{
	BPPChunk* pBPPChunk = (BPPChunk*)getData();
	pBPPChunk->visible = val;

	BlastAsset* pBlastAsset = (BlastAsset*)_assetPtr;

	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = sampleManager.getAssetFamiliesMap();
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator it = AssetFamiliesMap.find(pBlastAsset);
	if (it == AssetFamiliesMap.end())
	{
		return;
	}

	std::vector<BlastFamily*>& fs = it->second;
	setChunkVisible(fs, pBPPChunk->ID, val);
}

void BlastChunkNode::setSelected(bool val)
{
	BPPChunk* pBPPChunk = (BPPChunk*)getData();

	BlastAsset* pBlastAsset = (BlastAsset*)_assetPtr;

	SampleManager& sampleManager = SimpleScene::Inst()->GetSampleManager();

	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = sampleManager.getAssetFamiliesMap();
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator it = AssetFamiliesMap.find(pBlastAsset);
	if (it == AssetFamiliesMap.end())
	{
		return;
	}

	std::vector<BlastFamily*>& fs = it->second;
	setChunkSelected(fs, pBPPChunk->ID, val);
}

void BlastAssetNode::setSelected(bool val)
{
	BPPAsset* pBPPAsset = (BPPAsset*)getData();
	std::string strAsset = pBPPAsset->name.buf;

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

	sampleManager.setCurrentSelectedInstance(pBlastAsset, -1);
	
	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = sampleManager.getAssetFamiliesMap();
	std::map<BlastAsset*, std::vector<BlastFamily*>>::iterator itAFM = AssetFamiliesMap.find(pBlastAsset);
	if (itAFM == AssetFamiliesMap.end())
	{
		return;
	}
	std::vector<BlastFamily*>& fs = itAFM->second;
	for (BlastFamily* pBlastFamily : fs)
	{
		pBlastFamily->highlightChunks();
	}
}

bool BlastProjectileNode::getVisible()
{
	return SampleManager::ins()->getSceneController().getProjectileVisible((PhysXSceneActor*)getData());
}

void BlastProjectileNode::setVisible(bool val)
{
	SampleManager::ins()->getSceneController().setProjectileVisible((PhysXSceneActor*)getData(), val);
}

void BlastAssetInstanceNode::setSelected(bool val)
{
	BPPAssetInstance* pBPPAssetInstance = (BPPAssetInstance*)getData();	
	if (pBPPAssetInstance == nullptr)
		return;
	std::string name = pBPPAssetInstance->name.buf;

	std::string strAsset = name.substr(0, name.find_last_of("_"));
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

	std::vector<BlastFamily*> fs = { SampleManager::ins()->getFamilyByInstance(pBPPAssetInstance) };
	setChunkSelected(fs, 0, val);
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
			if (i != j && isChild(nodes[j], nodes[i]))
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

int BlastTreeData::getDepth(BlastNode* node)
{
	int depth = -1; // here it's from -1 because it traverse from Blast asset node
	while (nullptr != node && (eBond == node->getType() || eChunk == node->getType()))
	{
		++depth;
		node = node->getParent();
	}

	return depth;
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
	chunk->staticFlag = false;

	for (BlastNode* curNode : node->children)
	{
		if (eChunk == curNode->getType())
		{
			removeChunkNodeSupport((BlastChunkNode*)curNode);
		}
	}
}

void setAncestorSupportFlag(BlastChunkNode* ancestor, BlastChunkNode* startChild)
{
	if (nullptr == ancestor || nullptr == startChild)
		return;

	{
		BPPChunk* bppChunk = (BPPChunk*)(ancestor->getData());
		bppChunk->support = false;
		bppChunk->staticFlag = false;
	}

	std::deque<BlastChunkNode*> ancestors;
	for (BlastNode* node : ancestor->children)
	{
		if (eChunk == node->getType())
		{
			ancestors.push_back(static_cast<BlastChunkNode*>(node));
		}
	}

	while (ancestors.size() > 0)
	{
		BlastChunkNode* curAncestor = ancestors.front();
		ancestors.pop_front();

		bool isChild = BlastTreeData::isChild(curAncestor, startChild);
		if (isChild)
		{
			if (curAncestor != startChild)
			{
				for (BlastNode* node : curAncestor->children)
				{
					if (eChunk == node->getType())
					{
						ancestors.push_back(static_cast<BlastChunkNode*>(node));
					}
				}
			}
		}
		else
		{
			BPPChunk* bppChunk = (BPPChunk*)(curAncestor->getData());
			bppChunk->support = true;
			bppChunk->staticFlag = false;
		}
	}
}

void BlastTreeData::makeSupport(BlastChunkNode* node)
{
	if (node == nullptr)
		return;

	// 1 set flag for current node
	BPPChunk* chunk = static_cast<BPPChunk*>(node->getData());
	chunk->staticFlag = false;
	chunk->support = true;

	// 2 set flag for ancestors
	BlastChunkNode* supportAncestor = ins().getSupportAncestor(node);
	if (supportAncestor)
	{
		setAncestorSupportFlag(supportAncestor, node);
	}

	// 3 set flag for children
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

	// 1 set flag for current node
	BPPChunk* chunk = static_cast<BPPChunk*>(node->getData());
	chunk->staticFlag = true;
	chunk->support = true;

	// 2 set flag for ancestors
	BlastChunkNode* supportAncestor = ins().getSupportAncestor(node);
	if (supportAncestor)
	{
		setAncestorSupportFlag(supportAncestor, node);
	}

	// 3 set flag for children
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
			curChunk->staticFlag = false;
		}
	}
}

std::string BlastTreeData::getAssetName(BlastAsset* asset)
{
	std::map<BlastAsset*, AssetList::ModelAsset>& assetDescMap = SampleManager::ins()->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itrAssetDesc = assetDescMap.find(asset);

	if (itrAssetDesc != assetDescMap.end())
	{
		return itrAssetDesc->second.name;
	}

	return "";
}

BlastAsset* BlastTreeData::getAsset(std::string assetName)
{
	std::map<BlastAsset*, AssetList::ModelAsset>& assetDescMap = SampleManager::ins()->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itrAssetDesc = assetDescMap.begin();
	for (; itrAssetDesc != assetDescMap.end(); ++itrAssetDesc)
	{
		if (itrAssetDesc->second.name == assetName)
		{
			return itrAssetDesc->first;
		}
	}

	return nullptr;
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

BlastAssetInstanceNode* BlastTreeData::getAssetInstanceNode(BlastFamily* family)
{
	BPPAssetInstance* instance = SampleManager::ins()->getInstanceByFamily(family);
	return getAssetInstanceNode(instance);
}

BlastAssetInstanceNode* BlastTreeData::getAssetInstanceNode(BPPAssetInstance* instance)
{
	if (nullptr == instance)
		return nullptr;

	for (BlastNode* node : _assetInstancesNode->children)
	{
		if ((BPPAssetInstance*)(node->getData()) == instance)
		{
			return (BlastAssetInstanceNode*)node;
		}
	}

	return nullptr;
}

std::vector<BlastAssetInstanceNode*> BlastTreeData::getAssetInstanceNodes(BlastChunkNode* chunkNode)
{
	std::vector<BlastAssetInstanceNode*> instanceNodes;

	if (nullptr == chunkNode)
		return instanceNodes;

	BlastAsset* asset = getAsset(chunkNode);

	if (isRoot(chunkNode))
	{
		BPPAssetInstance* instance = (BPPAssetInstance*)(chunkNode->getData());

		for (BlastNode* instanceNode : _assetInstancesNode->children)
		{
			BPPAssetInstance* instance = (BPPAssetInstance*)(instanceNode->getData());
			BlastFamily* family = SampleManager::ins()->getFamilyByInstance(instance);
			if (&(family->getBlastAsset()) == asset)
				instanceNodes.push_back((BlastAssetInstanceNode*)instanceNode);
		}
	}

	return instanceNodes;
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

	}

	BPPAssetArray& assetArray = blast.blastAssets;

	int count = assetArray.arraySizes[0];
	if (BlastAssetVec.size() != count)
	{
		return;
	}
	for (int c = 0; c < count; ++c)
	{
		BPPAsset& asset = assetArray.buf[c];
		BlastAssetNode* assetNode = new BlastAssetNode(asset.name.buf, asset);
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

	_assetInstancesNode = new BlastAssetInstancesNode("BlastAssetInstances");
	_blastProjectDataToNodeMap.insert(std::make_pair(nullptr, _assetInstancesNode));
	BPPAssetInstanceArray& assetInstanceArray = blast.blastAssetInstances;
	count = assetInstanceArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		BPPAssetInstance& blastAssetInstance = assetInstanceArray.buf[i];
		BlastAssetInstanceNode* blastAssetInstanceNode = new BlastAssetInstanceNode(blastAssetInstance.name.buf, blastAssetInstance);
		_assetInstancesNode->children.push_back(blastAssetInstanceNode);
		blastAssetInstanceNode->setParent(_assetInstancesNode);
		_blastProjectDataToNodeMap.insert(std::make_pair((void*)&blastAssetInstance, blastAssetInstanceNode));
	}

	SceneController& sceneController = SampleManager::ins()->getSceneController();
	std::vector<PhysXSceneActor*> projectiles = sceneController.getPrejectiles();
	for (PhysXSceneActor* projectile : projectiles)
	{
		BlastProjectileNode* projectileNode = new BlastProjectileNode(sceneController.getProjectileName(projectile), projectile);
		_projectiles.push_back(projectileNode);
		_blastProjectDataToNodeMap.insert(std::make_pair((void*)&projectile, projectileNode));
	}

	//BPPGraphicsMeshArray& graphicsMeshArray = blast.graphicsMeshes;
	//count = graphicsMeshArray.arraySizes[0];
	//for (int i = 0; i < count; ++i)
	//{
	//	BPPGraphicsMesh& graphicsMesh = graphicsMeshArray.buf[i];
	//	BlastGraphicsMeshNode* graphicsNode = new BlastGraphicsMeshNode(graphicsMesh.name.buf, graphicsMesh);
	//	_graphicsMeshes.push_back(graphicsNode);
	//}
}

BlastNode* BlastTreeData::getNodeByIndex(uint32_t assetIndex, uint32_t chunkIndex)
{
	BlastNode* pNode = nullptr;
	BPPBlast& blast = BlastProject::ins().getParams().blast;
	BPPAssetArray& assetArray = blast.blastAssets;
	if (assetIndex < assetArray.arraySizes[0])
	{
		BPPAsset& asset = assetArray.buf[assetIndex];

		std::vector<BPPChunk*> childChunks = BlastProject::ins().getChildrenChunks(asset);
		if (chunkIndex < childChunks.size())
		{
			BPPChunk& chunk = *(childChunks[chunkIndex]);	
			pNode = getBlastNodeByProjectData(&chunk);
		}
	}
	return pNode;
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
		return getAsset(node->name);
	}

	return nullptr;
}

BlastAssetNode* BlastTreeData::getAssetNode(BlastAsset* asset)
{
	if (nullptr == asset)
		return nullptr;

	for (BlastAssetNode* node : _assets)
	{
		if (node->name == getAssetName(asset))
			return node;
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

std::vector<BlastNode*> _getNodesByDepth(BlastNode* curNode, uint32_t depth, int32_t curDepth)
{
	std::vector<BlastNode*> res;
	if (depth == curDepth && curNode != nullptr)
	{
		res.push_back(curNode);
	}
	else if (curNode != nullptr)
	{
		for (BlastNode* node : curNode->children)
		{
			std::vector<BlastNode*> nodes = _getNodesByDepth(node, depth, curDepth + 1);
			res.insert(res.begin(), nodes.begin(), nodes.end());
		}
	}

	return res;
}

std::vector<BlastChunkNode*> BlastTreeData::getRootChunkNodeByInstance(const BlastAssetInstanceNode* node)
{
	std::vector<BlastChunkNode*> chunks;
	if (nullptr != node)
	{
		const BPPAssetInstance* pBPPAssetInstance = (BPPAssetInstance*)(const_cast<BlastAssetInstanceNode*>(node)->getData());
		BlastFamily* family = SampleManager::ins()->getFamilyByInstance(const_cast<BPPAssetInstance*>(pBPPAssetInstance));
		if (family)
		{
			const BlastAsset& asset = family->getBlastAsset();
			BlastAssetNode* assetNode = getAssetNode(const_cast<BlastAsset*>(&asset));
			if (assetNode)
			{
				for (BlastNode* curNode : assetNode->children)
				{
					if (eChunk == curNode->getType())
					{
						chunks.push_back((BlastChunkNode*)curNode);
					}
				}
			}
		}
	}
	return chunks;
}

std::vector<BlastNode*> BlastTreeData::getNodesByDepth(BlastAssetNode* node, uint32_t depth)
{
	return _getNodesByDepth(node, depth, -1); // here it's from -1 because it traverse from Blast asset node
}

std::vector<BlastNode*> BlastTreeData::getNodesByDepth(uint32_t depth)
{
	std::vector<BlastNode*> res;
	for (BlastAssetNode* node : _assets)
	{
		std::vector<BlastNode*> nodes = getNodesByDepth(node, depth);
		res.insert(res.begin(), nodes.begin(), nodes.end());
	}
	return res;
}

std::vector<BlastNode*> BlastTreeData::getNodesByDepth(std::vector<uint32_t> depths)
{
	std::vector<BlastNode*> res;
	for (uint32_t depth : depths)
	{
		std::vector<BlastNode*> nodes = getNodesByDepth(depth);
		res.insert(res.begin(), nodes.begin(), nodes.end());
	}
	return res;
}

std::vector<BlastChunkNode*> _getSupportChunkNodes(BlastNode* curNode)
{
	std::vector<BlastChunkNode*> res;
	if (nullptr != curNode && eChunk == curNode->getType() && static_cast<BlastChunkNode*>(curNode)->isSupport())
	{
		res.push_back(static_cast<BlastChunkNode*>(curNode));
	}
	else if (curNode != nullptr)
	{
		for (BlastNode* node : curNode->children)
		{
			std::vector<BlastChunkNode*> nodes = _getSupportChunkNodes(node);
			res.insert(res.begin(), nodes.begin(), nodes.end());
		}
	}

	return res;
}

std::vector<BlastChunkNode*> BlastTreeData::getSupportChunkNodes(BlastAssetNode* node)
{
	return _getSupportChunkNodes(node);
}

std::vector<BlastChunkNode*> BlastTreeData::getSupportChunkNodes()
{
	std::vector<BlastChunkNode*> res;
	for (BlastAssetNode* node : _assets)
	{
		std::vector<BlastChunkNode*> nodes = getSupportChunkNodes(node);
		res.insert(res.begin(), nodes.begin(), nodes.end());
	}
	return res;
}

const std::vector<BlastNode*>& _getAllChunkNodes(std::vector<BlastNode*>& res, BlastNode* curNode)
{
	if (nullptr == curNode)
		return res;
	if (eChunk == curNode->getType())
	{
		res.push_back(static_cast<BlastNode*>(curNode));
	}
	for (BlastNode* node : curNode->children)
	{
		_getAllChunkNodes(res, node);
	}

	return res;
}

std::vector<BlastChunkNode*> BlastTreeData::getSiblingChunkNodes(BlastChunkNode* node)
{
	std::vector<BlastChunkNode*> res;

	if (nullptr == node)
		return res;

	BlastNode* parent = node->getParent();
	if (nullptr == parent || eChunk != parent->getType())
	{
		return res;
	}

	BlastChunkNode* chunkNodeParent = static_cast<BlastChunkNode*>(parent);

	for (BlastNode* child : chunkNodeParent->children)
	{
		if (eChunk == child->getType() && child != node)
			res.push_back(static_cast<BlastChunkNode*>(child));
	}
	return res;
}

BlastChunkNode* BlastTreeData::getSupportAncestor(BlastChunkNode* node)
{
	if (nullptr == node)
		return nullptr;

	BlastNode* parent = node->getParent();
	while (parent && eChunk == parent->getType())
	{
		BlastChunkNode* chunkNodeParent = static_cast<BlastChunkNode*>(parent);
		BPPChunk* bppChunk = (BPPChunk*)(chunkNodeParent->getData());
		if (bppChunk->support)
			return chunkNodeParent;
		parent = chunkNodeParent->getParent();
	}

	return nullptr;
}

const std::vector<BlastNode*>& BlastTreeData::getAllChunkNodes(std::vector<BlastNode*>& res, BlastAssetNode* node)
{
	return _getAllChunkNodes(res, node);
}

const std::vector<BlastNode*>& BlastTreeData::getAllChunkNodes(std::vector<BlastNode*>& res)
{
	for (BlastAssetNode* node : _assets)
	{
		getAllChunkNodes(res, node);
	}
	return res;
}

const std::vector<BlastNode*>& _getAllLeavesChunkNodes(std::vector<BlastNode*>& res, BlastNode* curNode)
{
	if (nullptr == curNode)
		return res;
	if (eChunk == curNode->getType())
	{
		if (BlastTreeData::isLeaf(dynamic_cast<BlastChunkNode*>(curNode)))
			res.push_back(curNode);
	}
	for (BlastNode* node : curNode->children)
	{
		_getAllLeavesChunkNodes(res, node);
	}

	return res;
}

const std::vector<BlastNode*>& BlastTreeData::getAllLeavesChunkNodes(std::vector<BlastNode*>& res, BlastAssetNode* node)
{
	return _getAllLeavesChunkNodes(res, node);
}

const std::vector<BlastNode*>& BlastTreeData::getAllLeavesChunkNodes(std::vector<BlastNode*>& res)
{
	for (BlastAssetNode* node : _assets)
	{
		getAllLeavesChunkNodes(res, node);
	}
	return res;
}

// start from -1 because asset also takes one level.
const std::vector<BlastNode*>& _getChunkNodesFullCoverage(std::vector<BlastNode*>& res, BlastNode* curNode, int depth, int currDepth = -1)
{
	if (nullptr == curNode)
		return res;
	if (eChunk == curNode->getType())
	{
		if((currDepth == depth) || ((currDepth < depth) && BlastTreeData::isLeaf(dynamic_cast<BlastChunkNode*>(curNode))))
			res.push_back(curNode);
	}
	if (currDepth < depth)
	{
		for (BlastNode* node : curNode->children)
		{
			_getChunkNodesFullCoverage(res, node, depth, currDepth + 1);
		}
	}

	return res;
}

const std::vector<BlastNode*>& BlastTreeData::getChunkNodesFullCoverage(std::vector<BlastNode*>& res, BlastAssetNode* node, int depth)
{
	return _getChunkNodesFullCoverage(res, node, depth);
}

const std::vector<BlastNode*>& BlastTreeData::getChunkNodesFullCoverage(std::vector<BlastNode*>& res, int depth)
{
	for (BlastAssetNode* node : _assets)
	{
		getChunkNodesFullCoverage(res, node, depth);
	}
	return res;
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

void BlastTreeData::traverse(BlastVisitorBase& visitor)
{
	for (BlastAssetNode* assetNode : _assets)
	{
		if (!visitor.continueTraversing())
			break;

		assetNode->traverse(visitor);
	}
}

BlastTreeData::BlastTreeData()
{
	_assetInstancesNode = new BlastAssetInstancesNode("BlastAssetInstances");

	_blastProjectDataToNodeMap.clear();
}

void BlastTreeData::_addChunkNode(BPPChunk& parentData, BPPAsset& asset, BlastChunkNode* parentNode, void* assetPtr)
{
	if (parentNode == nullptr)
	{
		return;
	}

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

void BlastTreeData::_removeChunkNode(BPPAsset& asset)
{
	std::vector<BPPChunk*> childChunks = BlastProject::ins().getChildrenChunks(asset);
	int childChunksSize = childChunks.size();
	for (size_t i = 0; i < childChunksSize; i++)
	{
		std::map<void*, BlastNode*>::iterator it = _blastProjectDataToNodeMap.find(childChunks[i]);
		if (it == _blastProjectDataToNodeMap.end())
		{
			continue;
		}

		BlastNode* node = it->second;
		_blastProjectDataToNodeMap.erase(it);
		delete node;
	}

	std::vector<BPPBond*> childBonds = BlastProject::ins().getChildrenBonds(asset);
	int childBondsSize = childBonds.size();
	for (size_t i = 0; i < childBondsSize; i++)
	{
		std::map<void*, BlastNode*>::iterator it = _blastProjectDataToNodeMap.find(childBonds[i]);
		if (it == _blastProjectDataToNodeMap.end())
		{
			continue;
		}

		BlastNode* node = it->second;
		_blastProjectDataToNodeMap.erase(it);
		delete node;
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
	if (_assetInstancesNode)
	{
		size_t count = _assetInstancesNode->children.size();
		for (size_t i = 0; i < count; ++i)
		{
			delete _assetInstancesNode->children[i];
		}
		delete _assetInstancesNode;
		_assetInstancesNode = nullptr;
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

QRect _getVisualIconArea(const QStyleOptionViewItem &option)
{
	int iconLen = option.rect.height() - 2;
	return QRect(option.rect.right() - iconLen, option.rect.top() + 1, iconLen, iconLen);
}

BlastTreeViewDelegate::BlastTreeViewDelegate(QObject* parent, QStandardItemModel* model)
	: QStyledItemDelegate(parent)
	, _treeModel(model)
{
	setObjectName("BlastTreeViewDelegate");
}

void BlastTreeViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyledItemDelegate::paint(painter, option, index);

	QStandardItem *treeItem = _treeModel->itemFromIndex(index);
	BlastSceneTree* tree = BlastSceneTree::ins();
	BlastNode* blastNode = tree->getBlastNodeByItem(treeItem);
	PhysXSceneActor* projectileActor = tree->getProjectileActorByItem(treeItem);

	if ((nullptr == blastNode && nullptr == projectileActor))
		return;

	if(nullptr != blastNode && eChunk != blastNode->getType() && eBond != blastNode->getType())
		return;

	if (nullptr != blastNode)
		painter->drawPixmap(_getVisualIconArea(option), blastNode->getVisible() ? sVisibleIcon : sInVisibleIcon);
	else if (nullptr != projectileActor)
		painter->drawPixmap(_getVisualIconArea(option), SampleManager::ins()->getSceneController().getProjectileVisible(projectileActor) ? sVisibleIcon : sInVisibleIcon);
}

bool BlastTreeViewDelegate::editorEvent(QEvent* evt, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
	if (evt->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent* mouseEvt = (QMouseEvent*)evt;
		if (_getVisualIconArea(option).contains(mouseEvt->pos()))
		{
			QStandardItem *treeItem = _treeModel->itemFromIndex(index);
			BlastSceneTree* tree = BlastSceneTree::ins();
			BlastNode* blastNode = tree->getBlastNodeByItem(treeItem);
			PhysXSceneActor* projectileActor = tree->getProjectileActorByItem(treeItem);

			if ((nullptr == blastNode && nullptr == projectileActor))
				return QStyledItemDelegate::editorEvent(evt, model, option, index);

			if (nullptr != blastNode && eChunk != blastNode->getType() && eBond != blastNode->getType())
				return QStyledItemDelegate::editorEvent(evt, model, option, index);

			if (nullptr != blastNode)
			{
				blastNode->setVisible(!blastNode->getVisible());
			}
			else if (nullptr != projectileActor)
			{
				SceneController& sceneController = SampleManager::ins()->getSceneController();
				sceneController.setProjectileVisible(projectileActor, !sceneController.getProjectileVisible(projectileActor));
			}

			BlastSceneTree::ins()->update();
			return true;
		}
	}
	return QStyledItemDelegate::editorEvent(evt, model, option, index);
}

bool BlastTreeViewDelegate::eventFilter(QObject* object, QEvent* event)
{
	return true;
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

	sCompositeIcon		= QIcon(":/AppMainWindow/images/AssetComposite.png");
	sAssetIcon			= QIcon(":/AppMainWindow/images/Asset.png");
	sChunkUUIcon		= QIcon(":/AppMainWindow/images/Chunk_Unsupport_Unstatic.png");
	sChunkSUIcon		= QIcon(":/AppMainWindow/images/Chunk_Support_Unstatic.png");
	sChunkSSIcon		= QIcon(":/AppMainWindow/images/Chunk_Support_Static.png");
	sBondIcon			= QIcon(":/AppMainWindow/images/Bond.png");
	sProjectileIcon		= QIcon(":/AppMainWindow/images/Projectile.png");

	_treeModel = new QStandardItemModel();
	ui.blastSceneTree->setModel(_treeModel);
	QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
	connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(on_blastSceneTree_itemSelectionChanged(const QItemSelection&, const QItemSelection&)));

	BlastTreeViewDelegate* itemDelegate = new BlastTreeViewDelegate(ui.blastSceneTree, _treeModel);
	ui.blastSceneTree->setItemDelegate((QAbstractItemDelegate*)itemDelegate);

	ui.blastSceneTree->setStyleSheet("QTreeView::item{height:24px}");
	ui.blastSceneTree->setContextMenuPolicy(Qt::CustomContextMenu);

	_treeContextMenu = new QMenu(this);
	_makeSupportAction = new QAction(tr("Make Support"), this);
	_treeContextMenu->addAction(_makeSupportAction);
	connect(_makeSupportAction, SIGNAL(triggered()), this, SLOT(onMakeSupportMenuItemClicked()));

	_makeStaticSupportAction = new QAction(tr("Make Static Support"), this);
	_treeContextMenu->addAction(_makeStaticSupportAction);
	connect(_makeStaticSupportAction, SIGNAL(triggered()), this, SLOT(onMakeStaticSupportMenuItemClicked()));

	_removeSupportAction = new QAction(tr("Remove Support"), this);
	_treeContextMenu->addAction(_removeSupportAction);
	connect(_removeSupportAction, SIGNAL(triggered()), this, SLOT(onRemoveSupportMenuItemClicked()));

	_makeWorldAction = new QAction(tr("Make World"), this);
	_treeContextMenu->addAction(_makeWorldAction);
	connect(_makeWorldAction, SIGNAL(triggered()), this, SLOT(onMakeWorldMenuItemClicked()));

	_removeWorldAction = new QAction(tr("Remove World"), this);
	_treeContextMenu->addAction(_removeWorldAction);
	connect(_removeWorldAction, SIGNAL(triggered()), this, SLOT(onRemoveWorldMenuItemClicked()));

	//_bondChunksAction = new QAction(tr("Bond Chunks"), this);
	//_treeContextMenu->addAction(_bondChunksAction);
	//connect(_bondChunksAction, SIGNAL(triggered()), this, SLOT(onBondChunksMenuItemClicked()));


	//_bondChunksWithJointsAction = new QAction(tr("Bond Chunks With Joints"), this);
	//_treeContextMenu->addAction(_bondChunksWithJointsAction);
	//connect(_bondChunksWithJointsAction, SIGNAL(triggered()), this, SLOT(onBondChunksWithJointsMenuItemClicked()));

	//_removeAllBondsAction = new QAction(tr("Remove All Bonds"), this);
	//_treeContextMenu->addAction(_removeAllBondsAction);
	//connect(_removeAllBondsAction, SIGNAL(triggered()), this, SLOT(onRemoveAllBondsMenuItemClicked()));

	QShortcut* shortCut;
	shortCut = new QShortcut(QKeySequence("Alt+C"), this);
	connect(shortCut, SIGNAL(activated()), this, SLOT(onCollapseExpandClicked()));
	/*
	BlastAssetInstancesNode* assetInstancesNode = BlastTreeData::ins().getBlastAssetInstancesNode();
	_compositeTreeItem = new QStandardItem();
	_treeModel->appendRow(_compositeTreeItem);
	_compositeTreeItem->setText(assetInstancesNode->name.c_str());
	_compositeTreeItem->setIcon(sCompositeIcon);
	_treeItemDataMap.insert(_compositeTreeItem, assetInstancesNode);
	_treeDataItemMap.insert(assetInstancesNode, _compositeTreeItem);
	*/

	m_pNewBlastAsset = nullptr;
	m_NewChunkIndexes.clear();
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

	std::map<BPPAssetInstance*, std::set<uint32_t>> selectChunks;

	SelectionToolController* m_selectionToolController = &SampleManager::ins()->getSelectionToolController();
	GizmoToolController*  m_gizmoToolController = &SampleManager::ins()->getGizmoToolController();
	BlastController* m_blastController = &SampleManager::ins()->getBlastController();

	if (m_selectionToolController->IsEnabled())
	{
		std::set<PxActor*> actors = m_selectionToolController->getTargetActors();
		for (PxActor* actor : actors)
		{
			BlastFamily* pBlastFamily = m_blastController->getFamilyByPxActor(*actor);
			if (pBlastFamily)
			{
				BPPAssetInstance* assetInstance = SampleManager::ins()->getInstanceByFamily(pBlastFamily);
				uint32_t chunkIndex = pBlastFamily->getChunkIndexByPxActor(*actor);
				selectChunks[assetInstance].insert(chunkIndex);
			}
		}
	}
	else if (m_gizmoToolController->IsEnabled())
	{
		PxActor* actor = m_gizmoToolController->getTargetActor();

		if (actor)
		{
			BlastFamily* pBlastFamily = m_blastController->getFamilyByPxActor(*actor);
			if (pBlastFamily)
			{
				BPPAssetInstance* assetInstance = SampleManager::ins()->getInstanceByFamily(pBlastFamily);
				uint32_t chunkIndex = pBlastFamily->getChunkIndexByPxActor(*actor);
				selectChunks[assetInstance].insert(chunkIndex);
			}
		}
	}

	_updateTreeUIs();

	BlastSceneTreeDataLock lock;
	std::set<PxActor*> actors;
	for (std::map<BPPAssetInstance*, std::set<uint32_t>>::iterator itr = selectChunks.begin(); itr != selectChunks.end(); ++itr)
	{
		BlastFamily* family = SampleManager::ins()->getFamilyByInstance(itr->first);
		std::set<uint32_t>& chunkIndexes = itr->second;

		if (nullptr != family)
		{
			for (uint32_t chunkIndex : chunkIndexes)
			{
				PxActor* actor = nullptr;
				family->getPxActorByChunkIndex(chunkIndex, &actor);

				if (actor)
					actors.insert(actor);
			}
		}
	}

	if (m_selectionToolController->IsEnabled())
	{
		m_selectionToolController->setTargetActors(actors);
	}
	else if (m_gizmoToolController->IsEnabled())
	{
		if (actors.size() > 0)
			m_gizmoToolController->setTargetActor(*actors.begin());
	}
}

void BlastSceneTree::clear()
{
	_treeModel->clear();
	_treeItemDataMap.clear();
	_treeDataItemMap.clear();

	// notify no selection
	std::vector<BlastNode*> nodes;
	for (size_t i = 0; i < _observers.size(); ++i)
	{
		_observers[i]->dataSelected(nodes);
	}
}

void BlastSceneTree::dataSelected(std::vector<BlastNode*> selections)
{
	for (size_t i = 0; i < selections.size(); ++i)
	{
		selectTreeItem(selections[i]);
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
	BlastNode* node = BlastTreeData::ins().getNodeByIndex(assetIndex, chunkIndex);
	if (node != nullptr && eChunk == node->getType())
	{
		static_cast<BlastChunkNode*>(node)->setVisible(visible);
	}
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
		selectTreeItem(node, false);
	}

	_updateData = true;
}

void BlastSceneTree::makeSupport()
{
	std::vector<BlastChunkNode*> selectedChunkNodes;
	QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
	for (int i = 0; i < selectedIndexes.count(); ++i)
	{
		QMap<QStandardItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(_treeModel->itemFromIndex(selectedIndexes.at(i)));

		if (eChunk == itr.value()->getType())
		{
			selectedChunkNodes.push_back((BlastChunkNode*)itr.value());
		}
	}

	std::vector<BlastChunkNode*> topChunkNodes = BlastTreeData::getTopChunkNodes(selectedChunkNodes);
	std::set<BlastAsset*> assets;
	for (BlastChunkNode* chunkNode : topChunkNodes)
	{
		if (chunkNode->isSupport() && !((BPPChunk*)chunkNode->getData())->staticFlag)
			continue;
		BlastTreeData::makeSupport(chunkNode);
		BlastAsset* pBlastAsset = BlastTreeData::ins().getAsset(chunkNode);
		assets.insert(pBlastAsset);
	}

	if (0 == assets.size())
		return;

	SampleManager* pSampleManager = SampleManager::ins();
	for (BlastAsset* asset : assets)
	{
		pSampleManager->refreshAsset(asset);
	}

	return;
}

void BlastSceneTree::makeStaticSupport()
{
	std::vector<BlastChunkNode*> selectedChunkNodes;
	QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
	for (int i = 0; i < selectedIndexes.count(); ++i)
	{
		QMap<QStandardItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(_treeModel->itemFromIndex(selectedIndexes.at(i)));

		if (eChunk == itr.value()->getType())
		{
			selectedChunkNodes.push_back((BlastChunkNode*)itr.value());
		}
	}

	std::vector<BlastChunkNode*> topChunkNodes = BlastTreeData::getTopChunkNodes(selectedChunkNodes);
	std::set<BlastAsset*> assets;
	for (BlastChunkNode* chunkNode : topChunkNodes)
	{
		if (chunkNode->isSupport() && ((BPPChunk*)chunkNode->getData())->staticFlag)
			continue;
		BlastTreeData::makeStaticSupport(chunkNode);
		BlastAsset* pBlastAsset = BlastTreeData::ins().getAsset(chunkNode);
		assets.insert(pBlastAsset);
	}

	if (0 == assets.size())
		return;

	SampleManager* pSampleManager = SampleManager::ins();
	for (BlastAsset* asset : assets)
	{
		pSampleManager->refreshAsset(asset);
	}

	return;
}

void BlastSceneTree::removeSupport()
{
	std::vector<BlastChunkNode*> selectedChunkNodes;
	QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
	for (int i = 0; i < selectedIndexes.count(); ++i)
	{
		QMap<QStandardItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(_treeModel->itemFromIndex(selectedIndexes.at(i)));

		if (eChunk == itr.value()->getType())
		{
			selectedChunkNodes.push_back((BlastChunkNode*)itr.value());
		}
	}

	std::vector<BlastChunkNode*> topChunkNodes = BlastTreeData::getTopChunkNodes(selectedChunkNodes);
	std::set<BlastAsset*> assets;
	for (BlastChunkNode* chunkNode : topChunkNodes)
	{
		if (!chunkNode->isSupport() || BlastTreeData::isLeaf(chunkNode))
			continue;
		BlastTreeData::removeSupport(chunkNode);
		BlastAsset* pBlastAsset = BlastTreeData::ins().getAsset(chunkNode);
		assets.insert(pBlastAsset);
	}

	if (0 == assets.size())
		return;

	SampleManager* pSampleManager = SampleManager::ins();
	for (BlastAsset* asset : assets)
	{
		pSampleManager->refreshAsset(asset);
	}

	return;
}

void BlastSceneTree::makeWorld()
{
	std::vector<BlastBondNode*> selectedBondNodes;
	QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
	for (int i = 0; i < selectedIndexes.count(); ++i)
	{
		QMap<QStandardItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(_treeModel->itemFromIndex(selectedIndexes.at(i)));

		if (eBond == itr.value()->getType())
		{
			selectedBondNodes.push_back((BlastBondNode*)itr.value());
		}
	}
	if (selectedBondNodes.size() == 0)
	{
		return;
	}

	std::vector<BlastBondNode*>::iterator itSBN;
	std::map<BlastAsset*, BlastAsset*> UniqueAssets;
	for (itSBN = selectedBondNodes.begin(); itSBN != selectedBondNodes.end(); itSBN++)
	{
		BlastBondNode* node = *itSBN;

		BPPBond* bond = static_cast<BPPBond*>(node->getData());
		bond->toChunk = 0xFFFFFFFF;

		BlastAsset* pBlastAsset = BlastTreeData::ins().getAsset(node);
		UniqueAssets[pBlastAsset] = pBlastAsset;
	}

	
	SampleManager* pSampleManager = SampleManager::ins();
	std::map<BlastAsset*, BlastAsset*>::iterator itUA;
	for (itUA = UniqueAssets.begin(); itUA != UniqueAssets.end(); itUA++)
	{
		BlastAsset* pBlastAsset = itUA->second;
		pSampleManager->refreshAsset(pBlastAsset);
	}
}

void BlastSceneTree::removeWorld()
{
	std::vector<BlastBondNode*> selectedBondNodes;
	QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
	for (int i = 0; i < selectedIndexes.count(); ++i)
	{
		QMap<QStandardItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(_treeModel->itemFromIndex(selectedIndexes.at(i)));

		if (eBond == itr.value()->getType())
		{
			selectedBondNodes.push_back((BlastBondNode*)itr.value());
		}
	}
	if (selectedBondNodes.size() == 0)
	{
		return;
	}

	std::vector<BlastBondNode*>::iterator itSBN;
	std::map<BlastAsset*, BlastAsset*> UniqueAssets;
	for (itSBN = selectedBondNodes.begin(); itSBN != selectedBondNodes.end(); itSBN++)
	{
		BlastBondNode* node = *itSBN;

		BPPBond* bond = static_cast<BPPBond*>(node->getData());
		bond->toChunk = 0xFFFFFFFF - 1;

		BlastAsset* pBlastAsset = BlastTreeData::ins().getAsset(node);
		UniqueAssets[pBlastAsset] = pBlastAsset;
	}

	
	SampleManager* pSampleManager = SampleManager::ins();
	std::map<BlastAsset*, BlastAsset*>::iterator itUA;
	for (itUA = UniqueAssets.begin(); itUA != UniqueAssets.end(); itUA++)
	{
		BlastAsset* pBlastAsset = itUA->second;
		pSampleManager->refreshAsset(pBlastAsset);
	}
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

void BlastSceneTree::setChunkSelected(std::vector<uint32_t> depths, bool selected)
{
	std::vector<BlastNode*> nodes = BlastTreeData::ins().getNodesByDepth(depths);
	for (BlastNode* node : nodes)
	{
		if (eChunk == node->getType())
		{
			static_cast<BlastChunkNode*>(node)->setSelected(selected);
			selectTreeItem(node);
		}
	}
}

void BlastSceneTree::setChunkVisible(std::vector<uint32_t> depths, bool bVisible)
{
	std::vector<BlastNode*> nodes = BlastTreeData::ins().getNodesByDepth(depths);
	for (BlastNode* node : nodes)
	{
		if (eChunk == node->getType())
		{
			static_cast<BlastChunkNode*>(node)->setVisible(bVisible);
		}
	}
}

void BlastSceneTree::hideAllChunks()
{
	std::vector<BlastNode*> nodes;
	BlastTreeData::ins().getAllChunkNodes(nodes);
	for (BlastNode* node : nodes)
	{
		if (eChunk == node->getType())
		{
			static_cast<BlastChunkNode*>(node)->setVisible(false);
		}
	}
}

void BlastSceneTree::setChunkVisibleFullCoverage(int depth)
{
	std::vector<BlastNode*> nodes;
	BlastTreeData::ins().getChunkNodesFullCoverage(nodes, depth);
	for (BlastNode* node : nodes)
	{
		if (eChunk == node->getType())
		{
			static_cast<BlastChunkNode*>(node)->setVisible(true);
		}
	}
}

void BlastSceneTree::on_btnAsset_clicked()
{
    QMessageBox::information(NULL, "Blast Tool", "This feature isn't implemented currently!");
}

void BlastSceneTree::on_assetComposite_clicked()
{
    QMessageBox::information(NULL, "Blast Tool", "This feature isn't implemented currently!");
}

void BlastSceneTree::on_btnChunk_clicked()
{
    QMessageBox::information(NULL, "Blast Tool", "This feature isn't implemented currently!");
}

void BlastSceneTree::on_btnBond_clicked()
{
    QMessageBox::information(NULL, "Blast Tool", "This feature isn't implemented currently!");
}

void BlastSceneTree::on_btnProjectile_clicked()
{
    QMessageBox::information(NULL, "Blast Tool", "This feature isn't implemented currently!");
}

void BlastSceneTree::on_btnExpandCollapse_clicked()
{
	onCollapseExpandClicked();
}

void BlastSceneTree::on_blastSceneTree_customContextMenuRequested(const QPoint &pos)
{
	QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

	std::vector<BlastChunkNode*> chunkNodes;
	std::vector<BlastBondNode*> bondNodes;
	for (int i = 0; i < selectedIndexes.count(); ++i)
	{
		QMap<QStandardItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(_treeModel->itemFromIndex(selectedIndexes.at(i)));
		if (itr != _treeItemDataMap.end())
		{
			if (eChunk == itr.value()->getType())
			{
				chunkNodes.push_back((BlastChunkNode*)itr.value());
			}
			else if (eBond == itr.value()->getType())
			{
				bondNodes.push_back((BlastBondNode*)itr.value());
			}
		}
	}

	{
		std::vector<BlastChunkNode*> topChunkNodes = BlastTreeData::getTopChunkNodes(chunkNodes);
		_makeSupportAction->setEnabled(true);
		_makeStaticSupportAction->setEnabled(true);
		_removeSupportAction->setEnabled(true);
		_makeWorldAction->setEnabled(true);
		_removeWorldAction->setEnabled(true);

		//select chunk nodes have parent child relation ship, disable all menu items
		if (topChunkNodes.size() < chunkNodes.size())
		{
			_makeSupportAction->setEnabled(false);
			_makeStaticSupportAction->setEnabled(false);
			_removeSupportAction->setEnabled(false);
		}
		else
		{
			bool allSupported = true, allStaticSupport = true, allUnSupported = true, hasLeaf = false;

			for (BlastChunkNode* chunkNode : chunkNodes)
			{
				BPPChunk* chunk = (BPPChunk*)(chunkNode->getData());
				if (chunk->support)
				{
					allUnSupported = false;
				}
				else
				{
					allSupported = false;
				}

				if (!chunk->staticFlag)
				{
					allStaticSupport = false;
				}

				if (BlastTreeData::isLeaf(chunkNode))
				{
					hasLeaf = true;
				}
			}

			if (allSupported && !allStaticSupport)
			{
				_makeSupportAction->setEnabled(false);
			}

			if (allStaticSupport)
			{
				_makeStaticSupportAction->setEnabled(false);
			}

			if (allUnSupported || hasLeaf)
			{
				_removeSupportAction->setEnabled(false);
			}
		}
	}

	if (chunkNodes.size() > 0 && bondNodes.size() > 0)
	{
		_makeSupportAction->setEnabled(false);
		_makeStaticSupportAction->setEnabled(false);
		_removeSupportAction->setEnabled(false);
		_makeWorldAction->setEnabled(false);
		_removeWorldAction->setEnabled(false);
	}
	else if (chunkNodes.size() > 0 && bondNodes.size() == 0)
	{
		_makeWorldAction->setEnabled(false);
		_removeWorldAction->setEnabled(false);
	}
	else if (chunkNodes.size() == 0 && bondNodes.size() > 0)
	{
		_makeSupportAction->setEnabled(false);
		_makeStaticSupportAction->setEnabled(false);
		_removeSupportAction->setEnabled(false);
	}

	if (0 < chunkNodes.size() || 0 < bondNodes.size())
	{
		_treeContextMenu->exec(QCursor::pos());
	}

}

void BlastSceneTree::on_blastSceneTree_itemSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	if (!_updateData)
		return;

	SampleManager::ins()->clearChunksSelected();

	QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

	std::vector<BlastNode*> nodes;
	for (int i = 0; i < selectedIndexes.count(); ++i)
	{
		QMap<QStandardItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(_treeModel->itemFromIndex(selectedIndexes.at(i)));
		if (itr != _treeItemDataMap.end())
		{
			nodes.push_back(itr.value());

			BlastNode* node = itr.value();
			if (eChunk == node->getType())
			{
				((BlastChunkNode*)node)->setSelected(true);

				if (BlastTreeData::isRoot((BlastChunkNode*)node))
				{
					std::vector<BlastAssetInstanceNode*> instanceNodes = BlastTreeData::ins().getAssetInstanceNodes((BlastChunkNode*)node);
					for (BlastAssetInstanceNode* instanceNode : instanceNodes)
					{
						selectTreeItem(instanceNode, false);
						nodes.push_back(instanceNode);
					}
				}
			}
			else if (eAssetInstance == node->getType())
			{
				((BlastAssetInstanceNode*)node)->setSelected(true);

				BlastSceneTreeDataLock lock;
				std::vector<BlastChunkNode*> chunkNodes = BlastTreeData::ins().getRootChunkNodeByInstance((BlastAssetInstanceNode*)node);
				for (BlastChunkNode* chunkNode : chunkNodes)
				{
					selectTreeItem(chunkNode, false);
					nodes.push_back(chunkNode);
				}
			}
			else if (eAsset == node->getType())
			{
				((BlastAssetNode*)node)->setSelected(true);
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

void BlastSceneTree::onMakeWorldMenuItemClicked()
{
	makeWorld();
}

void BlastSceneTree::onRemoveWorldMenuItemClicked()
{
	removeWorld();
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

void BlastSceneTree::onCollapseExpandClicked()
{
	static bool expand = true;
	if (expand)
	{
		ui.blastSceneTree->collapseAll();
		expand = false;
	}
	else
	{
		ui.blastSceneTree->expandAll();
		expand = true;
	}
}

void BlastSceneTree::_updateTreeUIs()
{
#ifdef _DEBUG
	static int gcounter = 0;
	static char gbuf[128];
	sprintf(gbuf, "_updateTreeUIs called %d", ++gcounter);
	viewer_msg(gbuf);
#endif
	//ui.blastSceneTree->setUpdatesEnabled(false);
	_treeModel->clear();
	_treeItemDataMap.clear();
	_treeDataItemMap.clear();

	BlastAssetInstancesNode* assetInstancesNode = BlastTreeData::ins().getBlastAssetInstancesNode();
	if (assetInstancesNode != nullptr)
	{
		QStandardItem* compositeTreeItem = new QStandardItem();
		compositeTreeItem->setText(assetInstancesNode->name.c_str());
		compositeTreeItem->setIcon(sCompositeIcon);
		_treeItemDataMap.insert(compositeTreeItem, assetInstancesNode);
		_treeDataItemMap.insert(assetInstancesNode, compositeTreeItem);
		_treeModel->appendRow(compositeTreeItem);
		size_t count = assetInstancesNode->children.size();
		for (size_t i = 0; i < count; ++i)
		{
			BlastNode* assetInstanceNode = assetInstancesNode->children[i];
			QStandardItem* assetInstanceItem = new QStandardItem();
			compositeTreeItem->appendRow(assetInstanceItem);
			assetInstanceItem->setText(assetInstanceNode->name.c_str());
			if (assetInstanceNode->getType() == eAssetInstance)
				assetInstanceItem->setIcon(QIcon(":/AppMainWindow/images/Asset.png"));
			_treeItemDataMap.insert(assetInstanceItem, assetInstanceNode);
			_treeDataItemMap.insert(assetInstanceNode, assetInstanceItem);
		}
	}

	std::vector<BlastAssetNode*>& assets = BlastTreeData::ins().getAssetNodes();
	size_t count = assets.size();
	for (size_t i = 0; i < count; ++i)
	{
		BlastAssetNode* assetNode = assets[i];

		QStandardItem* assetTreeWidgetItem = new QStandardItem();
		_treeModel->appendRow(assetTreeWidgetItem);
		assetTreeWidgetItem->setText(assetNode->name.c_str());
		assetTreeWidgetItem->setIcon(sAssetIcon);
		_treeItemDataMap.insert(assetTreeWidgetItem, assetNode);
		_treeDataItemMap.insert(assetNode, assetTreeWidgetItem);

		_addChunkUI(assetNode, assetTreeWidgetItem);
	}

	std::vector<BlastProjectileNode*>& projectiles = BlastTreeData::ins().getProjectileNodes();
	count = projectiles.size();
	for (int i = 0; i < count; ++i)
	{
		BlastProjectileNode* projectileNode = projectiles[i];

		QStandardItem* projectileTreeItem = new QStandardItem();
		_treeModel->appendRow(projectileTreeItem);
		projectileTreeItem->setText(projectileNode->name.c_str());
		projectileTreeItem->setIcon(sProjectileIcon);
		_treeItemDataMap.insert(projectileTreeItem, projectileNode);
		_treeDataItemMap.insert(projectileNode, projectileTreeItem);
	}

	std::vector<BlastGraphicsMeshNode*>& graphicsMeshes = BlastTreeData::ins().getGraphicsMeshNodes();
	count = graphicsMeshes.size();
	for (int i = 0; i < count; ++i)
	{
		BlastGraphicsMeshNode* graphicsMesheNode = graphicsMeshes[i];

		QStandardItem* graphicsMesheTreeWidgetItem = new QStandardItem();
		_treeModel->appendRow(graphicsMesheTreeWidgetItem);
		graphicsMesheTreeWidgetItem->setText(graphicsMesheNode->name.c_str());
		_treeItemDataMap.insert(graphicsMesheTreeWidgetItem, graphicsMesheNode);
		_treeDataItemMap.insert(graphicsMesheNode, graphicsMesheTreeWidgetItem);
	}

	ui.blastSceneTree->expandAll();

	// notify no selection
	//std::vector<BlastNode*> nodes;
	//for (size_t i = 0; i < _observers.size(); ++i)
	//{
	//	_observers[i]->dataSelected(nodes);
	//}

	bool autoSelectNewChunks = BlastProject::ins().getParams().fracture.general.autoSelectNewChunks;
	if (!autoSelectNewChunks)
	{
		m_pNewBlastAsset = nullptr;
		m_NewChunkIndexes.clear();
		return;
	}

	if (m_pNewBlastAsset == nullptr || m_NewChunkIndexes.size() == 0)
	{
		return;
	}

	_updateData = false;
	ui.blastSceneTree->clearSelection();
	std::vector<BlastChunkNode*> chunkNodes =
		BlastTreeData::ins().getChunkNodeByBlastChunk(m_pNewBlastAsset, m_NewChunkIndexes);
	for (BlastChunkNode* node : chunkNodes)
	{
		node->setVisible(true);
		selectTreeItem(node);
	}

	m_pNewBlastAsset = nullptr;
	m_NewChunkIndexes.clear();

	_updateData = true;
}

void BlastSceneTree::_addChunkUI(const BlastNode* parentNode, QStandardItem* parentTreeItem)
{
	if (parentNode != nullptr && parentTreeItem != nullptr)
	{
		for (size_t i = 0; i < parentNode->children.size(); ++i)
		{
			BlastNode* node = parentNode->children[i];
			if (node == nullptr)
				continue;

			QStandardItem* treeWidgetItem = new QStandardItem();
			parentTreeItem->appendRow(treeWidgetItem);

			if (node->getType() == eChunk)
			{
				BlastChunkNode* chunkNode = static_cast<BlastChunkNode*>(node);
				treeWidgetItem->setText(chunkNode->name.c_str());

				BPPChunk* chunk = static_cast<BPPChunk*>(chunkNode->getData());
				if (!chunk->support)
				{
					treeWidgetItem->setIcon(sChunkUUIcon);
				}
				else if (chunk->support && !chunk->staticFlag)
				{
					treeWidgetItem->setIcon(sChunkSUIcon);
				}
				else if (chunk->support && chunk->staticFlag)
				{
					treeWidgetItem->setIcon(sChunkSSIcon);
				}

				_addChunkUI(chunkNode, treeWidgetItem);
			}
			else if (node->getType() == eBond)
			{
				BlastBondNode* bond = static_cast<BlastBondNode*>(node);
				treeWidgetItem->setIcon(sBondIcon);
				treeWidgetItem->setText(bond->name.c_str());
			}

			if (treeWidgetItem == nullptr)
				continue;

			_treeItemDataMap.insert(treeWidgetItem, node);
			_treeDataItemMap.insert(node, treeWidgetItem);
		}
	}
}

void BlastSceneTree::_updateChunkTreeItemAndMenu(BPPChunk* chunk, QStandardItem* chunkItem)
{
	assert(chunk != nullptr);

	_removeSupportAction->setEnabled(true);
	_makeSupportAction->setEnabled(true);
	_makeStaticSupportAction->setEnabled(true);

	if (!chunk->support && !chunk->staticFlag)
	{
		_removeSupportAction->setEnabled(false);
		chunkItem->setIcon(sChunkUUIcon);
	}
	else if (chunk->support && !chunk->staticFlag)
	{
		_makeSupportAction->setEnabled(false);
		chunkItem->setIcon(sChunkSUIcon);
	}
	else if (chunk->support && chunk->staticFlag)
	{
		_makeStaticSupportAction->setEnabled(false);
		chunkItem->setIcon(sChunkSSIcon);
	}
}

void BlastSceneTree::_updateChunkTreeItems()
{
	for (QMap<BlastNode*, QStandardItem*>::iterator itr = _treeDataItemMap.begin(); itr != _treeDataItemMap.end(); ++itr)
	{
		BlastNode* node = itr.key();
		QStandardItem* treeItem = itr.value();
		if (eChunk == node->getType())
		{
			BPPChunk* chunk = static_cast<BPPChunk*>(node->getData());
			if (!chunk->support)
			{
				treeItem->setIcon(sChunkUUIcon);
			}
			else if (chunk->support && !chunk->staticFlag)
			{
				treeItem->setIcon(sChunkSUIcon);
			}
			else if (chunk->support && chunk->staticFlag)
			{
				treeItem->setIcon(sChunkSSIcon);
			}
		}
	}
}

void BlastSceneTree::ApplyAutoSelectNewChunks(BlastAsset* pNewBlastAsset, std::vector<uint32_t>& NewChunkIndexes)
{
	if (pNewBlastAsset == nullptr || NewChunkIndexes.size() == 0)
	{
		return;
	}

	m_pNewBlastAsset = pNewBlastAsset;
	m_NewChunkIndexes.clear();
	for (uint32_t nci : NewChunkIndexes)
	{
		m_NewChunkIndexes.push_back(nci);
	}
}

/*
BlastAssetNode* BlastTreeData::addBlastAsset(BPPAsset& asset)
{
	BlastAsset* pBlastAsset = nullptr;
	SampleManager* pSampleManager = SampleManager::ins();
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator it;
	for (it = AssetDescMap.begin(); it != AssetDescMap.end(); it++)
	{
		std::string assetname = asset.name.buf;
		if (it->second.name == assetname)
		{
			pBlastAsset = it->first;
		}
	}

	BlastAssetNode* assetNode = new BlastAssetNode(asset.name.buf, asset);
	_assets.push_back(assetNode);
	_blastProjectDataToNodeMap.insert(std::make_pair((void*)&asset, assetNode));

	std::vector<BPPChunk*> childChunks = BlastProject::ins().getChildrenChunks(asset, -1);
	int childChunksSize = childChunks.size();
	for (size_t i = 0; i < childChunksSize; ++i)
	{
		BPPChunk& chunk = *(childChunks[i]);
		BlastChunkNode* chunkNode = new BlastChunkNode(chunk.name.buf, chunk, pBlastAsset);
		assetNode->children.push_back(chunkNode);
		chunkNode->setParent(assetNode);
		_blastProjectDataToNodeMap.insert(std::make_pair((void*)&chunk, chunkNode));

		_addChunkNode(chunk, asset, chunkNode, pBlastAsset);
	}

	return assetNode;
}

void BlastTreeData::removeBlastAsset(BPPAsset& asset)
{
	_removeChunkNode(asset);

	std::map<void*, BlastNode*>::iterator it = _blastProjectDataToNodeMap.find(&asset);
	if (it == _blastProjectDataToNodeMap.end())
	{
		return;
	}

	BlastNode* node = it->second;
	_blastProjectDataToNodeMap.erase(it);
	std::vector<BlastAssetNode*>::iterator itAsset;
	for (itAsset = _assets.begin(); itAsset != _assets.end(); itAsset++)
	{
		if (node == *itAsset)
		{
			_assets.erase(itAsset);
			break;
		}
	}

	delete node;
}

BlastAssetInstanceNode* BlastTreeData::addBlastInstance(BPPAssetInstance& instance)
{
	BlastAssetInstanceNode* blastAssetInstanceNode = new BlastAssetInstanceNode(instance.name.buf, instance);
	_assetInstancesNode->children.push_back(blastAssetInstanceNode);
	blastAssetInstanceNode->setParent(_assetInstancesNode);
	void* pointer = (void*)&instance;
	_blastProjectDataToNodeMap.insert(std::make_pair(pointer, blastAssetInstanceNode));
	return blastAssetInstanceNode;
}

void BlastTreeData::removeBlastInstance(BPPAssetInstance& instance)
{
	void* pointer = (void*)&instance;
	std::map<void*, BlastNode*>::iterator it = _blastProjectDataToNodeMap.find(pointer);
	if (it == _blastProjectDataToNodeMap.end())
	{
		return;
	}

	BlastNode* node = it->second;
	_blastProjectDataToNodeMap.erase(it);
	delete node;

	int count = _assetInstancesNode->children.size();
	for (int i = count - 1; i >= 0; --i)
	{
		BlastNode* pBN = _assetInstancesNode->children[i];
		if (pBN == node)
		{
			_assetInstancesNode->children.erase(_assetInstancesNode->children.begin() + i);
		}
		else
		{
			BlastAssetInstanceNode* blastAssetInstanceNode = dynamic_cast<BlastAssetInstanceNode*>(_assetInstancesNode->children[i]);
			if (blastAssetInstanceNode && blastAssetInstanceNode->getData() == &instance)
			{
				_assetInstancesNode->children.erase(_assetInstancesNode->children.begin() + i);
			}
		}
	}
}

BlastProjectileNode* BlastTreeData::addProjectile(PhysXSceneActor* projectile)
{
	SceneController& sceneController = SampleManager::ins()->getSceneController();
	BlastProjectileNode* projectileNode = new BlastProjectileNode(sceneController.getProjectileName(projectile), projectile);
	_projectiles.push_back(projectileNode);
	return projectileNode;
}

void BlastTreeData::clearProjectile()
{
	std::vector<BlastProjectileNode*>::iterator it;
	for (it = _projectiles.begin(); it != _projectiles.end(); it++)
	{
		delete *it;
	}
	_projectiles.clear();
}

void BlastTreeData::refreshProjectDataToNodeMap(std::map<BPPAsset*, BPPAsset*>& changeMap)
{
	std::map<BPPAsset*, BPPAsset*>::iterator it;
	std::map<void*, BlastNode*>::iterator itNode;
	for (it = changeMap.begin(); it != changeMap.end(); it++)
	{
		itNode = _blastProjectDataToNodeMap.find(it->first);
		if (itNode == _blastProjectDataToNodeMap.end())
		{
			continue;
		}

		BlastNode* node = itNode->second;
		_blastProjectDataToNodeMap.erase(itNode);
		_blastProjectDataToNodeMap[it->second] = node;
	}
}

void BlastTreeData::refreshProjectDataToNodeMap(std::map<BPPAssetInstance*, BPPAssetInstance*>& changeMap)
{
	std::map<BPPAssetInstance*, BPPAssetInstance*>::iterator it;
	std::map<void*, BlastNode*>::iterator itNode;
	for (it = changeMap.begin(); it != changeMap.end(); it++)
	{
		itNode = _blastProjectDataToNodeMap.find(it->first);
		if (itNode == _blastProjectDataToNodeMap.end())
		{
			continue;
		}

		BlastNode* node = itNode->second;
		_blastProjectDataToNodeMap.erase(itNode);
		_blastProjectDataToNodeMap[it->second] = node;
		node->setData(it->second);
	}
}

void BlastSceneTree::addBlastAsset(BPPAsset& asset)
{
	BlastAssetNode* assetNode = BlastTreeData::ins().addBlastAsset(asset);
	if (assetNode == nullptr)
	{
		return;
	}

	QStandardItem* assetTreeWidgetItem = new QStandardItem();
	_treeModel->appendRow(assetTreeWidgetItem);
	assetTreeWidgetItem->setText(assetNode->name.c_str());
	assetTreeWidgetItem->setIcon(sAssetIcon);
	_treeItemDataMap.insert(assetTreeWidgetItem, assetNode);
	_treeDataItemMap.insert(assetNode, assetTreeWidgetItem);

	_addChunkUI(assetNode, assetTreeWidgetItem);

	ui.blastSceneTree->expandAll();
}

void BlastSceneTree::removeBlastAsset(BPPAsset& asset)
{
	BlastNode* node = BlastTreeData::ins().getBlastNodeByProjectData(&asset);

	QMap<BlastNode*, QStandardItem*>::iterator it = _treeDataItemMap.find(node);
	if (it != _treeDataItemMap.end())
	{
		QStandardItem* item = it.value();
		
		if (item != nullptr)
		{
			_treeModel->removeRow(_treeModel->indexFromItem(item).row());
			ui.blastSceneTree->expandAll();
		}
	}

	BlastTreeData::ins().removeBlastAsset(asset);
}

void BlastSceneTree::addBlastInstance(BPPAssetInstance& instance)
{
	BlastAssetInstanceNode* assetInstanceNode = BlastTreeData::ins().addBlastInstance(instance);
	if (assetInstanceNode == nullptr)
	{
		return;
	}

	QStandardItem* assetInstanceItem = new QStandardItem();
	_compositeTreeItem->appendRow(assetInstanceItem);
	assetInstanceItem->setText(assetInstanceNode->name.c_str());
	if (assetInstanceNode->getType() == eAssetInstance)
		assetInstanceItem->setIcon(sAssetIcon);
	_treeItemDataMap.insert(assetInstanceItem, assetInstanceNode);
	_treeDataItemMap.insert(assetInstanceNode, assetInstanceItem);

	ui.blastSceneTree->expandAll();
}

void BlastSceneTree::removeBlastInstance(BPPAssetInstance& instance)
{
	BlastNode* node = BlastTreeData::ins().getBlastNodeByProjectData(&instance);

	QMap<BlastNode*, QStandardItem*>::iterator it = _treeDataItemMap.find(node);
	if (it != _treeDataItemMap.end())
	{
		QStandardItem* item = it.value();

		if (item != nullptr)
		{
			_compositeTreeItem->removeRow(_treeModel->indexFromItem(_compositeTreeItem).row());
			ui.blastSceneTree->expandAll();
		}
	}

	BlastTreeData::ins().removeBlastInstance(instance);
}

void BlastSceneTree::removeBlastInstances(BPPAsset& asset)
{
	std::vector<BPPAssetInstance*> instances;
	BlastProject::ins().getAssetInstances(asset.ID, instances);

	std::vector<BPPAssetInstance*>::iterator it;
	for (it = instances.begin(); it != instances.end(); it++)
	{
		removeBlastInstance(**it);
		BlastTreeData::ins().removeBlastInstance(**it);
	}
}

void BlastSceneTree::addProjectile(PhysXSceneActor* projectile)
{
	BlastProjectileNode* projectileNode = BlastTreeData::ins().addProjectile(projectile);
	if (projectileNode == nullptr)
	{
		return;
	}

	QStandardItem* projectileTreeItem = new QStandardItem();
	projectileTreeItem->setText(projectileNode->name.c_str());
	projectileTreeItem->setIcon(sProjectileIcon);
	_treeModel->appendRow(projectileTreeItem);
	_projectileItemActorMap[projectileTreeItem] = projectile;;
}

void BlastSceneTree::clearProjectile()
{
	QMap<QStandardItem*, PhysXSceneActor*>::iterator it;
	for (it =_projectileItemActorMap.begin(); it != _projectileItemActorMap.end(); it++)
	{
		_treeModel->removeRow(_treeModel->indexFromItem(it.key()).row());
	}
	_projectileItemActorMap.clear();

	BlastTreeData::ins().clearProjectile();
}
*/
BlastNode* BlastSceneTree::getBlastNodeByItem(QStandardItem* item)
{
	QMap<QStandardItem*, BlastNode*>::iterator itr = _treeItemDataMap.find(item);
	if (itr == _treeItemDataMap.end())
		return nullptr;

	return itr.value();
}

PhysXSceneActor* BlastSceneTree::getProjectileActorByItem(QStandardItem* item)
{
	QMap<QStandardItem*, PhysXSceneActor*>::iterator itr = _projectileItemActorMap.find(item);
	if (itr == _projectileItemActorMap.end())
		return nullptr;

	return itr.value();
}

void BlastSceneTree::selectTreeItem(BlastNode* node, bool updateData)
{
	_updateData = updateData;
	QMap<BlastNode*, QStandardItem*>::iterator itr = _treeDataItemMap.find(node);
	if (itr != _treeDataItemMap.end())
	{
		QItemSelectionModel* selectionModel = ui.blastSceneTree->selectionModel();
		selectionModel->select(_treeModel->indexFromItem(itr.value()), QItemSelectionModel::Select);
	}

	_updateData = true;
}

void BlastSceneTree::selectTreeItem(BlastFamily* family)
{
	BlastAssetInstanceNode* instanceNode = BlastTreeData::ins().getAssetInstanceNode(family);
	selectTreeItem(instanceNode, false);
}