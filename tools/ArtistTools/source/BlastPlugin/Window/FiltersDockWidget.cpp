#include "FiltersDockWidget.h"
#include "ui_FiltersDockWidget.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include "SampleManager.h"
#include "BlastSceneTree.h"
#include "BlastFamily.h"
#include "ViewerOutput.h"

const QString LISTITEM_NORMAL_STYLESHEET = ":enabled { background: rgb(68,68,68); }";

const QString LISTITEM_SELECTED_STYLESHEET = ":enabled { background: rgba(118,185,0, 250); }";

const QString LISTITEM_BUTTON_STYLESHEET = ":enabled { border: 0px; }";

class FilterOperation
{
public:
	virtual void execute(BlastNode *node) = 0;
};

class FilterVisitor : public BlastVisitorBase
{
public:
	FilterVisitor(FilterPreset& filterPreset, FilterOperation&	filterOperation)
		: resultCount(0)
		, _filterPreset(filterPreset)
		, _filterOperation(filterOperation)
		
	{

	}

	virtual void visit(BlastNode *node);

	int					resultCount;
private:
	bool _canSatisfyFilter(BlastNode *node);

	FilterPreset&		_filterPreset;
	FilterOperation&	_filterOperation;
};

void FilterVisitor::visit(BlastNode *node)
{
	if (_canSatisfyFilter(node))
	{
		_filterOperation.execute(node);
		++resultCount;
	}
}

bool FilterVisitor::_canSatisfyFilter(BlastNode *node)
{
	EFilterRestriction restrictions = eFilterRestriction_Invalid;

	for (std::vector<EFilterRestriction>::iterator itr = _filterPreset.filters.begin(); itr != _filterPreset.filters.end(); ++itr)
	{
		restrictions = (EFilterRestriction)(restrictions | (*itr));
	}

	bool matchDepth = false, matchItemType = false;

	// jude whether current node can be filtered by depth restrictions
	if (restrictions & eFilterRestriction_DepthAll)
	{
		matchDepth = true;
	}
	else
	{
		int nodeDepth = BlastTreeData::getDepth(node);
		int allDepthRestrictions = eFilterRestriction_Depth0 | eFilterRestriction_Depth1 | eFilterRestriction_Depth2 | eFilterRestriction_Depth3 | eFilterRestriction_Depth4 | eFilterRestriction_Depth5;

		if ((restrictions & eFilterRestriction_AllDescendants) && (restrictions & eFilterRestriction_AllParents))
		{
			if (restrictions & allDepthRestrictions)
			{
				matchDepth = true;
			}
		}
		else if ((restrictions & eFilterRestriction_AllDescendants) && !(restrictions & eFilterRestriction_AllParents))
		{
			int expectMinDepthRestirction = 5;
			if ((restrictions & eFilterRestriction_Depth0))
			{
				expectMinDepthRestirction = 0;
			}
			else if ((restrictions & eFilterRestriction_Depth1))
			{
				expectMinDepthRestirction = 1;
			}
			else if ((restrictions & eFilterRestriction_Depth2))
			{
				expectMinDepthRestirction = 2;
			}
			else if ((restrictions & eFilterRestriction_Depth3))
			{
				expectMinDepthRestirction = 3;
			}
			else if ((restrictions & eFilterRestriction_Depth4))
			{
				expectMinDepthRestirction = 4;
			}
			else if ((restrictions & eFilterRestriction_Depth5))
			{
				expectMinDepthRestirction = 5;
			}

			if (expectMinDepthRestirction <= nodeDepth)
			{
				matchDepth = true;
			}
		}
		else if (!(restrictions & eFilterRestriction_AllDescendants) && (restrictions & eFilterRestriction_AllParents))
		{
			int expectMaxDepthRestirction = 0;
			if ((restrictions & eFilterRestriction_Depth5))
			{
				expectMaxDepthRestirction = 5;
			}
			else if ((restrictions & eFilterRestriction_Depth4))
			{
				expectMaxDepthRestirction = 4;
			}
			else if ((restrictions & eFilterRestriction_Depth3))
			{
				expectMaxDepthRestirction = 3;
			}
			else if ((restrictions & eFilterRestriction_Depth2))
			{
				expectMaxDepthRestirction = 2;
			}
			else if ((restrictions & eFilterRestriction_Depth1))
			{
				expectMaxDepthRestirction = 1;
			}
			else if ((restrictions & eFilterRestriction_Depth0))
			{
				expectMaxDepthRestirction = 0;
			}

			if (nodeDepth <= expectMaxDepthRestirction)
			{
				matchDepth = true;
			}
		}
		else if (!(restrictions & eFilterRestriction_AllDescendants) && !(restrictions & eFilterRestriction_AllParents))
		{
			EFilterRestriction expectDepthRestriction = (EFilterRestriction)(eFilterRestriction_Depth0 << nodeDepth);

			if (restrictions & expectDepthRestriction)
			{
				matchDepth = true;
			}
		}
	}

	// jude whether current node can be filtered by item type restrictions
	if ((restrictions & eFilterRestriction_ItemTypeAll))
	{
		matchItemType = true;
	}
	else
	{
		EBlastNodeType nodeType = node->getType();
		if (eChunk == nodeType)
		{
			BlastChunkNode* chunkNode = (BlastChunkNode*)node;
			if (restrictions & eFilterRestriction_Chunk)
			{
				matchItemType = true;
			}
			else if (restrictions & eFilterRestriction_SupportChunk)
			{
				if (chunkNode->isSupport())
				{
					matchItemType = true;
				}
			}
			else if (restrictions & eFilterRestriction_StaticSupportChunk)
			{
				if (chunkNode->isSupport() && chunkNode->isStatic())
				{
					matchItemType = true;
				}
			}
		}
		else if (eBond == nodeType)
		{
			BlastBondNode* bondNode = (BlastBondNode*)node;
			if (restrictions & eFilterRestriction_Bond)
			{
				matchItemType = true;
			}
			else if (restrictions & eFilterRestriction_WorldBond)
			{
				if (bondNode->isWolrd())
				{
					matchItemType = true;
				}
			}
		}
	}

	if ((restrictions & eFilterRestriction_EqualTo) && (restrictions & eFilterRestriction_NotEquaTo))
	{
		return true;
	} 
	else if (!(restrictions & eFilterRestriction_EqualTo) && !(restrictions & eFilterRestriction_NotEquaTo)
		|| (restrictions & eFilterRestriction_EqualTo))
	{
		if (matchDepth && matchItemType)
			return true;
	}
	else if ((restrictions & eFilterRestriction_NotEquaTo))
	{
		if (!(matchDepth && matchItemType))
			return true;
	}

	return false;
}

