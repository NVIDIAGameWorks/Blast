#include "BezierSpline.h"

namespace nvidia {
namespace CurveEditor {

std::vector<QPointF> BezierSpline::sample(const std::vector<BezierSplinePoint>& splinePoints, long segmentCount)
{
    std::vector<QPointF> samplePoints;

    if (segmentCount < 1)
        return samplePoints;

    long  lPointCount = (long)splinePoints.size();
    float fLengthStep = 1.0 / segmentCount;
    float ft;
    QPointF p0;
    QPointF p1;
    QPointF p2;
    QPointF p3;
    QPointF p01;
    QPointF p11;
    QPointF p21;
    QPointF p02;
    QPointF p12;
    QPointF p03;

    for (int i = 1; i < lPointCount; i++)
    {
        const BezierSplinePoint& pt1 = splinePoints[i - 1];
        const BezierSplinePoint& pt2 = splinePoints[i];

        // start point of each Bezier curve segment
        samplePoints.push_back(pt1.valuePoint);

        // sample each point on each Bezier curve segment by percent value and De Casteljau algorithm
        // the sample points are not including start point and end poit of the Bezier curve segement
        for (int j = 1; j < segmentCount; j++)
        {
            ft = fLengthStep * j;

            p0 = pt1.valuePoint;
            p1 = pt1.ctrlPoint1;
            p2 = pt2.ctrlPoint0;
            p3 = pt2.valuePoint;

            p01 = p0 + (p1 - p0) * ft;
            p11 = p1 + (p2 - p1) * ft;
            p21 = p2 + (p3 - p2) * ft;

            p02 = p01 + (p11 - p01) * ft;\
            p12 = p11 + (p21 - p11) * ft;
            p03 = p02 + (p12 - p02) * ft;

            samplePoints.push_back(p03);
        }

        if (i == lPointCount - 1)
        {// end point of last Bezier curve segment
            samplePoints.push_back(pt2.valuePoint);
        }
    }
    return samplePoints;
}

BezierSpline::BezierSpline(long segmentCount)
    : Spline(segmentCount)
    , _controlPoints()
{
}

bool BezierSpline::getControlPoint(int index, BezierSplinePoint &pt)
{
    long lPointCount = (long)_controlPoints.size();

    if (lPointCount == 0)
    {
        return false;
    }

    index = index < 0 ? 0 : (index >= lPointCount ? lPointCount - 1: index);

    pt = _controlPoints[index];

    return true;
}

void BezierSpline::appendControlPoint(BezierSplinePoint pt)
{
    _controlPoints.push_back(pt);

    _needSample = true;
}

void BezierSpline::insertControlPoint(int index, BezierSplinePoint pt)
{
    long lPointCount = (long)_controlPoints.size();

    index = index < 0 ? 0 : (index >= lPointCount ? lPointCount: index);

    _controlPoints.insert(_controlPoints.begin() + index, pt);

    _needSample = true;
}

bool BezierSpline::setControlPoint(int index, BezierSplinePoint pt)
{
    long lPointCount = (long)_controlPoints.size();

    if (0 <= index && index < lPointCount)
    {
        _controlPoints[index] = pt;
        _needSample = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool BezierSpline::removeControlPoint(int index)
{
    long lPointCount = (long)_controlPoints.size();

    if (0 <= index && index < lPointCount)
    {
        _controlPoints.erase(_controlPoints.begin() + index);
        _needSample = true;
        return true;
    }
    else
    {
        return false;
    }
}

void BezierSpline::_doSample()
{
    _samplePoints = sample(_controlPoints, _segmentCount);
}

} // namespace CurveEditor
} // namespace nvidia