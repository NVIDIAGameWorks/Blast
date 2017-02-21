#include "CurveEditorTestApp.h"
#include "ui_CurveEditorTestApp.h"
#include "CurveEditorMainWindow.h"
#include <QtCore/QDebug>

CurveEditorTestApp::CurveEditorTestApp(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
	, ui(new Ui::CurveEditorTestAppClass())
	, _curveEditor(new CurveEditorMainWindow(this))
{
    ui->setupUi(this);

	bool val = connect(_curveEditor, SIGNAL(CurveAttributeChanged(nvidia::CurveEditor::CurveAttribute*)), this, SLOT(onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute*)));
    val = connect(_curveEditor, SIGNAL(ColorAttributeChanged(nvidia::CurveEditor::ColorAttribute*)), this, SLOT(onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute*)) );
    val = connect(_curveEditor, SIGNAL(ReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute*, bool, int)), this, SLOT(onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute*, bool, int)) );

    _curveEditor->hide();

    _fillCurveAttributes();
    _fillColorAttributes();
}

CurveEditorTestApp::~CurveEditorTestApp()
{

}

void CurveEditorTestApp::on_btnCurveEditor_clicked(void)
{
    _curveEditor->show();
}

void CurveEditorTestApp::onCurveAttributeChanged(CurveAttribute* attribute)
{
    if (attribute)
    {
        qDebug()<<"-------------------------onCurveAttributeChanged---------------------------------";
        qDebug()<< attribute->getName().c_str() <<": changed";

        size_t count = attribute->curve.getControlPointCount();
        for (int i = 0; i < count; ++i)
        {
            const ControlPoint& ctrlPnt = attribute->curve.getControlPoint(i);
            if (attribute->getName() == "Y" && i == 9 && ctrlPnt.value.y() > 2.5)
            {
                int j = 0;
                ++j;
            }
            qDebug()<< ctrlPnt.value;
        }
    }
}

void CurveEditorTestApp::onColorAttributeChanged(ColorAttribute* attribute)
{
    if (attribute)
    {
        qDebug()<<"----------------------------------------------------------";
        qDebug()<< attribute->getName().c_str() <<": changed";
    }
}

void CurveEditorTestApp::onReloadColorAttributeTexture(ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex)
{
    if (attribute)
    {
        qDebug()<<"----------------------------------------------------------";
        qDebug()<< attribute->getName().c_str() <<": reloadColorTex" << reloadColorTex<< " selectedCtrlPntIndex: "<< selectedCtrlPntIndex;
    }
}

