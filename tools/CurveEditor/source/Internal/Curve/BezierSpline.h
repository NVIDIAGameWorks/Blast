#ifndef BEZIERSPLINE_H
#define BEZIERSPLINE_H
#include <vector>
#include <QtCore/QPointF>
#include "Spline.h"

namespace nvidia {
namespace CurveEditor {

class BezierSplinePoint
{
public:
    QPointF ctrlPoint0;
    QPointF valuePoint;
    QPointF ctrlPoint1;
};

class BezierSpline : public Spline
{ 
public:
    static std::vector<QPointF> sample(const std::vector<BezierSplinePoint>& splinePoints, long segmentCount = 100);

    BezierSpline(long segmentCount = 100);

    virtual ~BezierSpline() {}

    inline long getControlPointCount() {   return (long)_controlPoints.size();  }

    // index <= 0, return the first point;
    // index >= point count, return the last point.
    bool getControlPoint(int index, BezierSplinePoint &pt);

    // add a spline point to last
    void appendControlPoint(BezierSplinePoint pt);

    // insert a spline point at the identified position.
    // index <= 0, inert to fisrt;
    // index >= point count, insert to last.
    void insertControlPoint(int index, BezierSplinePoint pt);

    // change a spline point value at the identified position.
    bool setControlPoint(int index, BezierSplinePoint pt);

    // remove the identified spline point.
    bool removeControlPoint(int index);

private:
    virtual void _doSample();

private:
    std::vector<BezierSplinePoint>  _controlPoints;
};

} // namespace CurveEditor
} // namespace nvidia

#endif // BEZIERSPLINE_H
