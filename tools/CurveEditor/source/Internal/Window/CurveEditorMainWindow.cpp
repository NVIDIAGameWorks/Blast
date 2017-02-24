#include "CurveEditorMainWindow.h"
#include "ui_CurveEditorMainWindow.h"
#include <QtWidgets/QSplitter>
#include <QtWidgets/QColorDialog>
#include "CurveWidget.h"
#include "ColorWidget.h"
#include "AlphaDialog.h"
#include <QtCore/QDebug>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <float.h>

namespace nvidia {
namespace CurveEditor {

// each value is associated with the tab index of tab control of attributes
enum AttributesTabIndex
{
    eCurveTab = 0,
    eColorTab = 1,
};

/////////////////////////////////////////////////////////////////////////////////////
QString InterpolateModeToString(InterpolateMode mode)
{
    switch(mode)
    {
        case eDiscret:
            return "Discret";
        case eLinear:
            return "Linear";
        case eBezierSpline:
            return "BezierSpline";
        case eCatmullRomSpline:
            return "CatmullRomSpline";
        default:
            return "Discret";
    }
}

/////////////////////////////////////////////////////////////////////////////////////
InterpolateMode StringToInterpolateMode(const QString& mode)
{
    if (mode == "Discret")
    {
        return eDiscret;
    }
    else if (mode == "Linear")
    {
        return eLinear;
    }
    else if (mode == "BezierSpline")
    {
        return eBezierSpline;
    }
    else if (mode == "CatmullRomSpline")
    {
        return eCatmullRomSpline;
    }
    else
    {
        return eDiscret;
    }
}

QString OpenTextureFile( QString lastDir, QString title = "")
{
    QString titleStr = "Open Texture File";
    if(!title.isEmpty())
        titleStr = title;

    QString fileName = QFileDialog::getOpenFileName(nullptr, titleStr, lastDir, "Images (*.dds *.png *.bmp *.jpg *.tga)");

    return fileName;
}

/////////////////////////////////////////////////////////////////////////////////////
void setTextureButtons(QPushButton *pTex, QPushButton *pReload, QPushButton *pClear)
{
    pTex->setIcon(QIcon(":/AppMainWindow/Icon/TextureEnabled_icon.png"));
    pReload->setIcon(QIcon(":/AppMainWindow/Icon/Refresh_icon.png"));
    pClear->setIcon(QIcon(":/AppMainWindow/Icon/Remove_icon.png"));

    pTex->setIconSize(QSize(12,12));
    pReload->setIconSize(QSize(12,12));
    pClear->setIconSize(QSize(12,12));
}

//////////////////////////////////////////////////////////
QString addStar(QString text, bool add)
{
    QByteArray ba = text.toUtf8();

    const char* in = ba.data();
    char out[1024];

    int i = 0;
    for (i = 0; i < strlen(in); i++)
    {
        if (in[i] == '*')
            break;
        out[i] = in[i];
    }
    out[i] = 0;

    QString newtext;
    if (add)
        newtext = QString((const char*)out) + QString("*");
    else
        newtext = QString((const char*)out) ;
    return newtext;
}

//////////////////////////////////////////////////////////
void setFocusColor(QWidget* qWidget, bool sameForAllAttributes)
{
    if (!qWidget)
        return;

    QString sameStyle = QString("font: ; color: rgb(150,150,150);") ;
    QString differentStyle = QString("font: bold; color: rgb(255,55,55);");
    QString style = (sameForAllAttributes) ? sameStyle : differentStyle;

    qWidget->setStyleSheet(style);

    QLabel* label = dynamic_cast<QLabel*>(qWidget);
    if (label)
    {
        QString newtext = addStar(label->text(), !sameForAllAttributes);

        label->setFrameStyle(0);
        label->setText(newtext);
    }
}

//////////////////////////////////////////////////////////
void pickColor(QColor& color, QWidget* parent)
{
    QColor newColor = QColorDialog::getColor(color, parent);
    if(newColor.isValid())
    {
        color = newColor;
    }
}

//////////////////////////////////////////////////////////
void pickAlpha(int& alpha, QWidget* parent)
{
    alpha = AlphaDialog::getAlpha(alpha, parent);
}

//////////////////////////////////////////////////////////
void setButtonColor(QPushButton *button, const QColor& color)
{
    QString specBtnStyle = QString("background-color: rgb(%1,%2,%3);")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue());

    button->setStyleSheet(specBtnStyle);
}

//////////////////////////////////////////////////////////
void setButtonTex(QPushButton *button, bool used)
{
    if (used)
        button->setIcon(QIcon(":/AppMainWindow/Icon/TextureIsUsed_icon.png"));
    else
        button->setIcon(QIcon(":/AppMainWindow/Icon/TextureEnabled_icon.png"));
}

CurveEditorMainWindow::CurveEditorMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CurveEditorMainWindow)
    , _curveWidget(nullptr)
    , _colorWidget(nullptr)
    , _curveAttributeCache(nullptr)
    , _colorAttributeCache(nullptr)
    , _updateUIFromData(false)
    , _canMoveCurveControlPointHorizontally(true)
    , _canAddRemoveCurveControlPoint(true)
    , _canChangeCurveTangentType(true)
{
    Q_INIT_RESOURCE(CurveEditor);

    ui->setupUi(this);

    setTextureButtons(ui->btnColorTex, ui->btnColorReload, ui->btnColorClear);
    setTextureButtons(ui->btnAlphaTex, ui->btnAlphaReload, ui->btnAlphaClear);

    ui->centralWidget->setStretchFactor(0, 30);
    ui->centralWidget->setStretchFactor(1, 70);

    {
        QActionGroup* groupSnap = new QActionGroup(this);
        groupSnap->addAction(ui->actionSnap_All);
        groupSnap->addAction(ui->actionSnap_Horizontal);
        groupSnap->addAction(ui->actionSnap_Vertical);
    }

    {
        QActionGroup* groupAddRemoveCtrlPnt = new QActionGroup(this);
        groupAddRemoveCtrlPnt->addAction(ui->actionAdd_Control_Point_By_Click);
        groupAddRemoveCtrlPnt->addAction(ui->actionRemove_Control_Point_By_Click);
    }

    _fillCurveAttributesTree();

    _fillColorAttributesTree();

    {
        _curveWidget = new CurveWidget(ui->frameCurveEditorArea);
        ui->layoutCurveEditorArea->addWidget(_curveWidget);
        connect(_curveWidget, SIGNAL(PickedControlPointChanged(const std::vector<CurveEntity*>)), this, SLOT(onCurvePickedControlPointChanged(const std::vector<CurveEntity*>)));
        connect(_curveWidget, SIGNAL(PickedControlPointValueChanged(QPointF&)), this, SLOT(onCurvePickedControlPointValueChanged(QPointF&)));
    }

    {
        _colorWidget = new ColorWidget(ui->frameColorAlphaRamp);
        ui->layoutColorEditorArea->addWidget(_colorWidget);
        connect(_colorWidget, SIGNAL(PickedControlPointChanged(bool)), this, SLOT(onColorPickedControlPointChanged(bool)));

        QColor color = _colorWidget->getColor();
        setButtonColor(ui->btnColor, color);
        setButtonColor(ui->btnAlpha, QColor(color.alpha(), color.alpha(), color.alpha()));
    }

    connect(_curveWidget, SIGNAL(CurveAttributeChanged(nvidia::CurveEditor::CurveAttribute*)), this, SIGNAL(CurveAttributeChanged(nvidia::CurveEditor::CurveAttribute*)));
    connect(_colorWidget, SIGNAL(ColorAttributeChanged(nvidia::CurveEditor::ColorAttribute*)), this, SIGNAL(ColorAttributeChanged(nvidia::CurveEditor::ColorAttribute*)));
    connect(_colorWidget, SIGNAL(ReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute*, bool, int)), this, SIGNAL(ReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute*, bool, int)));

    QString defFilePath;

    QString appDir = qApp->applicationDirPath();
    QDir dir(appDir);
    if (dir.cd("../../media"))
        defFilePath = dir.absolutePath();

    _lastFilePath = defFilePath;
    _presetPath = _lastFilePath + "/Presets/";
}

