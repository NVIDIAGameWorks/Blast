#include "Curve.h"
#include "BezierSpline.h"
#include "CatmullRomSpline.h"
#include <QtCore/QDebug>
#include <QtWidgets/QMessageBox>
#include <QtGui/QImage>
#pragma warning( disable : 4312 )
#pragma warning( disable : 4996 )
#include "../../../external/stb_image/stb_image.c"

namespace nvidia {
namespace CurveEditor {

Curve::Curve(long segmentCount)
    : _needSample(true)
    , _initValueRange(false)
    , _minValue()
    , _maxValue(1.0f, 1.0f)
    , _segmentCount(1)
    , _samplePoints()
    , _controlPoints()
    , _defaultControlPoints()
{
    _segmentCount = segmentCount < 1 ? 1 : segmentCount;
}

void Curve::initValueRange(const QPointF& min, const QPointF& max)
{
    if (min.x() >= max.x() || min.y() >max.y())
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Max value must be larger than min value when initilize value range of curve"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return ;
    }

    _minValue = min;
    _maxValue = max;
    _initValueRange = true;
}

QPointF Curve::getMinValue()
{
    return _minValue;
}

QPointF Curve::getMaxValue()
{
    return _maxValue;
}

void Curve::appendControlPoint(ControlPoint& controlPoint, bool asDefaultToo)
{
    if (!_initValueRange)
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Curve must be initilized its value range before being used!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }

    if (controlPoint.value.x() < _minValue.x()
        || controlPoint.value.x() > _maxValue.x()
        || controlPoint.value.y() < _minValue.y()
        || controlPoint.value.y() > _maxValue.y())
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("The value of the control point to add isn't in value range of the curve to add!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }

    _controlPoints.push_back(controlPoint);

    if (asDefaultToo)
    {
        _defaultControlPoints.push_back(controlPoint);
    }

    _needSample = true;
}

int Curve::appendControlPoint(float x)
{
    size_t count = _controlPoints.size();
    for (int i = 0; i < count - 1; ++i)
    {
        ControlPoint& ctrlPntLeft = _controlPoints[i];
        ControlPoint& ctrlPntRight = _controlPoints[i + 1];
        if (ctrlPntLeft.value.x() <= x && x <= ctrlPntRight.value.x())
        {
            std::vector<ControlPoint>::iterator itr = _controlPoints.begin();
            std::advance(itr, i+1);
            ControlPoint pnt(x, getPointByX(x).y(), ctrlPntLeft.mode, ctrlPntLeft.splineData);
            _controlPoints.insert(itr, pnt);
            _needSample = true;
            return i + 1;
        }
    }

    return -1;
}

void Curve::setControlPoint(int index, const ControlPoint& point)
{
    if (index < 0 || index > (_controlPoints.size() - 1) )
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Meet invalid index when setting control point for curve!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }

