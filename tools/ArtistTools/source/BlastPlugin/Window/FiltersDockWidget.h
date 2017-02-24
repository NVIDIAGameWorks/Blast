#ifndef FILTERSDOCKWIDGET_H
#define FILTERSDOCKWIDGET_H

#include <QtWidgets/QDockWidget>
#include <vector>
#include <map>
using namespace std;

namespace Ui {
class FiltersDockWidget;
}

class QLabel;
class QPushButton;
class QListWidgetItem;
struct FilterItemInfo;
class FilterItemWidget;
class FiltersDockWidget;

class FilterItemWidget : public QWidget
{
	Q_OBJECT
public:
	FilterItemWidget(FiltersDockWidget* parent, QListWidgetItem* item, const QString& filterPreset, int depth);
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
	int	_depth;
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

    void on_btnDepth0_clicked(bool val);

	void on_btnDepth1_clicked(bool val);

	void on_btnDepth2_clicked(bool val);

	void on_btnDepth3_clicked(bool val);

	void on_btnDepth4_clicked(bool val);

	void on_btnDepth5_clicked(bool val);

    void on_btnAddDepthFilter_clicked();

    void on_btnAddFilter_clicked();

    void on_btnSelect_clicked();

    void on_btnVisible_clicked();

    void on_btnInVisible_clicked();

	void on_listWidget_currentRowChanged(int index);

	void onListWidgetRemoveBtnClicked(QListWidgetItem* item);

private:
	void _updateFilterItemList();
	void _updateFilterDepthBtns();
	void _addRemoveDepthFilter(int depth, bool add);

private:
    Ui::FiltersDockWidget *ui;
	map<QListWidgetItem*, FilterItemInfo*>	_filterUIItems;
	vector<FilterItemWidget*>				_filterItemWidgets;
	int										_lastSelectRow;
	vector<QPushButton*>					_depthButtons;
};

#endif // FILTERSDOCKWIDGET_H
