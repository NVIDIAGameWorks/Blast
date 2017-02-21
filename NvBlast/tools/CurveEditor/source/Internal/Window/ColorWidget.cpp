#include "ColorWidget.h"
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QImage>
#include <assert.h>
#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <QtWidgets/QToolTip>
#include "CurveEditorMainWindow.h"

namespace nvidia {
namespace CurveEditor {

const int MARGIN_X = 8;
const int MARGIN_Y = 4;
const int CONTROL_POINT_CURSOR_W = 12;
const int CONTROL_POINT_CURSOR_H = 12;
const int WEIGHT_CURSOR_W = 6;
const int WEIGHT_CURSOR_H = 6;
const int SPACING_V = 0;

static QImage S_CtrlPntWithTexCursorNormalImg;
static QImage S_CtrlPntWithTexCursorPickedImg;
static QImage S_WeightCursorNormalImg;
static QImage S_WeightCursorPickedImg;

#define ROUND(v) (int(v + 0.5))

class TimeConsumeTracker
{
public:
    TimeConsumeTracker(QString text)
        : _text(text)
    {
        _time.start();
    }

    ~TimeConsumeTracker()
    {
        qDebug()<<_text<<" consume: "<<_time.elapsed();
    }

    QTime       _time; 
    QString     _text;
};

//////////////////////////////////////////////////////////////////////////////
void InitCursorResources()
{
    int w = CONTROL_POINT_CURSOR_W;
    int h = CONTROL_POINT_CURSOR_H;
    S_CtrlPntWithTexCursorNormalImg = QImage(w, h, QImage::Format_ARGB32);
    S_CtrlPntWithTexCursorPickedImg  = QImage(w, h, QImage::Format_ARGB32);
    S_CtrlPntWithTexCursorNormalImg.fill(QColor(0, 0, 0, 0));
    S_CtrlPntWithTexCursorPickedImg.fill(QColor(0, 0, 0, 0));

    ///////////////////////////////////////////////////////////////////////////
    QPainter painter(&S_CtrlPntWithTexCursorNormalImg);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;    // trick to clear up a path
    path.moveTo(w >> 1, 0);
    path.lineTo(w >> 2, h >> 2);
    path.lineTo(w >> 1, h >> 1);
    path.lineTo((w >> 1) + (w >> 2), h >> 2);
    path.lineTo(w >> 1, 0);
    painter.setPen(Qt::NoPen);
    painter.fillPath(path, QBrush(QColor(255, 255, 255)));

    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w >> 2, h >> 2);
    path.lineTo(0, h >> 1);
    path.lineTo(w >> 2, (h >> 1) + (h >> 2));
    path.lineTo(w >> 1, h >> 1);
    path.lineTo(w >> 2, h >> 2);
    painter.setPen(Qt::NoPen);
    painter.fillPath(path, (QColor(0, 0, 0)));

    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w >> 1, h >> 1);
    path.lineTo(w >> 2, (h >> 1) + (h >> 2));
    path.lineTo(w >> 1, h);
    path.lineTo((w >> 1) + (w >> 2), (h >> 1) + (h >> 2));
    path.lineTo(w >> 1, h >> 1);
    painter.setPen(Qt::NoPen);
    painter.fillPath(path, QBrush(QColor(255, 255, 255)));

    path = QPainterPath();    // trick to clear up a path
    path.moveTo((w >> 1) + (w >> 2), h >> 2);
    path.lineTo(w >> 1, h >> 1);
    path.lineTo((w >> 1) + (w >> 2), (h >> 1) + (h >> 2));
    path.lineTo(w, h >> 1);
    path.lineTo((w >> 1) + (w >> 2), h >> 2);
    painter.setPen(Qt::NoPen);
    painter.fillPath(path, (QColor(0, 0, 0)));

    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w >> 1, 0);
    path.lineTo(0, h >> 1);
    path.lineTo(w >> 1, h);
    path.lineTo(w, h >> 1);
    path.lineTo(w >> 1, 0);
    painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));
    painter.drawPath(path);

    ///////////////////////////////////////////////////////////////////////////
    // a painter cannot switch device?
    QPainter painter2(&S_CtrlPntWithTexCursorPickedImg);
    painter2.setRenderHint(QPainter::Antialiasing,true);
    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w>>1, 0);
    path.lineTo(w>>2, h>>2);
    path.lineTo(w>>1, h>>1);
    path.lineTo((w>>1)+(w>>2), h>>2);
    path.lineTo(w>>1, 0);
    painter2.setPen(Qt::NoPen);
    painter2.fillPath(path, QBrush(QColor(255, 255, 255)));

    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w>>2, h>>2);
    path.lineTo(0, h>>1);
    path.lineTo(w>>2, (h>>1)+(h>>2));
    path.lineTo(w>>1, h>>1);
    path.lineTo(w>>2, h>>2);
    painter2.setPen(Qt::NoPen);
    painter2.fillPath(path, (QColor(0, 0, 0)));

    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w>>1, h>>1);
    path.lineTo(w>>2, (h>>1)+(h>>2));
    path.lineTo(w>>1, h);
    path.lineTo((w>>1)+(w>>2), (h>>1)+(h>>2));
    path.lineTo(w>>1, h>>1);
    painter2.setPen(Qt::NoPen);
    painter2.fillPath(path, QBrush(QColor(255, 255, 255)));

    path = QPainterPath();    // trick to clear up a path
    path.moveTo((w>>1)+(w>>2), h>>2);
    path.lineTo(w>>1, h>>1);
    path.lineTo((w>>1)+(w>>2), (h>>1)+(h>>2));
    path.lineTo(w, h>>1);
    path.lineTo((w>>1)+(w>>2), h>>2);
    painter2.setPen(Qt::NoPen);
    painter2.fillPath(path, (QColor(0, 0, 0)));

    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w>>1, 0);
    path.lineTo(0, h>>1);
    path.lineTo(w>>1, h);
    path.lineTo(w, h>>1);
    path.lineTo(w>>1, 0);
    painter2.setPen(QPen(Qt::green,1, Qt::SolidLine));
    painter2.drawPath(path);

    ///////////////////////////////////////////////////////////////////////////
    w = WEIGHT_CURSOR_W;
    h = WEIGHT_CURSOR_H;
    S_WeightCursorNormalImg = QImage(w, h, QImage::Format_ARGB32);
    S_WeightCursorPickedImg  = QImage(w, h, QImage::Format_ARGB32);
    S_WeightCursorNormalImg.fill(QColor(0, 0, 0, 0));
    S_WeightCursorPickedImg.fill(QColor(0, 0, 0, 0));
    ///////////////////////////////////////////////////////////////////////////
    QPainter painter3(&S_WeightCursorNormalImg);
    painter3.setRenderHints(QPainter::Antialiasing,true);

    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w>>1, 0);
    path.lineTo(0, h>>1);
    path.lineTo(w>>1, h);
    path.lineTo(w, h>>1);
    path.lineTo(w>>1, 0);
    painter3.setPen(QPen(Qt::black, 1, Qt::SolidLine));
    painter3.drawPath(path);
    painter3.fillPath(path, QBrush(QColor(255, 255, 255)));

    ///////////////////////////////////////////////////////////////////////////
    QPainter painter4(&S_WeightCursorPickedImg);
    painter4.setRenderHints(QPainter::Antialiasing,true);

    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w>>1, 0);
    path.lineTo(0, h>>1);
    path.lineTo(w>>1, h);
    path.lineTo(w, h>>1);
    path.lineTo(w>>1, 0);
    painter4.setPen(QPen(Qt::black, 1, Qt::SolidLine));
    painter4.drawPath(path);
    painter4.fillPath(path, QBrush(QColor(0, 0, 0)));
}