    if (point.value.x() < _minValue.x()
        || point.value.x() > _maxValue.x()
        || point.value.y() < _minValue.y()
        || point.value.y() > _maxValue.y())
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("The setting value isn't in value range of the curve!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }
    std::vector<ControlPoint>::iterator itr = _controlPoints.begin();
    std::advance(itr, index);
    *itr = point;
    _needSample = true;
}

void Curve::reset(void)
{
    if (_defaultControlPoints.size() > 0)
    {
        _controlPoints = _defaultControlPoints;
        _needSample = true;
    }
}

void Curve::insertControlPointAt(int index)
{
    if (index < 0)
        index = 0;
    else if (index > (_controlPoints.size() - 1) )
        index = (int)(_controlPoints.size() - 1);

    std::vector<ControlPoint>::iterator itrToAdd = _controlPoints.begin();
    std::advance(itrToAdd, index);

    ControlPoint& ctrlPntFront = _controlPoints[index - 1];
    ControlPoint& ctrlPntBehind = _controlPoints[index];

    switch (ctrlPntFront.mode)
    {
        case eDiscret:
        {
            ControlPoint ctrlPntToAdd;
            ctrlPntToAdd.mode = ctrlPntFront.mode;
            ctrlPntToAdd.value.setX((ctrlPntFront.value.x() + ctrlPntBehind.value.x()) / 2);
            ctrlPntToAdd.value.setY(ctrlPntFront.value.y());
            _controlPoints.insert(itrToAdd, ctrlPntToAdd);
            break;
        }
        case eLinear:
        {
            ControlPoint ctrlPntToAdd;
            ctrlPntToAdd.mode = ctrlPntFront.mode;
            ctrlPntToAdd.value.setX((ctrlPntFront.value.x() + ctrlPntBehind.value.x()) / 2);
            ctrlPntToAdd.value.setY(ctrlPntFront.value.y() + (ctrlPntBehind.value.y() - ctrlPntFront.value.y()) / 2);
            _controlPoints.insert(itrToAdd, ctrlPntToAdd);
            break;
        }
        case eBezierSpline:
        {
            BezierSplinePoint bezierPnt0;
            BezierSplinePoint bezierPnt1;

            float xStride = ctrlPntBehind.value.x() - ctrlPntFront.value.x();
            bezierPnt0.ctrlPoint0 = ctrlPntFront.value;
            bezierPnt0.valuePoint = ctrlPntFront.value;
            float xDelta = ctrlPntFront.splineData.outLen * xStride;
            bezierPnt0.ctrlPoint1 = ctrlPntFront.value + QPointF(xDelta, xDelta * ctrlPntFront.splineData.outTan);

            if (eBezierSpline == ctrlPntBehind.mode)
            {
                xDelta = ctrlPntBehind.splineData.inLen * xStride;
                bezierPnt1.ctrlPoint0 = ctrlPntBehind.value - QPointF(xDelta, xDelta * ctrlPntBehind.splineData.inTan);
                bezierPnt1.valuePoint = ctrlPntBehind.value;
                bezierPnt1.ctrlPoint1 = ctrlPntBehind.value;
            }
            else
            {
                bezierPnt1.ctrlPoint0 = ctrlPntBehind.value;
                bezierPnt1.valuePoint = ctrlPntBehind.value;
                bezierPnt1.ctrlPoint1 = ctrlPntBehind.value;
            }

            BezierSpline spline(100);
            spline.appendControlPoint(bezierPnt0);
            spline.appendControlPoint(bezierPnt1);
            QPointF pntOnSpline;
            spline.getPointByX(ctrlPntFront.value.x() + xStride / 2, pntOnSpline);

            ControlPoint ctrlPntToAdd;
            ctrlPntToAdd.mode = eBezierSpline;
            ctrlPntToAdd.value = pntOnSpline;
            ctrlPntToAdd.splineData.inLen = 1.0;
            ctrlPntToAdd.splineData.inTan = 0.0;
            ctrlPntToAdd.splineData.outLen = 1.0;
            ctrlPntToAdd.splineData.outTan = 0.0;
            _controlPoints.insert(itrToAdd, ctrlPntToAdd);
            break;
        }
        case eCatmullRomSpline:
        {
            CatmullRomSpline spline;

            int i = index - 1;
            for (; i >= 0; --i)
            {
                if (eCatmullRomSpline != _controlPoints[i].mode)
                {
                    ++i;
                    break;
                }
            }

            if (i < 0)
                i = 0;

            for (; i < (int)_controlPoints.size(); ++i)
            {
                spline.appendControlPoint(_controlPoints[i].value);
                if (eCatmullRomSpline != _controlPoints[i].mode)
                    break;
            }

            QPointF pntOnSpline;
            spline.getPointByX((ctrlPntFront.value.x() + ctrlPntBehind.value.x()) / 2, pntOnSpline);
            ControlPoint ctrlPntToAdd;
            ctrlPntToAdd.mode = eCatmullRomSpline;
            ctrlPntToAdd.value = pntOnSpline;
            _controlPoints.insert(itrToAdd, ctrlPntToAdd);
            break;
        }
        default:
            break;
    }

    _needSample = true;
}

void Curve::removeControlPoint(int index)
{
    std::vector<ControlPoint>::iterator itr = _controlPoints.begin();
    std::advance(itr, index);
    _controlPoints.erase(itr);
    _needSample = true;
}

void Curve::removeControlPoints(std::vector<int>& indexes)
{
    for (std::vector<int>::iterator itr = indexes.begin(); itr != indexes.end(); ++itr)
    {
        std::vector<ControlPoint>::iterator itrToRemove = _controlPoints.begin();
        std::advance(itrToRemove, *itr);
        _controlPoints.erase(itrToRemove);

        for (std::vector<int>::iterator itrRight = itr + 1; itrRight != indexes.end(); ++itrRight)
        {
            --(*itrRight);
        }
    }

    _needSample = true;
}

QPointF Curve::getPointByX(float x)
{
    if (!_initValueRange)
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Curve must be initilized its value range before being used!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return QPointF(0, 0);
    }

    if (x < _controlPoints[0].value.x())
        return _controlPoints[0].value;

    if (x >= _controlPoints[_controlPoints.size() - 1].value.x())
        return _controlPoints[_controlPoints.size() - 1].value;

    long lPointCount = (long)_samplePoints.size();

    if(lPointCount < 2)
    {
        return QPointF(0,0);
    }

    if (_needSample)
        _sample();

    QPointF point;

    for (int i = 0; i < lPointCount - 1; i++)
    {
        if(_samplePoints[i].x() <= x && _samplePoints[i + 1].x() > x)
        {
            point.setX( x );
            float fRate = (x - _samplePoints[i].x())/ (_samplePoints[i + 1].x() - _samplePoints[i].x());
            point.setY(_samplePoints[i].y() + (_samplePoints[i+1].y() - _samplePoints[i].y()) * fRate);
            return point;
        }
    }

