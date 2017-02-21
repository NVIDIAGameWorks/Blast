#include "DisplayMeshesPanel.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>

#include "AppMainWindow.h"

#include "SimpleScene.h"

#ifndef NV_ARTISTTOOLS
#include "FurCharacter.h"
#else
#endif // NV_ARTISTTOOLS

//////////////////////////////////////////////////////////////////////////
// DisplayMeshItem

StateViewItem::StateViewItem( QWidget* parent, unsigned int id, MeshViewState view /*= VS_VISIABLE*/ )
	: _id(id)
{
	static QIcon iconVisible(":/AppMainWindow/images/visibilityToggle_visible.png");
	static QIcon iconNotVisible(":/AppMainWindow/images/visibilityToggle_notVisible.png");

	_parent = qobject_cast<DisplayMeshesPanel*>(parent);

	_layout = new QHBoxLayout(this);
	_layout->setObjectName(QString::fromUtf8("boxLayout"));
	_layout->setMargin(0);

	this->setLayout(_layout);
	
	_btn = new QPushButton(this);
	
	_btn->setObjectName(QString::fromUtf8("btnToggleView"));
	QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Fixed);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(_btn->sizePolicy().hasHeightForWidth());
	_btn->setSizePolicy(sizePolicy1);
	_btn->setMinimumSize(QSize(16, 16));
	_btn->setMaximumSize(QSize(16, 16));
	_btn->setAutoFillBackground(false);
	_btn->setCheckable(true);
	_btn->setChecked(view == VS_VISIABLE);
	_btn->setIcon( (view == VS_VISIABLE) ? iconVisible : iconNotVisible);

	_layout->addWidget(_btn);

	_spacer = new QSpacerItem(20, 40, QSizePolicy::Expanding, QSizePolicy::Ignored);
	_layout->addItem(_spacer);

	QMetaObject::connectSlotsByName(this);
}

void StateViewItem::on_btnToggleView_clicked()
{
	static QIcon iconVisible(":/AppMainWindow/images/visibilityToggle_visible.png");
	static QIcon iconNotVisible(":/AppMainWindow/images/visibilityToggle_notVisible.png");

	bool bVis = _btn->isChecked();
	_btn->setIcon(bVis ? iconVisible : iconNotVisible);

	_parent->EmitToggleSignal(_id, bVis);

	qDebug("%s", __FUNCTION__);
}


//////////////////////////////////////////////////////////////////////////
DisplayMeshesPanel::DisplayMeshesPanel( QWidget* parent )
	:QFrame(parent)
{
	_layout = new QGridLayout(this);
	_layout->setObjectName(QString::fromUtf8("gridLayout"));

	this->setLayout(_layout);

	//QString styleSheet = 
	//	"QPushButton#btnToggleView:checked {border:2px solid gray; background:rgb(118,180,0);} \n"	\
	//	"QPushButton#btnToggleView:pressed {border:2px solid gray; background:rgb(118,180,0);}";
	//setStyleSheet(styleSheet);
}

/////////////////////////////////////////////////////////////////////////////////////////
void DisplayMeshesPanel::updateValues()
{
	ClearItems();

#ifndef NV_ARTISTTOOLS
	FurCharacter& character = SimpleScene::Inst()->GetFurCharacter();
	int nMesh = character.GetMeshCount();
	for (int i = 0; i < nMesh; ++i)
	{
		bool used = character.GetMeshUsed(i);
		if (!used)
			continue;

		const char* name = character.GetMeshName(i);
		bool visible = character.GetMeshVisible(i);

		StateViewItem::MeshViewState visState = visible ? StateViewItem::VS_VISIABLE : StateViewItem::VS_INVISIBLE;

		AddMeshItem(QString(name), i, visState);
	}
#else
	CoreLib::Inst()->DisplayMeshesPanel_updateValues();
#endif // NV_ARTISTTOOLS
}

/////////////////////////////////////////////////////////////////////////////////////////
void DisplayMeshesPanel::AddMeshItem( QString name, unsigned int id, StateViewItem::MeshViewState view )
{
	StateViewItem* item;
	item = new StateViewItem(this, id, view);

	QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);

	QLabel* label = new QLabel(name, this);
	label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
	label->setSizePolicy(sizePolicy1);

	int row = _items.size();
	_layout->addWidget(label, row, 0, 1, 1);

	_layout->addWidget(item, row, 1, 1, 1);
	
	Q_ASSERT(_items.find(id) == _items.end());

	ItemUI ui;
	ui.label = label;
	ui.viewItem = item;
	_items[id] = ui;
}

/////////////////////////////////////////////////////////////////////////////////////////
void DisplayMeshesPanel::EmitToggleSignal( unsigned int id, bool visible )
{
	emit MeshViewSignal(id, visible);

#ifndef NV_ARTISTTOOLS
	FurCharacter& character = SimpleScene::Inst()->GetFurCharacter();
	character.SetMeshVisible(id, visible);
#else
	CoreLib::Inst()->DisplayMeshesPanel_EmitToggleSignal(id, visible);
#endif // NV_ARTISTTOOLS
}

/////////////////////////////////////////////////////////////////////////////////////////
void DisplayMeshesPanel::RemoveMeshItem( unsigned int id )
{
	if(_items.find(id) != _items.end())
	{
		ItemUI ui = _items[id];
		delete ui.label;
		delete ui.viewItem;
		_items.remove(id);
	}
	else
	{
		Q_ASSERT("Mesh item doesn't exist!!");
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
void DisplayMeshesPanel::ClearItems()
{
	Q_FOREACH(ItemUI ui, _items)
	{
		delete ui.label;
		delete ui.viewItem;
	}

	_items.clear();
}
