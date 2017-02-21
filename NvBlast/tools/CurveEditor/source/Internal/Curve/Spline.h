#ifndef Spline_h__
#define Spline_h__
#include <vector>
#include <QtCore/QPointF>

namespace nvidia {
namespace CurveEditor {

class Spline
{
public:
    Spline(long segmentCount = 100);
    virtual ~Spline(void) = 0;

    //  return the point which lies on the polyline after sample.
    // percent: the length between the returned point and the begin point along the polyline after sample
    //             ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
    //             the length between the end point and the begin point along the polyline after sample
    //    [0.0, 1.0]
    bool getPercentPoint(float percent, QPointF& point);

    bool getPointByX(float x, QPointF& point);

    void setSegmentCount(long segmentCount);

    std::vector<QPointF> getSamplePoints();

protected:
     void _sample();
     virtual void _doSample() = 0;

protected:
    bool                    _needSample;
    long                    _segmentCount;
    std::vector<QPointF>    _samplePoints;
    std::vector<float>      _edgeLengths; // the number of edges + 1 == the number of sample points
    float                   _totolLength;// the total length of all edges
};

} // namespace CurveEditor
} // namespace nvidia

#endif // Spline_h__