    return QPointF(0,0);
}

std::vector<QPointF> Curve::getSamplePoints()
{
    if (!_initValueRange)
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Curve must be initilized its value range before being used!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return std::vector<QPointF>();
    }

    if (_needSample)
        _sample();

    return _samplePoints;
}

Curve Curve::resampleCurve(int resamplePnts, long segmentCount)
{
    if (!_initValueRange)
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Curve must be initilized its value range before being used!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return Curve();
    }

    Curve resmapleCurve(segmentCount);
    if (resamplePnts < 2)
        return resmapleCurve;
    else
    {
        int segment = resamplePnts - 1;
        float xStart = _controlPoints[0].value.x();
        float xEnd = _controlPoints[_controlPoints.size() - 1].value.x();
        float strideX = (xEnd - xStart) / segment;
        for (int i = 0; i <= segment; ++i)
        {
            float x = xStart + strideX * i;
            QPointF pnt = getPointByX(x);
            ControlPoint& ctrlPntNear = _getNearCtrlPnt(x);

            ControlPoint ctrlPntNew(pnt.x(), pnt.y(), ctrlPntNear.mode);

            if (eBezierSpline == ctrlPntNear.mode)
            {
                ctrlPntNew.splineData = ctrlPntNear.splineData;
            }

            resmapleCurve.appendControlPoint(ctrlPntNew);
        }
    }

    return resmapleCurve;
}

Curve& Curve::operator = (const Curve& right)
{
    if (this == &right)
        return *this;

    _needSample = right._needSample;
    _initValueRange = right._initValueRange;
    _minValue = right._minValue;
    _maxValue = right._maxValue;
    _segmentCount = right._segmentCount;
    _samplePoints = right._samplePoints;
    _controlPoints = right._controlPoints;

    return *this;
}

void Curve::_sample()
{
    _samplePoints.clear();

    for (size_t i = 1; i < _controlPoints.size(); ++i)
    {
        ControlPoint& ctrlPntFront = _controlPoints[i-1];
        ControlPoint& ctrlPntBehind = _controlPoints[i];

        switch (ctrlPntFront.mode)
        {
            case eDiscret:
            {
                _samplePoints.push_back(ctrlPntFront.value);
                _samplePoints.push_back(QPointF(ctrlPntBehind.value.x(), ctrlPntFront.value.y()));
                break;
            }
            case eLinear:
            {
                _samplePoints.push_back(ctrlPntFront.value);
                _samplePoints.push_back(ctrlPntBehind.value);
                break;
            }
            case eBezierSpline:
            {
                BezierSplinePoint bezierPnt0;
                BezierSplinePoint bezierPnt1;

                float xStride = ctrlPntBehind.value.x() - ctrlPntFront.value.x();
                bezierPnt0.ctrlPoint0 = ctrlPntFront.value;
                bezierPnt0.valuePoint = ctrlPntFront.value;
                float xDelta = ctrlPntFront.splineData.outLen * xStride;
                bezierPnt0.ctrlPoint1 = ctrlPntFront.value + QPointF(xDelta, xDelta * ctrlPntFront.splineData.outTan);

                if (eBezierSpline == ctrlPntBehind.mode)
                {
                    xDelta = ctrlPntBehind.splineData.inLen * xStride;
                    bezierPnt1.ctrlPoint0 = ctrlPntBehind.value - QPointF(xDelta, xDelta * ctrlPntBehind.splineData.inTan);
                    bezierPnt1.valuePoint = ctrlPntBehind.value;
                    bezierPnt1.ctrlPoint1 = ctrlPntBehind.value;
                }
                else
                {
                    bezierPnt1.ctrlPoint0 = ctrlPntBehind.value;
                    bezierPnt1.valuePoint = ctrlPntBehind.value;
                    bezierPnt1.ctrlPoint1 = ctrlPntBehind.value;
                }

                BezierSpline spline(_segmentCount);
                spline.appendControlPoint(bezierPnt0);
                spline.appendControlPoint(bezierPnt1);

                std::vector<QPointF> pnts = spline.getSamplePoints();
                _samplePoints.insert(_samplePoints.end(), pnts.begin(), pnts.end());
                break;
            }
            case eCatmullRomSpline:
            {
                CatmullRomSpline spline(_segmentCount);

                for (i = i - 1; i < _controlPoints.size(); ++i)
                {
                    spline.appendControlPoint(_controlPoints[i].value);
                    if (eCatmullRomSpline != _controlPoints[i].mode)
                    {
                        break;
                    }
                }

                std::vector<QPointF> pnts = spline.getSamplePoints();
                _samplePoints.insert(_samplePoints.end(), pnts.begin(), pnts.end());
                break;
            }
        }
    }

    _needSample = false;
}

