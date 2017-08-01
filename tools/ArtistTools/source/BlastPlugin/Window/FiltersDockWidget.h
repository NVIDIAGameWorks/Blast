#ifndef FILTERSDOCKWIDGET_H
#define FILTERSDOCKWIDGET_H

#include <QtWidgets/QDockWidget>
#include <vector>
#include <map>
#include "ProjectParams.h"
using namespace std;

namespace Ui {
class FiltersDockWidget;
}

class QLabel;
class QPushButton;
class QListWidgetItem;
class QAction;
struct FilterItemInfo;
class FilterItemWidget;
class FiltersDockWidget;

class FilterItemWidget : public QWidget
{
	Q_OBJECT
public:
	FilterItemWidget(FiltersDockWidget* parent, QListWidgetItem* item, const QString& filterPreset, EFilterRestriction restriction);
	~FilterItemWidget()
	{
	}

	void setText(const QString& title);
	void select();
	void deSelect();

signals:
	void RemoveItem(QListWidgetItem* item);

private slots:
	void onRemoveButtonClicked();

public:
	QLabel* _label;
	QPushButton* _removeBtn;
	QListWidgetItem* _relatedListWidgetItem;
	QString	_filterPreset;
	EFilterRestriction	_restriction;
};

class FiltersDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit FiltersDockWidget(QWidget *parent = 0);
    ~FiltersDockWidget();
	void updateValues();

private slots:
	void on_comboBoxFilterPreset_currentIndexChanged(int index);
	void on_btnAddFilterPrest_clicked();
	void on_btnModifyFilterPreset_clicked();
	void on_btnRemoveFilterPreset_clicked();
	void on_btnSaveFilterPreset_clicked();
	void on_actionAllDescendants_triggered(bool checked);
	void on_actionAllParents_triggered(bool checked);
	void on_actionDepthAll_triggered(bool checked);
	void on_actionDepth0_triggered(bool checked);
	void on_actionDepth1_triggered(bool checked);
	void on_actionDepth2_triggered(bool checked);
	void on_actionDepth3_triggered(bool checked);
	void on_actionDepth4_triggered(bool checked);
	void on_actionDepth5_triggered(bool checked);
	void on_actionItemTypeAll_triggered(bool checked);
	void on_actionChunk_triggered(bool checked);
	void on_actionSupportChunk_triggered(bool checked);
	void on_actionStaticSupportChunk_triggered(bool checked);
	void on_actionBond_triggered(bool checked);
	void on_actionWorldBond_triggered(bool checked);
	void on_actionEqualTo_triggered(bool checked);
	void on_actionNotEquaTo_triggered(bool checked);
	void on_btnSelect_clicked();
	void on_btnVisible_clicked();
	void on_btnInVisible_clicked();
	void on_listWidget_currentRowChanged(int index);
	void onListWidgetRemoveBtnClicked(QListWidgetItem* item);

private:
	void _updateFilterItemList();
	void _addFilterListItem(const char* filterPresetName, EFilterRestriction restriction);
	void _updateFilterUIs();
	void _addRemoveRestriction(EFilterRestriction restriction, bool add);

private:
    Ui::FiltersDockWidget *ui;
	bool									_updateData;
	map<QListWidgetItem*, FilterItemInfo*>	_filterUIItems;
	vector<FilterItemWidget*>				_filterItemWidgets;
	int										_lastSelectRow;
	map<EFilterRestriction, QAction*>		_restrictionActionMap;
};

#endif // FILTERSDOCKWIDGET_H
