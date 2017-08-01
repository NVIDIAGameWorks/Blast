#ifndef BLASTSCENETREE_H
#define BLASTSCENETREE_H

#include <QtWidgets/QDockWidget>
#include "ui_BlastSceneTree.h"
#include "ProjectParams.h"
#include <vector>
#include <string>
#include <QtCore/QMap>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QStyledItemDelegate>

class QTreeWidgetItem;
class BlastAsset;
class PhysXSceneActor;
class BlastNode;
class BlastFamily;

enum EBlastNodeType
{
	eBond,
	eChunk,
	eAsset,
	eProjectile,
	eGraphicsMesh,
	eAssetInstance,
	eAssetInstances,
};

class BlastVisitorBase
{
public:
	BlastVisitorBase() : _continueTraversing(true)
	{
	}

	virtual void visit(BlastNode *pNode) { return; }
	inline bool continueTraversing(void) const { return _continueTraversing; }

protected:
	bool	_continueTraversing;
};

class BlastNode
{
public:
	BlastNode(const std::string& inName, void* inData)
		: name(inName)
		, _data(inData)
		, _parent(nullptr)
	{
	}

	virtual ~BlastNode()
	{
	}

	inline void* getData() { return _data;  }
	void setParent(BlastNode* parent) { _parent = parent; }
	BlastNode* getParent() { return _parent; }
	void traverse(BlastVisitorBase& visitor);

	virtual EBlastNodeType getType() = 0;
	virtual bool getVisible() = 0;
	virtual void setVisible(bool val) = 0;

	std::string name;
	std::vector<BlastNode*> children;

protected:
	inline void setData(void* data) { _data = data; }

private:
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
	virtual bool getVisible() { return ((BPPBond*)getData())->visible; }
	virtual void setVisible(bool val) { ((BPPBond*)getData())->visible = val; }
	bool isWolrd() { return ((BPPBond*)getData())->toChunk == 0xFFFFFFFF; }

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
	virtual bool getVisible() { return ((BPPChunk*)getData())->visible; }
	virtual void setVisible(bool val);// { ((BPPChunk*)getData())->visible = val; }
	void setSelected(bool val);
	bool isSupport()	{ return ((BPPChunk*)getData())->support; }
	bool isStatic() { return ((BPPChunk*)getData())->staticFlag; }
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
	virtual bool getVisible() { return ((BPPAsset*)getData())->visible; }
	virtual void setVisible(bool val) { ((BPPAsset*)getData())->visible = val; }
	void setSelected(bool val);
};

class BlastProjectileNode : public BlastNode
{
public:
	BlastProjectileNode(const std::string& inName, PhysXSceneActor* inData)
		: BlastNode(inName, inData)
	{
	}
	virtual EBlastNodeType getType() { return eProjectile; }
	virtual bool getVisible();
	virtual void setVisible(bool val);
};

class BlastGraphicsMeshNode : public BlastNode
{
public:
	BlastGraphicsMeshNode(const std::string& inName, BPPGraphicsMesh& inData)
		: BlastNode(inName, &inData)
	{
	}
	virtual EBlastNodeType getType() { return eGraphicsMesh; }
	virtual bool getVisible() { return true; }
	virtual void setVisible(bool val) { }
};

class BlastAssetInstanceNode : public BlastNode
{
public:
	BlastAssetInstanceNode(const std::string& inName, BPPAssetInstance& inData)
		: BlastNode(inName, &inData)
	{
	}
	~BlastAssetInstanceNode()
	{
	}
	virtual EBlastNodeType getType() { return eAssetInstance; }
	virtual bool getVisible() { return ((BPPAssetInstance*)getData())->visible; }
	virtual void setVisible(bool val) { ((BPPAssetInstance*)getData())->visible = val; }
	void setSelected(bool val);
};

class BlastAssetInstancesNode : public BlastNode
{
public:
	BlastAssetInstancesNode(const std::string& inName)
		: BlastNode(inName, nullptr)
	{
	}
	virtual EBlastNodeType getType() { return eAssetInstances; }
	virtual bool getVisible() { return true; }
	virtual void setVisible(bool val) { return; }
};

class BlastTreeData
{
public:
	static BlastTreeData& ins();
	static bool isChild(BlastChunkNode* parent, BlastChunkNode* child);
	static std::vector<BlastChunkNode*> getTopChunkNodes(std::vector<BlastChunkNode*>& nodes);
	static int getDepth(BlastNode* node);// if node is not a chunk or bond, then depth is -1.
	static bool isRoot(BlastChunkNode* node);
	static bool isLeaf(BlastChunkNode* node);
	static void makeSupport(BlastChunkNode* node);
	static void makeStaticSupport(BlastChunkNode* node);
	static void removeSupport(BlastChunkNode* node);
	static std::string getAssetName(BlastAsset* asset);
	static BlastAsset* getAsset(std::string assetName);