ControlPoint& Curve::_getNearCtrlPnt(float x)
{
    size_t count = _controlPoints.size();
    for (int i = 0; i < count - 1; ++i)
    {
        ControlPoint& ctrlPntLeft = _controlPoints[i];
        ControlPoint& ctrlPntRight = _controlPoints[i + 1];
        if (x < ctrlPntLeft.value.x())
            return ctrlPntLeft;
        else if (ctrlPntLeft.value.x() <= x && x <= ctrlPntRight.value.x())
        {
            if (abs(x - ctrlPntLeft.value.x()) <= abs(x - ctrlPntRight.value.x()))
            {
                return ctrlPntLeft;
            }
            else
            {
                return ctrlPntRight;
            }
        }
    }
    return _controlPoints[0];
}

ColorCurve::ColorCurve(long segmentCount)
    : _needSample(true)
    , _initValueRange(false)
    , _minValue(0.0f)
    , _maxValue(1.0f)
    , _colorSamplePnts()
    , _controlPoints()
    , _defaultControlPoints()
{
    _segmentCount = segmentCount < 1 ? 1 : segmentCount;
}

void ColorCurve::initValueRange(float min, float max)
{
    if (min >= max )
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Max x must be larger than min x when initilize value range of color curve"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return ;
    }

    _minValue = min;
    _maxValue = max;
    _initValueRange = true;
}

float ColorCurve::getMinValue()
{
    return _minValue;
}

float ColorCurve::getMaxValue()
{
    return _maxValue;
}

void ColorCurve::appendControlPoint(ColorControlPoint& controlPoint, bool asDefaultToo)
{
    if (!_initValueRange)
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Color curve must be initilized its value range before being used!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }

    if (controlPoint.x < _minValue
        || controlPoint.x > _maxValue)
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("The x value of the control point to add isn't in x value range of the color curve to add!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }

    _controlPoints.push_back(controlPoint);

    if (asDefaultToo)
    {
        _defaultControlPoints.push_back(controlPoint);
    }

    if (!controlPoint.texturePath.empty())
    {
        _controlPoints[_controlPoints.size() - 1].textureAverageColor = _calcTextureAverageColor(controlPoint.texturePath.c_str());
    }

    _needSample = true;
}

void ColorCurve::setControlPoint(int index, const ColorControlPoint& point)
{
    if (index < 0 || index > (_controlPoints.size() - 1) )
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Meet invalid index when setting control point for color curve!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }

    if (point.x < _minValue
        || point.x > _maxValue)
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("The setting value isn't in x value range of the color curve!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }
    std::vector<ColorControlPoint>::iterator itr = _controlPoints.begin();
    std::advance(itr, index);

    if (!point.texturePath.empty() && point.texturePath != itr->texturePath)
    {
        *itr = point;
        itr->textureAverageColor = _calcTextureAverageColor(point.texturePath.c_str());
    }
    else
        *itr = point;

    _needSample = true;
}

void ColorCurve::reset(void)
{
    if (_defaultControlPoints.size() > 0)
    {
        _controlPoints = _defaultControlPoints;
        _needSample = true;
    }
}

void rectifyColorValue(float& colorComponent)
{
    if (colorComponent < 0)
        colorComponent = 0;
    else if(colorComponent > 255)
    {
        colorComponent = 255;
    }
}

void rectifyColorValue(QPointF& colorComponent)
{
    if (colorComponent.y() < 0)
        colorComponent.setY(0);
    else if(colorComponent.y() > 255)
    {
        colorComponent.setY(255);
    }
}

float getBezierColorByX(ColorControlPoint& ctrlPntFront, ColorControlPoint& ctrlPntBehind, int component, float x)
{
    BezierSplinePoint bezierPnt0;
    BezierSplinePoint bezierPnt1;

    float y0 = 0.0f, y1 = 0.0f;
    switch (component) {
    case 0:
        y0 = ctrlPntFront.color.red();
        y1 = ctrlPntBehind.color.red();
        break;
    case 1:
        y0 = ctrlPntFront.color.green();
        y1 = ctrlPntBehind.color.green();
        break;
    case 2:
        y0 = ctrlPntFront.color.blue();
        y1 = ctrlPntBehind.color.blue();
        break;
    case 3:
        y0 = ctrlPntFront.color.alpha();
        y1 = ctrlPntBehind.color.alpha();
        break;
    default:
        break;
    }

    QPointF pntFront = QPointF(ctrlPntFront.x, y0);
    QPointF pntBehind = QPointF(ctrlPntBehind.x, y1);
    float xStride = ctrlPntBehind.x - ctrlPntFront.x;
    bezierPnt0.ctrlPoint0 = pntFront;
    bezierPnt0.valuePoint = pntFront;
    float xDelta = ctrlPntFront.splineData.outLen * xStride;
    bezierPnt0.ctrlPoint1 = pntFront + QPointF(xDelta, xDelta * ctrlPntFront.splineData.outTan);

    if (eBezierSpline == ctrlPntBehind.mode)
    {
        xDelta = ctrlPntBehind.splineData.inLen * xStride;
        bezierPnt1.ctrlPoint0 = pntBehind - QPointF(xDelta, xDelta * ctrlPntBehind.splineData.inTan);
        bezierPnt1.valuePoint = pntBehind;
        bezierPnt1.ctrlPoint1 = pntBehind;
    }
    else
    {
        bezierPnt1.ctrlPoint0 = pntBehind;
        bezierPnt1.valuePoint = pntBehind;
        bezierPnt1.ctrlPoint1 = pntBehind;
    }

    BezierSpline spline(100);
    spline.appendControlPoint(bezierPnt0);
    spline.appendControlPoint(bezierPnt1);
    QPointF pntOnSpline;
    spline.getPointByX(x, pntOnSpline);

    rectifyColorValue(pntOnSpline);
    return pntOnSpline.y();
}

