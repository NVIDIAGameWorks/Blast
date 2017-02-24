#ifndef COLORWIDGET_H
#define COLORWIDGET_H

#include <QtWidgets/QFrame>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include "Attribute.h"

namespace nvidia {
namespace CurveEditor {

class ColorWidget : public QFrame
{
    Q_OBJECT
public:
    explicit ColorWidget(QWidget *parent = 0);

    inline bool isLink()   {   return _isLink; }
    QColor getColor();
    int getAlpha();
    void setColor(const QColor& color);
    void setAlpha(int alpha);
    float getColorFallOff();
    void setColorFallOff(float fallOff);
    float getAlphaFallOff();
    void setAlphaFallOff(float fallOff);

    // if alphaCurve of colorAttribute is with less than 2 control points,
    // it treats that rgb color curve and alpha curve share same control points
    // else, it uses rgb components of colorCurve of ColorAttribute, and it uses alpha component of alphaCurve of ColorAttribute
    void setColorAttribute(ColorAttribute* colorAttribute);
    void reset();
    inline void setCanAddRemoveControlPoint(bool val)    {   _canAddRemoveControlPoint = val;    }

    void addControlPointsBeforeSelected();
    void addControlPointsAfterSelected();
    void removeSelectedControlPoint();
     
    void setTangentType(InterpolateMode mode);
    void setSmoothTangent();
    void setEaseInTangent();
    void setEaseOutTangent();

    QString getColorTex();
    void setColorTex(const QString& strPath);
    void reloadColorTex();
    void clearColorTex();
    QString getAlphaTex();
    void setAlphaTex(const QString& strPath);
    void reloadAlphaTex();
    void clearAlphaTex();

    void setAddCtrlPntByClick(bool value)       { _canAddCtrlPntByClick = value; }
    void setRemoveCtrlPntByClick(bool value)    { _canRemoveCtrlPntByClick = value; }

    void setUseAlphaFromColor(bool val);

signals:
    void PickedControlPointChanged(bool isColorCtrlPnt);
    void ColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute);
    void ReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex);

private slots:
    void onShowContextMenu(const QPoint& pos);
    void onRemoveControlPoint();

protected:
    // QWidget events
    virtual bool event(QEvent *event);
    virtual void paintEvent(QPaintEvent * e);
    virtual void resizeEvent(QResizeEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);

private:
    void _updateCurveFitWindowPara();
    inline float _valueToScreen(float x);
    inline float _screenToValue(float x);

    void _fillRampImage(QImage& colorImg, QImage& alphaImg);
    void _drawRampArea(QPainter& painter);
    void _drawCtrlPntCursors(QPainter& painter);
    void _drawWeightCursors(QPainter& painter);
    void _drawColorWeightCursor(QPainter& painter, const ColorControlPoint& pnt0, const ColorControlPoint& pnt1, bool picked = false);
    void _drawAlphaWeightCursor(QPainter& painter, const ColorControlPoint& pnt0, const ColorControlPoint& pnt1, bool picked = false);
    int _checkColorCtrlPntCursorSelected(const QPointF& pickPos);
    int _checkAlphaCtrlPntCursorSelected(const QPointF& pickPos);
    int _checkColorWeightCursorSelected(const QPointF& pickPos);
    int _checkColorWeightCursorSelected(int pickedCtrlPnt, const QPointF& pickPos);
    int _checkAlphaWeightCursorSelected(const QPointF& pickPos);
    int _checkAlphaWeightCursorSelected(int pickedCtrlPnt, const QPointF& pickPos);
    void _addColorControlPoint(int xPos);
    void _addAlphaControlPoint(int xPos);

private:
    bool                    _isLink;    // if it's true, rgb color and alpha share the same control points
    ColorAttribute*         _colorAttribute;
    ColorCurve*             _colorCurve;
    ColorCurve*             _alphaCurve;
    bool                    _canAddRemoveControlPoint;
    bool                    _canAddCtrlPntByClick;
    bool                    _canRemoveCtrlPntByClick;

    float                   _curveFitWindowScale;
    float                   _curveFitWindowOffset;

    int                     _pickedColorCtrlPnt;
    int                     _pickedAlphaCtrlPnt;
    int                     _pickedColorWeight;
    int                     _pickedAlphaWeight;
    bool                    _dragColorCtrlPnt;
    bool                    _dragAlphaCtrlPnt;
    bool                    _dragColorWeight;
    bool                    _dragAlphaWeight;

    int                     _colorCtrlPntToRemove;
    int                     _alphaCtrlPntToRemove;
    QMenu*                  _contextMenu;
    QAction*                _removeCtrlPntAction;
};

} // namespace CurveEditor
} // namespace nvidia

#endif // COLORWIDGET_H
