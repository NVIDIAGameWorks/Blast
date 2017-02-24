#ifndef ALPHADIALOG_H
#define ALPHADIALOG_H

#include <QtWidgets/QDialog>

namespace Ui {
    class AlphaDialog;
}

namespace nvidia {
namespace CurveEditor {

class AlphaDialog : public QDialog
{
    Q_OBJECT

public:
    static int getAlpha(int alpha = 255, QWidget *parent = 0);

    explicit AlphaDialog(QWidget *parent = 0, int alpha = 255);
    ~AlphaDialog();

protected:
    // QWidget events
    virtual void paintEvent(QPaintEvent * e);

    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
private slots:
    void on_spinBoxAlpha_valueChanged(int arg1);

private:
    void drawAlphaRectangle(QPainter& painter);
    void drawCursor(QPainter& painter, int xPos);

private:
    Ui::AlphaDialog *ui;
    bool                    _drag;
    int                     _alpha;
    int                     _xOffset;
};

} // namespace CurveEditor
} // namespace nvidia

#endif // ALPHADIALOG_H