//////////////////////////////////////////////////////////////////////////////
float NDCToScreenCoord(QWidget* pWidget, float xValueOfUnitInterval)
{
    int w = pWidget->width();
    return MARGIN_X + (w - MARGIN_X * 2.0) * xValueOfUnitInterval;
}

//////////////////////////////////////////////////////////////////////////////
float _screenToNDC(QWidget* pWidget, int xPos)
{
    int w = pWidget->width();
    return ((float)(xPos - MARGIN_X)) / (w - MARGIN_X * 2.0f);
}

//////////////////////////////////////////////////////////////////////////////
bool isClickedInCtrlPntCursor(const QPointF& cursorPos, const QPointF& p)
{
    QVector<QPointF> points;
    points.push_back(cursorPos);
    points.push_back(QPointF(cursorPos.x() - CONTROL_POINT_CURSOR_W/2, cursorPos.y() + CONTROL_POINT_CURSOR_H/2));
    points.push_back(QPointF(cursorPos.x(), cursorPos.y() + CONTROL_POINT_CURSOR_H));
    points.push_back(QPointF(cursorPos.x() + CONTROL_POINT_CURSOR_W/2, cursorPos.y() + CONTROL_POINT_CURSOR_H/2));
    points.push_back(cursorPos);
    QPolygonF polygon(points);
    return polygon.containsPoint(p, Qt::OddEvenFill);
}

//////////////////////////////////////////////////////////////////////////////
bool isClickedInWeightCursor(const QPointF& cursorPos, const QPointF& p)
{
    QVector<QPointF> points;
    points.push_back(cursorPos);
    points.push_back(QPointF(cursorPos.x() - WEIGHT_CURSOR_W/2, cursorPos.y() + WEIGHT_CURSOR_H/2));
    points.push_back(QPointF(cursorPos.x(), cursorPos.y() + WEIGHT_CURSOR_H));
    points.push_back(QPointF(cursorPos.x() + WEIGHT_CURSOR_W/2, cursorPos.y() + WEIGHT_CURSOR_H/2));
    points.push_back(cursorPos);
    QPolygonF polygon(points);
    return polygon.containsPoint(p, Qt::OddEvenFill);
}

//////////////////////////////////////////////////////////////////////////////
QImage genControlPointCursorImg(const QColor& color, bool picked = false)
{
    int w = CONTROL_POINT_CURSOR_W;
    int h = CONTROL_POINT_CURSOR_H;
    QImage image = QImage(w, h, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));

    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing,true);

    QPainterPath path;
    path.moveTo(w>>1, 0);
    path.lineTo(0, h>>1);
    path.lineTo(w>>1, h);
    path.lineTo(w, h>>1);
    path.lineTo(w>>1, 0);
    if (picked)
    {
        painter.setPen(QPen(Qt::green, 1, Qt::SolidLine));
        painter.drawPath(path);
    }

    painter.fillPath(path, QBrush(color));

    return image;
}

//////////////////////////////////////////////////////////////////////////////
ColorWidget::ColorWidget(QWidget *parent) :
    QFrame(parent),
    _isLink(false),
    _colorAttribute(nullptr),
    _colorCurve(nullptr),
    _alphaCurve(nullptr),
    _canAddRemoveControlPoint(false),
    _canAddCtrlPntByClick(false),
    _canRemoveCtrlPntByClick(false),
    _curveFitWindowScale(1),
    _curveFitWindowOffset(0),
    _pickedColorCtrlPnt(-1),
    _pickedAlphaCtrlPnt(-1),
    _pickedColorWeight(-1),
    _pickedAlphaWeight(-1),
    _dragColorCtrlPnt(false),
    _dragAlphaCtrlPnt(false),
    _dragColorWeight(false),
    _dragAlphaWeight(false),
    _colorCtrlPntToRemove(-1),
    _alphaCtrlPntToRemove(-1),
    _contextMenu(nullptr),
    _removeCtrlPntAction(nullptr)
{
    setFocusPolicy(Qt::ClickFocus );

    InitCursorResources();

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onShowContextMenu(const QPoint&)));

    _contextMenu = new QMenu(this);;
    _removeCtrlPntAction = new QAction(this);
    _removeCtrlPntAction->setText(tr("Remove Control Point"));
    _contextMenu->addAction(_removeCtrlPntAction);

    connect(_removeCtrlPntAction, SIGNAL(triggered()), this, SLOT(onRemoveControlPoint()) );
}

//////////////////////////////////////////////////////////////////////////////
QColor ColorWidget::getColor()
{
    QColor color;
    if (_pickedColorCtrlPnt != -1)
    {
        color = _colorCurve->getControlPoint(_pickedColorCtrlPnt).color;
    }
    return color;
}