CurveEditorMainWindow::~CurveEditorMainWindow()
{
    delete ui;
    ui = nullptr;
}

void CurveEditorMainWindow::setCurveAttributes(const std::vector<CurveAttributeBase*>& attributes)
{
    _curveWidget->setCurveAttributes(std::vector<CurveAttributeBase*>());

    _curveAttributes = attributes;
    _fillCurveAttributesTree();

    _canMoveCurveControlPointHorizontally = false;
    _canAddRemoveCurveControlPoint = false;
     _canChangeCurveTangentType = false;
    size_t countAttributes = _curveAttributes.size();
    for (size_t i = 0; i < countAttributes; ++i)
    {
        CurveAttributeBase* attribute = _curveAttributes[i];

        if (attribute->getType() == eGroupAttr)
        {
            CurveAttributeGroup* attributeGroup = static_cast<CurveAttributeGroup*>(attribute);
            size_t countAttributesInGroup = attributeGroup->attributes.size();
            for (size_t j = 0; j < countAttributesInGroup; ++j)
            {
                CurveAttribute* attributeInGroup = static_cast<CurveAttribute*>(attributeGroup->attributes[j]);
                if (attributeInGroup->canMoveControlPointHorizontally())
                    _canMoveCurveControlPointHorizontally = true;
                if (attributeInGroup->canAddRemoveControlPoint())
                    _canAddRemoveCurveControlPoint = true;
                if (attributeInGroup->canChangeTangentType())
                    _canChangeCurveTangentType = true;
            }
        }
        else
        {
            CurveAttribute* attributeSecific = static_cast<CurveAttribute*>(attribute);
            if (attributeSecific->canMoveControlPointHorizontally())
                _canMoveCurveControlPointHorizontally = true;
            if (attributeSecific->canAddRemoveControlPoint())
                _canAddRemoveCurveControlPoint = true;
            if (attributeSecific->canChangeTangentType())
                _canChangeCurveTangentType = true;
        }
    }

    ui->spinBoxLocation->setEnabled(_canMoveCurveControlPointHorizontally);
    _syncUIStatusWithSelectedAttribute(_canAddRemoveCurveControlPoint, _canChangeCurveTangentType);
}

void CurveEditorMainWindow::setColorCurveAttributes(const std::vector<ColorAttribute*>& attributes)
{
    _colorWidget->setColorAttribute(nullptr);

    _colorAttributes = attributes;
    _fillColorAttributesTree();
}

void CurveEditorMainWindow::setSelectedCurveAttributes(const std::vector<CurveAttributeBase*>& attributes)
{
    ui->tabWidgetAttributes->setCurrentIndex(eCurveTab);

    for (size_t i = 0; i < attributes.size(); ++i)
    {
        CurveAttributeBase* attribute = attributes[i];
        QList<QTreeWidgetItem*> items = ui->treeWidgetCurveAttributes->findItems(attribute->getName().c_str(), Qt::MatchExactly);
        if (items.size() > 0)
            items[0]->setSelected(true);
    }
}

void CurveEditorMainWindow::setSelectedColorAttribute(const ColorAttribute* attribute)
{
    ui->tabWidgetAttributes->setCurrentIndex(eColorTab);

    if (attribute)
    {
        QList<QTreeWidgetItem*> items = ui->treeWidgetColorAttributes->findItems(attribute->getName().c_str(), Qt::MatchExactly);
        if (items.size() > 0)
            items[0]->setSelected(true);
    }
}

void CurveEditorMainWindow::setResampleEnabled(bool enable)
{
    ui->checkBoxResamplePoints->setVisible(enable);
    ui->spinBoxResamplePoints->setVisible(enable);
    ui->checkBoxResamplePoints->setEnabled(enable);
    ui->spinBoxResamplePoints->setEnabled(enable);
}

void CurveEditorMainWindow::on_actionCopy_triggered()
{
    if (eCurveTab == ui->tabWidgetAttributes->currentIndex())
    {
        QList<QTreeWidgetItem*> items = ui->treeWidgetCurveAttributes->selectedItems();
        if (1 == items.count())
        {
            CurveAttributeTreeItem* item = static_cast<CurveAttributeTreeItem*>(items.at(0));

            if (eSingleAttr == item->_attribute->getType())
            {
                if (_curveAttributeCache == nullptr)
                    _curveAttributeCache = new CurveAttribute("curveAttributeCache");

                CurveAttribute* selectedAttribute = static_cast<CurveAttribute*>(item->_attribute);
                _curveAttributeCache->curve =  selectedAttribute->curve;
                return ;
            }
        }

        QMessageBox::warning(this, tr("Warning"), tr("You should select only one attribute to copy attribute data."));
    }
    else if (eColorTab == ui->tabWidgetAttributes->currentIndex())
    {
        QList<QTreeWidgetItem*> items = ui->treeWidgetColorAttributes->selectedItems();
        if (1 == items.count())
        {
            ColorAttributeTreeItem* item = static_cast<ColorAttributeTreeItem*>(items.at(0));

            if (eColorAttr == item->_attribute->getType())
            {
                if (_colorAttributeCache == nullptr)
                    _colorAttributeCache = new ColorAttribute("colorAttributeCache");

                ColorAttribute* selectedAttribute = static_cast<ColorAttribute*>(item->_attribute);
                _colorAttributeCache->colorCurve =  selectedAttribute->colorCurve;
                _colorAttributeCache->alphaCurve =  selectedAttribute->alphaCurve;
                return ;
            }
        }
    }
}

