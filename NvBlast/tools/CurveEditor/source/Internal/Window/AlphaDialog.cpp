#include "AlphaDialog.h"
#include "ui_AlphaDialog.h"
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace nvidia {
namespace CurveEditor {

const int MARGIN_X = 8;
const int MARGIN_Y = 8;

const int ALPHA_W = 255;
const int ALPHA_H = 30;

const int CURSOR_W = 12;
const int CURSOR_H = 12;

static QImage s_triangleUp;

void InitTriangleResources(int w, int h)
{
    s_triangleUp  = QImage(w, h, QImage::Format_ARGB32);
    s_triangleUp.fill(QColor(0, 0, 0, 0));

    // a painter cannot switch device?
    QPainterPath path;
    QPainter painter(&s_triangleUp);
    painter.setRenderHint(QPainter::Antialiasing,true);
    path = QPainterPath();    // trick to clear up a path
    path.moveTo(w>>1, 0);
    path.lineTo(0, h);
    path.lineTo(w, h);
    path.lineTo(w>>1, 0);
    painter.setPen(Qt::NoPen);
    painter.fillPath(path, QBrush(QColor(50, 50, 50)));
}

//////////////////////////////////////////////////////////////////////////////
bool isClickedInCursor(const QPoint& cursorPos, const QPoint& p)
{
    QVector<QPoint> points;
    points.push_back(cursorPos);
    points.push_back(QPoint(cursorPos.x() - (CURSOR_W>>1), cursorPos.y() + CURSOR_H));
    points.push_back(QPoint(cursorPos.x() + (CURSOR_W>>1), cursorPos.y() + CURSOR_H));
    points.push_back(cursorPos);

    QPolygon polygon(points);
    return polygon.containsPoint(p, Qt::OddEvenFill);
}

//////////////////////////////////////////////////////////////////////////////
 int AlphaDialog::getAlpha(int alpha, QWidget *parent)
 {
     AlphaDialog dlg(parent, alpha);
     dlg.exec();
     return dlg._alpha;
 }

//////////////////////////////////////////////////////////////////////////////
AlphaDialog::AlphaDialog(QWidget *parent, int alpha) :
    QDialog(parent),
    ui(new Ui::AlphaDialog),
    _drag(false),
    _alpha(alpha),
    _xOffset(0)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
    setFixedWidth(271);
    setFixedHeight(120);

    ui->spinBoxAlpha->setValue(_alpha);

    InitTriangleResources(12, 12);
}

//////////////////////////////////////////////////////////////////////////////
AlphaDialog::~AlphaDialog()
{
    delete ui;
}

//////////////////////////////////////////////////////////////////////////////
void AlphaDialog::paintEvent(QPaintEvent * e)
{
    QDialog::paintEvent(e);

    QPainter painter;
    painter.begin(this);
    drawAlphaRectangle(painter);
    drawCursor(painter, _alpha + MARGIN_X);
    painter.end();
}

//////////////////////////////////////////////////////////////////////////////
void AlphaDialog::mousePressEvent( QMouseEvent* e )
{
    if(e->button() & Qt::LeftButton)
    {
        QPoint mousePos = e->pos();

        if (isClickedInCursor(QPoint(_alpha + MARGIN_X, MARGIN_Y + ALPHA_H), mousePos))
        {
            _xOffset = _alpha + MARGIN_X - mousePos.x();
            _drag = true;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void AlphaDialog::mouseReleaseEvent( QMouseEvent* e )
{
    _drag = false;
    _xOffset = -1;
    update();
}

//////////////////////////////////////////////////////////////////////////////
void AlphaDialog::mouseMoveEvent( QMouseEvent* e )
{
    Qt::MouseButtons buttons = e->buttons();

    if(buttons & Qt::LeftButton)
    {
        if (_drag)
        {
            QPoint mousePos = e->pos();

            _alpha = mousePos.x() + _xOffset - MARGIN_X;
            if (_alpha < 0)
            {
                _alpha = 0;
            }
            else if(_alpha > ALPHA_W)
            {
                _alpha = ALPHA_W;
            }
            ui->spinBoxAlpha->setValue(_alpha);
            update();
        }
    }
}

void AlphaDialog::drawAlphaRectangle(QPainter& painter)
{
    QPointF topLeftPnt(MARGIN_X, MARGIN_Y);
    QPointF topRightPnt(MARGIN_X + ALPHA_W, MARGIN_Y);
    QPointF bottomRightPnt(MARGIN_X + ALPHA_W, MARGIN_Y + ALPHA_H);
    QPointF bottomLeftPnt(MARGIN_X, MARGIN_Y + ALPHA_H);
    QPainterPath path;
    path.moveTo(topLeftPnt);
    path.lineTo(topRightPnt);
    path.lineTo(bottomRightPnt);
    path.lineTo(bottomLeftPnt);

    QColor colorLeft(0, 0, 0, 255);
    QColor colorRight(255, 255, 255, 255);
    colorLeft.setAlpha(255);
    colorRight.setAlpha(255);
    QLinearGradient indicatorGradient(topLeftPnt,topRightPnt);
    indicatorGradient.setColorAt(0.0, colorLeft);
    indicatorGradient.setColorAt(1.0, colorRight);

    painter.fillPath(path, indicatorGradient);
}

void AlphaDialog::drawCursor(QPainter& painter, int xPos)
{
    QRect rect(xPos - (CURSOR_W>>1), MARGIN_Y + ALPHA_H, CURSOR_W, CURSOR_H);
    painter.drawImage(rect, s_triangleUp);

}

void AlphaDialog::on_spinBoxAlpha_valueChanged(int arg1)
{
    _alpha = arg1;
}

} // namespace CurveEditor
} // namespace nvidia