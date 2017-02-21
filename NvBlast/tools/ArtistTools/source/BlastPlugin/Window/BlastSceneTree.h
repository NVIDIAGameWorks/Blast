#ifndef BLASTSCENETREE_H
#define BLASTSCENETREE_H

#include <QtWidgets/QDockWidget>
#include "ui_BlastSceneTree.h"
#include "ProjectParams.h"
#include <vector>
#include <string>
#include <QtCore/QMap>

class QTreeWidgetItem;
class BlastAsset;

enum EBlastNodeType
{
	eBond,
	eChunk,
	eAsset,
	eProjectile,
	eGraphicsMesh,
	eAssetInstance,
	eLandmark,
	eComposite,
};

class BlastNode
{
public:
	BlastNode(const std::string& inName, void* inData)
		: name(inName)
		, _data(inData)
	{
	}

	void* getData() { return _data;  }
	void setParent(BlastNode* parent) { _parent = parent; }
	BlastNode* getParent() { return _parent; }
	virtual EBlastNodeType getType() = 0;
	virtual bool getVisible() = 0;
	virtual void setVisible(bool val) = 0;

	std::string name;
	std::vector<BlastNode*> children;

protected:
	void* _data;
	BlastNode* _parent;
};

class BlastBondNode : public BlastNode
{
public:
	BlastBondNode(const std::string& inName, BPPBond& inData)
		: BlastNode(inName, &inData)
	{
	}
	virtual EBlastNodeType getType() { return eBond; }
	virtual bool getVisible() { return ((BPPBond*)_data)->visible; }
	virtual void setVisible(bool val) { ((BPPBond*)_data)->visible = val; }

private:
	std::vector<BlastNode*> children;
};

class BlastChunkNode : public BlastNode
{
public:
	BlastChunkNode(const std::string& inName, BPPChunk& inData, void* assetPtr)
		: BlastNode(inName, &inData)
	{
		_assetPtr = assetPtr;
	}
	virtual EBlastNodeType getType() { return eChunk; }
	virtual bool getVisible() { return ((BPPChunk*)_data)->visible; }
	virtual void setVisible(bool val);// { ((BPPChunk*)_data)->visible = val; }
	void setSelected(bool val);
	bool isSupport()	{ return ((BPPChunk*)_data)->support; }
	void* _assetPtr;
};

class BlastAssetNode : public BlastNode
{
public:
	BlastAssetNode(const std::string& inName, BPPAsset& inData)
		: BlastNode(inName, &inData)
	{
	}
	virtual EBlastNodeType getType() { return eAsset; }
	virtual bool getVisible() { return ((BPPAsset*)_data)->visible; }
	virtual void setVisible(bool val) { ((BPPAsset*)_data)->visible = val; }
};

class BlastProjectileNode : public BlastNode
{
public:
	BlastProjectileNode(const std::string& inName, BPPProjectile& inData)
		: BlastNode(inName, &inData)
	{
	}
	virtual EBlastNodeType getType() { return eProjectile; }
	virtual bool getVisible() { return ((BPPProjectile*)_data)->visible; }
	virtual void setVisible(bool val) { ((BPPProjectile*)_data)->visible = val; }
};

class BlastGraphicsMeshNode : public BlastNode
{
public:
	BlastGraphicsMeshNode(const std::string& inName, BPPGraphicsMesh& inData)
		: BlastNode(inName, &inData)
	{
	}
	virtual EBlastNodeType getType() { return eGraphicsMesh; }
	virtual bool getVisible() { return ((BPPGraphicsMesh*)_data)->visible; }
	virtual void setVisible(bool val) { ((BPPGraphicsMesh*)_data)->visible = val; }
};

class BlastAssetInstanceNode : public BlastNode
{
public:
	BlastAssetInstanceNode(const std::string& inName, BPPAssetInstance& inData)
		: BlastNode(inName, &inData)
	{
	}
	virtual EBlastNodeType getType() { return eAssetInstance; }
	virtual bool getVisible() { return ((BPPAssetInstance*)_data)->visible; }
	virtual void setVisible(bool val) { ((BPPAssetInstance*)_data)->visible = val; }
	void setSelected(bool val);
};

class BlastLandmarkNode : public BlastNode
{
public:
	BlastLandmarkNode(const std::string& inName, BPPLandmark& inData)
		: BlastNode(inName, &inData)
	{
	}
	virtual EBlastNodeType getType() { return eLandmark; }
	virtual bool getVisible() { return ((BPPLandmark*)_data)->visible; }
	virtual void setVisible(bool val) { ((BPPLandmark*)_data)->visible = val; }
};

class BlastCompositeNode : public BlastNode
{
public:
	BlastCompositeNode(const std::string& inName, BPPComposite& inData)
		: BlastNode(inName, &inData)
	{
	}
	virtual EBlastNodeType getType() { return eComposite; }
	virtual bool getVisible() { return ((BPPComposite*)_data)->visible; }
	virtual void setVisible(bool val) { ((BPPComposite*)_data)->visible = val; }
};