void CurveEditorMainWindow::on_actionPaste_triggered()
{
    if (eCurveTab == ui->tabWidgetAttributes->currentIndex())
    {
        if (nullptr == _curveAttributeCache)
        {
            QMessageBox::warning(this, tr("Warning"), tr("You should copy or load an curve attribute's data first."));
            return ;
        }

        QList<QTreeWidgetItem*> items = ui->treeWidgetCurveAttributes->selectedItems();
        if (1 == items.count())
        {
            CurveAttributeTreeItem* item = static_cast<CurveAttributeTreeItem*>(items.at(0));

            if (eSingleAttr == item->_attribute->getType())
            {
                CurveAttribute* selectedAttribute = static_cast<CurveAttribute*>(item->_attribute);
                bool compatible = true;
                if (compatible)
                {
                    selectedAttribute->curve = _curveAttributeCache->curve;
                    on_treeWidgetCurveAttributes_itemSelectionChanged();
                    CurveAttributeChanged(selectedAttribute);
                }
                else
                {
                    QMessageBox::warning(this, tr("Warning"), tr("The data types between the curve attribute copyed from and the curve attribute copy to are incompatible ."));
                }
                return ;
            }
        }

        QMessageBox::warning(this, tr("Warning"), tr("You should select only one curve attribute to paste curve attribute data."));
    }
    else if (eColorTab == ui->tabWidgetAttributes->currentIndex())
    {
        if (nullptr == _colorAttributeCache)
        {
            QMessageBox::warning(this, tr("Warning"), tr("You should copy or load an color attribute's data first."));
            return ;
        }

        QList<QTreeWidgetItem*> items = ui->treeWidgetColorAttributes->selectedItems();
        if (1 == items.count())
        {
            ColorAttributeTreeItem* item = static_cast<ColorAttributeTreeItem*>(items.at(0));

            if (eColorAttr == item->_attribute->getType())
            {
                ColorAttribute* selectedAttribute = static_cast<ColorAttribute*>(item->_attribute);
                bool compatible = true;
                if (compatible)
                {
                    selectedAttribute->colorCurve = _colorAttributeCache->colorCurve;
                    selectedAttribute->alphaCurve = _colorAttributeCache->alphaCurve;
                    on_treeWidgetColorAttributes_itemSelectionChanged();
                    ColorAttributeChanged(selectedAttribute);
                }
                else
                {
                    QMessageBox::warning(this, tr("Warning"), tr("The data types between copy from attribute and copy to attribute are incompatible ."));
                }
                return ;
            }
        }

        QMessageBox::warning(this, tr("Warning"), tr("You should select only one attribute to paste attribute data."));
    }
}

void CurveEditorMainWindow::on_actionSave_Selected_as_Preset_triggered()
{
    QDir dir(_presetPath);
    if (!dir.exists())
        dir.mkdir(_presetPath);

    CurveAttribute* curveAttribute = nullptr;
    ColorAttribute* colorAttribute = nullptr;
    QString presetFile = _presetPath;

    if (eCurveTab == ui->tabWidgetAttributes->currentIndex())
    {
        QList<QTreeWidgetItem*> items = ui->treeWidgetCurveAttributes->selectedItems();
        if (1 != items.count())
        {
            QMessageBox::warning(this, tr("Warning"), tr("You should select one attribute only to save preset."));
            return ;
        }

        CurveAttributeTreeItem* item = static_cast<CurveAttributeTreeItem*>(items.at(0));

        if (eSingleAttr != item->_attribute->getType())
        {
            return;
        }

        curveAttribute = static_cast<CurveAttribute*>(item->_attribute);
        presetFile += curveAttribute->getName().c_str();
        presetFile += ".cps";
    }
    else if (eColorTab == ui->tabWidgetAttributes->currentIndex())
    {
        QList<QTreeWidgetItem*> items = ui->treeWidgetColorAttributes->selectedItems();
        if (1 != items.count())
        {
            QMessageBox::warning(this, tr("Warning"), tr("You should select one attribute only to save preset."));
            return ;
        }

        ColorAttributeTreeItem* item = static_cast<ColorAttributeTreeItem*>(items.at(0));

        if (eColorAttr != item->_attribute->getType())
        {
            return;
        }

        colorAttribute = item->_attribute;
        presetFile += colorAttribute->getName().c_str();
        presetFile += ".cps";
    }

    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Curve PreSet"), presetFile, tr("Curve PreSet(*.cps)"));
    if (filePath.length() == 0)
    {
        QMessageBox::warning(this, tr("Path"), tr("You didn't select any files."));
        return ;
    }

    QFileInfo fileInfo(filePath);
    _presetPath = fileInfo.absoluteDir().absolutePath();
    if (!_presetPath.endsWith("/"))
        _presetPath += "/";

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        return;
    }
    QTextStream out(&file);

    QDomDocument    xmlDoc;
    QDomElement rootElm = xmlDoc.createElement(tr("CurvePreSet"));
    xmlDoc.appendChild(rootElm);

    if (curveAttribute != nullptr)
    {
        _saveAttribute(rootElm, curveAttribute);
    }
    else if (colorAttribute != nullptr)
    {
        _saveAttribute(rootElm, colorAttribute);
    }
    // 4 is count of indent
    xmlDoc.save(out, 4);
}

void CurveEditorMainWindow::on_actionLoad_Preset_to_Copy_Buffer_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Curve PreSet"), _presetPath, tr("Curve PreSet(*.cps)"));
    if (filePath.length() == 0)
    {
        QMessageBox::warning(this, tr("Path"), tr("You didn't select any files."));
        return;
    }

    QFileInfo fileInfo(filePath);
    _presetPath = fileInfo.absoluteDir().absolutePath();
    if (!_presetPath.endsWith("/"))
        _presetPath += "/";

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        return;
    }

    QDomDocument    xmlDoc;
    if (!xmlDoc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();

    if (xmlDoc.isNull() || xmlDoc.documentElement().tagName() != tr("CurvePreSet"))
    {
        QMessageBox::warning(this, tr("Warning"), tr("The file you selected is empty or not a cps file."));
        return;
    }

    QDomElement elm = xmlDoc.documentElement().firstChildElement();
    CurveAttributeBase* presetAttribute = _loadAttribute(elm);
    if (nullptr == presetAttribute)
    {
        QMessageBox::warning(this, tr("Warning"), tr("Faid to load the cps file!"));
        return;
    }

    if (eSingleAttr == presetAttribute->_type)
    {
        if (_curveAttributeCache == nullptr)
            _curveAttributeCache = new CurveAttribute("curveAttributeCache");

        CurveAttribute* presetCurveAttribute = static_cast<CurveAttribute*>(presetAttribute);
        _curveAttributeCache->curve = presetCurveAttribute->curve;
    }
    else if (eColorAttr == presetAttribute->_type)
    {
        if (_colorAttributeCache == nullptr)
            _colorAttributeCache = new ColorAttribute("colorAttributeCache");

        ColorAttribute* presetColorAttribute = static_cast<ColorAttribute*>(presetAttribute);
        _colorAttributeCache->colorCurve = presetColorAttribute->colorCurve;
        _colorAttributeCache->alphaCurve = presetColorAttribute->alphaCurve;
    }
}

