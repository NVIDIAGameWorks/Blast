#ifndef CURVEEDITOR_H
#define CURVEEDITOR_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTreeWidget>
#include <QtXml\QtXml>
#include "Attribute.h"

namespace Ui {
    class CurveEditorMainWindow;
}

namespace nvidia {
namespace CurveEditor {

class CurveWidget;
class CurveEntity;
class ColorWidget;

class CURVEEDITOR_EXPORT CurveEditorMainWindow : public QMainWindow
{
    Q_OBJECT

    friend class CurveWidget;
    friend class CurveEntity;
    friend class ColorWidget;

public:
    explicit CurveEditorMainWindow(QWidget *parent = 0);
    ~CurveEditorMainWindow();

    void setCurveAttributes(const std::vector<CurveAttributeBase*>& attributes);
    void setColorCurveAttributes(const std::vector<ColorAttribute*>& attributes);
    // if this method is called, tab widget switch to curve attribute tab
    void setSelectedCurveAttributes(const std::vector<CurveAttributeBase*>& attributes);
    // if this method is called, tab widget switch to color attribute tab
    void setSelectedColorAttribute(const ColorAttribute* attribute);
    void setResampleEnabled(bool enable);

signals:
    void CurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute);
    void ColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute);
    // if reloadColorTex is true, reload texture of the selected control point of the color curve of the color attribute
    // if reloadColorTex is false, reload texture of the selected control point of the alpha curve of the color attribute
    // selectedCtrlPntIndex index of the selected control point
    void ReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex);

private slots:
    /////////////////////////slots for signals of standard QT controls//////////////////////////////////////////////////////
    void on_actionCopy_triggered();

    void on_actionPaste_triggered();

    void on_actionSave_Selected_as_Preset_triggered();

    void on_actionLoad_Preset_to_Copy_Buffer_triggered();

    void on_actionImport_Preset_onto_Selected_triggered();

    void on_actionReset_Curve_triggered();

    void on_actionAdd_Before_Selected_triggered();

    void on_actionAdd_After_Selected_triggered();

    void on_actionRemove_Selected_triggered();

    void on_actionTangent_Stepped_triggered();

    void on_actionTangent_Linear_triggered();

    void on_actionTangent_Smooth_triggered();

    void on_actionTangent_Ease_Out_triggered();

    void on_actionTangent_Ease_In_triggered();

    void on_actionTangent_Spline_triggered();

    void on_actionSnap_All_triggered();

    void on_actionSnap_Horizontal_triggered();

    void on_actionSnap_Vertical_triggered();

    void on_actionContract_Horizontally_triggered();

    void on_actionExpand_Horizontally_triggered();

    void on_actionContract_Vertically_triggered();

    void on_actionExpand_Vertically_triggered();

    void on_actionFrame_Horizontally_triggered();

    void on_actionFrame_Vertically_triggered();

    void on_actionFrame_All_triggered();

    void on_spinBoxLocation_valueChanged(double value);

    void on_spinBoxValue_valueChanged(double value);

    void on_btnColor_clicked();

    void on_btnColorTex_clicked();

    void on_btnColorReload_clicked();

    void on_btnColorClear_clicked();

    void on_btnAlpha_clicked();

    void on_btnAlphaTex_clicked();

    void on_btnAlphaReload_clicked();

    void on_btnAlphaClear_clicked();

    void on_checkBoxUseAlphaChannellFromColor_stateChanged(int val);

    void on_treeWidgetCurveAttributes_itemSelectionChanged();

    void on_treeWidgetColorAttributes_itemSelectionChanged();

    void on_tabWidgetAttributes_currentChanged(int index);

    void on_sliderColorFallOff_sliderMoved(int value);

    void on_sliderAlphaFallOff_sliderMoved(int value);

    void on_actionAdd_Control_Point_By_Click_triggered(bool val);

    void on_actionRemove_Control_Point_By_Click_triggered(bool val);

    /////////////////////////slots for inside signals//////////////////////////////////////////////////////
    void onCurvePickedControlPointChanged(const std::vector<CurveEntity*>& pickedCurves);

    void onCurvePickedControlPointValueChanged(QPointF& value);

    void onColorPickedControlPointChanged(bool isColorCtrlPnt);

private:
    class CurveAttributeTreeItem : public QTreeWidgetItem
    {
    public:
        explicit CurveAttributeTreeItem(QTreeWidget *view, CurveAttributeBase* attribute)
            : QTreeWidgetItem(view)
            , _attribute(attribute)
        {

        }
        explicit CurveAttributeTreeItem(QTreeWidgetItem *parent, CurveAttributeBase* attribute)
            : QTreeWidgetItem(parent)
            , _attribute(attribute)
        {

        }

        CurveAttributeBase* _attribute;
    };

    class ColorAttributeTreeItem : public QTreeWidgetItem
    {
    public:
        explicit ColorAttributeTreeItem(QTreeWidget *view, ColorAttribute* attribute)
            : QTreeWidgetItem(view)
            , _attribute(attribute)
        {

        }

        ColorAttribute* _attribute;
    };

private:
    void _fillCurveAttributesTree();
    void _fillColorAttributesTree();
    void _syncUIStatusWithSelectedAttribute(bool canAddRemoveControlPoint, bool canChangeTangentType);
    void _setCurveExclusiveUIEnable(bool enable);
    void _setColorUIEnable(bool enable);
    void _setAlphaUIEnable(bool enable);
    void _saveAttributeGroup(QDomElement& parentElm, CurveAttributeGroup* attributeGroup);
    void _saveAttribute(QDomElement& parentElm, CurveAttribute* attribute);
    void _saveAttribute(QDomElement& parentElm, ColorAttribute* attribute);
    void _saveCurve(QDomElement& parentElm, Curve& curve);
    void _saveCurve(QDomElement& parentElm, ColorCurve& curve, const QString& curveName);
    void _saveCtrlPnt(QDomElement& parentElm, const ControlPoint& ctrlPnt);
    void _saveCtrlPnt(QDomElement& parentElm, const ColorControlPoint& ctrlPnt);
    CurveAttributeBase* _loadAttribute(QDomElement& elm);
    void _loadCurve(QDomElement& elm, Curve& curve);
    void _loadCurve(QDomElement& elm, ColorCurve& curve);
    void _loadCtrlPnt(QDomElement& elm, ControlPoint& ctrlPnt);
    void _loadCtrlPnt(QDomElement& elm, ColorControlPoint& ctrlPnt);

    void _setTangentTypeUIStatus(bool enable);
    void _setAddRemoveCtrlPntUIStatus(bool enable);

    Ui::CurveEditorMainWindow*          ui;
    std::vector<CurveAttributeBase*>    _curveAttributes;
    std::vector<ColorAttribute*>        _colorAttributes;
    CurveWidget*                        _curveWidget;
    ColorWidget*                        _colorWidget;
    CurveAttribute*                     _curveAttributeCache; // for copy operation of curve attribute
    ColorAttribute*                     _colorAttributeCache; // for copy operation of color attribute
    bool                                _updateUIFromData;
    bool                                _canMoveCurveControlPointHorizontally;
    bool                                _canAddRemoveCurveControlPoint;
    bool                                _canChangeCurveTangentType;
    QString                             _lastFilePath;
    QString                             _presetPath;
};

} // namespace CurveEditor
} // namespace nvidia

#endif // CURVEEDITOR_H