void ColorCurve::insertControlPointAt(int index)
{
    if (index < 0)
        index = 0;
    else if (index > (_controlPoints.size() - 1) )
        index = (int)(_controlPoints.size() - 1);

    std::vector<ColorControlPoint>::iterator itrToAdd = _controlPoints.begin();
    std::advance(itrToAdd, index);

    ColorControlPoint& ctrlPntFront = _controlPoints[index - 1];
    ColorControlPoint& ctrlPntBehind = _controlPoints[index];

    switch (ctrlPntFront.mode)
    {
        case eDiscret:
        {
            ColorControlPoint ctrlPntToAdd;
            ctrlPntToAdd.mode = ctrlPntFront.mode;
            ctrlPntToAdd.x = ((ctrlPntFront.x + ctrlPntBehind.x) / 2);
            ctrlPntToAdd.color = ctrlPntFront.color;
            _controlPoints.insert(itrToAdd, ctrlPntToAdd);
            break;
        }
        case eLinear:
        {
            ColorControlPoint ctrlPntToAdd;
            ctrlPntToAdd.mode = ctrlPntFront.mode;
            ctrlPntToAdd.x = ((ctrlPntFront.x + ctrlPntBehind.x) / 2);
            ctrlPntToAdd.color.setRed(ctrlPntFront.color.red() + (ctrlPntBehind.color.red() - ctrlPntFront.color.red()) / 2);
            ctrlPntToAdd.color.setRed(ctrlPntFront.color.green() + (ctrlPntBehind.color.green() - ctrlPntFront.color.green()) / 2);
            ctrlPntToAdd.color.setRed(ctrlPntFront.color.blue() + (ctrlPntBehind.color.blue() - ctrlPntFront.color.blue()) / 2);
            ctrlPntToAdd.color.setRed(ctrlPntFront.color.alpha() + (ctrlPntBehind.color.alpha() - ctrlPntFront.color.alpha()) / 2);
            _controlPoints.insert(itrToAdd, ctrlPntToAdd);
            break;
        }
        case eBezierSpline:
        {
            ColorControlPoint ctrlPntToAdd;
            ctrlPntToAdd.mode = eBezierSpline;
            ctrlPntToAdd.x = (ctrlPntFront.x + ctrlPntBehind.x) / 2;
            ctrlPntToAdd.color.setRed(getBezierColorByX(ctrlPntFront, ctrlPntBehind, 0, ctrlPntToAdd.x));
            ctrlPntToAdd.color.setGreen(getBezierColorByX(ctrlPntFront, ctrlPntBehind, 1, ctrlPntToAdd.x));
            ctrlPntToAdd.color.setBlue(getBezierColorByX(ctrlPntFront, ctrlPntBehind, 2, ctrlPntToAdd.x));
            ctrlPntToAdd.color.setAlpha(getBezierColorByX(ctrlPntFront, ctrlPntBehind, 3, ctrlPntToAdd.x));
            ctrlPntToAdd.splineData.inLen = 1.0;
            ctrlPntToAdd.splineData.inTan = 0.0;
            ctrlPntToAdd.splineData.outLen = 1.0;
            ctrlPntToAdd.splineData.outTan = 0.0;
            _controlPoints.insert(itrToAdd, ctrlPntToAdd);
            break;
        }
        case eCatmullRomSpline:
        {
            CatmullRomSpline splineRed;
            CatmullRomSpline splineGreen;
            CatmullRomSpline splineBlue;
            CatmullRomSpline splineAlpha;

            int i = index - 1;
            for (; i >= 0; --i)
            {
                if (eCatmullRomSpline != _controlPoints[i].mode)
                {
                    ++i;
                    break;
                }
            }

            for (; i < (int)_controlPoints.size(); ++i)
            {
                splineRed.appendControlPoint(QPointF(_controlPoints[i].x, _controlPoints[i].color.red()));
                splineGreen.appendControlPoint(QPointF(_controlPoints[i].x, _controlPoints[i].color.green()));
                splineBlue.appendControlPoint(QPointF(_controlPoints[i].x, _controlPoints[i].color.blue()));
                splineAlpha.appendControlPoint(QPointF(_controlPoints[i].x, _controlPoints[i].color.alpha()));
                if (eCatmullRomSpline != _controlPoints[i].mode)
                    break;
            }

            QPointF pntOnSpline;
            ColorControlPoint ctrlPntToAdd;
            ctrlPntToAdd.mode = eCatmullRomSpline;
            ctrlPntToAdd.x = (ctrlPntFront.x + ctrlPntBehind.x) / 2;
            splineRed.getPointByX(ctrlPntToAdd.x, pntOnSpline);
            rectifyColorValue(pntOnSpline);
            ctrlPntToAdd.color.setRed(pntOnSpline.y());
            splineGreen.getPointByX(ctrlPntToAdd.x, pntOnSpline);
            rectifyColorValue(pntOnSpline);
            ctrlPntToAdd.color.setGreen(pntOnSpline.y());
            splineBlue.getPointByX(ctrlPntToAdd.x, pntOnSpline);
            rectifyColorValue(pntOnSpline);
            ctrlPntToAdd.color.setBlue(pntOnSpline.y());
            splineAlpha.getPointByX(ctrlPntToAdd.x, pntOnSpline);
            rectifyColorValue(pntOnSpline);
            ctrlPntToAdd.color.setAlpha(pntOnSpline.y());
            _controlPoints.insert(itrToAdd, ctrlPntToAdd);
            break;
        }
        default:
            break;
    }

    _needSample = true;
}

