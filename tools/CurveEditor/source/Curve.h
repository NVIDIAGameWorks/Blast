#ifndef CURVE_H
#define CURVE_H
#include <QtCore/QPointF>
#include <QtGui/QColor>
#include <vector>
#include "Common.h"

namespace nvidia {
namespace CurveEditor {

enum InterpolateMode
{
    eDiscret,
    eLinear,
    eBezierSpline,
    eCatmullRomSpline,
};

struct BezierSplineData
{
    float    inTan;
    float    inLen;
    float    outTan;
    float    outLen;
};

struct ControlPoint
{
    ControlPoint()
        : value(0, 0)
        , mode (eDiscret)
    {
        memset( &splineData, 0, sizeof(BezierSplineData));
    }

    ControlPoint(float inX, float inY, InterpolateMode inMode)
        : value(inX, inY)
        , mode(inMode)
    {
        memset( &splineData, 0, sizeof(BezierSplineData));
    }

    ControlPoint(float inX, float inY, InterpolateMode inMode, BezierSplineData& inSplineData)
        : value(inX, inY)
        , mode(inMode)
        , splineData(inSplineData)
    {

    }

    QPointF value;
    InterpolateMode mode;
    BezierSplineData splineData;
};

class CURVEEDITOR_EXPORT Curve
{
    friend class CurveWidget;
    friend class CurveEntity;
public:
    Curve(long segmentCount = 100);

    void initValueRange(const QPointF& min, const QPointF& max);
    QPointF getMinValue();
    QPointF getMaxValue();

    int getControlPointCount()  { return (int) _controlPoints.size(); }
    const ControlPoint& getControlPoint(int index)    { return _controlPoints[index]; }
    void appendControlPoint(ControlPoint& controlPoint, bool asDefaultToo = true);
    int appendControlPoint(float x);
    void setControlPoint(int index, const ControlPoint& point);
    void reset(void);
    void insertControlPointAt(int index);
    void removeControlPoint(int index);
    void removeControlPoints(std::vector<int>& indexes);

    QPointF getPointByX(float x);
    std::vector<QPointF> getSamplePoints();
    Curve resampleCurve(int resamplePnts, long segmentCount = 100);

    Curve& operator = (const Curve& right);

private:
    void _sample();
    ControlPoint& _getNearCtrlPnt(float x);

private:
    bool                    _needSample;
    bool                    _initValueRange;
    QPointF                 _minValue;
    QPointF                 _maxValue;
    long                    _segmentCount;
    std::vector<QPointF>    _samplePoints;
    std::vector<ControlPoint>    _controlPoints;
    std::vector<ControlPoint>    _defaultControlPoints;
};

struct ColorControlPoint
{
    ColorControlPoint()
        : x(0.0)
        , color()
        , mode (eDiscret)
        , weight(0.5f)
        , fallOff(0.5f)
        , texturePath()
    {
        memset( &splineData, 0, sizeof(BezierSplineData));
    }

    ColorControlPoint(float inX, const QColor& inColor, InterpolateMode inMode, float inWeight = 0.5f, float inFallOff = 0.5f)
        : x(inX)
        , color(inColor)
        , mode(inMode)
        , weight(inWeight)
        , fallOff(inFallOff)
        , texturePath()
    {
        memset( &splineData, 0, sizeof(BezierSplineData));
    }

    ColorControlPoint(float inX, const QColor& inColor, InterpolateMode inMode, const BezierSplineData& inSplineData, float inWeight = 0.5f, float inFallOff = 0.5f)
        : x(inX)
        , color(inColor)
        , mode(inMode)
        , splineData(inSplineData)
        , weight(inWeight)
        , fallOff(inFallOff)
        , texturePath()
    {

    }

    float               x;
    QColor              color;
    InterpolateMode     mode;
    BezierSplineData    splineData;
    float               weight;         // if it's less than 0, the segememnt between this control point and next control point won't use weight algorithm
    float               fallOff;
    std::string         texturePath;
    QColor              textureAverageColor;
};

struct ColorPoint
{
    ColorPoint()
        : x(0.0)
        , color()
    {
    }

    ColorPoint(float inX, const QColor& inColor)
        : x(inX)
        , color(inColor)
    {
    }

    float               x;
    QColor              color;
};

class ColorWidget;
class CURVEEDITOR_EXPORT ColorCurve
{
    friend class ColorWidget;
public:
    ColorCurve(long segmentCount = 100);

    void initValueRange(float min, float max);
    float getMinValue();
    float getMaxValue();

    int getControlPointCount()  { return (int) _controlPoints.size(); }
    const ColorControlPoint& getControlPoint(int index)    { return _controlPoints[index]; }
    void appendControlPoint(ColorControlPoint& controlPoint, bool asDefaultToo = true);
    void setControlPoint(int index, const ColorControlPoint& point);
    void reset(void);
    void insertControlPointAt(int index);
    void removeControlPoint(int index);

    QColor getColorByX(float x);

    ColorCurve& operator = (const ColorCurve& right);

private:
    void _doSamplePoints();
    void _reOrderControlPoints(int& pickedPoint);
    QColor _calcTextureAverageColor(const char* texturePath);

private:
    bool                        _needSample;
    bool                        _initValueRange;
    float                       _minValue;
    float                       _maxValue;
    long                        _segmentCount;
    std::vector<ColorPoint>          _colorSamplePnts;
    std::vector<ColorControlPoint>   _controlPoints;
    std::vector<ColorControlPoint>   _defaultControlPoints;
};

} // namespace CurveEditor
} // namespace nvidia

#endif // CURVE_H
