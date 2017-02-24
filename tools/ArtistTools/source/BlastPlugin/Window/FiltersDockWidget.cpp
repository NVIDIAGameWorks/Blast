#include "FiltersDockWidget.h"
#include "ui_FiltersDockWidget.h"
#include "ProjectParams.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include "SampleManager.h"

const QString LISTITEM_NORMAL_STYLESHEET = ":enabled { background: rgb(68,68,68); }";

const QString LISTITEM_SELECTED_STYLESHEET = ":enabled { background: rgba(118,185,0, 250); }";

const QString LISTITEM_BUTTON_STYLESHEET = ":enabled { border: 0px; }";

class FilterItemWidget;

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

FilterItemWidget::FilterItemWidget(FiltersDockWidget* parent, QListWidgetItem* item, const QString& filterPreset, int depth)
	: QWidget(parent)
	, _relatedListWidgetItem(item)
	, _filterPreset(filterPreset)
	, _depth(depth)
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
	_filterUIItems(),
	_filterItemWidgets(),
	_lastSelectRow(-1)
{
    ui->setupUi(this);

	_depthButtons.push_back(ui->btnDepth0);
	_depthButtons.push_back(ui->btnDepth1);
	_depthButtons.push_back(ui->btnDepth2);
	_depthButtons.push_back(ui->btnDepth3);
	_depthButtons.push_back(ui->btnDepth4);
	_depthButtons.push_back(ui->btnDepth5);

	_updateFilterItemList();
	_updateFilterDepthBtns();
}

FiltersDockWidget::~FiltersDockWidget()
{
	delete ui;
}

void FiltersDockWidget::updateValues()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPFilterPresetArray& filterPresetArray = projectParams.filter.filters;

	ui->comboBoxFilterPreset->clear();
	QStringList filterNames;
	int count = filterPresetArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		filterNames.append(filterPresetArray.buf[i].name.buf);
	}
	ui->comboBoxFilterPreset->addItems(filterNames);

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
}

void FiltersDockWidget::on_comboBoxFilterPreset_currentIndexChanged(int index)
{
	BPParams& projectParams = BlastProject::ins().getParams();
	projectParams.filter.activeFilter = index;

	_updateFilterItemList();
	_updateFilterDepthBtns();
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
	QByteArray tmp = ui->comboBoxFilterPreset->currentText().toUtf8();
	const char* name = tmp.data();
	BlastProject::ins().removeFilterPreset(name);
	updateValues();
}

void FiltersDockWidget::on_btnDepth0_clicked(bool val)
{
	_addRemoveDepthFilter(0, val);
}

void FiltersDockWidget::on_btnDepth1_clicked(bool val)
{
	_addRemoveDepthFilter(1, val);
}

void FiltersDockWidget::on_btnDepth2_clicked(bool val)
{
	_addRemoveDepthFilter(2, val);
}

void FiltersDockWidget::on_btnDepth3_clicked(bool val)
{
	_addRemoveDepthFilter(3, val);
}

void FiltersDockWidget::on_btnDepth4_clicked(bool val)
{
	_addRemoveDepthFilter(4, val);
}

void FiltersDockWidget::on_btnDepth5_clicked(bool val)
{
	_addRemoveDepthFilter(5, val);
}

void FiltersDockWidget::on_btnAddDepthFilter_clicked()
{

}

void FiltersDockWidget::on_btnAddFilter_clicked()
{

}

void FiltersDockWidget::on_btnSelect_clicked()
{
	std::vector<uint32_t> depths;
	int row = ui->listWidget->currentRow();
	if (-1 != row)
	{
		depths.push_back(_filterItemWidgets[row]->_depth);
	}
	SampleManager::ins()->setChunkSelected(depths, true);
}

void FiltersDockWidget::on_btnVisible_clicked()
{
	std::vector<uint32_t> depths;
	int row = ui->listWidget->currentRow();
	if (-1 != row)
	{
		depths.push_back(_filterItemWidgets[row]->_depth);
	}
	SampleManager::ins()->setChunkVisible(depths, true);
}

void FiltersDockWidget::on_btnInVisible_clicked()
{
	std::vector<uint32_t> depths;
	int row = ui->listWidget->currentRow();
	if (-1 != row)
	{
		depths.push_back(_filterItemWidgets[row]->_depth);
	}
	SampleManager::ins()->setChunkVisible(depths, false);
}

void FiltersDockWidget::on_listWidget_currentRowChanged(int index)
{
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

		if (ui->comboBoxFilterPreset->currentIndex() != -1)
		{
			QByteArray filterPreset = itemWidget->_filterPreset.toUtf8();
			BlastProject::ins().removeFilterDepth(filterPreset.data(), itemWidget->_depth);
		}

		ui->listWidget->takeItem(ui->listWidget->row(item));
		ui->listWidget->removeItemWidget(item);
		delete item;
		delete itemWidget;
		_filterUIItems.erase(toRemoveItem);

		//_updateFilterItemList();
		_updateFilterDepthBtns();
	}
}

void FiltersDockWidget::_updateFilterItemList()
{
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

	int filterPresetIndex = ui->comboBoxFilterPreset->currentIndex();
	if (filterPresetIndex < 0)
		return;

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPFilterPresetArray& filterPresetArray = projectParams.filter.filters;
	BPPFilterPreset& filterPresset = filterPresetArray.buf[filterPresetIndex];

	int filterCount = filterPresset.depthFilters.arraySizes[0];

	for (int i = 0; i < filterCount; ++i)
	{
		QListWidgetItem* item = new QListWidgetItem(NULL);
		int depth = filterPresset.depthFilters.buf[i];
		FilterItemWidget* itemWidget = new FilterItemWidget(this, item, filterPresset.name.buf, depth);

		QString depthFilterLabel = QString(QObject::tr("Depth-%1")).arg(depth);
		_filterUIItems.insert(std::make_pair(item, new FilterItemInfo(itemWidget, depthFilterLabel)));
		itemWidget->setText(depthFilterLabel);
		_filterItemWidgets.push_back(itemWidget);

		ui->listWidget->addItem(item);
		ui->listWidget->setItemWidget(item, itemWidget);
	}
}

void FiltersDockWidget::_updateFilterDepthBtns()
{
	for (int i = 0; i <= 5; ++i)
		_depthButtons[i]->setChecked(false);

	int filterPresetIndex = ui->comboBoxFilterPreset->currentIndex();
	if (filterPresetIndex < 0)
		return;

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPFilterPresetArray& filterPresetArray = projectParams.filter.filters;
	BPPFilterPreset& filterPresset = filterPresetArray.buf[filterPresetIndex];

	int filterCount = filterPresset.depthFilters.arraySizes[0];

	for (int i = 0; i < filterCount; ++i)
	{
		int depth = filterPresset.depthFilters.buf[i];

		_depthButtons[depth]->setChecked(true);
	}
}

void FiltersDockWidget::_addRemoveDepthFilter(int depth, bool add)
{
	if (ui->comboBoxFilterPreset->currentIndex() != -1)
	{
		QByteArray filterPreset = ui->comboBoxFilterPreset->currentText().toUtf8();

		if (add)
			BlastProject::ins().addFilterDepth(filterPreset.data(), depth);
		else
			BlastProject::ins().removeFilterDepth(filterPreset.data(), depth);

		_updateFilterItemList();
	}
}
