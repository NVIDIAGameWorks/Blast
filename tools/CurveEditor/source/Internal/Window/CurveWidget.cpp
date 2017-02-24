#include "CurveWidget.h"

#include <QtWidgets/QLabel>
#include <QtGui/QImage>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtCore/QDebug>
#include "CurveEditorMainWindow.h"

namespace nvidia {
namespace CurveEditor {

const int MARGIN_X = 2;
const int MARGIN_Y = 2;
const int SNAP_DISTANCE = 5;
const float CROSS_THRESHOLD = 5.0f;
const int GRID_INTIAL_SEGMENT_COUNT = 4;
const int CTRL_PNT_ICON_SIZE = 6;
const int CLICK_ON_CURVE_THRESHOLD = 4;
const int CURVE_WIDTH = 2;
const QColor GRID_COLOR = QColor(32, 32, 32, 64);

//////////////////////////////////////////////////////////////////////////////
QPointF NDCToScreenCoord(QWidget* pWidget, QPointF p)
{
    int w = pWidget->width();
    int h = pWidget->height();

    float x = MARGIN_X + p.x() * (w - MARGIN_X * 2.0f);
    float y = h - MARGIN_Y * 2.0 + p.y() * (MARGIN_Y * 3.0f - h);

    return QPointF(x, y);
}

//////////////////////////////////////////////////////////////////////////////
QPointF ScreenToNDC(QWidget* pWidget, QPointF p)
{
    int w = pWidget->width();
    int h = pWidget->height();

    float sizey = 1.0f - 2.0f * MARGIN_Y;

    float x = (p.x() - MARGIN_X) / (2 - MARGIN_X * 2.0f);
    float y = (MARGIN_Y * 2.0f + p.y() - h) / (MARGIN_Y * 3.0f - h);

    return QPointF(x, y);
}

//////////////////////////////////////////////////////////////////////////////
bool IsInSelectedCtrlPnts(const std::vector<int>& pickedPoints, int pointIndex)
{
    for (std::vector<int>::const_iterator itr = pickedPoints.begin(); itr != pickedPoints.end(); ++itr)
    {
        if (*itr == pointIndex)
            return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
bool IsSamePoint(const QPointF& pntA, const QPointF& pntB, int threshold)
{
    float dx = pntB.x() - pntA.x();
    float dy = pntB.y() - pntA.y();

    float r = sqrt(dx * dx + dy * dy);
    if (r <= threshold)
        return true;
    else
        return false;
}

//////////////////////////////////////////////////////////////////////////////
int FindSelectedCtrlPnt(const std::vector<QPointF>& points, const QPointF& pickPos)
{
    for (size_t i = 0; i < points.size(); i++)
    {
        if (IsSamePoint(points[i], pickPos, CTRL_PNT_ICON_SIZE/2))
            return (int)i;
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////////
CurveEntity::CurveEntity(CurveWidget* holder, CurveAttribute* attribute, const QColor& color)
    : _holder(holder)
    , _attribute(attribute)
    , _curve(&(attribute->curve))
    , _color(color)
    , _pickedPoint(-1)
    , _pickedPoints()
    , _lastMousePosScreen()
    , _ctrlPntToRemove(-1)
{

}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::setLocation(float location)
{
    if (_pickedPoints.size() != 1)
        return ;

    ControlPoint pnt = _curve->getControlPoint(_pickedPoints[0]);
    pnt.value.setX(location);
    _curve->setControlPoint(_pickedPoints[0], pnt);
    emit _holder->CurveAttributeChanged(_attribute);
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::setValue(float value)
{
    if (_pickedPoints.size() != 1)
        return ;

    ControlPoint pnt = _curve->getControlPoint(_pickedPoints[0]);
    pnt.value.setY(value);
    _curve->setControlPoint(_pickedPoints[0], pnt);
    emit _holder->CurveAttributeChanged(_attribute);
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::addControlPointsBeforeSelected()
{
    bool update = false;
    for (std::vector<int>::iterator itr = _pickedPoints.begin(); itr != _pickedPoints.end(); ++itr)
    {
        if (*itr == 0)
            continue;

        _curve->insertControlPointAt(*itr);
        ++(*itr);
        update = true;

        if (*itr <= _pickedPoint)
        {
            ++_pickedPoint;
        }

        for (std::vector<int>::iterator itrRight = itr + 1; itrRight != _pickedPoints.end(); ++itrRight)
        {
            ++(*itrRight);
        }
    }

    if (update)
    {
        _curve->_needSample = true;
        emit _holder->CurveAttributeChanged(_attribute);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::addControlPointsAfterSelected()
{
    bool update = false;
    for (std::vector<int>::iterator itr = _pickedPoints.begin(); itr != _pickedPoints.end(); ++itr)
    {
        _curve->insertControlPointAt(*itr + 1);
        update = true;

        if ((*itr + 1) < _pickedPoint)
        {
            ++_pickedPoint;
        }

        for (std::vector<int>::iterator itrRight = itr + 1; itrRight != _pickedPoints.end(); ++itrRight)
        {
            ++(*itrRight);
        }
    }

    if (update)
    {
        _curve->_needSample = true;
        emit _holder->CurveAttributeChanged(_attribute);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::removeSelectedControlPoints()
{
    if (_pickedPoints.size() > 0)
    {
        _curve->removeControlPoints(_pickedPoints);
        _pickedPoints.clear();
        _pickedPoint = -1;
        _curve->_needSample = true;
        emit _holder->CurveAttributeChanged(_attribute);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::setTangentType(InterpolateMode mode)
{
    size_t pointsCount = _curve->getControlPointCount();
    for (int i = 0; i < pointsCount; i++)
    {
        if (IsInSelectedCtrlPnts(_pickedPoints, i))
        {
            ControlPoint ctrlPnt = _curve->getControlPoint(i);
            ctrlPnt.mode = mode;
            _curve->setControlPoint(i, ctrlPnt);
            emit _holder->CurveAttributeChanged(_attribute);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::setSmoothTangent()
{
    size_t pointsCount = _curve->getControlPointCount();
    for (int i = 0; i < pointsCount; i++)
    {
        if (IsInSelectedCtrlPnts(_pickedPoints, i) && i != pointsCount - 1)
        {
            ControlPoint ctrlctrlPntFront = _curve->getControlPoint(i);
            ctrlctrlPntFront.mode = eBezierSpline;
            ctrlctrlPntFront.splineData.outLen = 1.0;
            ctrlctrlPntFront.splineData.outTan = 0;
            _curve->setControlPoint(i, ctrlctrlPntFront);

            ControlPoint ctrlPntBehiand = _curve->getControlPoint(i + 1);
            ctrlPntBehiand.mode = eBezierSpline;
            ctrlPntBehiand.splineData.inLen = 1.0;
            ctrlPntBehiand.splineData.inTan = 0.0;
            _curve->setControlPoint(i + 1, ctrlPntBehiand);

            emit _holder->CurveAttributeChanged(_attribute);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::setEaseInTangent()
{
    size_t pointsCount = _curve->getControlPointCount();
    for (int i = 0; i < pointsCount; i++)
    {
        if (IsInSelectedCtrlPnts(_pickedPoints, i) && i != pointsCount - 1)
        {
            ControlPoint ctrlctrlPntFront = _curve->getControlPoint(i);
            ctrlctrlPntFront.mode = eBezierSpline;
            ctrlctrlPntFront.splineData.outLen = 0.0;
            ctrlctrlPntFront.splineData.outTan = 0;
            _curve->setControlPoint(i, ctrlctrlPntFront);

            ControlPoint ctrlPntBehiand = _curve->getControlPoint(i + 1);
            ctrlPntBehiand.mode = eBezierSpline;
            ctrlPntBehiand.splineData.inLen = 1.0;
            ctrlPntBehiand.splineData.inTan = 0;
            _curve->setControlPoint(i + 1, ctrlPntBehiand);

            emit _holder->CurveAttributeChanged(_attribute);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::setEaseOutTangent()
{
    size_t pointsCount = _curve->getControlPointCount();
    for (int i = 0; i < pointsCount; i++)
    {
        if (IsInSelectedCtrlPnts(_pickedPoints, i) && i != pointsCount - 1)
        {
            ControlPoint ctrlctrlPntFront = _curve->getControlPoint(i);
            ctrlctrlPntFront.mode = eBezierSpline;
            ctrlctrlPntFront.splineData.outLen = 1.0;
            ctrlctrlPntFront.splineData.outTan = 0;
            _curve->setControlPoint(i, ctrlctrlPntFront);

            ControlPoint ctrlPntBehiand = _curve->getControlPoint(i + 1);
            ctrlPntBehiand.mode = eBezierSpline;
            ctrlPntBehiand.splineData.inLen = 0.0;
            ctrlPntBehiand.splineData.inTan = 0;
            _curve->setControlPoint(i + 1, ctrlPntBehiand);

            emit _holder->CurveAttributeChanged(_attribute);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
int CurveEntity::getControlPointCount()
{
    return _curve->getControlPointCount();
}

//////////////////////////////////////////////////////////////////////////////
QRectF CurveEntity::getBoundingBox()
{
    return QRectF(_curve->getMinValue(), _curve->getMaxValue());
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::mousePressEvent( QMouseEvent* e, bool& pickedPoint)
{
    if (!(e->modifiers() & Qt::ControlModifier))
    {
        _pickedPoints.clear();
    }

    std::vector<QPointF> points;
    _makePoints(points);
    QPoint mousePos = e->pos();
    _lastMousePosScreen = mousePos;

    _pickedPoint = FindSelectedCtrlPnt(points, mousePos);
    if (-1 != _pickedPoint)
    {
        if (_holder->_canRemoveCtrlPntByClick && _attribute->canAddRemoveControlPoint())
        {
            _curve->removeControlPoint(_pickedPoint);
            _pickedPoint = -1;
            return;
        }
        pickedPoint = true;
    }
    else 
    {
        if (_holder->_canAddCtrlPntByClick)
        {
            _addCtrlPntByClick(mousePos);
        }
        return ;
    }

    std::vector<int>::iterator itr = _pickedPoints.begin();
    for (; itr != _pickedPoints.end(); ++itr)
    {
        if (*itr == _pickedPoint)
            break;
        else if (*itr > _pickedPoint)
        {
            itr = _pickedPoints.insert(itr, _pickedPoint);
            break;
        }
    }

    if (itr == _pickedPoints.end())
    {
        _pickedPoints.insert(itr, _pickedPoint);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::mouseReleaseEvent( QMouseEvent* e )
{
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::mouseMoveEvent( QMouseEvent* e )
{
    Qt::MouseButton buttons = e->button();

    {
        //if (_pickedPoint >= 0)
        QPointF mousePosScreen = e->pos();
        QPointF deltaPosScreen = mousePosScreen - _lastMousePosScreen;
        QPointF destinationPosScreen = mousePosScreen;
        _lastMousePosScreen = mousePosScreen;
        std::vector<int>::iterator itr = _pickedPoints.begin();
        for (; itr != _pickedPoints.end(); ++itr)
        {
            int pickedPoint = *itr;
            ControlPoint pickedCtrlPnt = _curve->getControlPoint(pickedPoint);
            QPointF ctrlPntLastScreenPos = _holder->_valueToScreen(pickedCtrlPnt.value);

            destinationPosScreen = ctrlPntLastScreenPos + deltaPosScreen;
            destinationPosScreen = _holder->_getSnapSreenPos(destinationPosScreen);

            if (!_attribute->canMoveControlPointHorizontally())
                destinationPosScreen.setX(ctrlPntLastScreenPos.x());
            else
            {
                // make sure this picked control point not move accross other control point
                if (pickedPoint == 0)
                {
                    QPointF nextCtrlPntScreenPos = _holder->_valueToScreen(_curve->getControlPoint(pickedPoint + 1).value);
                    if (destinationPosScreen.x() > (nextCtrlPntScreenPos.x() - CROSS_THRESHOLD))
                        destinationPosScreen.setX(nextCtrlPntScreenPos.x() - CROSS_THRESHOLD);
                }
                else if (pickedPoint == (_curve->getControlPointCount() - 1) )
                {
                    QPointF fomerCtrlPntScreenPos = _holder->_valueToScreen(_curve->getControlPoint(pickedPoint - 1).value);
                    if (destinationPosScreen.x() < (fomerCtrlPntScreenPos.x() + CROSS_THRESHOLD))
                        destinationPosScreen.setX(fomerCtrlPntScreenPos.x() + CROSS_THRESHOLD);
                }
                else
                {
                    QPointF fomerCtrlPntScreenPos = _holder->_valueToScreen(_curve->getControlPoint(pickedPoint - 1).value);
                    QPointF nextCtrlPntScreenPos = _holder->_valueToScreen(_curve->getControlPoint(pickedPoint + 1).value);
                    if (destinationPosScreen.x() < (fomerCtrlPntScreenPos.x() + CROSS_THRESHOLD))
                        destinationPosScreen.setX(fomerCtrlPntScreenPos.x() + CROSS_THRESHOLD);
                    if (destinationPosScreen.x() > (nextCtrlPntScreenPos.x() - CROSS_THRESHOLD))
                        destinationPosScreen.setX(nextCtrlPntScreenPos.x() - CROSS_THRESHOLD);
                }
            }

            QPointF p = _holder->_screenToValue(destinationPosScreen);

            QPointF min = _curve->getMinValue();
            QPointF max = _curve->getMaxValue();

            //make control point move in value range
            {
                if (p.x() < min.x())
                    p.setX(min.x());
                else if (p.x() > max.x())
                    p.setX(max.x());

                if (p.y() < min.y())
                    p.setY(min.y());
                else if (p.y() > max.y())
                    p.setY(max.y());
            }

            pickedCtrlPnt.value = p;
            _curve->setControlPoint(pickedPoint, pickedCtrlPnt);
            emit _holder->PickedControlPointValueChanged(p);
            
            emit _holder->CurveAttributeChanged(_attribute);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::draw(QPainter &painter)
{
    std::vector<QPointF> points;
    _makePoints(points);

    _drawCurve(painter);

    _drawPoints(points, painter, _pickedPoints);
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::_drawCurve(QPainter &painter)
{
    painter.save();

    {
        QPen pen( Qt::black );
        pen.setColor(_color);
        pen.setWidth(CURVE_WIDTH);
        pen.setStyle(Qt::SolidLine);

        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }

    QPainterPath path;

    std::vector<QPointF> samplePoints = _curve->getSamplePoints();
    path.moveTo(_holder->_valueToScreen(samplePoints[0]));

    for (size_t i = 0; i < samplePoints.size(); ++i)
    {
        path.lineTo(_holder->_valueToScreen(samplePoints[i]));
    }
    painter.drawPath(path);

    painter.restore();
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::_makePoints(std::vector<QPointF>& points)
{
    size_t ctrlPntsCount = _curve->getControlPointCount();
    for (int i = 0; i < ctrlPntsCount; i++)
        points.push_back( _holder->_valueToScreen(_curve->getControlPoint(i).value) );
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::_drawPoints(const std::vector<QPointF>& points, QPainter &painter, const std::vector<int>& pickedPoints)
{
    {
        QPen pen( Qt::black );
        pen.setColor(QColor(255,255,255));
        pen.setWidth(6);
        pen.setStyle(Qt::SolidLine);

        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }

    for (int i = 0; i < points.size(); i++)
    {
        QPen pen( Qt::black );
        pen.setWidth(CTRL_PNT_ICON_SIZE);
        pen.setStyle(Qt::SolidLine);

        if (IsInSelectedCtrlPnts(pickedPoints,i))
            pen.setColor(QColor(255,0,0));
        else
            pen.setColor(QColor(255,255,255));

        painter.setPen(pen);
        painter.drawPoint( points[i] );
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveEntity::_addCtrlPntByClick(const QPointF& mouseScreenPos)
{
    if (!_attribute->canAddRemoveControlPoint())
        return ;

    QPointF pos= _holder->_screenToValue(mouseScreenPos);

    QPointF pntOnCurve = _curve->getPointByX(pos.x());
    QPointF pntOnCurveScreen = _holder->_valueToScreen(pntOnCurve);

    if ( IsSamePoint(mouseScreenPos, pntOnCurveScreen, CLICK_ON_CURVE_THRESHOLD) )
    {
        _pickedPoint = _curve->appendControlPoint(pntOnCurve.x());
        _holder->update();
    }

}

//////////////////////////////////////////////////////////////////////////////
bool CurveEntity::_canRemoveCtrlPntByRightClick(const QPointF& mouseScreenPos)
{
    std::vector<QPointF> points;
    _makePoints(points);

    _ctrlPntToRemove = FindSelectedCtrlPnt(points, mouseScreenPos);

    if ( !_attribute->canAddRemoveControlPoint()
        || (-1 == _ctrlPntToRemove)
        || 2 == _curve->_controlPoints.size() && -1 != _ctrlPntToRemove)
    {
        return false;
    }
    else
        return true;
}

void CurveEntity::_removeCtrlPntByRightClick()
{
    if (-1 != _ctrlPntToRemove && _attribute->canAddRemoveControlPoint())
    {
        _curve->removeControlPoint(_ctrlPntToRemove);
    }
    _ctrlPntToRemove = -1;
    _pickedPoint = -1;
    _pickedPoints.clear();
}

//////////////////////////////////////////////////////////////////////////////
CurveWidget::CurveWidget(QWidget* parent)
    : QFrame(parent)
    , _parent(parent)
    , _moveCrossOtherCtrlPnt(false)
    , _moveCtrlPnt(false)
    , _pan(false)
    , _snapHorizontal(false)
    , _snapVertical(false)
    , _canAddCtrlPntByClick(false)
    , _canRemoveCtrlPntByClick(false)
    , _curveFitWindowScale(1.0, 1.0)
    , _curveFitWindowOffset(0.0, 0.0)
    , _curveScaleLevel(1.0, 1.0)
    , _contextMenu(nullptr)
    , _removeCtrlPntAction(nullptr)
{
    setFocusPolicy(Qt::ClickFocus );
    //QString focusStyle = QString("QFrame:focus { border:1px solid #FF0000; }");
    //setStyleSheet(focusStyle);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onShowContextMenu(const QPoint&)));

    _contextMenu = new QMenu(this);;
    _removeCtrlPntAction = new QAction(this);
    _removeCtrlPntAction->setText(tr("Remove Control Point"));
    _contextMenu->addAction(_removeCtrlPntAction);

    connect(_removeCtrlPntAction, SIGNAL(triggered()), this, SLOT(onRemoveControlPoint()) );
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::onShowContextMenu(const QPoint& pos)
{
    bool enableRemoveCtrlPntAction = false;
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        if (curveEntity._canRemoveCtrlPntByRightClick(pos))
            enableRemoveCtrlPntAction = true;
    }

    _removeCtrlPntAction->setEnabled(enableRemoveCtrlPntAction);
    _contextMenu->exec(QCursor::pos());
}

void CurveWidget::onRemoveControlPoint()
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity._removeCtrlPntByRightClick();
    }

    std::vector<CurveEntity*> pickedCurves;
    emit PickedControlPointChanged(pickedCurves);
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::paintEvent(QPaintEvent * e)
{
    QFrame::paintEvent(e);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing, true);

    _drawGrid(painter);

    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.draw(painter);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::resizeEvent(QResizeEvent* e)
{
    _updateCurveFitWindowPara();

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::mousePressEvent( QMouseEvent* e )
{
    if (e->buttons() == Qt::MiddleButton)
    {
        setCursor(Qt::OpenHandCursor);
        _mousePressScreenPos = QPointF(e->pos().x(), e->pos().y());
        _pan = true;
    }
    else
    {
        std::vector<CurveEntity*> pickedCurves;
        size_t curveCount = _curves.size();
        for (size_t i = 0; i < curveCount; ++i)
        {
            CurveEntity& curveEntity = _curves[i];
            curveEntity.mousePressEvent(e, _moveCtrlPnt);
            if (curveEntity.getPickedControlPointIndexes().size() > 0)
                pickedCurves.push_back(&curveEntity);
        }

        emit PickedControlPointChanged(pickedCurves);

        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::mouseReleaseEvent( QMouseEvent* e )
{
    if (_moveCtrlPnt)
    {
        size_t curveCount = _curves.size();
        for (size_t i = 0; i < curveCount; ++i)
        {
            CurveEntity& curveEntity = _curves[i];
            curveEntity.mouseReleaseEvent(e);
        }
    }
    else if (_pan)
    {
        setCursor(Qt::ArrowCursor);
        _curOrinScreenPos = _lastOrinScreenPos + (QPointF(e->pos().x() - _mousePressScreenPos.x(), _mousePressScreenPos.y() - e->pos().y()));
        _lastOrinScreenPos = _curOrinScreenPos;
        _updateCurveFitWindowPara();
        
    }

    _moveCtrlPnt = false;
    _pan = false;
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::mouseMoveEvent( QMouseEvent* e )
{
    if (_moveCtrlPnt)
    {
        size_t curveCount = _curves.size();
        for (size_t i = 0; i < curveCount; ++i)
        {
            CurveEntity& curveEntity = _curves[i];
            curveEntity.mouseMoveEvent(e);
        }
    }
    else if (e->buttons() & Qt::MiddleButton)
    {
        _curOrinScreenPos = _lastOrinScreenPos + (QPointF(e->pos().x() - _mousePressScreenPos.x(), _mousePressScreenPos.y() - e->pos().y()));
        _updateCurveFitWindowPara();
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::wheelEvent(QWheelEvent *e)
{
    int numDegrees = e->delta() / 8;

    float zoom = 1.0 + numDegrees / 360.0f;
    _curveScaleLevel.setX(_curveScaleLevel.x() * zoom);
    _curveScaleLevel.setY(_curveScaleLevel.y() * zoom);

    _updateCurveFitWindowPara();
    update();
    e->accept();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::keyPressEvent(QKeyEvent *e)
{
    bool move = false;
    switch(e->key())
    {
    case Qt::Key_Left:
        _curOrinScreenPos = _lastOrinScreenPos + QPointF(-1.0f, 0.0f);
        _lastOrinScreenPos = _curOrinScreenPos;
        move = true;
        break;
    case Qt::Key_Up:
        _curOrinScreenPos = _lastOrinScreenPos + QPointF(0.0f, 1.0f);
        _lastOrinScreenPos = _curOrinScreenPos;
        move = true;
        break;
    case Qt::Key_Right:
        _curOrinScreenPos = _lastOrinScreenPos + QPointF(1.0f, 0.0f);
        _lastOrinScreenPos = _curOrinScreenPos;
        move = true;
        break;
    case Qt::Key_Down:
        _curOrinScreenPos = _lastOrinScreenPos + QPointF(0.0f, -1.0f);
        _lastOrinScreenPos = _curOrinScreenPos;
        move = true;
        break;
    }

    if (move)
    {
        _updateCurveFitWindowPara();
        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::setLocation(float location)
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.setLocation(location);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::setValue(float value)
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.setValue(value);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::setCurveAttributes(std::vector<CurveAttributeBase*> attributes)
{
    _curves.clear();

    for (std::vector<CurveAttributeBase*>::iterator itr = attributes.begin(); itr != attributes.end(); ++itr)
    {
        CurveAttributeBase* attribute = *itr;
        if (attribute->getType() == eSingleAttr)
        {
            CurveAttribute* attributeSpecific = static_cast<CurveAttribute*>(attribute);

            CurveEntity curveEntity(this, attributeSpecific, attributeSpecific->color);
            _curves.push_back(curveEntity);
        }
        else if (attribute->getType() == eGroupAttr)
        {
            CurveAttributeGroup* attributeGroup = static_cast<CurveAttributeGroup*>(attribute);
            size_t countAttributesInGroup = attributeGroup->attributes.size();
            for (size_t i = 0; i < countAttributesInGroup; ++i)
            {
                CurveAttribute* attributeInGroup = attributeGroup->attributes[i];

                CurveEntity curveEntity(this, attributeInGroup, attributeInGroup->color);
                _curves.push_back(curveEntity);
            }
        }
    }

    _updateCurveFitWindowPara();

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::reset()
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity._attribute->curve.reset();
        emit CurveAttributeChanged(curveEntity._attribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::addControlPointsBeforeSelected()
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.addControlPointsBeforeSelected();
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::addControlPointsAfterSelected()
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.addControlPointsAfterSelected();
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::removeSelectedControlPoints()
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.removeSelectedControlPoints();
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::setTangentType(InterpolateMode mode)
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.setTangentType(mode);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::setSmoothTangent()
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.setSmoothTangent();
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::setEaseInTangent()
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.setEaseInTangent();
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::setEaseOutTangent()
{
    size_t curveCount = _curves.size();
    for (size_t i = 0; i < curveCount; ++i)
    {
        CurveEntity& curveEntity = _curves[i];
        curveEntity.setEaseOutTangent();
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::increaseCurveScaleHorizontally()
{
    _curveScaleLevel.setX(_curveScaleLevel.x() * 2);
    _updateCurveFitWindowPara();
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::decreaseCurveScaleHorizontally()
{
    _curveScaleLevel.setX(_curveScaleLevel.x() / 2);
    _updateCurveFitWindowPara();
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::increaseCurveScaleVertically()
{
    _curveScaleLevel.setY(_curveScaleLevel.y() * 2);
    _updateCurveFitWindowPara();
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::decreaseCurveScaleVertically()
{
    _curveScaleLevel.setY(_curveScaleLevel.y() / 2);
    _updateCurveFitWindowPara();
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::frameCurveScaleHorizontally()
{
    _curveScaleLevel.setX(1.0f);
    _curOrinScreenPos.setX(0);
    _lastOrinScreenPos.setX(0);
    _updateCurveFitWindowPara();
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::frameCurveScaleVertically()
{
    _curveScaleLevel.setY(1.0f);
    _curOrinScreenPos.setY(0);
    _lastOrinScreenPos.setY(0);
    _updateCurveFitWindowPara();
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::frameCurveScale()
{
    _curveScaleLevel.setX(1.0f);
    _curveScaleLevel.setY(1.0f);
    _curOrinScreenPos = QPointF(0,0);
    _lastOrinScreenPos = QPointF(0,0);
    _updateCurveFitWindowPara();
    update();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::_updateCurveFitWindowPara()
{
    QRect winSize = rect();

    float minX = 0;
    float minY = 0;
    float maxX = 0;
    float maxY = 0;

    for (size_t i = 0; i < _curves.size(); ++i)
    {
        CurveEntity& curveEntity = _curves[i];

        QRectF boundingBox = curveEntity.getBoundingBox();

        if (minX > boundingBox.x())
            minX = boundingBox.x();

        if (minY > boundingBox.y())
            minY = boundingBox.y();

        if (maxX < boundingBox.right())
            maxX = boundingBox.right();

        if (maxY < boundingBox.bottom())
            maxY = boundingBox.bottom();
    }

    float curveWidth = maxX - minX;
    float curveHeight = maxY - minY;

    if (0 == curveWidth && 0 == curveHeight)
    {
        curveWidth = 1.0;
        curveHeight = 1.0;
    }
    else if (0 == curveWidth)
    {
        curveWidth = curveHeight;
    }
    else if (0 == curveHeight)
    {
        curveHeight = curveWidth;
    }

    float widthScale = (winSize.width() - MARGIN_X * 2.0f) / curveWidth;
    float heightScale = (MARGIN_Y * 2.0f - winSize.height()) / curveHeight;
    _curveFitWindowScale = QPointF(widthScale, heightScale);

    float widthOffset = MARGIN_X + _curOrinScreenPos.x() - minX * widthScale * _curveScaleLevel.x();
    float heightOffset = (winSize.height() - MARGIN_Y - _curOrinScreenPos.y())- minY * heightScale * _curveScaleLevel.y();
    _curveFitWindowOffset = QPointF(widthOffset, heightOffset);
}

//////////////////////////////////////////////////////////////////////////////
QPointF CurveWidget::_valueToScreen(const QPointF& pnt)
{
    QPointF curveScale = QPointF(_curveFitWindowScale.x() * _curveScaleLevel.x(), _curveFitWindowScale.y() * _curveScaleLevel.y());

    float x = pnt.x() * curveScale.x() + _curveFitWindowOffset.x();
    float y = pnt.y() * curveScale.y() + _curveFitWindowOffset.y();
    return QPointF(x, y);
}

//////////////////////////////////////////////////////////////////////////////
QPointF CurveWidget::_screenToValue(const QPointF& pnt)
{
    QPointF curveScale = QPointF(_curveFitWindowScale.x() * _curveScaleLevel.x(), _curveFitWindowScale.y() * _curveScaleLevel.y());

    float x = (pnt.x() - _curveFitWindowOffset.x()) / curveScale.x();
    float y = (pnt.y() - _curveFitWindowOffset.y()) / curveScale.y();

    return QPointF(x, y);
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::_drawAxis(QPainter &painter)
{
    painter.save();
    {
        QPen pen( Qt::black );
        pen.setColor(GRID_COLOR);
        pen.setWidth(2);
        pen.setStyle(Qt::SolidLine);

        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }

    {
        int w = width() -  MARGIN_X * 2;
        int h = height() -  MARGIN_Y * 2;

        QPainterPath path;

        QPointF x = _dragedCoordinateToScreen(QPointF(w, 0));
        QPointF y = _dragedCoordinateToScreen(QPointF(0, h));

        path.moveTo(_dragedCoordinateToScreen(QPointF(0, 0)));
        path.lineTo(x);
        path.moveTo(_dragedCoordinateToScreen(QPointF(0, 0)));
        path.lineTo(y);

        painter.drawPath(path);
    }

    painter.restore();
}

//////////////////////////////////////////////////////////////////////////////
void CurveWidget::_drawGrid(QPainter &painter)
{
    painter.save();
    {
        QPen pen( Qt::black );
        pen.setColor(GRID_COLOR);
        pen.setWidth(1);
        pen.setStyle(Qt::SolidLine);

        painter.setPen(pen);
        painter.setRenderHint(QPainter::Antialiasing, false);
    }

    {
        QPainterPath path;

        int w = width() -  MARGIN_X * 2;
        int h = height() -  MARGIN_Y * 2;

        QPointF newOri = _dragedCoordinateToScreen(_curOrinScreenPos);

        float width = w;
        float height = h;
        int gridXSegmentCount = GRID_INTIAL_SEGMENT_COUNT / _curveScaleLevel.x();
        int gridYSegmentCount = GRID_INTIAL_SEGMENT_COUNT / _curveScaleLevel.y();

        if (_curveScaleLevel.x() > 1)
        {
            width = w * _curveScaleLevel.x();
            gridXSegmentCount *= _curveScaleLevel.x();
        }

        if (_curveScaleLevel.y() > 1)
        {
            height = h * _curveScaleLevel.y();
            gridYSegmentCount *= _curveScaleLevel.y();
        }

        // draw horizontal lines
        for (int i = 0 - gridXSegmentCount; i <= gridXSegmentCount * 2; ++i)
        {
            QPointF bottom    = _dragedCoordinateToScreen(QPointF(width * i / (float)gridXSegmentCount, -height));
            QPointF up        = _dragedCoordinateToScreen(QPointF(width * i / (float)gridXSegmentCount, height * 2));
            path.moveTo(bottom);
            path.lineTo(up);

            painter.drawPath(path);
        }

        // draw vertical lines
        for (int i = 0 - gridYSegmentCount; i <= gridYSegmentCount * 2; ++i)
        {
            QPointF left    = _dragedCoordinateToScreen(QPointF(-width, i * height / (float)gridYSegmentCount));
            QPointF right        = _dragedCoordinateToScreen(QPointF(2 * width, i * height / (float)gridYSegmentCount));
            path.moveTo(left);
            path.lineTo(right);

            painter.drawPath(path);
        }

    }
    painter.restore();
}

//////////////////////////////////////////////////////////////////////////////
QPointF CurveWidget::_getSnapSreenPos(const QPointF& mouseScreenPos)
{
    int w = width() -  MARGIN_X * 2;
    int h = height() -  MARGIN_Y * 2;

    QPointF pos = _screenToDragedCoordinate(mouseScreenPos);

    // snap horizontally
    if (_snapHorizontal)
    {
        int gridXSegmentCount = GRID_INTIAL_SEGMENT_COUNT / _curveScaleLevel.x();
        if (gridXSegmentCount < 2)
            gridXSegmentCount = 2;
        for (int i = 0; i <= gridXSegmentCount; ++i)
        {
            if (abs(i * w / gridXSegmentCount - pos.x()) <= SNAP_DISTANCE )
            {
                pos.setX(i * w / gridXSegmentCount);
                break;
            }
        }
    }

    // snap vertically
    if (_snapVertical)
    {
        int gridYSegmentCount = GRID_INTIAL_SEGMENT_COUNT / _curveScaleLevel.y();
        if (gridYSegmentCount < 2)
            gridYSegmentCount = 2;
        for (int i = 0; i <= gridYSegmentCount; ++i)
        {
            if (abs(i * h / gridYSegmentCount - pos.y()) <= SNAP_DISTANCE)
            {
                pos.setY(i * h / gridYSegmentCount);
            }
        }
    }

    return _dragedCoordinateToScreen(pos);
}

//////////////////////////////////////////////////////////////////////////////
QPointF CurveWidget::_dragedCoordinateToScreen(const QPointF& pnt)
{
    float x = pnt.x() + MARGIN_X + _curOrinScreenPos.x();
    float y = height() -  MARGIN_Y - pnt.y() - _curOrinScreenPos.y();
    return QPointF(x, y);
}

//////////////////////////////////////////////////////////////////////////////
QPointF CurveWidget::_screenToDragedCoordinate(const QPointF& pnt)
{
    float x = pnt.x() - MARGIN_X - _curOrinScreenPos.x();
    float y = height() -  MARGIN_Y - pnt.y() - _curOrinScreenPos.y();
    return QPointF(x, y); 
}

} // namespace CurveEditor
} // namespace nvidia