#include "CatmullRomSpline.h"

namespace nvidia {
namespace CurveEditor {

float CatmullRomSpline::splineInterpolate(float v0, float v1, float v2, float v3, float u)
{
    float basis[4];

    {
        _computeSplineBasis(u, basis);
        return
            basis[0] * v0 + 
            basis[1] * v1 +
            basis[2] * v2 +
            basis[3] * v3;
    }
}

QPointF CatmullRomSpline::splineInterpolate(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, float u)
{
    float basis[4];

    {
        _computeSplineBasis(u, basis);
        return
            QPointF(basis[0] * p0.x() + 
                    basis[1] * p1.x() +
                    basis[2] * p2.x() +
                    basis[3] * p3.x(),
                    basis[0] * p0.y() + 
                    basis[1] * p1.y() +
                    basis[2] * p2.y() +
                    basis[3] * p3.y());
    }
}

std::vector<QPointF> CatmullRomSpline::sample(const std::vector<QPointF>& inControlPoints, int numSegments)
{
    std::vector<QPointF> samplePoints;
    int numPoints = (int)inControlPoints.size();

    for (int i = 0; i < numPoints - 1; i++)
    {
        QPointF p0, p1, p2, p3;

        if (i == 0)
        {
            p0 = inControlPoints[0], p1 = inControlPoints[0], p2 = inControlPoints[1];
            if (2 < numPoints)
                p3 = inControlPoints[2];
            else
                p3 = inControlPoints[1];

        }
        else if (0 < i && i < numPoints - 2)
        {
            p0 = inControlPoints[i-1], p1 = inControlPoints[i], p2 = inControlPoints[i+1], p3 = inControlPoints[i+2];
        }
        else
        {
            if (2 < numPoints)
                p0 = inControlPoints[i-1];
            else
                p0 = inControlPoints[i];
            p1 = inControlPoints[i], p2 = inControlPoints[i+1], p3 = inControlPoints[i+1];
        }

        for (int j = 0; j <= numSegments; j++)
        {
            float u = j / float(numSegments);

            samplePoints.push_back(splineInterpolate(p0, p1, p2, p3, u));
        }
    }

    return samplePoints;
}

void CatmullRomSpline::_computeSplineBasis(float u, float* basis)
{
    float u2 = u * u;
    float u3 = u2 * u;

    basis[0] = -0.5f * u3 + 1.0f * u2 - 0.5f * u + 0.0f;
    basis[1] = 1.5f  * u3 - 2.5f * u2 + 0.0f * u + 1.0f;
    basis[2] = -1.5f * u3 + 2.0f * u2 + 0.5f * u + 0.0f;
    basis[3] = 0.5f  * u3 - 0.5f * u2 + 0.0f * u + 0.0f;
}

CatmullRomSpline::CatmullRomSpline(long segmentCount)
    : Spline(segmentCount)
    , _controlPoints()
{
    _segmentCount = segmentCount < 1 ? 1 : segmentCount;
}

CatmullRomSpline::CatmullRomSpline(int numControlPoints, long segmentCount)
    : Spline(segmentCount)
    , _controlPoints()
{
    for (int i = 0; i < numControlPoints; ++i)
    {
        _controlPoints.push_back(QPointF(i / float(numControlPoints - 1), 1.0f));
    }
}

CatmullRomSpline::~CatmullRomSpline(void)
{
}

void CatmullRomSpline::setControlPoint(int index, const QPointF& point)
{
    std::vector<QPointF>::iterator itr = _controlPoints.begin();
    std::advance(itr, index);
    *itr = point;
}

QPointF CatmullRomSpline::getControlPoint(int index)
{
    std::vector<QPointF>::iterator itr = _controlPoints.begin();
    std::advance(itr, index);
    return *itr;
}

std::vector<QPointF> CatmullRomSpline::getControlPoints()
{
    return _controlPoints;
}

void CatmullRomSpline::reset(void)
{
    int i = 0;
    for (std::vector<QPointF>::iterator itr = _controlPoints.begin(); itr != _controlPoints.end(); ++itr, ++i)
    {
        itr->setX(i / float(_controlPoints.size() - 1));
        itr->setY(1.0f);
    }
}

void CatmullRomSpline::insertControlPointAt(int index)
{
    QPointF p0, p1, p2, p3;

    QPointF newPoint;
    std::vector<QPointF>::iterator itr = _controlPoints.begin();

    if (index == 0)
    {
        return;
    }
    else if (index == 1)
    {
        p0 = _controlPoints[0], p1 = _controlPoints[0], p2 = _controlPoints[1];
        if (_controlPoints.size() > 2)
            p3 = _controlPoints[2];
        else
            p3 = _controlPoints[1];
        newPoint = splineInterpolate(p0, p1, p2, p3, 0.5);
    }
    else if (index > (int)(_controlPoints.size() - 1))
    {
        return;
    }
    else if (index == (_controlPoints.size() - 1))
    {
        if (_controlPoints.size() > 2)
            p0 = _controlPoints[index-2];
        else
            p0 = _controlPoints[index-1];
        p1 = _controlPoints[index-1], p2 = _controlPoints[index], p3 = _controlPoints[index];
        newPoint = splineInterpolate(p0, p1, p2, p3, 0.5);
    }
    else
    {
        p0 = _controlPoints[index - 2], p1 = _controlPoints[index - 1], p2 = _controlPoints[index], p3 = _controlPoints[index + 1];
        newPoint = splineInterpolate(p0, p1, p2, p3, 0.5);
    }

    std::advance(itr, index);
    _controlPoints.insert(itr, newPoint);
}

void CatmullRomSpline::removeControlPoints(std::vector<int>& points)
{
    for (std::vector<int>::iterator itr = points.begin(); itr != points.end(); ++itr)
    {
        std::vector<QPointF>::iterator itrToRemove = _controlPoints.begin();
        std::advance(itrToRemove, *itr);
        _controlPoints.erase(itrToRemove);

        for (std::vector<int>::iterator itrRight = itr + 1; itrRight != points.end(); ++itrRight)
        {
            --(*itrRight);
        }
    }
}

void CatmullRomSpline::_doSample()
{
    _samplePoints = sample(_controlPoints, _segmentCount);
}

} // namespace CurveEditor
} // namespace nvidia