void ColorCurve::removeControlPoint(int index)
{
    std::vector<ColorControlPoint>::iterator itrToRemove = _controlPoints.begin();
    std::advance(itrToRemove, index);
    _controlPoints.erase(itrToRemove);

    _needSample = true;
}

QColor ColorCurve::getColorByX(float x)
{
    if (!_initValueRange)
    {
        QMessageBox::critical(nullptr, QObject::tr("critical"), QObject::tr("Color curve must be initilized its value range before being used!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return QColor();
    }

    if (x < _controlPoints[0].x)
        return _controlPoints[0].color;

    if (x >= _controlPoints[_controlPoints.size() - 1].x)
        return _controlPoints[_controlPoints.size() - 1].color;

    if (_needSample)
        _doSamplePoints();

    size_t pntCount = _colorSamplePnts.size();

    if(pntCount < 2)
    {
        return QColor();
    }

    for (size_t i = 0; i < pntCount - 1; i++)
    {
        if(_colorSamplePnts[i].x <= x && _colorSamplePnts[i + 1].x > x)
        {
            ColorPoint& colorPnt0 = _colorSamplePnts[i];
            ColorPoint& colorPnt1 = _colorSamplePnts[i + 1];
            float fRate = (x - colorPnt0.x) / (colorPnt1.x - colorPnt0.x);

            float red = colorPnt0.color.red() + fRate * (colorPnt1.color.red() - colorPnt0.color.red());
            rectifyColorValue(red);
            float green = colorPnt0.color.green() + fRate * (colorPnt1.color.green() - colorPnt0.color.green());
            rectifyColorValue(green);
            float blue = colorPnt0.color.blue() + fRate * (colorPnt1.color.blue() - colorPnt0.color.blue());
            rectifyColorValue(blue);
            float alpha = colorPnt0.color.alpha() + fRate * (colorPnt1.color.alpha() - colorPnt0.color.alpha());
            rectifyColorValue(alpha);

            return QColor(red, green, blue, alpha);
        }
    }

    return QColor();
}

ColorCurve& ColorCurve::operator = (const ColorCurve& right)
{
    if (this == &right)
        return *this;

    _needSample = right._needSample;
    _initValueRange = right._initValueRange;
    _minValue = right._minValue;
    _maxValue = right._maxValue;
    _segmentCount = right._segmentCount;
    _colorSamplePnts = right._colorSamplePnts;
    _controlPoints = right._controlPoints;

    return *this;
}

BezierSpline _generateColorBezierSpline(ColorControlPoint& ctrlPntFront, ColorControlPoint& ctrlPntBehind, int component, int numSegments = 100)
{
    BezierSplinePoint bezierPnt0;
    BezierSplinePoint bezierPnt1;

    QColor colorFront = ctrlPntFront.color;
    if (!ctrlPntFront.texturePath.empty())
        colorFront = ctrlPntFront.textureAverageColor;
    QColor colorBehind = ctrlPntBehind.color;
    if (!ctrlPntBehind.texturePath.empty())
        colorBehind = ctrlPntBehind.textureAverageColor;

    float y0 = 0.0f, y1 = 0.0f;
    switch (component) {
    case 0:
        y0 = colorFront.red();
        y1 = colorBehind.red();
        break;
    case 1:
        y0 = colorFront.green();
        y1 = colorBehind.green();
        break;
    case 2:
        y0 = colorFront.blue();
        y1 = colorBehind.blue();
        break;
    case 3:
        y0 = colorFront.alpha();
        y1 = colorBehind.alpha();
        break;
    default:
        break;
    }

    QPointF pntFront = QPointF(ctrlPntFront.x, y0);
    QPointF pntBehind = QPointF(ctrlPntBehind.x, y1);
    float xStride = ctrlPntBehind.x - ctrlPntFront.x;
    bezierPnt0.ctrlPoint0 = pntFront;
    bezierPnt0.valuePoint = pntFront;
    float xDelta = ctrlPntFront.splineData.outLen * xStride;
    bezierPnt0.ctrlPoint1 = pntFront + QPointF(xDelta, xDelta * ctrlPntFront.splineData.outTan);

    if (eBezierSpline == ctrlPntBehind.mode)
    {
        xDelta = ctrlPntBehind.splineData.inLen * xStride;
        bezierPnt1.ctrlPoint0 = pntBehind - QPointF(xDelta, xDelta * ctrlPntBehind.splineData.inTan);
        bezierPnt1.valuePoint = pntBehind;
        bezierPnt1.ctrlPoint1 = pntBehind;
    }
    else
    {
        bezierPnt1.ctrlPoint0 = pntBehind;
        bezierPnt1.valuePoint = pntBehind;
        bezierPnt1.ctrlPoint1 = pntBehind;
    }

    BezierSpline spline(numSegments);
    spline.appendControlPoint(bezierPnt0);
    spline.appendControlPoint(bezierPnt1);
    return spline;
}

void _doBezierColorSample(ColorControlPoint& ctrlPntFront, ColorControlPoint& ctrlPntBehind, std::vector<ColorPoint>& colorSamplePnts, int numSegments = 100)
{
    BezierSpline redSpline = _generateColorBezierSpline(ctrlPntFront, ctrlPntBehind, 0, numSegments);
    BezierSpline greenSpline = _generateColorBezierSpline(ctrlPntFront, ctrlPntBehind, 1, numSegments);
    BezierSpline blueSpline = _generateColorBezierSpline(ctrlPntFront, ctrlPntBehind, 2, numSegments);
    BezierSpline alphaSpline = _generateColorBezierSpline(ctrlPntFront, ctrlPntBehind, 3, numSegments);

    std::vector<QPointF> redSamplePnts = redSpline.getSamplePoints();
    std::vector<QPointF> greenSamplePnts = greenSpline.getSamplePoints();
    std::vector<QPointF> blueSamplePnts = blueSpline.getSamplePoints();
    std::vector<QPointF> alphaSamplePnts = alphaSpline.getSamplePoints();

    size_t samplePntsCount = redSamplePnts.size();
    for (size_t i = 0; i < samplePntsCount; ++i)
    {
        colorSamplePnts.push_back(ColorPoint(redSamplePnts[i].x() , QColor(redSamplePnts[i].y()
                                                                            , greenSamplePnts[i].y()
                                                                            , blueSamplePnts[i].y()
                                                                            , alphaSamplePnts[i].y())));
    }
}

float saturate( float v)
{
    if( v < 0.0 ) v = 0.0;
    if( v > 1.0 ) v = 1.0;
    return v;
}

float _GetRatio(float s, float weight, float fallOff)
{
    float ratio = s;

    // add bias for first/second color variation
    if (weight < 0.5f)
    {
        float slope = 2.0f * weight;
        ratio = slope * ratio;
    }
    else
    {
        float slope = 2.0f * (1.0f - weight) ;
        ratio = slope * (ratio - 1.0f) + 1.0f;
    }

    // modify ratio for falloff
    float slope = 1.0f / (fallOff + 0.001f);
    ratio = saturate(0.5f + slope * (ratio - 0.5f));

    return ratio;
}

void ColorCurve::_doSamplePoints()
{
    _colorSamplePnts.clear();

    for (long i = 1; i < _controlPoints.size(); ++i)
    {
        ColorControlPoint& ctrlPntFront = _controlPoints[i-1];
        ColorControlPoint& ctrlPntBehind = _controlPoints[i];
        QColor colorFront = ctrlPntFront.color;
        if (!ctrlPntFront.texturePath.empty())
            colorFront = ctrlPntFront.textureAverageColor;
        QColor colorBehind = ctrlPntBehind.color;
        if (!ctrlPntBehind.texturePath.empty())
            colorBehind = ctrlPntBehind.textureAverageColor;

        switch (ctrlPntFront.mode)
        {
            case eDiscret:
            {
                _colorSamplePnts.push_back(ColorPoint(ctrlPntFront.x, colorFront));
                _colorSamplePnts.push_back(ColorPoint(ctrlPntBehind.x, colorFront));
                break;
            }
            case eLinear:
            {
                for (long j = 0; j <= _segmentCount; ++j)
                {
                    float s = (float)j / _segmentCount;
                    float ratio = s;
                    if (ctrlPntBehind.weight >= 0)
                        ratio = _GetRatio(s, ctrlPntFront.weight, ctrlPntFront.fallOff);

                    float xPos = ctrlPntFront.x + s * (ctrlPntBehind.x - ctrlPntFront.x);

                    int red = colorBehind.red() - colorFront.red();
                    int green = colorBehind.green() - colorFront.green();
                    int blue = colorBehind.blue() - colorFront.blue();
                    int alpha = colorBehind.alpha() - colorFront.alpha();

                    QColor color(colorFront.red() + ratio * red
                                    , colorFront.green() + ratio * green
                                    , colorFront.blue() + ratio * blue
                                    , colorFront.alpha() + ratio * alpha);
                    _colorSamplePnts.push_back(ColorPoint(xPos, color));
                }
                break;
            }
            case eBezierSpline:
            {
                _doBezierColorSample(ctrlPntFront, ctrlPntBehind, _colorSamplePnts, _segmentCount);
                break;
            }
            case eCatmullRomSpline:
            {
                CatmullRomSpline redSpline(_segmentCount);
                CatmullRomSpline greenSpline(_segmentCount);
                CatmullRomSpline blueSpline(_segmentCount);
                CatmullRomSpline alphaSpline(_segmentCount);

                for (i = i - 1; i < _controlPoints.size(); ++i)
                {
                    QColor color = _controlPoints[i].color;
                    if (!_controlPoints[i].texturePath.empty())
                        color = _controlPoints[i].textureAverageColor;
                    redSpline.appendControlPoint(QPointF(_controlPoints[i].x, color.red()));
                    greenSpline.appendControlPoint(QPointF(_controlPoints[i].x, color.green()));
                    blueSpline.appendControlPoint(QPointF(_controlPoints[i].x, color.blue()));
                    alphaSpline.appendControlPoint(QPointF(_controlPoints[i].x, color.alpha()));
                    if (eCatmullRomSpline != _controlPoints[i].mode)
                    {
                        break;
                    }
                }

                std::vector<QPointF> redSamplePnts = redSpline.getSamplePoints();
                std::vector<QPointF> greenSamplePnts = greenSpline.getSamplePoints();
                std::vector<QPointF> blueSamplePnts = blueSpline.getSamplePoints();
                std::vector<QPointF> alphaSamplePnts = alphaSpline.getSamplePoints();

                size_t samplePntsCount = redSamplePnts.size();
                for (size_t i = 0; i < samplePntsCount; ++i)
                {
                    rectifyColorValue(redSamplePnts[i]);
                    rectifyColorValue(greenSamplePnts[i]);
                    rectifyColorValue(blueSamplePnts[i]);
                    rectifyColorValue(alphaSamplePnts[i]);
                    _colorSamplePnts.push_back(ColorPoint(redSamplePnts[i].x() , QColor(redSamplePnts[i].y()
                                                                                        , greenSamplePnts[i].y()
                                                                                        , blueSamplePnts[i].y()
                                                                                        , alphaSamplePnts[i].y())));
                }

                break;
            }
        }
    }

    _needSample = false;
}

void ColorCurve::_reOrderControlPoints(int& pickedPoint)
{
    for (size_t i = 0; i < _controlPoints.size() - 1; ++i)
    {
        if (_controlPoints[i].x > _controlPoints[i+1].x)
        {
            ColorControlPoint temp = _controlPoints[i];
            _controlPoints[i] = _controlPoints[i + 1];
            _controlPoints[i + 1] = temp;

            if (pickedPoint == i)
                pickedPoint = (int)(i + 1);
            else if (pickedPoint == (i + 1) )
                pickedPoint = (int)i;

            break;
        }
    }
}

QColor ColorCurve::_calcTextureAverageColor(const char* texturePath)
{
    QImage img(texturePath);
    double red = 0, green = 0, blue = 0, alpha = 0;

    // Try stb_image for .TGA
    int width = 0;
    int height = 0;
    int numComponents = 0;
    unsigned char *pSTBIRes = stbi_load(texturePath, &width, &height, &numComponents, 4);

    if (!pSTBIRes)
        return QColor(0, 0, 0);

    int colorCount = width * height;
    unsigned int* pixels = (unsigned int*)pSTBIRes;

    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            QRgb pixel = *pixels++;
            QColor color(pixel);
            red += color.redF();
            green += color.greenF();
            blue += color.blueF();
            alpha += color.alphaF();
        }
    }

    if (colorCount != 0)
    {
        red /= colorCount;
        green /= colorCount;
        blue /= colorCount;
        alpha /= colorCount;
    }

    QColor color;
    color.setRgbF(red, green, blue, alpha);
    return color;
}

} // namespace CurveEditor
} // namespace nvidia