void CurveEditorMainWindow::on_actionImport_Preset_onto_Selected_triggered()
{
    CurveAttribute* curveAttribute = nullptr;
    ColorAttribute* colorAttribute = nullptr;
    if (eCurveTab == ui->tabWidgetAttributes->currentIndex())
    {
        QList<QTreeWidgetItem*> items = ui->treeWidgetCurveAttributes->selectedItems();
        if (1 != items.count())
        {
            QMessageBox::warning(this, tr("Warning"), tr("You should select only one attribute to import preset."));
            return;
        }

        CurveAttributeTreeItem* item = static_cast<CurveAttributeTreeItem*>(items.at(0));

        if (eSingleAttr != item->_attribute->getType())
        {
            return;
        }

        curveAttribute = static_cast<CurveAttribute*>(item->_attribute);
    }
    else if (eColorTab == ui->tabWidgetAttributes->currentIndex())
    {
        QList<QTreeWidgetItem*> items = ui->treeWidgetColorAttributes->selectedItems();
        if (1 != items.count())
        {
            QMessageBox::warning(this, tr("Warning"), tr("You should select only one attribute to import preset."));
            return;
        }

        ColorAttributeTreeItem* item = static_cast<ColorAttributeTreeItem*>(items.at(0));

        if (eColorAttr != item->_attribute->getType())
        {
            return;
        }

        colorAttribute = item->_attribute;
    }

    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Curve PreSet"), _presetPath, tr("Curve PreSet(*.cps)"));
    if (filePath.length() == 0)
    {
        QMessageBox::warning(this, tr("Path"), tr("You didn't select any files."));
        return;
    }

    QFileInfo fileInfo(filePath);
    _presetPath = fileInfo.absoluteDir().absolutePath();
    if (!_presetPath.endsWith("/"))
        _presetPath += "/";

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        return;
    }

    QDomDocument    xmlDoc;
    if (!xmlDoc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();

    if (xmlDoc.isNull() || xmlDoc.documentElement().tagName() != tr("CurvePreSet"))
    {
        QMessageBox::warning(this, tr("Warning"), tr("The file you selected is empty or not a cps file."));
        return;
    }

    QDomElement elm = xmlDoc.documentElement().firstChildElement();
    CurveAttributeBase* presetAttribute = _loadAttribute(elm);
    if (nullptr == presetAttribute)
    {
        QMessageBox::warning(this, tr("Warning"), tr("Faid to load the cps file!"));
        return;
    }

    if (eSingleAttr == presetAttribute->_type)
    {
        CurveAttribute* presetCurveAttribute = static_cast<CurveAttribute*>(presetAttribute);

        bool compatible = (curveAttribute != nullptr) ? true:false;
        if (compatible)
        {
            curveAttribute->curve = presetCurveAttribute->curve;
            on_treeWidgetCurveAttributes_itemSelectionChanged();
            CurveAttributeChanged(curveAttribute);
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("The selected attribute is incompatible with preset attribute."));
        }
    }
    else if (eColorAttr == presetAttribute->_type)
    {
        ColorAttribute* presetColorAttribute = static_cast<ColorAttribute*>(presetAttribute);

        bool compatible = (colorAttribute != nullptr) ? true:false;;
        if (compatible)
        {
            colorAttribute->colorCurve = presetColorAttribute->colorCurve;
            colorAttribute->alphaCurve = presetColorAttribute->alphaCurve;
            on_treeWidgetColorAttributes_itemSelectionChanged();
            ColorAttributeChanged(colorAttribute);
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("The selected attribute is incompatible with preset attribute."));
        }
    }
}

void CurveEditorMainWindow::on_actionReset_Curve_triggered()
{
    if (ui->tabWidgetAttributes->currentIndex() == eCurveTab)
    {
        _curveWidget->reset();
    }
    else if (ui->tabWidgetAttributes->currentIndex() == eColorTab)
    {
        _colorWidget->reset();
    }

}

void CurveEditorMainWindow::on_actionAdd_Before_Selected_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->addControlPointsBeforeSelected();

    if (_colorWidget->hasFocus())
        _colorWidget->addControlPointsBeforeSelected();
}

void CurveEditorMainWindow::on_actionAdd_After_Selected_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->addControlPointsAfterSelected();

    if (_colorWidget->hasFocus())
        _colorWidget->addControlPointsAfterSelected();
}

void CurveEditorMainWindow::on_actionRemove_Selected_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->removeSelectedControlPoints();

    if (_colorWidget->hasFocus())
        _colorWidget->removeSelectedControlPoint();
}

void CurveEditorMainWindow::on_actionTangent_Stepped_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->setTangentType(eDiscret);

    if (_colorWidget->hasFocus())
        _colorWidget->setTangentType(eDiscret);
}

void CurveEditorMainWindow::on_actionTangent_Linear_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->setTangentType(eLinear);

    if (_colorWidget->hasFocus())
        _colorWidget->setTangentType(eLinear);
}

void CurveEditorMainWindow::on_actionTangent_Smooth_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->setSmoothTangent();

    if (_colorWidget->hasFocus())
        _colorWidget->setSmoothTangent();
}

void CurveEditorMainWindow::on_actionTangent_Ease_Out_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->setEaseOutTangent();

    if (_colorWidget->hasFocus())
        _colorWidget->setEaseOutTangent();
}

void CurveEditorMainWindow::on_actionTangent_Ease_In_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->setEaseInTangent();

    if (_colorWidget->hasFocus())
        _colorWidget->setEaseInTangent();
}

void CurveEditorMainWindow::on_actionTangent_Spline_triggered()
{
    if (_curveWidget->hasFocus())
        _curveWidget->setTangentType(eCatmullRomSpline);

    if (_colorWidget->hasFocus())
        _colorWidget->setTangentType(eCatmullRomSpline);
}

void CurveEditorMainWindow::on_actionSnap_All_triggered()
{
    _curveWidget->setSnapAll();
}

void CurveEditorMainWindow::on_actionSnap_Horizontal_triggered()
{
    _curveWidget->setSnapHorizontal();
}

void CurveEditorMainWindow::on_actionSnap_Vertical_triggered()
{
    _curveWidget->setSnapVertical();
}

void CurveEditorMainWindow::on_actionContract_Horizontally_triggered()
{
    _curveWidget->decreaseCurveScaleHorizontally();
}

void CurveEditorMainWindow::on_actionExpand_Horizontally_triggered()
{
    _curveWidget->increaseCurveScaleHorizontally();
}

void CurveEditorMainWindow::on_actionContract_Vertically_triggered()
{
    _curveWidget->decreaseCurveScaleVertically();
}

void CurveEditorMainWindow::on_actionExpand_Vertically_triggered()
{
    _curveWidget->increaseCurveScaleVertically();
}

void CurveEditorMainWindow::on_actionFrame_Horizontally_triggered()
{
    _curveWidget->frameCurveScaleHorizontally();
}

void CurveEditorMainWindow::on_actionFrame_Vertically_triggered()
{
    _curveWidget->frameCurveScaleVertically();
}

void CurveEditorMainWindow::on_actionFrame_All_triggered()
{
    _curveWidget->frameCurveScale();
}

void CurveEditorMainWindow::on_spinBoxLocation_valueChanged(double value)
{
    if (!_updateUIFromData)
        _curveWidget->setLocation(value);
}

void CurveEditorMainWindow::on_spinBoxValue_valueChanged(double value)
{
    if (!_updateUIFromData)
        _curveWidget->setValue(value);
}

