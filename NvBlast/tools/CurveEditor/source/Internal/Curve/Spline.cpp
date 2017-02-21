#include "Spline.h"

namespace nvidia {
namespace CurveEditor {

Spline::Spline(long segmentCount)
    : _needSample(true)
    , _samplePoints()
    , _edgeLengths()
    , _totolLength(0.0f)
{
    _segmentCount = segmentCount < 1 ? 1 : segmentCount;
}

Spline::~Spline(void)
{
}

bool Spline::getPercentPoint(float percent, QPointF& point)
{
    long lPointCount = (long)_samplePoints.size();

    if (lPointCount == 0)
    {
        return false;
    }
    else if (lPointCount == 1)
    {
        point = _samplePoints[0];
        return true;
    }

    if (_needSample)
        _sample();

    if (percent > 1.0)
    {
        percent -= (int)percent;
    }

    if (percent < 0.0)
    {
        percent += (int)(-percent) + 1;
    }

    if (percent <= 0.0)
    {
        // return begin point
        point = _samplePoints[0];
        return true;
    }
    else if (percent >= 1.0)
    {
        // return end point
        point = _samplePoints[_samplePoints.size() - 1];
        return true;
    }

    float fCurPos = _totolLength * percent;
    int      index = 0;

    {// get edge's index the point is on based on the percent value
        long lEdgeCount = (long)_edgeLengths.size();
        for ( ; index < lEdgeCount; index++)
        {
            if (fCurPos < _edgeLengths[index])
            {
                break;
            }

            fCurPos -= _edgeLengths[index];
        }

        if (index == lEdgeCount)
        {
            point = _samplePoints[_samplePoints.size() - 1];
            return true;
        }
    }

    QPointF v1 = _samplePoints[index];
    QPointF v2 = _samplePoints[index + 1];

    point = v1 + (v2 - v1) * (fCurPos / _edgeLengths[index]);

    return true;
}

bool Spline::getPointByX(float x, QPointF& point)
{
    if (_needSample)
        _sample();

    long lPointCount = (long)_samplePoints.size();

    if(lPointCount < 2)
    {
        return false;
    }

    for (int i = 0; i < lPointCount - 1; i++)
    {
        if(_samplePoints[i].x() <= x && _samplePoints[i + 1].x() > x)
        {
            point.setX( x );
            float fRate = (x - _samplePoints[i].x())/ (_samplePoints[i + 1].x() - _samplePoints[i].x());
            point.setY(_samplePoints[i].y() + (_samplePoints[i+1].y() - _samplePoints[i].y()) * fRate);
            return true;
        }
    }
    return false;
}

void Spline::setSegmentCount(long segmentCount)
{
    _segmentCount = segmentCount < 1 ? 1 : segmentCount;
    _needSample = true;
}

std::vector<QPointF> Spline::getSamplePoints()
{
    if (_needSample)
        _sample();
    return _samplePoints;
}

void Spline::_sample()
{
    _samplePoints.clear();

    _doSample();

    // get all edges length and total length
    _totolLength = 0;
    _edgeLengths.clear();
    float fEdgeLength;

    long lPointCount = (long)_samplePoints.size();
    for (int i = 1; i < lPointCount; i++)
    {
        QPointF dist = _samplePoints[i] - _samplePoints[i - 1];
        fEdgeLength = (sqrt(dist.x()*dist.x() + dist.y() + dist.y()));
        _edgeLengths.push_back(fEdgeLength);
        _totolLength += fEdgeLength;
    }

    _needSample = false;
}

} // namespace CurveEditor
} // namespace nvidia