	BlastNode* getBlastNodeByProjectData(void* blastProjectData);
	BlastAssetInstancesNode* getBlastAssetInstancesNode()						{ return _assetInstancesNode; }
	std::vector<BlastAssetNode*>& getAssetNodes()				{ return _assets; }
	BlastAsset* getAsset(BlastNode* node);
	BlastAssetNode* getAssetNode(BlastAsset* asset);
	std::vector<BlastProjectileNode*>& getProjectileNodes()		{ return _projectiles; }
	std::vector<BlastGraphicsMeshNode*>& getGraphicsMeshNodes()	{ return _graphicsMeshes; }
	std::vector<BlastChunkNode*> getChunkNodeByBlastChunk(const BlastAsset* asset, const std::vector<uint32_t>& chunkIndexes);
	std::vector<BlastChunkNode*> getRootChunkNodeByInstance(const BlastAssetInstanceNode* node);
	std::vector<BlastNode*> getNodesByDepth(BlastAssetNode* node, uint32_t depth);
	std::vector<BlastNode*> getNodesByDepth(uint32_t depth);
	std::vector<BlastNode*> getNodesByDepth(std::vector<uint32_t> depths);
	BlastNode* getNodeByIndex(uint32_t assetIndex, uint32_t chunkIndex);	
	std::vector<BlastChunkNode*> getSupportChunkNodes(BlastAssetNode* node);
	std::vector<BlastChunkNode*> getSupportChunkNodes();
	std::vector<BlastChunkNode*> getSiblingChunkNodes(BlastChunkNode* node);
	BlastChunkNode* getSupportAncestor(BlastChunkNode* node);
	const std::vector<BlastNode*>& getAllChunkNodes(std::vector<BlastNode*>& res, BlastAssetNode* node);
	const std::vector<BlastNode*>& getAllChunkNodes(std::vector<BlastNode*>& res);
	const std::vector<BlastNode*>& getAllLeavesChunkNodes(std::vector<BlastNode*>& res, BlastAssetNode* node);
	const std::vector<BlastNode*>& getAllLeavesChunkNodes(std::vector<BlastNode*>& res);
	const std::vector<BlastNode*>& getChunkNodesFullCoverage(std::vector<BlastNode*>& res, BlastAssetNode* node, int depth);
	const std::vector<BlastNode*>& getChunkNodesFullCoverage(std::vector<BlastNode*>& res, int depth);

	bool isCompleteSupportAsset(const BlastAsset* asset);
	bool isCompleteSupportAsset(const BlastAssetNode* node);
	bool isOverlapSupportAsset(const BlastAsset* asset);
	bool isOverlapSupportAsset(const BlastAssetNode* node);

	BlastAssetNode* addAsset(const BlastAsset* asset);
	BlastAssetInstanceNode* addAssetInstance(const BlastAsset* asset);
	void remove(const BlastAssetNode* node);
	void remove(const BlastAssetInstanceNode* node);
	BlastAssetInstanceNode* getAssetInstanceNode(BlastFamily* family);
	BlastAssetInstanceNode* getAssetInstanceNode(BPPAssetInstance* instance);
	std::vector<BlastAssetInstanceNode*> getAssetInstanceNodes(BlastChunkNode* chunkNode);

	void update();
	void update(const BlastAsset* asset);
	/*
	BlastAssetNode* addBlastAsset(BPPAsset& asset);
	void removeBlastAsset(BPPAsset& asset);

	BlastAssetInstanceNode* addBlastInstance(BPPAssetInstance& instance);
	void removeBlastInstance(BPPAssetInstance& instance);

	BlastProjectileNode* addProjectile(PhysXSceneActor* projectile);
	void clearProjectile();

	void refreshProjectDataToNodeMap(std::map<BPPAsset*, BPPAsset*>& changeMap);
	void refreshProjectDataToNodeMap(std::map<BPPAssetInstance*, BPPAssetInstance*>& changeMap);
	*/