void CurveEditorTestApp::_fillCurveAttributes()
{
    {
        std::vector<CurveAttributeBase*>   attributes;

        CurveAttributeGroup* attrGroup = new CurveAttributeGroup("Position");
        attributes.push_back(attrGroup);

        CurveAttribute* attribute = new CurveAttribute("X", false, true, true);
        //attribute->color = QColor(0x11, 0x22, 0x33, 0x44);//
        attribute->color = QColor("#FF0000");
        attrGroup->attributes.push_back(attribute);

        {
            Curve& curve = attribute->curve;
            curve.initValueRange(QPointF(-1, 0), QPointF(6, 1));
            curve.appendControlPoint(ControlPoint(-1, 1, eDiscret));
            curve.appendControlPoint(ControlPoint(0, 0, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(0.25, 0.5, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(0.5, 0, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(0.75, 0.5, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(1, 0, eCatmullRomSpline));
            {
                BezierSplineData splineData;
                splineData.inLen = 0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(2, 1, eBezierSpline, splineData));
                splineData.inLen = 0.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(3, 0, eBezierSpline, splineData));
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(4, 1, eBezierSpline, splineData));
            }
            curve.appendControlPoint(ControlPoint(5, 0, eLinear));
            curve.appendControlPoint(ControlPoint(6, 1, eLinear));
        }

        attribute = new CurveAttribute("Y", true);
        attribute->color = QColor("#FFFF00");
        attrGroup->attributes.push_back(attribute);
        {
            Curve& curve = attribute->curve;
            curve.initValueRange(QPointF(-2, 0), QPointF(8, 2));
            curve.appendControlPoint(ControlPoint(-2, 2, eDiscret));
            curve.appendControlPoint(ControlPoint(1, 0, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(1.25, 0.5, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(1.5, 0, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(1.75, 0.5, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(2, 0, eCatmullRomSpline));
            {
                BezierSplineData splineData;
                splineData.inLen = 0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(3, 1, eBezierSpline, splineData));
                splineData.inLen = 0.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(4, 0, eBezierSpline, splineData));
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(5, 1, eBezierSpline, splineData));
            }
            curve.appendControlPoint(ControlPoint(6, 0, eLinear));
            curve.appendControlPoint(ControlPoint(8, 2, eLinear));
        }

        attribute = new CurveAttribute("Z", true);
        attribute->color = QColor("#00FF00");
        attrGroup->attributes.push_back(attribute);
        {
            Curve& curve = attribute->curve;
            curve.initValueRange(QPointF(-2, 0), QPointF(8, 4));
            curve.appendControlPoint(ControlPoint(-2, 2, eDiscret));
            curve.appendControlPoint(ControlPoint(1, 0, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(1.5, 4, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(1.75, 0.5, eCatmullRomSpline));
            curve.appendControlPoint(ControlPoint(2, 1, eCatmullRomSpline));
            {
                BezierSplineData splineData;
                splineData.inLen = 0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(3, 3, eBezierSpline, splineData));
                splineData.inLen = 0.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(4, 0, eBezierSpline, splineData));
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(5, 2, eBezierSpline, splineData));
            }
            curve.appendControlPoint(ControlPoint(6, 0, eLinear));
            curve.appendControlPoint(ControlPoint(8, 4, eLinear));
        }

        attribute = new CurveAttribute("AddRemove_ChangeTangent", true, true, true);

        attributes.push_back(attribute);

        {
            Curve& curve = attribute->curve;
            curve.initValueRange(QPointF(-1, 0), QPointF(4, 255));
            curve.appendControlPoint(ControlPoint(-1, 0, eLinear));
            {
                BezierSplineData splineData;
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ControlPoint(4, 255, eBezierSpline, splineData));
            }
        }

        
        _curveAttributes = attributes;
        _curveEditor->setCurveAttributes(_curveAttributes);
    }
}