class FilterSelectOperation : public FilterOperation
{
public:
	virtual void execute(BlastNode *node)
	{
		if (nullptr != node)
		{
			BlastSceneTree::ins()->selectTreeItem(node);
		}
	}
};

class FilterVisibilityOperation : public FilterOperation
{
public:
	FilterVisibilityOperation(bool visible)
		: _visible(visible)
	{
	}

	virtual void execute(BlastNode *node)
	{
		if (nullptr != node)
		{
			node->setVisible(_visible);
		}
	}

private:
	bool	_visible;
};

struct FilterItemInfo
{
public:
	FilterItemInfo(FilterItemWidget* inWidget, const QString& inText)
		: itemWidget(inWidget)
		, text(inText)
	{
	}

	FilterItemWidget* itemWidget;
	QString text;
};

FilterItemWidget::FilterItemWidget(FiltersDockWidget* parent, QListWidgetItem* item, const QString& filterPreset, EFilterRestriction restriction)
	: QWidget(parent)
	, _relatedListWidgetItem(item)
	, _filterPreset(filterPreset)
	, _restriction(restriction)
{
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	_label = new QLabel(this);

	_removeBtn = new QPushButton(this);
	_removeBtn->setMaximumSize(20, 20);
	_removeBtn->setText("X");
	_removeBtn->setStyleSheet(LISTITEM_BUTTON_STYLESHEET);

	layout->addWidget(_label);
	layout->addWidget(_removeBtn);

	this->setLayout(layout);

	QObject::connect(_removeBtn, SIGNAL(clicked()), this, SLOT(onRemoveButtonClicked()));
	QObject::connect(this, SIGNAL(RemoveItem(QListWidgetItem*)), parent, SLOT(onListWidgetRemoveBtnClicked(QListWidgetItem*)));

	deSelect();
}

void FilterItemWidget::setText(const QString& title)
{
	_label->setText(title);
}

void FilterItemWidget::select()
{
	_label->setStyleSheet(LISTITEM_SELECTED_STYLESHEET);
}

void FilterItemWidget::deSelect()
{
	_label->setStyleSheet(LISTITEM_NORMAL_STYLESHEET);
}