void CurveEditorMainWindow::on_btnColor_clicked()
{
    QColor color = _colorWidget->getColor();
    pickColor(color, nullptr);
    _colorWidget->setColor(color);
    setButtonColor(ui->btnColor, color);
}

void CurveEditorMainWindow::on_btnColorTex_clicked()
{
    QString filePath = OpenTextureFile(_colorWidget->getColorTex().isEmpty() ? _lastFilePath : _colorWidget->getColorTex());
    _colorWidget->setColorTex(filePath);
    setButtonTex(ui->btnColorTex, !filePath.isEmpty());

    if (!filePath.isEmpty())
    {
        QFileInfo fileInfo(filePath);
        _lastFilePath = fileInfo.absoluteDir().absolutePath();
    }
}

void CurveEditorMainWindow::on_btnColorReload_clicked()
{
    QList<QTreeWidgetItem*> items = ui->treeWidgetColorAttributes->selectedItems();
    if (0 ==items.count())
        return;

    _colorWidget->reloadColorTex();
}

void CurveEditorMainWindow::on_btnColorClear_clicked()
{
    _colorWidget->clearColorTex();
    setButtonTex(ui->btnColorTex, false);
}

void CurveEditorMainWindow::on_btnAlpha_clicked()
{
    int alpha = _colorWidget->getAlpha();
    pickAlpha(alpha, nullptr);
    _colorWidget->setAlpha(alpha);
    setButtonColor(ui->btnAlpha, QColor(alpha, alpha, alpha));
}

void CurveEditorMainWindow::on_btnAlphaTex_clicked()
{
    QString filePath = OpenTextureFile(_colorWidget->getAlphaTex().isEmpty() ? _lastFilePath : _colorWidget->getAlphaTex());
    _colorWidget->setAlphaTex(filePath);
    setButtonTex(ui->btnAlphaTex, !filePath.isEmpty());

    if (!filePath.isEmpty())
    {
        QFileInfo fileInfo(filePath);
        _lastFilePath = fileInfo.absoluteDir().absolutePath();
    }
}

void CurveEditorMainWindow::on_btnAlphaReload_clicked()
{
    QList<QTreeWidgetItem*> items = ui->treeWidgetColorAttributes->selectedItems();
    if (0 ==items.count())
        return;

    _colorWidget->reloadAlphaTex();
}

void CurveEditorMainWindow::on_btnAlphaClear_clicked()
{
    _colorWidget->clearAlphaTex();
    setButtonTex(ui->btnAlphaTex, false);
}

void CurveEditorMainWindow::on_checkBoxUseAlphaChannellFromColor_stateChanged(int val)
{
    _colorWidget->setUseAlphaFromColor(0 != val);
}

void CurveEditorMainWindow::on_treeWidgetCurveAttributes_itemSelectionChanged()
{
    std::vector<CurveAttributeBase*> attributes;

    QList<QTreeWidgetItem*> items = ui->treeWidgetCurveAttributes->selectedItems();
    int count = items.count();
    for (int i = 0; i < items.count(); ++i)
    {
        CurveAttributeTreeItem* item = static_cast<CurveAttributeTreeItem*>(items.at(i));

        attributes.push_back(item->_attribute);

        qDebug()<< item->_attribute->getName().c_str();
    }

    _curveWidget->setCurveAttributes(attributes);

    bool canMoveControlPointHorizontally = _canMoveCurveControlPointHorizontally;
    bool canAddRemoveControlPoint = _canAddRemoveCurveControlPoint;
    bool canChangeTangentType = _canChangeCurveTangentType;
    for (size_t i = 0; i < attributes.size(); ++i)
    {
        CurveAttributeBase* attribute = attributes[i];
        if (eGroupAttr == attribute->getType())
        {
            CurveAttributeGroup* attributeGroup = static_cast<CurveAttributeGroup*>(attribute);
            for (size_t j = 0; j < attributeGroup->attributes.size(); ++j)
            {
                CurveAttributeBase* attributeInGroup = attributeGroup->attributes[j];

                if (attributeInGroup->canMoveControlPointHorizontally())
                    canMoveControlPointHorizontally = true;

                if (attributeInGroup->canAddRemoveControlPoint())
                    canAddRemoveControlPoint = true;

                if (attributeInGroup->canChangeTangentType())
                    canChangeTangentType = true;
            }
        }
        else
        {
            if (attribute->canMoveControlPointHorizontally())
                canMoveControlPointHorizontally = true;

            if (attribute->canAddRemoveControlPoint())
                canAddRemoveControlPoint = attribute->canAddRemoveControlPoint();

            if (attribute->canChangeTangentType())
                canChangeTangentType = true;
        }
    }

    ui->spinBoxLocation->setEnabled(canMoveControlPointHorizontally);
    _syncUIStatusWithSelectedAttribute(canAddRemoveControlPoint, canChangeTangentType);
}

void CurveEditorMainWindow::on_treeWidgetColorAttributes_itemSelectionChanged()
{
    QList<QTreeWidgetItem*> items = ui->treeWidgetColorAttributes->selectedItems();
    if (0 == items.count())
        return;

    ColorAttributeTreeItem* item = static_cast<ColorAttributeTreeItem*>(items.at(0));
    _colorWidget->setColorAttribute(item->_attribute);
    _colorWidget->setCanAddRemoveControlPoint(item->_attribute->canAddRemoveControlPoint());

    _syncUIStatusWithSelectedAttribute(item->_attribute->canAddRemoveControlPoint(), item->_attribute->canChangeTangentType());
}

void CurveEditorMainWindow::on_tabWidgetAttributes_currentChanged(int index)
{
    //if (eCurveTab == index)
    //{
    //    _setCurveExclusiveUIEnable(true);
    //}
    //else if (eColorTab == index)
    //{
    //    _setCurveExclusiveUIEnable(false);
    //}
}

void CurveEditorMainWindow::on_sliderColorFallOff_sliderMoved(int value)
{
    if (!_updateUIFromData)
    {
        _colorWidget->setColorFallOff((float)value/100);

        if (_colorWidget->isLink())
        {
            _updateUIFromData = true;
            ui->sliderAlphaFallOff->setValue(value);
            _updateUIFromData = false;
        }
    }
}

void CurveEditorMainWindow::on_sliderAlphaFallOff_sliderMoved(int value)
{
    if (!_updateUIFromData)
    {
        _colorWidget->setAlphaFallOff((float)value/100);

        if (_colorWidget->isLink())
        {
            _updateUIFromData = true;
            ui->sliderAlphaFallOff->setValue(value);
            _updateUIFromData = false;
        }
    }
}

void CurveEditorMainWindow::on_actionAdd_Control_Point_By_Click_triggered(bool val)
{
    _curveWidget->setAddCtrlPntByClick(val);
    _curveWidget->setRemoveCtrlPntByClick(false);

    _colorWidget->setAddCtrlPntByClick(val);
    _colorWidget->setRemoveCtrlPntByClick(false);
}

void CurveEditorMainWindow::on_actionRemove_Control_Point_By_Click_triggered(bool val)
{
    _curveWidget->setAddCtrlPntByClick(false);
    _curveWidget->setRemoveCtrlPntByClick(val);

    _colorWidget->setAddCtrlPntByClick(false);
    _colorWidget->setRemoveCtrlPntByClick(val);
}

