#ifndef CurveWidget_h__
#define CurveWidget_h__

#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <vector>
#include "Curve.h"

namespace nvidia {
namespace CurveEditor {

class CurveAttributeBase;
class CurveAttribute;
class CurveWidget;

class CurveEntity
{
    friend class CurveWidget;
    friend class CurveEditorMainWindow;
public:
    CurveEntity(CurveWidget* holder, CurveAttribute* attribute, const QColor& color);

    void setLocation(float location);
    void setValue(float value);

    void addControlPointsBeforeSelected();
    void addControlPointsAfterSelected();
    void removeSelectedControlPoints();
    void setTangentType(InterpolateMode mode);
    void setSmoothTangent();
    void setEaseInTangent();
    void setEaseOutTangent();

    int getControlPointCount();
    int getPickedControlPointIndex()                { return _pickedPoint; }
    std::vector<int>& getPickedControlPointIndexes()     { return _pickedPoints; }

    QRectF getBoundingBox();

    void mousePressEvent(QMouseEvent* e, bool& pickedPoint);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);

    void draw(QPainter &painter);

private:
    void _drawCurve(QPainter &painter);
    void _makePoints(std::vector<QPointF>& points);
    void _drawPoints(const std::vector<QPointF>& points, QPainter &painter, const std::vector<int>& pickedPoints);
    void _addCtrlPntByClick(const QPointF& mouseScreenPos);
    bool _canRemoveCtrlPntByRightClick(const QPointF& mouseScreenPos);
    void _removeCtrlPntByRightClick();

    CurveWidget*        _holder;
    CurveAttribute*     _attribute;
    Curve*              _curve;
    QColor              _color;
    int                 _pickedPoint;
    std::vector<int>    _pickedPoints;
    QPointF             _lastMousePosScreen;
    int                 _ctrlPntToRemove;
};

class CurveWidget : public QFrame
{
    Q_OBJECT

    friend class CurveEntity;

public:
    explicit CurveWidget(QWidget* parent);

    void setLocation(float location);
    void setValue(float value);

    void setCurveAttributes(std::vector<CurveAttributeBase*> attributes);
    void reset();

    void addControlPointsBeforeSelected();
    void addControlPointsAfterSelected();
    void removeSelectedControlPoints();
    void setTangentType(InterpolateMode mode);
    void setSmoothTangent();
    void setEaseInTangent();
    void setEaseOutTangent();

    void setSnapAll()   { _snapHorizontal = true; _snapVertical = true; }
    void setSnapHorizontal()   { _snapHorizontal = true; _snapVertical = false; }
    void setSnapVertical()   { _snapHorizontal = false; _snapVertical = true; }

    void increaseCurveScaleHorizontally();
    void decreaseCurveScaleHorizontally();
    void increaseCurveScaleVertically();
    void decreaseCurveScaleVertically();
    void frameCurveScaleHorizontally();
    void frameCurveScaleVertically();
    void frameCurveScale();

    void setAddCtrlPntByClick(bool value)       { _canAddCtrlPntByClick = value; }
    void setRemoveCtrlPntByClick(bool value)    { _canRemoveCtrlPntByClick = value; }

signals:
    void PickedControlPointValueChanged(QPointF& value);
    void PickedControlPointChanged(const std::vector<CurveEntity*> pickedCurves);
    void CurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute);

private slots:
    void onShowContextMenu(const QPoint& pos);
    void onRemoveControlPoint();

private:
    // QWidget events
    virtual void paintEvent(QPaintEvent * e);
    virtual void resizeEvent(QResizeEvent* e);

    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void wheelEvent(QWheelEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);

    void _updateCurveFitWindowPara();
    QPointF _valueToScreen(const QPointF& pnt);
    QPointF _screenToValue(const QPointF& pnt);
    void _drawAxis(QPainter &painter);
    void _drawGrid(QPainter &painter);
    QPointF _getSnapSreenPos(const QPointF& mouseScreenPos);
    QPointF _dragedCoordinateToScreen(const QPointF& pnt);
    QPointF _screenToDragedCoordinate(const QPointF& pnt);

private:
    QWidget*                    _parent;
    bool                        _moveCrossOtherCtrlPnt;// whether can move control point across other control points
    bool                        _moveCtrlPnt;
    bool                        _pan;
    bool                        _snapHorizontal;
    bool                        _snapVertical;
    bool                        _canAddCtrlPntByClick;
    bool                        _canRemoveCtrlPntByClick;
    std::vector<CurveEntity>    _curves;
    QPointF                     _curveFitWindowScale;
    QPointF                     _curveFitWindowOffset;
    QPointF                     _curveScaleLevel;
    QPointF                     _mousePressScreenPos;
    QPointF                     _lastOrinScreenPos;
    QPointF                     _curOrinScreenPos;

    QMenu*                      _contextMenu;
    QAction*                    _removeCtrlPntAction;
};

} // namespace CurveEditor
} // namespace nvidia

#endif // CurveWidget_h__