void FilterItemWidget::onRemoveButtonClicked()
{
	emit RemoveItem(_relatedListWidgetItem);
}

FiltersDockWidget::FiltersDockWidget(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::FiltersDockWidget),
	_updateData(true),
	_filterUIItems(),
	_filterItemWidgets(),
	_lastSelectRow(-1)
{
    ui->setupUi(this);

	_restrictionActionMap[eFilterRestriction_AllDescendants] = ui->actionAllDescendants;
	_restrictionActionMap[eFilterRestriction_AllParents] = ui->actionAllParents;
	_restrictionActionMap[eFilterRestriction_DepthAll] = ui->actionDepthAll;
	_restrictionActionMap[eFilterRestriction_Depth0] = ui->actionDepth0;
	_restrictionActionMap[eFilterRestriction_Depth1] = ui->actionDepth1;
	_restrictionActionMap[eFilterRestriction_Depth2] = ui->actionDepth2;
	_restrictionActionMap[eFilterRestriction_Depth3] = ui->actionDepth3;
	_restrictionActionMap[eFilterRestriction_Depth4] = ui->actionDepth4;
	_restrictionActionMap[eFilterRestriction_Depth5] = ui->actionDepth5;
	_restrictionActionMap[eFilterRestriction_ItemTypeAll] = ui->actionItemTypeAll;
	_restrictionActionMap[eFilterRestriction_Chunk] = ui->actionChunk;
	_restrictionActionMap[eFilterRestriction_SupportChunk] = ui->actionSupportChunk;
	_restrictionActionMap[eFilterRestriction_StaticSupportChunk] = ui->actionStaticSupportChunk;
	_restrictionActionMap[eFilterRestriction_Bond] = ui->actionBond;
	_restrictionActionMap[eFilterRestriction_WorldBond] = ui->actionWorldBond;
	_restrictionActionMap[eFilterRestriction_EqualTo] = ui->actionEqualTo;
	_restrictionActionMap[eFilterRestriction_NotEquaTo] = ui->actionNotEquaTo;

	_updateFilterItemList();
}

FiltersDockWidget::~FiltersDockWidget()
{
	delete ui;
}

void FiltersDockWidget::updateValues()
{
	_updateData = false;
	ui->comboBoxFilterPreset->clear();
	std::vector<FilterPreset>& filterPresets = BlastProject::ins().getFilterPresets();
	QStringList filterNames;
	filterNames.append("Default");
	int count = (int)filterPresets.size();
	for (int i = 0; i < count; ++i)
	{
		filterNames.append(filterPresets[i].name.c_str());
	}
	ui->comboBoxFilterPreset->addItems(filterNames);

	BPParams& projectParams = BlastProject::ins().getParams();
	if (nullptr == projectParams.filter.activeFilter.buf
		|| 0 == strlen(projectParams.filter.activeFilter.buf)
		|| !(BlastProject::ins().isFilterPresetNameExist(projectParams.filter.activeFilter.buf)))
	{
		ui->comboBoxFilterPreset->setCurrentIndex(0);
	}
	else
	{
		ui->comboBoxFilterPreset->setCurrentText(projectParams.filter.activeFilter.buf);
	}

	_updateFilterItemList();

	if (count > 0)
	{
		ui->btnModifyFilterPreset->setEnabled(true);
		ui->btnRemoveFilterPreset->setEnabled(true);
	}
	else
	{
		ui->btnModifyFilterPreset->setEnabled(false);
		ui->btnRemoveFilterPreset->setEnabled(false);
	}
	_updateData = true;
}

void FiltersDockWidget::on_comboBoxFilterPreset_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	BPParams& projectParams = BlastProject::ins().getParams();
	copy(projectParams.filter.activeFilter, ui->comboBoxFilterPreset->currentText().toUtf8().data());

	_updateFilterItemList();
}

void FiltersDockWidget::on_btnAddFilterPrest_clicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input name for new filter preset:"),
		QLineEdit::Normal,
		"",
		&ok);
	bool nameExist = BlastProject::ins().isFilterPresetNameExist(name.toUtf8().data());
	if (ok && !name.isEmpty() && !nameExist)
	{
		BlastProject::ins().addFilterPreset(name.toUtf8().data());
		updateValues();
		ui->comboBoxFilterPreset->setCurrentIndex(ui->comboBoxFilterPreset->count() - 1);
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && name.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the new filter preset!");
	}
}