class BlastTreeData
{
public:
	static BlastTreeData& ins();
	static bool isChild(BlastChunkNode* parent, BlastChunkNode* child);
	static std::vector<BlastChunkNode*> getTopChunkNodes(std::vector<BlastChunkNode*>& nodes);
	static bool isRoot(BlastChunkNode* node);
	static bool isLeaf(BlastChunkNode* node);
	static void makeSupport(BlastChunkNode* node);
	static void makeStaticSupport(BlastChunkNode* node);
	static void removeSupport(BlastChunkNode* node);

	BlastNode* getBlastNodeByProjectData(void* blastProjectData);
	BlastCompositeNode* getCompsiteNode()						{ return _composite; }
	std::vector<BlastAssetNode*>& getAssetNodes()				{ return _assets; }
	std::vector<BlastProjectileNode*>& getProjectileNodes()		{ return _projectiles; }
	std::vector<BlastGraphicsMeshNode*>& getGraphicsMeshNodes()	{ return _graphicsMeshes; }
	std::vector<BlastChunkNode*> getChunkNodeByBlastChunk(const BlastAsset* asset, const std::vector<uint32_t>& chunkIndexes);
	bool isCompleteSupportAsset(const BlastAsset* asset);
	bool isCompleteSupportAsset(const BlastAssetNode* node);
	bool isOverlapSupportAsset(const BlastAsset* asset);
	bool isOverlapSupportAsset(const BlastAssetNode* node);
	void update();

	void updateVisible(uint32_t assetIndex, uint32_t chunkIndex, bool visible);
	
private:
	BlastTreeData();
	void _addChunkNode(const BPPChunk& parentData, BPPAsset& asset, BlastChunkNode* parentNode, void* assetPtr);
	void _freeBlastNode();
	BlastAssetNode* _getAssetNode(const BlastAsset* asset);

private:
	BlastCompositeNode*					_composite;
	std::vector<BlastAssetNode*>		_assets;
	std::vector<BlastProjectileNode*>	_projectiles;
	std::vector<BlastGraphicsMeshNode*>	_graphicsMeshes;
	std::map<void*, BlastNode*>			_blastProjectDataToNodeMap;
};

class ISceneObserver
{
public:
	virtual void dataSelected(std::vector<BlastNode*> selections) = 0;
};

class VisualButton : public QWidget
{
	Q_OBJECT
public:
	VisualButton(QWidget* parent, BlastNode* blastItem);

protected slots:
	void on_visualbility_toggled(bool checked);
private:
	void _updateBlast(bool visible);

private:
	QPushButton* _button;
	BlastNode* _blastItem;
};

class BlastSceneTree : public QDockWidget, public ISceneObserver
{
	Q_OBJECT

public:
	static BlastSceneTree* ins();

	BlastSceneTree(QWidget *parent = 0);
	~BlastSceneTree();

	void updateValues(bool updataData = true);

	virtual void dataSelected(std::vector<BlastNode*> selections);

	void addObserver(ISceneObserver* observer);
	void removeObserver(ISceneObserver* observer);

	void updateVisible(uint32_t assetIndex, uint32_t chunkIndex, bool visible);
	void updateChunkItemSelection();

	void makeSupport();
	void makeStaticSupport();
	void removeSupport();
	void bondChunks();
	void bondChunksWithJoints();
	void removeAllBonds();

protected slots:
	void on_btnAsset_clicked();
	void on_assetComposite_clicked();
	void on_btnChunk_clicked();
	void on_btnBond_clicked();
	void on_btnProjectile_clicked();
	void on_blastSceneTree_customContextMenuRequested(const QPoint &pos);
	void on_blastSceneTree_itemSelectionChanged();
	void onMakeSupportMenuItemClicked();
	void onMakeStaticSupportMenuItemClicked();
	void onRemoveSupportMenuItemClicked();
	void onBondChunksMenuItemClicked();
	void onBondChunksWithJointsMenuItemClicked();
	void onRemoveAllBondsMenuItemClicked();

private:
	void _updateTreeUIs();
	void _addChunkUI(const BlastNode* parentNode, QTreeWidgetItem* parentTreeItem);
	void _updateChunkTreeItemAndMenu(BPPChunk* chunk, QTreeWidgetItem* chunkItem);
	void _updateChunkTreeItems();

	void _selectTreeItem(BlastNode* node);

	//void _createTestData();

private:
	Ui::BlastSceneTree ui;
	QMap<QTreeWidgetItem*, BlastNode*>	_treeItemDataMap;
	QMap<BlastNode*, QTreeWidgetItem*>	_treeDataItemMap;
	QMenu*								_treeChunkContextMenu;
	QMenu*								_treeBondContextMenu;
	QAction*							_makeSupportAction;
	QAction*							_makeStaticSupportAction;
	QAction*							_removeSupportAction;
	QAction*							_bondChunksAction;
	QAction*							_bondChunksWithJointsAction;
	QAction*							_removeAllBondsAction;
	std::vector<ISceneObserver*>		_observers;
	bool								_updateData;
};

#endif // BLASTSCENETREE_H