void CurveEditorMainWindow::onCurvePickedControlPointChanged(const std::vector<CurveEntity*>& pickedCurves)
{
    bool enableChangeTangentType = true;
    bool enableAddRemoveCtrlPnts = true;
    bool enableLoactionEditor = true;
    bool locationSameForAllAttributes = true;
    bool valueSameForAllAttributes = true;
    float location = FLT_MIN, value = FLT_MIN;
    _updateUIFromData = true;
    ui->spinBoxLocation->setValue(location);
    ui->spinBoxValue->setValue(value);
    _updateUIFromData = false;
    for(size_t i = 0; i < pickedCurves.size(); ++i)
    {
        CurveEntity* curveEntity = pickedCurves[i];
        std::vector<int>& pickedControlPoints = pickedCurves[i]->getPickedControlPointIndexes();
        const ControlPoint& ctrlPnt = pickedCurves[i]->_curve->getControlPoint(pickedControlPoints[0]);

        if (!curveEntity->_attribute->canMoveControlPointHorizontally())
            enableLoactionEditor = false;

        if (pickedControlPoints.size() > 1)
            enableLoactionEditor = false;

        if (location == FLT_MIN)
        {
            location = ctrlPnt.value.x();
            _updateUIFromData = true;
            ui->spinBoxLocation->setValue(location);
            _updateUIFromData = false;
        }
        else if (location != ctrlPnt.value.x())
            locationSameForAllAttributes = false;

        if (value == FLT_MIN)
        {
            value = ctrlPnt.value.y();
            _updateUIFromData = true;
            ui->spinBoxValue->setValue(value);
            _updateUIFromData = false;
        }
        else if (value != ctrlPnt.value.y())
            valueSameForAllAttributes = false;

        if (!curveEntity->_attribute->canChangeTangentType() && pickedControlPoints.size() != 0)
            enableChangeTangentType = false;

        if (!curveEntity->_attribute->canAddRemoveControlPoint() && pickedControlPoints.size() != 0)
            enableAddRemoveCtrlPnts = false;

    }

    if (pickedCurves.size() > 0)
    {
        ui->spinBoxLocation->setEnabled(enableLoactionEditor);

        setFocusColor(ui->labelLoacation, locationSameForAllAttributes);
        setFocusColor(ui->labelValue, valueSameForAllAttributes);

        _setTangentTypeUIStatus(enableChangeTangentType);
        _setAddRemoveCtrlPntUIStatus(enableAddRemoveCtrlPnts);
    }
    else
    {
        on_treeWidgetCurveAttributes_itemSelectionChanged();
    }

}

void CurveEditorMainWindow::onCurvePickedControlPointValueChanged(QPointF& value)
{
    _updateUIFromData = true;
    ui->spinBoxLocation->setValue(value.x());
    ui->spinBoxValue->setValue(value.y());
    _updateUIFromData = false;

    QList<QTreeWidgetItem*> selectedAttributes = ui->treeWidgetCurveAttributes->selectedItems();
    if (selectedAttributes.count() > 0)
    {

    }
}

void CurveEditorMainWindow::_fillCurveAttributesTree()
{
    ui->treeWidgetCurveAttributes->clear();

    size_t countAttributes = _curveAttributes.size();
    for (size_t i = 0; i < countAttributes; ++i)
    {
        CurveAttributeBase* attribute = _curveAttributes[i];
        CurveAttributeTreeItem* itemFirstLevel = new CurveAttributeTreeItem(ui->treeWidgetCurveAttributes, attribute);
        itemFirstLevel->setText(0, attribute->getName().c_str());

        if (attribute->getType() == eGroupAttr)
        {
            CurveAttributeGroup* attributeGroup = static_cast<CurveAttributeGroup*>(attribute);
            size_t countAttributesInGroup = attributeGroup->attributes.size();
            for (size_t j = 0; j < countAttributesInGroup; ++j)
            {
                CurveAttribute* attributeInGroup = static_cast<CurveAttribute*>(attributeGroup->attributes[j]);
                CurveAttributeTreeItem* itemSecondLevel = new CurveAttributeTreeItem(itemFirstLevel, attributeInGroup);
                itemSecondLevel->setText(0, attributeInGroup->getName().c_str());
                itemSecondLevel->setForeground(0, QBrush(attributeInGroup->color));
            }
        }
        else
        {
            CurveAttribute* attributeSecific = static_cast<CurveAttribute*>(attribute);
            itemFirstLevel->setForeground(0, QBrush(attributeSecific->color));
        }
    }
}

void CurveEditorMainWindow::_fillColorAttributesTree()
{
    ui->treeWidgetColorAttributes->clear();

    size_t countAttributes = _colorAttributes.size();
    for (size_t i = 0; i < countAttributes; ++i)
    {
        ColorAttribute* attribute = _colorAttributes[i];
        ColorAttributeTreeItem* itemFirstLevel = new ColorAttributeTreeItem(ui->treeWidgetColorAttributes, attribute);
        itemFirstLevel->setText(0, attribute->getName().c_str());
    }
}

void CurveEditorMainWindow::_syncUIStatusWithSelectedAttribute(bool canAddRemoveControlPoint, bool canChangeTangentType)
{
    _setAddRemoveCtrlPntUIStatus(canAddRemoveControlPoint);
    _setTangentTypeUIStatus(canChangeTangentType);
}

void CurveEditorMainWindow::_setCurveExclusiveUIEnable(bool enable)
{
    ui->actionSnap_All->setEnabled(enable);
    ui->actionSnap_Horizontal->setEnabled(enable);
    ui->actionSnap_Vertical->setEnabled(enable);
    ui->actionExpand_Horizontally->setEnabled(enable);
    ui->actionExpand_Vertically->setEnabled(enable);
    ui->actionFrame_Horizontally->setEnabled(enable);
    ui->actionFrame_Vertically->setEnabled(enable);
    ui->actionFrame_All->setEnabled(enable);
    ui->actionContract_Horizontally->setEnabled(enable);
    ui->actionContract_Vertically->setEnabled(enable);
}

void CurveEditorMainWindow::_setColorUIEnable(bool enable)
{
    ui->btnColor->setEnabled(enable);
    ui->btnColorTex->setEnabled(enable);
    ui->btnColorReload->setEnabled(enable);
    ui->btnColorClear->setEnabled(enable);
    ui->sliderColorFallOff->setEnabled(enable);
}

void CurveEditorMainWindow::_setAlphaUIEnable(bool enable)
{
    if (_colorWidget->isLink())
        ui->btnAlpha->setEnabled(true);
    else
        ui->btnAlpha->setEnabled(enable);
    ui->btnAlphaTex->setEnabled(enable);
    ui->btnAlphaReload->setEnabled(enable);
    ui->btnAlphaClear->setEnabled(enable);
    ui->sliderAlphaFallOff->setEnabled(enable);
}