void FiltersDockWidget::on_btnModifyFilterPreset_clicked()
{
	if (ui->comboBoxFilterPreset->currentIndex() < 1)
	{
		QMessageBox::warning(this, "Blast Tool", "You should select an filter preset!");
		return;
	}

	QByteArray tmp = ui->comboBoxFilterPreset->currentText().toUtf8();
	const char* oldName = tmp.data();

	bool ok = false;
	QString newName = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input new name for the selected filter preset:"),
		QLineEdit::Normal,
		oldName,
		&ok);
	bool nameExist = BlastProject::ins().isFilterPresetNameExist(newName.toUtf8().data());
	if (ok && !newName.isEmpty() && !nameExist)
	{
		int index = ui->comboBoxFilterPreset->currentIndex();
		BlastProject::ins().renameFilterPreset(oldName, newName.toUtf8().data());
		updateValues();
		ui->comboBoxFilterPreset->setCurrentIndex(index);
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && newName.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the selected filter preset!");
	}
}

void FiltersDockWidget::on_btnRemoveFilterPreset_clicked()
{
	if (ui->comboBoxFilterPreset->currentIndex() < 1)
	{
		QMessageBox::warning(this, "Blast Tool", "You should select an filter preset!");
		return;
	}

	QByteArray tmp = ui->comboBoxFilterPreset->currentText().toUtf8();
	const char* name = tmp.data();
	BlastProject::ins().removeFilterPreset(name);
	updateValues();
}

void FiltersDockWidget::on_btnSaveFilterPreset_clicked()
{
	BlastProject::ins().saveFilterPreset();
}

void FiltersDockWidget::on_actionAllDescendants_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_AllDescendants, checked);
}

void FiltersDockWidget::on_actionAllParents_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_AllParents, checked);
}

void FiltersDockWidget::on_actionDepthAll_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_DepthAll, checked);
}

void FiltersDockWidget::on_actionDepth0_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_Depth0, checked);
}

void FiltersDockWidget::on_actionDepth1_triggered(bool checked)
{
	_addRemoveRestriction(eFilterRestriction_Depth1, checked);
}

void FiltersDockWidget::on_actionDepth2_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_Depth2, checked);
}

void FiltersDockWidget::on_actionDepth3_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_Depth3, checked);
}

void FiltersDockWidget::on_actionDepth4_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_Depth4, checked);
}

void FiltersDockWidget::on_actionDepth5_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_Depth5, checked);
}

void FiltersDockWidget::on_actionItemTypeAll_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_ItemTypeAll, checked);
}

void FiltersDockWidget::on_actionChunk_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_Chunk, checked);
}

void FiltersDockWidget::on_actionSupportChunk_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_SupportChunk, checked);
}

void FiltersDockWidget::on_actionStaticSupportChunk_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_StaticSupportChunk, checked);
}

void FiltersDockWidget::on_actionBond_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_Bond, checked);
}

void FiltersDockWidget::on_actionWorldBond_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_WorldBond, checked);
}

void FiltersDockWidget::on_actionEqualTo_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_EqualTo, checked);
}

void FiltersDockWidget::on_actionNotEquaTo_triggered(bool checked)
{
	if (!_updateData)
		return;

	_addRemoveRestriction(eFilterRestriction_NotEquaTo, checked);
}

void FiltersDockWidget::on_btnSelect_clicked()
{
	FilterPreset filterPreset("Default");
	FilterPreset* filterPresetPtr = BlastProject::ins().getFilterPreset(ui->comboBoxFilterPreset->currentText().toUtf8().data());
	if (nullptr == filterPresetPtr)
	{
		BPPFilter& bppFilter = BlastProject::ins().getParams().filter;
		for (int i = 0; i < bppFilter.filterRestrictions.arraySizes[0]; ++i)
		{
			filterPreset.filters.push_back(convertStringToFilterRestriction(bppFilter.filterRestrictions.buf[i]));
		}
	}
	else
	{
		filterPreset = *filterPresetPtr;
	}

	// clear old selection
	BlastSceneTree::ins()->updateValues(false);

	FilterSelectOperation operation;
	FilterVisitor visitor(filterPreset, operation);
	BlastTreeData::ins().traverse(visitor);

	if (0 == visitor.resultCount)
	{
		viewer_msg("No items statify current filters");
	}
	//BlastSceneTree::ins()->updateValues(false);

	//std::vector<uint32_t> depths;
	//for (FilterItemWidget* itemWidget : _filterItemWidgets)
	//{
	//	depths.push_back(itemWidget->_restriction);
	//}
	////SampleManager::ins()->setChunkSelected(depths, true);
	//BlastSceneTree::ins()->setChunkSelected(depths, true);
}