	void traverse(BlastVisitorBase& visitor);

private:
	BlastTreeData();
	void _addChunkNode(BPPChunk& parentData, BPPAsset& asset, BlastChunkNode* parentNode, void* assetPtr);
	void _removeChunkNode(BPPAsset& asset);
	void _freeBlastNode();
	BlastAssetNode* _getAssetNode(const BlastAsset* asset);

private:
	BlastAssetInstancesNode*			_assetInstancesNode;
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

class BlastTreeViewDelegate : QStyledItemDelegate
{
	Q_OBJECT

public:
	BlastTreeViewDelegate(QObject* parent, QStandardItemModel* model);
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	bool editorEvent(QEvent* evt, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);

protected:
	bool eventFilter(QObject* object, QEvent* event);

private:
	QStandardItemModel*		_treeModel;
};

class BlastSceneTreeDataLock;

class BlastSceneTree : public QDockWidget, public ISceneObserver
{
	Q_OBJECT

	friend class BlastSceneTreeDataLock;
public:
	static BlastSceneTree* ins();

	BlastSceneTree(QWidget *parent = 0);
	~BlastSceneTree();

	void updateValues(bool updataData = true);

	void clear();

	virtual void dataSelected(std::vector<BlastNode*> selections);

	void addObserver(ISceneObserver* observer);
	void removeObserver(ISceneObserver* observer);

	void updateVisible(uint32_t assetIndex, uint32_t chunkIndex, bool visible);
	void updateChunkItemSelection();

	void makeSupport();
	void makeStaticSupport();
	void removeSupport();
	void makeWorld();
	void removeWorld();
	void bondChunks();
	void bondChunksWithJoints();
	void removeAllBonds();

	void setChunkSelected(std::vector<uint32_t> depths, bool selected);
	void setChunkVisible(std::vector<uint32_t> depths, bool bVisible);

	void setChunkVisibleFullCoverage(int depth);
	void hideAllChunks();
	/*
	void addBlastAsset(BPPAsset& asset);
	void removeBlastAsset(BPPAsset& asset);

	void addBlastInstance(BPPAssetInstance& instance);
	void removeBlastInstance(BPPAssetInstance& instance);
	void removeBlastInstances(BPPAsset& asset);

	void addProjectile(PhysXSceneActor* projectile);
	void clearProjectile();
	*/
	BlastNode* getBlastNodeByItem(QStandardItem* item);
	PhysXSceneActor* getProjectileActorByItem(QStandardItem* item);

	void selectTreeItem(BlastNode* node, bool updateData = true);
	void selectTreeItem(BlastFamily* family);

	void ApplyAutoSelectNewChunks(BlastAsset* pNewBlastAsset, std::vector<uint32_t>& NewChunkIndexes);

protected slots:
	void on_btnAsset_clicked();
	void on_assetComposite_clicked();
	void on_btnChunk_clicked();
	void on_btnBond_clicked();
	void on_btnProjectile_clicked();
	void on_btnExpandCollapse_clicked();
	void on_blastSceneTree_customContextMenuRequested(const QPoint& pos);
	void on_blastSceneTree_itemSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void onMakeSupportMenuItemClicked();
	void onMakeStaticSupportMenuItemClicked();
	void onRemoveSupportMenuItemClicked();
	void onMakeWorldMenuItemClicked();
	void onRemoveWorldMenuItemClicked();
	void onBondChunksMenuItemClicked();
	void onBondChunksWithJointsMenuItemClicked();
	void onRemoveAllBondsMenuItemClicked();
	void onCollapseExpandClicked();

private:
	void _updateTreeUIs();
	void _addChunkUI(const BlastNode* parentNode, QStandardItem* parentTreeItem);
	void _updateChunkTreeItemAndMenu(BPPChunk* chunk, QStandardItem* chunkItem);
	void _updateChunkTreeItems();

private:
	Ui::BlastSceneTree ui;
	QStandardItemModel*					_treeModel;
	QMap<QStandardItem*, BlastNode*>	_treeItemDataMap;
	QMap<BlastNode*, QStandardItem*>	_treeDataItemMap;
	QMenu*								_treeContextMenu;
	QAction*							_makeSupportAction;
	QAction*							_makeStaticSupportAction;
	QAction*							_removeSupportAction;
	QAction*							_makeWorldAction;
	QAction*							_removeWorldAction;
	QAction*							_bondChunksAction;
	QAction*							_bondChunksWithJointsAction;
	QAction*							_removeAllBondsAction;
	std::vector<ISceneObserver*>		_observers;
	bool								_updateData;
//	QStandardItem*						_compositeTreeItem;
	QMap<QStandardItem*, PhysXSceneActor*>	_projectileItemActorMap;

	BlastAsset* m_pNewBlastAsset;
	std::vector<uint32_t> m_NewChunkIndexes;
};

#endif // BLASTSCENETREE_H