void CurveEditorMainWindow::onColorPickedControlPointChanged(bool isColorCtrlPnt)
{
    QColor color = _colorWidget->getColor();
    setButtonColor(ui->btnColor, color);
    int alpha = _colorWidget->getAlpha();
    setButtonColor(ui->btnAlpha, QColor(alpha, alpha, alpha));

    QString colorTex = _colorWidget->getColorTex();
    setButtonTex(ui->btnColorTex, !colorTex.isEmpty());
    QString alphaTex = _colorWidget->getAlphaTex();
    setButtonTex(ui->btnAlphaTex, !alphaTex.isEmpty());

    _updateUIFromData = true;
    ui->sliderColorFallOff->setValue(_colorWidget->getColorFallOff() * 100);
    ui->sliderAlphaFallOff->setValue(_colorWidget->getAlphaFallOff() * 100);
    _setColorUIEnable(isColorCtrlPnt);
    _setAlphaUIEnable(!isColorCtrlPnt);
    _updateUIFromData = false;
}

void CurveEditorMainWindow::_saveAttribute(QDomElement& parentElm, CurveAttribute* attribute)
{
    QDomElement newElm = parentElm.ownerDocument().createElement(tr("Attribute"));
    parentElm.appendChild(newElm);
    newElm.setAttribute(tr("Name"), attribute->getName().c_str());
    newElm.setAttribute(tr("Color"), QString().setNum(attribute->color.rgba(), 16));
    if (!ui->checkBoxResamplePoints->isChecked())
    {
        _saveCurve(newElm, attribute->curve);
    }
    else
    {
        _saveCurve(newElm, attribute->curve.resampleCurve(ui->spinBoxResamplePoints->value()));
    }
}

void CurveEditorMainWindow::_saveAttribute(QDomElement& parentElm, ColorAttribute* attribute)
{
    QDomElement newElm = parentElm.ownerDocument().createElement(tr("Attribute"));
    parentElm.appendChild(newElm);
    newElm.setAttribute(tr("Name"), attribute->getName().c_str());

    _saveCurve(newElm, attribute->colorCurve, tr("ColorCurve"));
    if (attribute->alphaCurve.getControlPointCount() > 0)
        _saveCurve(newElm, attribute->alphaCurve, tr("AlphaCurve"));
}

void CurveEditorMainWindow::_saveCurve(QDomElement& parentElm, Curve& curve)
{
    QDomDocument domDoc = parentElm.ownerDocument();
    QDomElement curveElm = domDoc.createElement(tr("Curve"));
    parentElm.appendChild(curveElm);
    curveElm.setAttribute(tr("MinValueX"), curve.getMinValue().x());
    curveElm.setAttribute(tr("MinValueY"), curve.getMinValue().y());
    curveElm.setAttribute(tr("MaxValueX"), curve.getMaxValue().x());
    curveElm.setAttribute(tr("MaxValueY"), curve.getMaxValue().y());

    int ctrlPntCount = curve.getControlPointCount();
    for (int i = 0; i < ctrlPntCount; ++i)
    {
        const ControlPoint& ctrlPnt = curve.getControlPoint(i);
        _saveCtrlPnt(curveElm, ctrlPnt);
    }
}

void CurveEditorMainWindow::_saveCurve(QDomElement& parentElm, ColorCurve& curve, const QString& curveName)
{
    QDomDocument domDoc = parentElm.ownerDocument();
    QDomElement curveElm = domDoc.createElement(curveName);
    parentElm.appendChild(curveElm);
    curveElm.setAttribute(tr("MinValue"), curve.getMinValue());
    curveElm.setAttribute(tr("MaxValue"), curve.getMaxValue());

    int ctrlPntCount = curve.getControlPointCount();
    for (int i = 0; i < ctrlPntCount; ++i)
    {
        const ColorControlPoint& ctrlPnt = curve.getControlPoint(i);
        _saveCtrlPnt(curveElm, ctrlPnt);
    }
}

void CurveEditorMainWindow::_saveCtrlPnt(QDomElement& parentElm, const ControlPoint& ctrlPnt)
{
    QDomDocument domDoc = parentElm.ownerDocument();
    QDomElement ctrlPntElm = domDoc.createElement(tr("ControlPoint"));
    parentElm.appendChild(ctrlPntElm);

    QDomElement valueElm = domDoc.createElement(tr("Value"));
    ctrlPntElm.appendChild(valueElm);
    valueElm.setAttribute(tr("X"), ctrlPnt.value.x());
    valueElm.setAttribute(tr("Y"), ctrlPnt.value.y());

    QDomElement modeElm = domDoc.createElement(tr("InterpolateMode"));
    ctrlPntElm.appendChild(modeElm);
    QDomNode txt = domDoc.createTextNode(InterpolateModeToString(ctrlPnt.mode));
    modeElm.appendChild(txt);

    if (eBezierSpline == ctrlPnt.mode)
    {
        QDomElement bezierSplineDataElm = domDoc.createElement(tr("BezierSplineData"));
        ctrlPntElm.appendChild(bezierSplineDataElm);

        bezierSplineDataElm.setAttribute(tr("InTan"), QString("%0").arg(ctrlPnt.splineData.inTan));
        bezierSplineDataElm.setAttribute(tr("InLen"), QString("%0").arg(ctrlPnt.splineData.inLen));
        bezierSplineDataElm.setAttribute(tr("OutTan"), QString("%0").arg(ctrlPnt.splineData.outTan));
        bezierSplineDataElm.setAttribute(tr("OutLen"), QString("%0").arg(ctrlPnt.splineData.outLen));
    }
}

void CurveEditorMainWindow::_saveCtrlPnt(QDomElement& parentElm, const ColorControlPoint& ctrlPnt)
{
    QDomDocument domDoc = parentElm.ownerDocument();
    QDomElement ctrlPntElm = domDoc.createElement(tr("ColorControlPoint"));
    parentElm.appendChild(ctrlPntElm);

    QDomElement valueElm = domDoc.createElement(tr("Value"));
    ctrlPntElm.appendChild(valueElm);
    valueElm.setAttribute(tr("X"), ctrlPnt.x);
    valueElm.setAttribute(tr("Color"), QString().setNum(ctrlPnt.color.rgba(), 16));
    valueElm.setAttribute(tr("Weight"), ctrlPnt.weight);
    valueElm.setAttribute(tr("FallOff"), ctrlPnt.fallOff);
    valueElm.setAttribute(tr("TexturePath"), ctrlPnt.texturePath.c_str());

    QDomElement modeElm = domDoc.createElement(tr("InterpolateMode"));
    ctrlPntElm.appendChild(modeElm);
    QDomNode txt = domDoc.createTextNode(InterpolateModeToString(ctrlPnt.mode));
    modeElm.appendChild(txt);

    if (eBezierSpline == ctrlPnt.mode)
    {
        QDomElement bezierSplineDataElm = domDoc.createElement(tr("BezierSplineData"));
        ctrlPntElm.appendChild(bezierSplineDataElm);

        bezierSplineDataElm.setAttribute(tr("InTan"), QString("%0").arg(ctrlPnt.splineData.inTan));
        bezierSplineDataElm.setAttribute(tr("InLen"), QString("%0").arg(ctrlPnt.splineData.inLen));
        bezierSplineDataElm.setAttribute(tr("OutTan"), QString("%0").arg(ctrlPnt.splineData.outTan));
        bezierSplineDataElm.setAttribute(tr("OutLen"), QString("%0").arg(ctrlPnt.splineData.outLen));
    }
}