void FiltersDockWidget::on_btnVisible_clicked()
{
	FilterPreset filterPreset("Default");
	FilterPreset* filterPresetPtr = BlastProject::ins().getFilterPreset(ui->comboBoxFilterPreset->currentText().toUtf8().data());
	if (nullptr == filterPresetPtr)
	{
		BPPFilter& bppFilter = BlastProject::ins().getParams().filter;
		for (int i = 0; i < bppFilter.filterRestrictions.arraySizes[0]; ++i)
		{
			filterPreset.filters.push_back(convertStringToFilterRestriction(bppFilter.filterRestrictions.buf[i]));
		}
	}
	else
	{
		filterPreset = *filterPresetPtr;
	}

	FilterVisibilityOperation operation(true);
	FilterVisitor visitor(filterPreset, operation);
	BlastTreeData::ins().traverse(visitor);
	BlastSceneTree::ins()->updateValues(false);
	if (0 == visitor.resultCount)
	{
		viewer_msg("No items statify current filters");
	}
	//std::vector<uint32_t> depths;
	//for (FilterItemWidget* itemWidget : _filterItemWidgets)
	//{
	//	depths.push_back(itemWidget->_restriction);
	//}
	////SampleManager::ins()->setChunkVisible(depths, true);
	//BlastSceneTree::ins()->setChunkVisible(depths, true);
	//// refresh display in scene tree
	//BlastSceneTree* pBlastSceneTree = BlastSceneTree::ins();
	//pBlastSceneTree->updateValues(false);
}

void FiltersDockWidget::on_btnInVisible_clicked()
{
	FilterPreset filterPreset("Default");
	FilterPreset* filterPresetPtr = BlastProject::ins().getFilterPreset(ui->comboBoxFilterPreset->currentText().toUtf8().data());
	if (nullptr == filterPresetPtr)
	{
		BPPFilter& bppFilter = BlastProject::ins().getParams().filter;
		for (int i = 0; i < bppFilter.filterRestrictions.arraySizes[0]; ++i)
		{
			filterPreset.filters.push_back(convertStringToFilterRestriction(bppFilter.filterRestrictions.buf[i]));
		}
	}
	else
	{
		filterPreset = *filterPresetPtr;
	}

	FilterVisibilityOperation operation(false);
	FilterVisitor visitor(filterPreset, operation);
	BlastTreeData::ins().traverse(visitor);
	BlastSceneTree::ins()->updateValues(false);
	if (0 == visitor.resultCount)
	{
		viewer_msg("No items statify current filters");
	}
	//std::vector<uint32_t> depths;
	//for (FilterItemWidget* itemWidget : _filterItemWidgets)
	//{
	//	depths.push_back(itemWidget->_restriction);
	//}
	////SampleManager::ins()->setChunkVisible(depths, false);
	//BlastSceneTree::ins()->setChunkVisible(depths, false);
	//// refresh display in scene tree
	//BlastSceneTree* pBlastSceneTree = BlastSceneTree::ins();
	//pBlastSceneTree->updateValues(false);
}

void FiltersDockWidget::on_listWidget_currentRowChanged(int index)
{
	if (!_updateData)
		return;

	if (-1 != _lastSelectRow && _lastSelectRow < _filterItemWidgets.size())
		static_cast<FilterItemWidget*>(_filterItemWidgets[_lastSelectRow])->deSelect();

	if (-1 != index && index < _filterItemWidgets.size())
		static_cast<FilterItemWidget*>(_filterItemWidgets[index])->select();

	_lastSelectRow = index;
}

void FiltersDockWidget::onListWidgetRemoveBtnClicked(QListWidgetItem* item)
{
	/// remove former ui info
	map<QListWidgetItem*, FilterItemInfo*>::iterator toRemoveItem = _filterUIItems.find(item);
	if (toRemoveItem != _filterUIItems.end())
	{
		FilterItemWidget* itemWidget = toRemoveItem->second->itemWidget;
		itemWidget->setParent(nullptr);

		_filterItemWidgets.erase(std::find(_filterItemWidgets.begin(), _filterItemWidgets.end(), toRemoveItem->second->itemWidget));

		if (0 < ui->comboBoxFilterPreset->currentIndex())
		{
			QByteArray filterPreset = itemWidget->_filterPreset.toUtf8();
			BlastProject::ins().removeFilterRestriction(filterPreset.data(), itemWidget->_restriction);
		}
		else if (0 == ui->comboBoxFilterPreset->currentIndex())
		{
			removeItem(BlastProject::ins().getParams().filter.filterRestrictions, convertFilterRestrictionToString(itemWidget->_restriction));
		}

		ui->listWidget->takeItem(ui->listWidget->row(item));
		ui->listWidget->removeItemWidget(item);
		delete item;
		delete itemWidget;
		_filterUIItems.erase(toRemoveItem);

		_updateFilterItemList();
		
	}
}

