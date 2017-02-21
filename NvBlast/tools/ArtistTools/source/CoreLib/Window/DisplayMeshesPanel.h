#ifndef DisplayMeshesPanel_h__
#define DisplayMeshesPanel_h__

#include <QtWidgets/QFrame>
#include <QtCore/QMap>
#ifndef NV_ARTISTTOOLS
#include "ui_LodPanel.h"
#else
#include <QtWidgets/QLabel>
#endif // NV_ARTISTTOOLS

#include "corelib_global.h"

class QPushButton;
class QGridLayout;
class QHBoxLayout;
class QSpacerItem;

class DisplayMeshesPanel;
class StateViewItem : public QWidget
{
	Q_OBJECT

public:
	enum MeshViewState
	{
		VS_INVISIBLE = 0,	
		VS_VISIABLE	 = 1,
	};

	StateViewItem(QWidget* parent, unsigned int id, MeshViewState view = VS_VISIABLE);

	public slots:
	CORELIB_EXPORT void on_btnToggleView_clicked();

private:
	unsigned int _id;

	QHBoxLayout* _layout;
	QPushButton* _btn;
	QSpacerItem* _spacer;

	DisplayMeshesPanel* _parent;
};

class DisplayMeshesPanel : public QFrame
{
	Q_OBJECT

public:
	DisplayMeshesPanel(QWidget* parent);

	void updateValues();
	
	// client code should make sure the 'id' is unique!
	CORELIB_EXPORT void AddMeshItem(QString name, unsigned int id, StateViewItem::MeshViewState view = StateViewItem::VS_VISIABLE);
	CORELIB_EXPORT void RemoveMeshItem(unsigned int id);
	CORELIB_EXPORT void ClearItems();

signals:
	void MeshViewSignal(unsigned int id, bool visible);
	
protected:
	virtual void EmitToggleSignal(unsigned int id, bool visible);

private:
	QGridLayout* _layout;

	struct ItemUI
	{
		StateViewItem* viewItem;
		QLabel*	label;
	};

	QMap<unsigned int, ItemUI> _items;

	friend class StateViewItem;
};

#endif // GraphicalMaterialPanel_h__