void CurveEditorTestApp::_fillColorAttributes()
{
    {
        std::vector<ColorAttribute*>   attributes;

        ColorAttribute* attribute = new ColorAttribute("Hair Color");
        attributes.push_back(attribute);

        {
            ColorCurve& colorCurve = attribute->colorCurve;
            colorCurve.initValueRange(0, 1);
            ColorCurve& alphaCurve = attribute->alphaCurve;
            alphaCurve.initValueRange(0, 1);

            colorCurve.appendControlPoint(ColorControlPoint(0, QColor(255, 0, 0, 255), eLinear));
            colorCurve.appendControlPoint(ColorControlPoint(1, QColor(0, 255, 0, 255), eLinear));

            alphaCurve.appendControlPoint(ColorControlPoint(0, QColor(0, 0, 0, 0), eLinear));
            alphaCurve.appendControlPoint(ColorControlPoint(1, QColor(0, 0, 0, 255), eLinear));
        }

        attribute = new ColorAttribute("XColor", true, true, true);
        //attribute->color = QColor(0x11, 0x22, 0x33, 0x44);//
        attributes.push_back(attribute);

        {
            ColorCurve& curve = attribute->colorCurve;
            curve.initValueRange(-1, 6);
            curve.appendControlPoint(ColorControlPoint(-1, QColor(255, 0, 0, 0), eDiscret));
            curve.appendControlPoint(ColorControlPoint(0, QColor(0, 255, 0, 255), eCatmullRomSpline));
            curve.appendControlPoint(ColorControlPoint(0.25, QColor(128, 0, 255, 128), eCatmullRomSpline));
            curve.appendControlPoint(ColorControlPoint(0.5, QColor(255, 0, 0, 255), eCatmullRomSpline));
            curve.appendControlPoint(ColorControlPoint(0.75, QColor(0, 255, 0, 128), eCatmullRomSpline));
            curve.appendControlPoint(ColorControlPoint(1, QColor(0, 0, 255, 0), eCatmullRomSpline));
            {
                BezierSplineData splineData;
                splineData.inLen = 0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ColorControlPoint(2, QColor(255, 0, 0, 255), eBezierSpline, splineData));
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ColorControlPoint(3, QColor(0, 255, 0, 128), eBezierSpline, splineData));
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                curve.appendControlPoint(ColorControlPoint(4, QColor(0, 0, 255, 0), eBezierSpline, splineData));
            }
            curve.appendControlPoint(ColorControlPoint(5, QColor(255, 0, 0, 255), eLinear));
            curve.appendControlPoint(ColorControlPoint(6, QColor(0, 255, 0, 128), eLinear));
        }

        attribute = new ColorAttribute("Color_NoAddRemove_NotChangeTangent", true, false, false);

        attributes.push_back(attribute);

        {
            ColorCurve& colorCurve = attribute->colorCurve;
            colorCurve.initValueRange(-1, 4);
            colorCurve.appendControlPoint(ColorControlPoint(-1, QColor(255, 0, 0, 0), eLinear));
            colorCurve.appendControlPoint(ColorControlPoint(0, QColor(0, 255, 0, 255), eLinear));
            colorCurve.appendControlPoint(ColorControlPoint(0.5, QColor(0, 0, 255, 255), eLinear));
            colorCurve.appendControlPoint(ColorControlPoint(1, QColor(255, 0, 0, 0), eLinear));
            {
                BezierSplineData splineData;
                splineData.inLen = 0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                colorCurve.appendControlPoint(ColorControlPoint(2, QColor(0, 255, 0, 255), eBezierSpline, splineData));
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                colorCurve.appendControlPoint(ColorControlPoint(3, QColor(0, 0, 255, 128), eBezierSpline, splineData));
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                colorCurve.appendControlPoint(ColorControlPoint(4, QColor(255, 0, 0, 0), eBezierSpline, splineData));
            }

            ColorCurve& alphaCurve = attribute->alphaCurve;
            alphaCurve.initValueRange(-1, 4);
            alphaCurve.appendControlPoint(ColorControlPoint(-1, QColor(0, 0, 0, 0), eDiscret));
            alphaCurve.appendControlPoint(ColorControlPoint(1, QColor(0, 0, 0, 255), eLinear));
            alphaCurve.appendControlPoint(ColorControlPoint(2, QColor(0, 0, 0, 80), eLinear));
            alphaCurve.appendControlPoint(ColorControlPoint(4, QColor(0, 0, 0, 255), eLinear));
        }

        attribute = new ColorAttribute("Color_AddRemove_ChangeTangent", true, true, true);

        attributes.push_back(attribute);

        {
            ColorCurve& colorCurve = attribute->colorCurve;
            colorCurve.initValueRange(-1, 4);
            colorCurve.appendControlPoint(ColorControlPoint(-1, QColor(255, 0, 0, 0), eLinear));
            //colorCurve.appendControlPoint(ColorControlPoint(0, QColor(0, 255, 0, 255), eLinear));
            //colorCurve.appendControlPoint(ColorControlPoint(0.5, QColor(0, 0, 255, 255), eLinear));
            //colorCurve.appendControlPoint(ColorControlPoint(1, QColor(255, 0, 0, 0), eLinear));
            {
                BezierSplineData splineData;
                //splineData.inLen = 0;
                //splineData.inTan = 0;
                //splineData.outLen = 1.0;
                //splineData.outTan = 0;
                //colorCurve.appendControlPoint(ColorControlPoint(2, QColor(0, 255, 0, 255), eBezierSpline, splineData));
                //splineData.inLen = 1.0;
                //splineData.inTan = 0;
                //splineData.outLen = 1.0;
                //splineData.outTan = 0;
                //colorCurve.appendControlPoint(ColorControlPoint(3, QColor(0, 0, 255, 128), eBezierSpline, splineData));
                splineData.inLen = 1.0;
                splineData.inTan = 0;
                splineData.outLen = 1.0;
                splineData.outTan = 0;
                colorCurve.appendControlPoint(ColorControlPoint(4, QColor(0, 255, 0, 0), eBezierSpline, splineData));
            }

            ColorCurve& alphaCurve = attribute->alphaCurve;
            alphaCurve.initValueRange(-1, 4);
            alphaCurve.appendControlPoint(ColorControlPoint(-1, QColor(0, 0, 0, 0), eDiscret));
            //alphaCurve.appendControlPoint(ColorControlPoint(1, QColor(0, 0, 0, 255), eLinear));
            //alphaCurve.appendControlPoint(ColorControlPoint(2, QColor(0, 0, 0, 80), eLinear));
            alphaCurve.appendControlPoint(ColorControlPoint(4, QColor(0, 0, 0, 255), eLinear));
        }

        _colorAttributes = attributes;
        _curveEditor->setColorCurveAttributes(_colorAttributes);
    }
}