CurveAttributeBase* CurveEditorMainWindow::_loadAttribute(QDomElement& elm)
{
    QString name = elm.attribute(tr("Name"));

    QDomElement curveElm = elm.firstChildElement(tr("Curve"));
    if (!curveElm.isNull())
    {
        CurveAttribute* attribute = new CurveAttribute("");
        attribute->_name = name.toStdString();
        //qDebug()<<"attribute Name"<<name<<" Color:"<<elm.attribute(tr("Color"));
        attribute->color.setRgba(elm.attribute(tr("Color")).toUInt(0, 16));
        //qDebug()<<"color:::"<<QString().setNum(attrData->color.rgba(), 16);
        float minValueX = curveElm.attribute(tr("MinValueX")).toFloat();
        float minValueY = curveElm.attribute(tr("MinValueY")).toFloat();
        float maxValueX = curveElm.attribute(tr("MaxValueX")).toFloat();
        float maxValueY = curveElm.attribute(tr("MaxValueY")).toFloat();
        attribute->curve.initValueRange(QPointF(minValueX, minValueY), QPointF(maxValueX, maxValueY));
        _loadCurve(curveElm, attribute->curve);

        return attribute;
    }
    else
    {
        curveElm = elm.firstChildElement(tr("ColorCurve"));

        if (!curveElm.isNull())
        {
            ColorAttribute* attribute = new ColorAttribute("");
            attribute->_name = name.toStdString();
            float minValue = curveElm.attribute(tr("MinValue")).toFloat();
            float maxValue = curveElm.attribute(tr("MaxValue")).toFloat();
            attribute->colorCurve.initValueRange(minValue, maxValue);
            _loadCurve(curveElm, attribute->colorCurve);

            curveElm = elm.firstChildElement(tr("AlphaCurve"));
            minValue = curveElm.attribute(tr("MinValue")).toFloat();
            maxValue = curveElm.attribute(tr("MaxValue")).toFloat();
            attribute->alphaCurve.initValueRange(minValue, maxValue);
            _loadCurve(curveElm, attribute->alphaCurve);
            return attribute;
        }
    }

    return nullptr;
}

void CurveEditorMainWindow::_loadCurve(QDomElement& elm, Curve& curve)
{
    QDomNodeList nodeList = elm.elementsByTagName(tr("ControlPoint"));
    for (int i = 0; i < nodeList.count(); ++i)
    {
        QDomElement ctrlElm = nodeList.at(i).toElement();

        ControlPoint ctrlPnt;
        if (!ctrlElm.isNull())
        {
            _loadCtrlPnt(ctrlElm, ctrlPnt);
            curve.appendControlPoint(ctrlPnt, false);
        }
    }
}

void CurveEditorMainWindow::_loadCurve(QDomElement& elm, ColorCurve& curve)
{
    QDomNodeList nodeList = elm.elementsByTagName(tr("ColorControlPoint"));
    for (int i = 0; i < nodeList.count(); ++i)
    {
        QDomElement ctrlElm = nodeList.at(i).toElement();

        ColorControlPoint ctrlPnt;
        if (!ctrlElm.isNull())
        {
            _loadCtrlPnt(ctrlElm, ctrlPnt);
            curve.appendControlPoint(ctrlPnt, false);
        }
    }
}

void CurveEditorMainWindow::_loadCtrlPnt(QDomElement& elm, ControlPoint& ctrlPnt)
{
    QDomElement valueElm = elm.firstChildElement(tr("Value"));
    ctrlPnt.value.setX(valueElm.attribute(tr("X")).toFloat());
    ctrlPnt.value.setY(valueElm.attribute(tr("Y")).toFloat());

    QDomElement modeElm = elm.firstChildElement(tr("InterpolateMode"));

    ctrlPnt.mode =  StringToInterpolateMode(modeElm.text());

    if (eBezierSpline == ctrlPnt.mode)
    {
        QDomElement bezierElm = elm.firstChildElement(tr("BezierSplineData"));

        ctrlPnt.splineData.inTan = bezierElm.attribute(tr("InTan")).toFloat();
        ctrlPnt.splineData.inLen = bezierElm.attribute(tr("InLen")).toFloat();
        ctrlPnt.splineData.outTan = bezierElm.attribute(tr("OutTan")).toFloat();
        ctrlPnt.splineData.outLen = bezierElm.attribute(tr("OutLen")).toFloat();
    }
}

void CurveEditorMainWindow::_loadCtrlPnt(QDomElement& elm, ColorControlPoint& ctrlPnt)
{
    QDomElement valueElm = elm.firstChildElement(tr("Value"));
    ctrlPnt.x = valueElm.attribute(tr("X")).toFloat();
    ctrlPnt.color.setRgba(valueElm.attribute(tr("Color")).toUInt(0, 16));
    ctrlPnt.weight = valueElm.attribute(tr("Weight")).toFloat();
    ctrlPnt.fallOff = valueElm.attribute(tr("FallOff")).toFloat();
    ctrlPnt.weight = valueElm.attribute(tr("Weight")).toFloat();
    ctrlPnt.texturePath = valueElm.attribute(tr("TexturePath")).toStdString();

    QDomElement modeElm = elm.firstChildElement(tr("InterpolateMode"));

    ctrlPnt.mode =  StringToInterpolateMode(modeElm.text());

    if (eBezierSpline == ctrlPnt.mode)
    {
        QDomElement bezierElm = elm.firstChildElement(tr("BezierSplineData"));

        ctrlPnt.splineData.inTan = bezierElm.attribute(tr("InTan")).toFloat();
        ctrlPnt.splineData.inLen = bezierElm.attribute(tr("InLen")).toFloat();
        ctrlPnt.splineData.outTan = bezierElm.attribute(tr("OutTan")).toFloat();
        ctrlPnt.splineData.outLen = bezierElm.attribute(tr("OutLen")).toFloat();
    }
}

void CurveEditorMainWindow::_setTangentTypeUIStatus(bool enable)
{
    ui->actionTangent_Stepped->setEnabled(enable);
    ui->actionTangent_Linear->setEnabled(enable);
    ui->actionTangent_Smooth->setEnabled(enable);
    ui->actionTangent_Ease_Out->setEnabled(enable);
    ui->actionTangent_Ease_In->setEnabled(enable);
    ui->actionTangent_Spline->setEnabled(enable);
}

void CurveEditorMainWindow::_setAddRemoveCtrlPntUIStatus(bool enable)
{
    ui->actionAdd_Before_Selected->setEnabled(enable);
    ui->actionAdd_After_Selected->setEnabled(enable);
    ui->actionRemove_Selected->setEnabled(enable);
    ui->actionAdd_Control_Point_By_Click->setEnabled(enable);
    ui->actionRemove_Control_Point_By_Click->setEnabled(enable);
}

} // namespace CurveEditor
} // namespace nvidia