void FiltersDockWidget::_updateFilterItemList()
{
	_updateData = false;
	/// remove former ui info
	map<QListWidgetItem*, FilterItemInfo*>::iterator theEnd = _filterUIItems.end();
	for (map<QListWidgetItem*, FilterItemInfo*>::iterator itr = _filterUIItems.begin(); itr != theEnd; ++itr)
	{
		itr->second->itemWidget->setParent(nullptr);
		delete itr->first;
		delete itr->second->itemWidget;
		delete itr->second;
	}
	_filterUIItems.clear();
	ui->listWidget->clear();
	_filterItemWidgets.clear();

	FilterPreset* filterPreset = BlastProject::ins().getFilterPreset(ui->comboBoxFilterPreset->currentText().toUtf8().data());

	if (nullptr != filterPreset)
	{
		for (EFilterRestriction restriction : filterPreset->filters)
		{
			_addFilterListItem(filterPreset->name.c_str(), restriction);
		}
	}
	else
	{
		BPParams& projectParams = BlastProject::ins().getParams();
		BPPStringArray& filterRestrictions = projectParams.filter.filterRestrictions;

		for (int i = 0; i < filterRestrictions.arraySizes[0]; ++i)
		{
			_addFilterListItem("Default", convertStringToFilterRestriction(filterRestrictions.buf[i].buf));
		}
	}

	_updateFilterUIs();

	_updateData = true;
}

void FiltersDockWidget::_addFilterListItem(const char* filterPresetName, EFilterRestriction restriction)
{
	QListWidgetItem* item = new QListWidgetItem(NULL);
	FilterItemWidget* itemWidget = new FilterItemWidget(this, item, filterPresetName, restriction);

	QString depthFilterLabel = convertFilterRestrictionToString(restriction);
	_filterUIItems.insert(std::make_pair(item, new FilterItemInfo(itemWidget, depthFilterLabel)));
	itemWidget->setText(depthFilterLabel);
	_filterItemWidgets.push_back(itemWidget);

	ui->listWidget->addItem(item);
	ui->listWidget->setItemWidget(item, itemWidget);
}

void FiltersDockWidget::_updateFilterUIs()
{
	for (map<EFilterRestriction, QAction*>::iterator itr = _restrictionActionMap.begin(); itr != _restrictionActionMap.end(); ++itr)
		itr->second->setChecked(false);

	FilterPreset* filterPreset = BlastProject::ins().getFilterPreset(ui->comboBoxFilterPreset->currentText().toUtf8().data());

	if (nullptr != filterPreset)
	{
		for (EFilterRestriction restriction : filterPreset->filters)
		{
			_restrictionActionMap[restriction]->setChecked(true);
		}
	}
	else
	{
		BPPStringArray& filterRestrictions = BlastProject::ins().getParams().filter.filterRestrictions;

		for (int i = 0; i < filterRestrictions.arraySizes[0]; ++i)
		{
			_restrictionActionMap[convertStringToFilterRestriction(filterRestrictions.buf[i].buf)]->setChecked(true);
		}
	}
}

void FiltersDockWidget::_addRemoveRestriction(EFilterRestriction restriction, bool add)
{
	if (0 < ui->comboBoxFilterPreset->currentIndex())
	{
		QByteArray filterPreset = ui->comboBoxFilterPreset->currentText().toUtf8();

		if (add)
			BlastProject::ins().addFilterRestriction(filterPreset.data(), restriction);
		else
			BlastProject::ins().removeFilterRestriction(filterPreset.data(), restriction);
	}
	else if (0 == ui->comboBoxFilterPreset->currentIndex())
	{
		BPPFilter& filter = BlastProject::ins().getParams().filter;
		if (add)
			addItem(filter.filterRestrictions, convertFilterRestrictionToString(restriction));
		else
			removeItem(filter.filterRestrictions, convertFilterRestrictionToString(restriction));
	}
	_updateFilterItemList();
}