//////////////////////////////////////////////////////////////////////////////
int ColorWidget::getAlpha()
{
    if (_pickedAlphaCtrlPnt != -1)
    {
        if (_isLink)
        {
            return _colorCurve->getControlPoint(_pickedAlphaCtrlPnt).color.alpha();
        }
        else
        {
            return _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt).color.alpha();
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setColor(const QColor& color)
{
    if (_pickedColorCtrlPnt != -1)
    {
        ColorControlPoint pickedColorPnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);

        pickedColorPnt.color.setRed(color.red());
        pickedColorPnt.color.setGreen(color.green());
        pickedColorPnt.color.setBlue(color.blue());
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, pickedColorPnt);

        emit ColorAttributeChanged(_colorAttribute);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setAlpha(int alpha)
{
    if (_pickedColorCtrlPnt != -1)
    {
        ColorControlPoint pickedColorPnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        pickedColorPnt.color.setAlpha(alpha);
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, pickedColorPnt);

        emit ColorAttributeChanged(_colorAttribute);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
float ColorWidget::getColorFallOff()
{
    if (_colorCurve && -1 != _pickedColorCtrlPnt)
    {
        const ColorControlPoint& pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        return pnt.fallOff;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setColorFallOff(float fallOff)
{
    if (_colorCurve && -1 != _pickedColorCtrlPnt)
    {
        ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        pnt.fallOff = fallOff;
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
float ColorWidget::getAlphaFallOff()
{
    if (_isLink)
    {
        if (_colorCurve && -1 != _pickedColorCtrlPnt)
        {
            const ColorControlPoint& pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
            return pnt.fallOff;
        }
    }
    else
    {
        if (_alphaCurve && -1 != _pickedAlphaCtrlPnt)
        {
            const ColorControlPoint& pnt = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);
            return pnt.fallOff;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setAlphaFallOff(float fallOff)
{
    if (_isLink)
    {
        if (_colorCurve && -1 != _pickedColorCtrlPnt)
        {
            ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
            pnt.fallOff = fallOff;
            _colorCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
            emit ColorAttributeChanged(_colorAttribute);
            update();
        }
    }
    else
    {
        if (_alphaCurve && -1 != _pickedAlphaCtrlPnt)
        {
            ColorControlPoint pnt = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);
            pnt.fallOff = fallOff;
            _alphaCurve->setControlPoint(_pickedAlphaCtrlPnt, pnt);

            emit ColorAttributeChanged(_colorAttribute);
            update();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setColorAttribute(ColorAttribute* colorAttribute)
{
    _colorAttribute = colorAttribute;
    if (colorAttribute != nullptr)
    {
        _colorCurve = &(colorAttribute->colorCurve);
        _alphaCurve = &(colorAttribute->alphaCurve);
    }
    else
    {
        _colorCurve = nullptr;
        _alphaCurve = nullptr;
    }

    if (_alphaCurve !=nullptr && _alphaCurve->getControlPointCount() < 2)
    {
        _isLink = true;
    }
    else
    {
        _isLink = false;
    }

    _pickedColorCtrlPnt = -1;
    _pickedAlphaCtrlPnt = -1;
    _pickedColorWeight = -1;
    _pickedAlphaWeight = -1;
    _dragColorCtrlPnt = false;
    _dragAlphaCtrlPnt = false;
    _dragColorWeight = false;
    _dragAlphaWeight = false;

    _updateCurveFitWindowPara();
    if (_colorCurve)
        _colorCurve->_needSample = true;
    if (_alphaCurve)
        _alphaCurve->_needSample = true;

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::reset()
{
    if (_colorAttribute)
    {
        _colorAttribute->colorCurve.reset();
        _colorAttribute->alphaCurve.reset();
        emit ColorAttributeChanged(_colorAttribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::addControlPointsBeforeSelected()
{
    if (-1 != _pickedColorCtrlPnt)
    {
        if (_pickedColorCtrlPnt == 0)
            return;

        _colorCurve->insertControlPointAt(_pickedColorCtrlPnt);
        ++_pickedColorCtrlPnt;
    }
    else if (-1 != _pickedAlphaCtrlPnt)
    {
        if (_pickedAlphaCtrlPnt == 0)
            return;

        _alphaCurve->insertControlPointAt(_pickedAlphaCtrlPnt);
        ++_pickedAlphaCtrlPnt;
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::addControlPointsAfterSelected()
{
    if (-1 != _pickedColorCtrlPnt )
    {
        _colorCurve->insertControlPointAt(_pickedColorCtrlPnt + 1);
    }
    else if (-1 != _pickedAlphaCtrlPnt)
    {
        _alphaCurve->insertControlPointAt(_pickedAlphaCtrlPnt + 1);
    }
    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::removeSelectedControlPoint()
{
    if (-1 != _pickedColorCtrlPnt)
    {
        _colorCurve->removeControlPoint(_pickedColorCtrlPnt);
        _pickedColorCtrlPnt = -1;
    }
    else if (-1 != _pickedAlphaCtrlPnt)
    {
        _alphaCurve->removeControlPoint(_pickedAlphaCtrlPnt);
        _pickedAlphaCtrlPnt = -1;
    }
    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setTangentType(InterpolateMode mode)
{
    if (-1 != _pickedColorCtrlPnt)
    {
        ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        pnt.mode = mode;
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
    }
    else if (-1 != _pickedAlphaCtrlPnt)
    {
        ColorControlPoint pnt = _alphaCurve->getControlPoint(_pickedColorCtrlPnt);
        pnt.mode = mode;
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setSmoothTangent()
{
    if (-1 != _pickedColorCtrlPnt && _pickedColorCtrlPnt < (_colorCurve->getControlPointCount() - 1) )
    {
        ColorControlPoint ctrlctrlPntFront = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        ctrlctrlPntFront.mode = eBezierSpline;
        ctrlctrlPntFront.splineData.outLen = 1.0;
        ctrlctrlPntFront.splineData.outTan = 0;
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, ctrlctrlPntFront);

        ColorControlPoint ctrlPntBehiand = _colorCurve->getControlPoint(_pickedColorCtrlPnt + 1);
        ctrlPntBehiand.mode = eBezierSpline;
        ctrlPntBehiand.splineData.inLen = 1.0;
        ctrlPntBehiand.splineData.inTan = 0.0;
        _colorCurve->setControlPoint(_pickedColorCtrlPnt + 1, ctrlPntBehiand);

        emit ColorAttributeChanged(_colorAttribute);
    }
    else if (-1 != _pickedAlphaCtrlPnt && _pickedAlphaCtrlPnt < (_alphaCurve->getControlPointCount() - 1) )
    {
        ColorControlPoint ctrlctrlPntFront = _alphaCurve->getControlPoint(_pickedColorCtrlPnt);
        ctrlctrlPntFront.mode = eBezierSpline;
        ctrlctrlPntFront.splineData.outLen = 1.0;
        ctrlctrlPntFront.splineData.outTan = 0;
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt, ctrlctrlPntFront);

        ColorControlPoint ctrlPntBehiand = _alphaCurve->getControlPoint(_pickedColorCtrlPnt + 1);
        ctrlPntBehiand.mode = eBezierSpline;
        ctrlPntBehiand.splineData.inLen = 1.0;
        ctrlPntBehiand.splineData.inTan = 0.0;
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt + 1, ctrlPntBehiand);
        emit ColorAttributeChanged(_colorAttribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setEaseInTangent()
{
    if (-1 != _pickedColorCtrlPnt && _pickedColorCtrlPnt < (_colorCurve->getControlPointCount() - 1) )
    {
        ColorControlPoint ctrlctrlPntFront = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        ctrlctrlPntFront.mode = eBezierSpline;
        ctrlctrlPntFront.splineData.outLen = 0.0;
        ctrlctrlPntFront.splineData.outTan = 0;
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, ctrlctrlPntFront);

        ColorControlPoint ctrlPntBehiand = _colorCurve->getControlPoint(_pickedColorCtrlPnt + 1);
        ctrlPntBehiand.mode = eBezierSpline;
        ctrlPntBehiand.splineData.inLen = 1.0;
        ctrlPntBehiand.splineData.inTan = 0;
        _colorCurve->setControlPoint(_pickedColorCtrlPnt + 1, ctrlPntBehiand);

        emit ColorAttributeChanged(_colorAttribute);
    }
    else if (-1 != _pickedAlphaCtrlPnt && _pickedAlphaCtrlPnt < (_alphaCurve->getControlPointCount() - 1) )
    {
        ColorControlPoint ctrlctrlPntFront = _alphaCurve->getControlPoint(_pickedColorCtrlPnt);
        ctrlctrlPntFront.mode = eBezierSpline;
        ctrlctrlPntFront.splineData.outLen = 0.0;
        ctrlctrlPntFront.splineData.outTan = 0;
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt, ctrlctrlPntFront);

        ColorControlPoint ctrlPntBehiand = _alphaCurve->getControlPoint(_pickedColorCtrlPnt + 1);
        ctrlPntBehiand.mode = eBezierSpline;
        ctrlPntBehiand.splineData.inLen = 1.0;
        ctrlPntBehiand.splineData.inTan = 0;
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt + 1, ctrlPntBehiand);
        emit ColorAttributeChanged(_colorAttribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setEaseOutTangent()
{
    if (-1 != _pickedColorCtrlPnt && _pickedColorCtrlPnt < (_colorCurve->getControlPointCount() - 1) )
    {
        ColorControlPoint ctrlctrlPntFront = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        ctrlctrlPntFront.mode = eBezierSpline;
        ctrlctrlPntFront.splineData.outLen = 1.0;
        ctrlctrlPntFront.splineData.outTan = 0;
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, ctrlctrlPntFront);

        ColorControlPoint ctrlPntBehiand = _colorCurve->getControlPoint(_pickedColorCtrlPnt + 1);
        ctrlPntBehiand.mode = eBezierSpline;
        ctrlPntBehiand.splineData.inLen = 0.0;
        ctrlPntBehiand.splineData.inTan = 0;
        _colorCurve->setControlPoint(_pickedColorCtrlPnt + 1, ctrlPntBehiand);

        emit ColorAttributeChanged(_colorAttribute);
    }
    else if (-1 != _pickedAlphaCtrlPnt && _pickedAlphaCtrlPnt < (_alphaCurve->getControlPointCount() - 1) )
    {
        ColorControlPoint ctrlctrlPntFront = _alphaCurve->getControlPoint(_pickedColorCtrlPnt);
        ctrlctrlPntFront.mode = eBezierSpline;
        ctrlctrlPntFront.splineData.outLen = 1.0;
        ctrlctrlPntFront.splineData.outTan = 0;
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt, ctrlctrlPntFront);

        ColorControlPoint ctrlPntBehiand = _alphaCurve->getControlPoint(_pickedColorCtrlPnt + 1);
        ctrlPntBehiand.mode = eBezierSpline;
        ctrlPntBehiand.splineData.inLen = 0.0;
        ctrlPntBehiand.splineData.inTan = 0;
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt + 1, ctrlPntBehiand);
        emit ColorAttributeChanged(_colorAttribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
QString ColorWidget::getColorTex()
{
    if (-1 != _pickedColorCtrlPnt)
    {
        ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        return pnt.texturePath.c_str();
    }
    return "";
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setColorTex(const QString& strPath)
{
    if (-1 != _pickedColorCtrlPnt)
    {
        ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        pnt.texturePath = strPath.toStdString();
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::reloadColorTex()
{
    if (-1 != _pickedColorCtrlPnt)
    {
        emit ReloadColorAttributeTexture(_colorAttribute, true, _pickedColorCtrlPnt);
    }
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::clearColorTex()
{
    if (-1 != _pickedColorCtrlPnt)
    {
        ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        pnt.texturePath = "";
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
QString ColorWidget::getAlphaTex()
{
    if (-1 != _pickedColorCtrlPnt && _isLink)
    {
        ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        return pnt.texturePath.c_str();
    }
    else if (-1 != _pickedAlphaCtrlPnt)
    {
        ColorControlPoint pnt = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);
        return pnt.texturePath.c_str();
    }

    return "";
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setAlphaTex(const QString& strPath)
{
    if (-1 != _pickedColorCtrlPnt && _isLink)
    {
        ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        pnt.texturePath = strPath.toStdString();
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
    }
    else if (-1 != _pickedAlphaCtrlPnt)
    {
        ColorControlPoint pnt = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);
        pnt.texturePath = strPath.toStdString();
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::reloadAlphaTex()
{
    if (-1 != _pickedColorCtrlPnt && _isLink)
    {
        emit ReloadColorAttributeTexture(_colorAttribute, false, _pickedColorCtrlPnt);
    }
    else if (-1 != _pickedAlphaCtrlPnt)
    {
        emit ReloadColorAttributeTexture(_colorAttribute, false, _pickedAlphaCtrlPnt);
    }
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::clearAlphaTex()
{
    if (-1 != _pickedColorCtrlPnt && _isLink)
    {
        ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        pnt.texturePath = "";
        _colorCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
    }
    else if (-1 != _pickedAlphaCtrlPnt)
    {
        ColorControlPoint pnt = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);
        pnt.texturePath = "";
        _alphaCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
        emit ColorAttributeChanged(_colorAttribute);
    }

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::setUseAlphaFromColor(bool val)
{
    if (_colorAttribute && _colorAttribute->useAlphaFromColor != val)
    {
        _colorAttribute->useAlphaFromColor = val;
        emit ColorAttributeChanged(_colorAttribute);
    }

}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::onShowContextMenu(const QPoint& pos)
{
    _colorCtrlPntToRemove = _checkColorCtrlPntCursorSelected(pos);
    _alphaCtrlPntToRemove = _checkAlphaCtrlPntCursorSelected(pos);

    if ( !_canAddRemoveControlPoint
        || (-1 == _colorCtrlPntToRemove && -1 == _alphaCtrlPntToRemove)
        || 2 == _colorCurve->_controlPoints.size() && -1 != _colorCtrlPntToRemove
        || 2 == _alphaCurve->_controlPoints.size()  && -1 != _alphaCtrlPntToRemove)
    {
        _removeCtrlPntAction->setEnabled(false);
    }
    else
    {
        _removeCtrlPntAction->setEnabled(true);
    }
    _contextMenu->exec(QCursor::pos());
}

void ColorWidget::onRemoveControlPoint()
{
    if (-1 != _colorCtrlPntToRemove)
    {
        _colorCurve->removeControlPoint(_colorCtrlPntToRemove);

        if (_colorCtrlPntToRemove == _pickedColorCtrlPnt)
        {
            if (_isLink)
            {
                _colorCtrlPntToRemove = -1;
                _pickedColorCtrlPnt = -1;
                _pickedAlphaCtrlPnt = -1;
            }
            else
            {
                _colorCtrlPntToRemove = -1;
                _pickedColorCtrlPnt = -1;
            }
        }

        _colorCurve->_needSample = true;
        emit ColorAttributeChanged(_colorAttribute);
        update();
    }
    else if (-1 != _alphaCtrlPntToRemove && _isLink)
    {
        _colorCurve->removeControlPoint(_alphaCtrlPntToRemove);

        if (_alphaCtrlPntToRemove == _pickedAlphaCtrlPnt)
        {
            _alphaCtrlPntToRemove = -1;
            _pickedColorCtrlPnt = -1;
            _pickedAlphaCtrlPnt = -1;
        }

        _colorCurve->_needSample = true;
        emit ColorAttributeChanged(_colorAttribute);
        update();
    }
    else if (-1 != _alphaCtrlPntToRemove && !_isLink)
    {
        _alphaCurve->removeControlPoint(_alphaCtrlPntToRemove);

        if (_alphaCtrlPntToRemove == _pickedAlphaCtrlPnt)
        {
            _colorCtrlPntToRemove = -1;
            _pickedColorCtrlPnt = -1;
        }

        _alphaCurve->_needSample = true;
        emit ColorAttributeChanged(_colorAttribute);
        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
bool ColorWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        QPoint mousePos = helpEvent->pos();

        QPointF mousePosF(mousePos.x(), mousePos.y());
        int pickedColorCtrlPnt = _checkColorCtrlPntCursorSelected(mousePosF);
        int pickedAlphaCtrlPnt = _checkAlphaCtrlPntCursorSelected(mousePosF);

        if (-1 != pickedColorCtrlPnt)
        {
            const std::string& tex = _colorCurve->getControlPoint(pickedColorCtrlPnt).texturePath;
            if (!tex.empty())
                QToolTip::showText(helpEvent->globalPos(), tex.c_str());
            
        }
        else if (-1 != pickedAlphaCtrlPnt)
        {
            if (_isLink)
            {
                const std::string& tex = _colorCurve->getControlPoint(pickedAlphaCtrlPnt).texturePath;
                if (!tex.empty())
                    QToolTip::showText(helpEvent->globalPos(), tex.c_str());
            }
            else
            {
                const std::string& tex = _alphaCurve->getControlPoint(pickedAlphaCtrlPnt).texturePath;
                if (!tex.empty())
                    QToolTip::showText(helpEvent->globalPos(), tex.c_str());
            }
        }
        else
        {
            QToolTip::hideText();
            event->ignore();
        }

        return true;
    }
    return QWidget::event(event);
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::paintEvent(QPaintEvent * e)
{
    //TimeConsumeTracker t("update");
    QFrame::paintEvent(e);

    QPainter painter;
    painter.begin(this);
    painter.setRenderHints(QPainter::Antialiasing, true);

    if (_colorCurve)
    {
        _drawRampArea(painter);
        _drawCtrlPntCursors(painter);
        _drawWeightCursors(painter);
    }
    painter.end();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::resizeEvent(QResizeEvent* e)
{
    if (_colorCurve)
        _updateCurveFitWindowPara();

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::mousePressEvent( QMouseEvent* e )
{
    if(_colorCurve && e->button() & Qt::LeftButton)
    {
        QPoint mousePos = e->pos();
        QPointF mousePosF(mousePos.x(), mousePos.y());
        int pickedColorCtrlPnt = _checkColorCtrlPntCursorSelected(mousePosF);
        int pickedAlphaCtrlPnt = _checkAlphaCtrlPntCursorSelected(mousePosF);

        if (-1 != pickedColorCtrlPnt || -1 != pickedAlphaCtrlPnt)
        {
            if (_canAddRemoveControlPoint && _canRemoveCtrlPntByClick)
            {
                if (-1 != pickedColorCtrlPnt)
                {
                    _colorCurve->removeControlPoint(pickedColorCtrlPnt);
                    _pickedColorCtrlPnt = -1;
                }

                if (-1 != pickedAlphaCtrlPnt)
                {
                    if (_isLink)
                    {
                        _colorCurve->removeControlPoint(pickedAlphaCtrlPnt);
                        _pickedColorCtrlPnt = -1;
                        pickedAlphaCtrlPnt = -1;
                    }
                    else
                    {
                        _alphaCurve->removeControlPoint(pickedAlphaCtrlPnt);
                        pickedAlphaCtrlPnt = -1;
                    }
                }
                return;
            }

            _pickedColorCtrlPnt = pickedColorCtrlPnt;
            _pickedAlphaCtrlPnt = pickedAlphaCtrlPnt;

            if (-1 < _pickedColorCtrlPnt)
                _dragColorCtrlPnt = true;

            if (-1 < _pickedAlphaCtrlPnt)
                _dragAlphaCtrlPnt = true;

            if (_isLink)
            {
                if (-1 != pickedColorCtrlPnt)
                {
                    _pickedAlphaCtrlPnt = _pickedColorCtrlPnt;
                    _dragAlphaCtrlPnt = _dragColorCtrlPnt;
                }

                if (-1 != pickedAlphaCtrlPnt)
                {
                    _pickedColorCtrlPnt = _pickedAlphaCtrlPnt;
                    _dragColorCtrlPnt = _dragAlphaCtrlPnt;
                }
                PickedControlPointChanged(true);
            }
            else
            {
                if (-1 != pickedColorCtrlPnt)
                {
                    PickedControlPointChanged(true);
                }

                if (-1 != pickedAlphaCtrlPnt)
                {
                    PickedControlPointChanged(false);
                }
            }

            
        }
        else
        {
            int pickedColorWeight = _checkColorWeightCursorSelected(mousePosF);
            int pickedAlphaWeight = _checkAlphaWeightCursorSelected(mousePosF);

            if (-1 != pickedColorWeight || -1 != pickedAlphaWeight)
            {
                _pickedColorWeight = pickedColorWeight;
                _pickedAlphaWeight = pickedAlphaWeight;

                if (-1 < _pickedColorWeight)
                    _dragColorWeight = true;

                if (-1 < _pickedAlphaCtrlPnt)
                    _dragAlphaWeight = true;

                if (_isLink)
                {
                    if (-1 != _pickedColorWeight)
                    {
                        _pickedAlphaWeight = _pickedColorWeight;
                        _dragAlphaWeight = _dragColorWeight;
                    }

                    if (-1 != _pickedAlphaWeight)
                    {
                        _pickedColorWeight = _pickedAlphaWeight;
                        _dragColorWeight = _dragAlphaWeight;
                    }
                }
            }
            else
            {
                if (_canAddRemoveControlPoint && _canAddCtrlPntByClick)
                {
                    int yMiddle = height() / 2;
                    if (mousePos.y() <= yMiddle)
                    {
                        _addColorControlPoint(mousePos.x());
                        PickedControlPointChanged(true);
                    }
                    else if (mousePos.y() > yMiddle)
                    {
                        if (_isLink)
                        {
                            _addColorControlPoint(mousePos.x());
                            PickedControlPointChanged(true);
                        }
                        else
                        {
                            _addAlphaControlPoint(mousePos.x());
                            PickedControlPointChanged(false);
                        }
                    }
                }
            }
        }
        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::mouseReleaseEvent( QMouseEvent* /*e*/ )
{
    _dragColorCtrlPnt = false;
    _dragAlphaCtrlPnt = false;
    _dragColorWeight = false;
    _dragAlphaWeight = false;
    _pickedColorWeight = -1;
    _pickedAlphaWeight = -1;

    update();
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::mouseMoveEvent( QMouseEvent* e )
{
    Qt::MouseButtons buttons = e->buttons();

    if(_colorCurve && buttons & Qt::LeftButton)
    {
        if (_dragColorCtrlPnt)
        {
            if (!_colorAttribute->canMoveControlPointHorizontally())
                return;

            QPoint mousePos = e->pos();
            ColorControlPoint pnt = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
            pnt.x = _screenToValue(mousePos.x());

            //make control point move in value range
            if (pnt.x < _colorCurve->getMinValue())
                pnt.x = _colorCurve->getMinValue();
            else if (pnt.x > _colorCurve->getMaxValue())
                pnt.x = _colorCurve->getMaxValue();

            _colorCurve->setControlPoint(_pickedColorCtrlPnt, pnt);
            _colorCurve->_reOrderControlPoints(_pickedColorCtrlPnt);
            emit ColorAttributeChanged(_colorAttribute);
        }

        if (_dragColorWeight)
        {
            QPoint mousePos = e->pos();
            ColorControlPoint pnt0 = _colorCurve->getControlPoint(_pickedColorWeight);
            ColorControlPoint pnt1 = _colorCurve->getControlPoint(_pickedColorWeight + 1);
            int xPnt0Screen = _valueToScreen(pnt0.x);
            int xPnt1Screen = _valueToScreen(pnt1.x);
            if (mousePos.x() < (xPnt0Screen + WEIGHT_CURSOR_W))
                mousePos.setX(xPnt0Screen + WEIGHT_CURSOR_W);
            if (mousePos.x() > (xPnt1Screen - WEIGHT_CURSOR_W))
                mousePos.setX(xPnt1Screen - WEIGHT_CURSOR_W);
            pnt0.weight = ((float)(mousePos.x() - xPnt0Screen))/(xPnt1Screen - xPnt0Screen);
            _colorCurve->setControlPoint(_pickedColorWeight, pnt0);
            emit ColorAttributeChanged(_colorAttribute);
        }

        if (!_isLink)
        {
            if (_dragAlphaCtrlPnt)
            {
                if (!_colorAttribute->canMoveControlPointHorizontally())
                    return;

                QPoint mousePos = e->pos();
                ColorControlPoint pnt = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);
                pnt.x = _screenToValue(mousePos.x());

                //make control point move in value range
                if (pnt.x < _alphaCurve->getMinValue())
                    pnt.x = _alphaCurve->getMinValue();
                else if (pnt.x > _alphaCurve->getMaxValue())
                    pnt.x = _alphaCurve->getMaxValue();

                _alphaCurve->setControlPoint(_pickedAlphaCtrlPnt, pnt);
                _alphaCurve->_reOrderControlPoints(_pickedAlphaCtrlPnt);
                _alphaCurve->_needSample = true;
                emit ColorAttributeChanged(_colorAttribute);
            }

            if (_dragAlphaWeight)
            {
                QPoint mousePos = e->pos();
                ColorControlPoint pnt0 = _alphaCurve->getControlPoint(_pickedAlphaWeight);
                ColorControlPoint pnt1 = _alphaCurve->getControlPoint(_pickedAlphaWeight + 1);
                int xPnt0Screen = _valueToScreen(pnt0.x);
                int xPnt1Screen = _valueToScreen(pnt1.x);
                if (mousePos.x() < (xPnt0Screen + WEIGHT_CURSOR_W))
                    mousePos.setX(xPnt0Screen + WEIGHT_CURSOR_W);
                if (mousePos.x() > (xPnt1Screen - WEIGHT_CURSOR_W))
                    mousePos.setX(xPnt1Screen - WEIGHT_CURSOR_W);
                pnt0.weight = ((float)(mousePos.x() - xPnt0Screen))/(xPnt1Screen - xPnt0Screen);
                _alphaCurve->setControlPoint(_pickedAlphaWeight, pnt0);
                emit ColorAttributeChanged(_colorAttribute);
            }
        }

        update();
    }
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::_updateCurveFitWindowPara()
{
    int widgetWidth = width();

    if (_colorCurve && _colorCurve->getControlPointCount() > 1)
    {
        float minX = _colorCurve->getMinValue();
        float maxX = _colorCurve->getMaxValue();

        if (!_isLink)
        {
            if (minX < _alphaCurve->getMinValue())
                minX = _alphaCurve->getMinValue();

            if (maxX < _alphaCurve->getMaxValue())
                maxX = _alphaCurve->getMaxValue();
        }

        float curveWidth = maxX - minX;

        if (0 == curveWidth)
        {
            curveWidth = 1.0;
        }

        _curveFitWindowScale = (widgetWidth - MARGIN_X * 2) / curveWidth;
        _curveFitWindowOffset = MARGIN_X - minX * _curveFitWindowScale;
    }
}

//////////////////////////////////////////////////////////////////////////////
float ColorWidget::_valueToScreen(float x)
{
    return ROUND(_curveFitWindowScale * x + _curveFitWindowOffset);
}

//////////////////////////////////////////////////////////////////////////////
float ColorWidget::_screenToValue(float x)
{
    return (x - _curveFitWindowOffset) / _curveFitWindowScale;
}

//////////////////////////////////////////////////////////////////////////////
void ColorWidget::_fillRampImage(QImage& colorImg, QImage& alphaImg)
{
    assert(colorImg.width() == alphaImg.width());
    assert(colorImg.height() == alphaImg.height());

    int width = colorImg.width();
    int height = colorImg.height();

    QRgb *colorPixels = nullptr;
    QRgb *alphaPixels = nullptr;
    {
        colorPixels = new QRgb[width];
        //TimeConsumeTracker t("calcuate one line colorPixels");
        for (int i = 0; i < width; ++i)
        {
            colorPixels[i] = _colorCurve->getColorByX(_screenToValue(i + MARGIN_X)).rgba();
        }

        if (!_isLink)
        {
            alphaPixels = new QRgb[width];
            //TimeConsumeTracker t("calcuate one line alphaPixels");
            for (int i = 0; i < width; ++i)
            {
                alphaPixels[i] = _alphaCurve->getColorByX(_screenToValue(i + MARGIN_X)).rgba();
            }
        }
    }

    {
        //TimeConsumeTracker tt("fill images' pixel");
        for (int m = 0; m < height; ++m)
        {
            void* bytesPerLine = const_cast<uchar*>(colorImg.scanLine(m));
            memcpy(bytesPerLine, colorPixels, sizeof(QRgb) * width);

            bytesPerLine = const_cast<uchar*>(alphaImg.scanLine(m));
            if (nullptr == alphaPixels)
                memcpy(bytesPerLine, colorPixels, sizeof(QRgb) * width);
            else
                memcpy(bytesPerLine, alphaPixels, sizeof(QRgb) * width);
        }
    }

    delete[] colorPixels;
}

void ColorWidget::_drawRampArea(QPainter& painter)
{
    QImage colorImg(width() - MARGIN_X * 2, (height() - SPACING_V - CONTROL_POINT_CURSOR_H * 2 - MARGIN_Y * 2) / 2, QImage::Format_RGB32);
    QImage alphaImg(width() - MARGIN_X * 2, (height() - SPACING_V - CONTROL_POINT_CURSOR_H * 2 - MARGIN_Y * 2) / 2, QImage::Format_RGB32);
    _fillRampImage(colorImg, alphaImg);
    painter.drawImage(MARGIN_X, MARGIN_Y + CONTROL_POINT_CURSOR_H, colorImg);
    painter.drawImage(MARGIN_X, MARGIN_Y + CONTROL_POINT_CURSOR_H + (height() - SPACING_V - CONTROL_POINT_CURSOR_H * 2 - MARGIN_Y * 2) / 2, alphaImg);
}

void ColorWidget::_drawCtrlPntCursors(QPainter& painter)
{
    int colorCtrlPntCount = _colorCurve->getControlPointCount();

    for (int i = 0; i < colorCtrlPntCount; ++i)
    {
        const ColorControlPoint& pnt = _colorCurve->getControlPoint(i);

        float xPosCtrlPnt = ROUND(_valueToScreen(pnt.x));
        float yColorCtrlPnt = MARGIN_Y;
        float yAlphaCtrlPnt = height() - SPACING_V - CONTROL_POINT_CURSOR_H - MARGIN_Y;

        QRect rcColorCtrnPntCursor(xPosCtrlPnt - CONTROL_POINT_CURSOR_W / 2, yColorCtrlPnt, CONTROL_POINT_CURSOR_W, CONTROL_POINT_CURSOR_H);
        QRect rcAlphaCtrnPntCursor(xPosCtrlPnt - CONTROL_POINT_CURSOR_W / 2, yAlphaCtrlPnt, CONTROL_POINT_CURSOR_W, CONTROL_POINT_CURSOR_H);
        if (i == _pickedColorCtrlPnt)
        {
            if (pnt.texturePath.empty())
                painter.drawImage(rcColorCtrnPntCursor, genControlPointCursorImg(QColor(pnt.color.red(), pnt.color.green(), pnt.color.blue(), 255), true));
            else
                painter.drawImage(rcColorCtrnPntCursor, S_CtrlPntWithTexCursorPickedImg);

            if (_isLink)
            {
                if (pnt.texturePath.empty())
                    painter.drawImage(rcAlphaCtrnPntCursor, genControlPointCursorImg(QColor(pnt.color.alpha(), pnt.color.alpha(), pnt.color.alpha()), true));
                else
                    painter.drawImage(rcAlphaCtrnPntCursor, S_CtrlPntWithTexCursorPickedImg);
            }
        }
        else
        {
            if (pnt.texturePath.empty())
                painter.drawImage(rcColorCtrnPntCursor, genControlPointCursorImg(QColor(pnt.color.red(), pnt.color.green(), pnt.color.blue(), 255)));
            else
                painter.drawImage(rcColorCtrnPntCursor, S_CtrlPntWithTexCursorNormalImg);

            if (_isLink)
            {
                if (pnt.texturePath.empty())
                    painter.drawImage(rcAlphaCtrnPntCursor, genControlPointCursorImg(QColor(pnt.color.alpha(), pnt.color.alpha(), pnt.color.alpha())));
                else
                    painter.drawImage(rcAlphaCtrnPntCursor, S_CtrlPntWithTexCursorNormalImg);
            }
                
        }
    }

    if (!_isLink)
    {
        int alphaCtrlPntCount = _alphaCurve->getControlPointCount();

        for (int i = 0; i < alphaCtrlPntCount; ++i)
        {
            const ColorControlPoint& pnt = _alphaCurve->getControlPoint(i);

            float xPosCtrlPnt = ROUND(_valueToScreen(pnt.x));
            float yAlphaCtrlPnt = height() - SPACING_V - CONTROL_POINT_CURSOR_H - MARGIN_Y;

            QRect rcAlphaCtrnPntCursor(xPosCtrlPnt - CONTROL_POINT_CURSOR_W / 2, yAlphaCtrlPnt, CONTROL_POINT_CURSOR_W, CONTROL_POINT_CURSOR_H);

            if (i == _pickedAlphaCtrlPnt)
            {
                if (pnt.texturePath.empty())
                    painter.drawImage(rcAlphaCtrnPntCursor, genControlPointCursorImg(QColor(pnt.color.alpha(), pnt.color.alpha(), pnt.color.alpha()), true));
                else
                    painter.drawImage(rcAlphaCtrnPntCursor, S_CtrlPntWithTexCursorPickedImg);
            }
            else
            {
                if (pnt.texturePath.empty())
                    painter.drawImage(rcAlphaCtrnPntCursor, genControlPointCursorImg(QColor(pnt.color.alpha(), pnt.color.alpha(), pnt.color.alpha()) ));
                else
                    painter.drawImage(rcAlphaCtrnPntCursor, S_CtrlPntWithTexCursorNormalImg);
            }
        }
    }
}

void ColorWidget::_drawWeightCursors(QPainter& painter)
{
    float yColorWeight = MARGIN_Y + CONTROL_POINT_CURSOR_H - WEIGHT_CURSOR_H;

    int colorCtrlPntCount = _colorCurve->getControlPointCount();
    if (0  == _pickedColorCtrlPnt)
    {
        const ColorControlPoint& pnt0 = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        const ColorControlPoint& pnt1 = _colorCurve->getControlPoint(_pickedColorCtrlPnt + 1);

        _drawColorWeightCursor(painter, pnt0, pnt1, _pickedColorCtrlPnt == _pickedColorWeight);
    }
    else if ((colorCtrlPntCount - 1) == _pickedColorCtrlPnt)
    {
        const ColorControlPoint& pnt0 = _colorCurve->getControlPoint(_pickedColorCtrlPnt - 1);
        const ColorControlPoint& pnt1 = _colorCurve->getControlPoint(_pickedColorCtrlPnt);

        _drawColorWeightCursor(painter, pnt0, pnt1, (_pickedColorCtrlPnt - 1) == _pickedColorWeight);
    }
    else if (0  < _pickedColorCtrlPnt && _pickedColorCtrlPnt < (colorCtrlPntCount - 1))
    {
        const ColorControlPoint& pnt0 = _colorCurve->getControlPoint(_pickedColorCtrlPnt - 1);
        const ColorControlPoint& pnt1 = _colorCurve->getControlPoint(_pickedColorCtrlPnt);
        const ColorControlPoint& pnt2 = _colorCurve->getControlPoint(_pickedColorCtrlPnt + 1);

        _drawColorWeightCursor(painter, pnt0, pnt1, (_pickedColorCtrlPnt - 1) == _pickedColorWeight);
        _drawColorWeightCursor(painter, pnt1, pnt2, _pickedColorCtrlPnt == _pickedColorWeight);
    }

    if (!_isLink)
    {
        float yAlphaWeight = height() - SPACING_V - WEIGHT_CURSOR_H - MARGIN_Y;
        int alphaCtrlPntCount = _alphaCurve->getControlPointCount();
        if (0  == _pickedAlphaCtrlPnt)
        {
            const ColorControlPoint& pnt0 = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);
            const ColorControlPoint& pnt1 = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt + 1);

            _drawAlphaWeightCursor(painter, pnt0, pnt1, _pickedAlphaCtrlPnt == _pickedAlphaWeight);
        }
        else if ((alphaCtrlPntCount - 1) == _pickedAlphaCtrlPnt)
        {
            const ColorControlPoint& pnt0 = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt - 1);
            const ColorControlPoint& pnt1 = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);

            _drawAlphaWeightCursor(painter, pnt0, pnt1, (_pickedAlphaCtrlPnt - 1) == _pickedAlphaWeight);
        }
        else if (0  < _pickedAlphaCtrlPnt && _pickedAlphaCtrlPnt < (alphaCtrlPntCount - 1))
        {
            const ColorControlPoint& pnt0 = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt - 1);
            const ColorControlPoint& pnt1 = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt);
            const ColorControlPoint& pnt2 = _alphaCurve->getControlPoint(_pickedAlphaCtrlPnt + 1);

            _drawAlphaWeightCursor(painter, pnt0, pnt1, (_pickedAlphaCtrlPnt - 1) == _pickedAlphaWeight);
            _drawAlphaWeightCursor(painter, pnt1, pnt2, _pickedAlphaCtrlPnt == _pickedAlphaWeight);
        }
    }
}

void ColorWidget::_drawColorWeightCursor(QPainter& painter, const ColorControlPoint& pnt0, const ColorControlPoint& pnt1, bool picked)
{
    if (pnt0.mode != eLinear)
        return ;

    float yColorWeight = MARGIN_Y + CONTROL_POINT_CURSOR_H - WEIGHT_CURSOR_H;
    float yAlphaWeight = height() - SPACING_V - CONTROL_POINT_CURSOR_H - MARGIN_Y;

    float xPosWeightCursor = ROUND(_valueToScreen(pnt0.x + pnt0.weight * (pnt1.x - pnt0.x)));
    QRect rcColorWeightCursor(xPosWeightCursor - WEIGHT_CURSOR_W / 2, yColorWeight, WEIGHT_CURSOR_W, WEIGHT_CURSOR_H);
    QRect rcAlphaWeightCursor(xPosWeightCursor - WEIGHT_CURSOR_W / 2, yAlphaWeight, WEIGHT_CURSOR_W, WEIGHT_CURSOR_H);

    painter.drawImage(rcColorWeightCursor, picked?S_WeightCursorPickedImg:S_WeightCursorNormalImg);

    if (_isLink)
        painter.drawImage(rcAlphaWeightCursor, picked?S_WeightCursorPickedImg:S_WeightCursorNormalImg);
}

void ColorWidget::_drawAlphaWeightCursor(QPainter& painter, const ColorControlPoint& pnt0, const ColorControlPoint& pnt1, bool picked)
{
    if (pnt0.mode != eLinear)
        return ;

    float yColorWeight = MARGIN_Y + CONTROL_POINT_CURSOR_H - WEIGHT_CURSOR_H;
    float yAlphaWeight = height() - SPACING_V - CONTROL_POINT_CURSOR_H - MARGIN_Y;

    float xPosWeightCursor = ROUND(_valueToScreen(pnt0.x + pnt0.weight * (pnt1.x - pnt0.x)));
    QRect rcColorWeightCursor(xPosWeightCursor - WEIGHT_CURSOR_W / 2, yColorWeight, WEIGHT_CURSOR_W, WEIGHT_CURSOR_H);
    QRect rcAlphaWeightCursor(xPosWeightCursor - WEIGHT_CURSOR_W / 2, yAlphaWeight, WEIGHT_CURSOR_W, WEIGHT_CURSOR_H);

    painter.drawImage(rcAlphaWeightCursor, picked?S_WeightCursorPickedImg:S_WeightCursorNormalImg);

    if (_isLink)
        painter.drawImage(rcColorWeightCursor, picked?S_WeightCursorPickedImg:S_WeightCursorNormalImg);
}

int ColorWidget::_checkColorCtrlPntCursorSelected(const QPointF& pickPos)
{
    if (nullptr == _colorCurve)
        return -1;
    int colorCtrlPntCount = _colorCurve->getControlPointCount();
    for (int i = 0; i < colorCtrlPntCount; ++i)
    {
        const ColorControlPoint& pnt = _colorCurve->getControlPoint(i);

        float xPos = _valueToScreen(pnt.x);
        float yPos = MARGIN_Y;
        bool picked = isClickedInCtrlPntCursor(QPointF(xPos, yPos), pickPos);
        if (picked)
        {
            return i;
        }
    }

    return -1;
}

int ColorWidget::_checkAlphaCtrlPntCursorSelected(const QPointF& pickPos)
{
    if (_isLink)
    {
        if (nullptr == _colorCurve)
            return -1;
        int colorCtrlPntCount = _colorCurve->getControlPointCount();
        for (int i = 0; i < colorCtrlPntCount; ++i)
        {
            const ColorControlPoint& pnt = _colorCurve->getControlPoint(i);

            float xPos = _valueToScreen(pnt.x);
            float yPos = height() - SPACING_V - CONTROL_POINT_CURSOR_H - MARGIN_Y;
            bool picked = isClickedInCtrlPntCursor(QPointF(xPos, yPos), pickPos);
            if (picked)
            {
                return i;
            }
        }
    }
    else
    {
        if (nullptr == _alphaCurve)
            return -1;
        int colorCtrlPntCount = _alphaCurve->getControlPointCount();
        for (int i = 0; i < colorCtrlPntCount; ++i)
        {
            const ColorControlPoint& pnt = _alphaCurve->getControlPoint(i);

            float xPos = _valueToScreen(pnt.x);
            float yPos = height() - SPACING_V - CONTROL_POINT_CURSOR_H - MARGIN_Y;
            bool picked = isClickedInCtrlPntCursor(QPointF(xPos, yPos), pickPos);
            if (picked)
            {
                return i;
            }
        }
    }

    return -1;
}

int ColorWidget::_checkColorWeightCursorSelected(const QPointF& pickPos)
{
    int colorCtrlPntCount = _colorCurve->getControlPointCount();
    if (-1 != _pickedColorCtrlPnt)
    {
        if (0 == _pickedColorCtrlPnt)
        {
            return _checkColorWeightCursorSelected(0, pickPos);
        }
        else if ((colorCtrlPntCount - 1) == _pickedColorCtrlPnt)
        {
            return _checkColorWeightCursorSelected(_pickedColorCtrlPnt - 1, pickPos);
        }
        else if (0 < _pickedColorCtrlPnt && _pickedColorCtrlPnt < (colorCtrlPntCount - 1))
        {
            int pickedWeightCursor = _checkColorWeightCursorSelected(_pickedColorCtrlPnt - 1, pickPos);
            if (-1 != pickedWeightCursor)
                return pickedWeightCursor;
            else
                return _checkColorWeightCursorSelected(_pickedColorCtrlPnt, pickPos);
        }
    }

    return -1;
}

int ColorWidget::_checkColorWeightCursorSelected(int pickedCtrlPnt, const QPointF& pickPos)
{
    const ColorControlPoint& pnt0 = _colorCurve->getControlPoint(pickedCtrlPnt);
    const ColorControlPoint& pnt1 = _colorCurve->getControlPoint(pickedCtrlPnt + 1);

    if (pnt0.weight >= 0)
    {
        float xPos = _valueToScreen(pnt0.x + pnt0.weight * (pnt1.x - pnt0.x));
        float yPos = MARGIN_Y + CONTROL_POINT_CURSOR_H - WEIGHT_CURSOR_H;
        bool picked = isClickedInWeightCursor(QPointF(xPos, yPos), pickPos);
        if (picked)
        {
            return pickedCtrlPnt;
        }
    }

    return - 1;
}

int ColorWidget::_checkAlphaWeightCursorSelected(const QPointF& pickPos)
{
    if (_isLink)
    {
        int count = _colorCurve->getControlPointCount();
        if (-1 != _pickedAlphaCtrlPnt)
        {
            if (0 == _pickedAlphaCtrlPnt)
            {
                return _checkAlphaWeightCursorSelected(0, pickPos);
            }
            else if ((count - 1) == _pickedAlphaCtrlPnt)
            {
                return _checkAlphaWeightCursorSelected(_pickedColorCtrlPnt - 1, pickPos);
            }
            else if (0 < _pickedAlphaCtrlPnt && _pickedAlphaCtrlPnt < (count - 1))
            {
                int pickedWeightCursor = _checkAlphaWeightCursorSelected(_pickedColorCtrlPnt - 1, pickPos);
                if (-1 != pickedWeightCursor)
                    return pickedWeightCursor;
                else
                    return _checkAlphaWeightCursorSelected(_pickedColorCtrlPnt, pickPos);
            }
        }
    }
    else
    {
        int count = _alphaCurve->getControlPointCount();
        if (-1 != _pickedAlphaCtrlPnt)
        {
            if (0 == _pickedAlphaCtrlPnt)
            {
                return _checkAlphaWeightCursorSelected(0, pickPos);
            }
            else if ((count - 1) == _pickedAlphaCtrlPnt)
            {
                return _checkAlphaWeightCursorSelected(count - 1, pickPos);
            }
            else if (0 < _pickedAlphaCtrlPnt && _pickedAlphaCtrlPnt < (count - 1))
            {
                int pickedWeightCursor = _checkAlphaWeightCursorSelected(_pickedColorCtrlPnt - 1, pickPos);
                if (-1 != pickedWeightCursor)
                    return pickedWeightCursor;
                else
                    return _checkAlphaWeightCursorSelected(_pickedColorCtrlPnt, pickPos);
            }
        }
    }

    return -1;
}

int ColorWidget::_checkAlphaWeightCursorSelected(int pickedCtrlPnt, const QPointF& pickPos)
{
    if (_isLink)
    {
        const ColorControlPoint& pnt0 = _colorCurve->getControlPoint(pickedCtrlPnt);
        const ColorControlPoint& pnt1 = _colorCurve->getControlPoint(pickedCtrlPnt + 1);

        if (pnt0.weight >= 0)
        {
            float xPos = _valueToScreen(pnt0.x + pnt0.weight * (pnt1.x - pnt0.x));
            float yPos = height() - SPACING_V - CONTROL_POINT_CURSOR_H - MARGIN_Y;
            bool picked = isClickedInWeightCursor(QPointF(xPos, yPos), pickPos);
            if (picked)
            {
                return pickedCtrlPnt;
            }
        }
    }
    else
    {
        int colorCtrlPntCount = _alphaCurve->getControlPointCount();
        for (int i = 0; i < colorCtrlPntCount - 1; ++i)
        {
            const ColorControlPoint& pnt0 = _alphaCurve->getControlPoint(i);
            const ColorControlPoint& pnt1 = _alphaCurve->getControlPoint(i + 1);

            if (pnt0.weight >= 0)
            {
                float xPos = _valueToScreen(pnt0.x + pnt0.weight * (pnt1.x - pnt0.x));
                float yPos = height() - SPACING_V - CONTROL_POINT_CURSOR_H - MARGIN_Y;
                bool picked = isClickedInCtrlPntCursor(QPointF(xPos, yPos), pickPos);
                if (picked)
                {
                    return i;
                }
            }
            else
                break;
        }
    }

    return -1;
}

void ColorWidget::_addColorControlPoint(int xPos)
{
    float x = _screenToValue(xPos);

    int ctrlPntCount = _colorCurve->getControlPointCount();
    for (int i = 0; i < ctrlPntCount - 1; ++i)
    {
        const ColorControlPoint& pntLeft = _colorCurve->getControlPoint(i);
        const ColorControlPoint& pntRight = _colorCurve->getControlPoint(i + 1);

        if (pntLeft.x < x && x < pntRight.x)
        {
            QColor color = _colorCurve->getColorByX(x);
            ColorControlPoint newCtrlPnt(x, color, pntLeft.mode);

            std::vector<ColorControlPoint>::iterator itr = _colorCurve->_controlPoints.begin();
            std::advance(itr, i + 1);
            _colorCurve->_controlPoints.insert(itr, newCtrlPnt);

            _pickedColorCtrlPnt = i + 1;
            if (_isLink)
                _pickedAlphaCtrlPnt = _pickedColorCtrlPnt;

            break;
        }
    }
}

void ColorWidget::_addAlphaControlPoint(int xPos)
{
    float x = _screenToValue(xPos);

    int ctrlPntCount = _alphaCurve->getControlPointCount();
    for (int i = 0; i < ctrlPntCount - 1; ++i)
    {
        const ColorControlPoint& pntLeft = _alphaCurve->getControlPoint(i);
        const ColorControlPoint& pntRight = _alphaCurve->getControlPoint(i + 1);

        if (pntLeft.x < x && x < pntRight.x)
        {
            QColor color = _alphaCurve->getColorByX(x);
            ColorControlPoint newCtrlPnt(x, color, pntLeft.mode);

            std::vector<ColorControlPoint>::iterator itr = _alphaCurve->_controlPoints.begin();
            std::advance(itr, i + 1);
            _alphaCurve->_controlPoints.insert(itr, newCtrlPnt);

            _pickedAlphaCtrlPnt = i + 1;
            if (_isLink)
                _pickedColorCtrlPnt = _pickedAlphaCtrlPnt;

            break;
        }
    }
}

} // namespace CurveEditor
} // namespace nvidia