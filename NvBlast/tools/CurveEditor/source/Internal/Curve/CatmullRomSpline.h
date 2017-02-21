#ifndef CatmullRomSpline_h__
#define CatmullRomSpline_h__

#include <QtCore/QPoint>
#include <vector>
#include "Spline.h"

namespace nvidia {
namespace CurveEditor {

class CatmullRomSpline : public Spline
{
public:
    static float splineInterpolate(float v0, float v1, float v2, float v3, float u);
    static QPointF splineInterpolate(const QPointF& p0, const QPointF& p1, const QPointF& p2, const QPointF& p3, float u);
    static std::vector<QPointF> sample(const std::vector<QPointF>& inControlPoints, int numSegments = 100);
private:
    static void _computeSplineBasis(float u, float* basis);

public:
    CatmullRomSpline(long segmentCount = 100);
    CatmullRomSpline(int numControlPoints, long segmentCount = 100);
    virtual ~CatmullRomSpline(void);

    int getControlPointCount()  {   return (int)_controlPoints.size(); }
    // add a spline point to last
    void appendControlPoint(QPointF point) { _controlPoints.push_back(point); }
    void setControlPoint(int index, const QPointF& point);
    QPointF getControlPoint(int index);
    std::vector<QPointF> getControlPoints();
    void reset(void);
    void insertControlPointAt(int index);
    void removeControlPoints(std::vector<int>& points);

private:
    virtual void _doSample();
private:
    std::vector<QPointF>         _controlPoints;
};

} // namespace CurveEditor
} // namespace nvidia

#endif // CatmullRomSpline_h__
