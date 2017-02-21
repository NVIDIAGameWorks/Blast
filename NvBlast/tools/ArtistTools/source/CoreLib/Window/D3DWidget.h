#ifndef D3DWidget_h__
#define D3DWidget_h__

#include <QtWidgets/QWidget>
#include "Nv.h"

#include "corelib_global.h"

class AppMainWindow;
class D3DWidget : public QWidget
{
	Q_OBJECT

public:

	D3DWidget(QWidget* parent);
	virtual ~D3DWidget(){}

signals:
	void DropSignal();

public slots:
CORELIB_EXPORT void Timeout();
CORELIB_EXPORT void Shutdown();

protected:
	// return NULL to ignore system painter
	virtual QPaintEngine *paintEngine() const { return NV_NULL; }

	// QWidget events
	virtual void resizeEvent(QResizeEvent* e);
	virtual void paintEvent(QPaintEvent* e);
	virtual void mousePressEvent(QMouseEvent* e);
	virtual void mouseReleaseEvent(QMouseEvent* e);
	virtual void mouseMoveEvent(QMouseEvent* e);
	virtual void wheelEvent ( QWheelEvent * e);
	virtual void keyPressEvent(QKeyEvent* e);
	virtual void keyReleaseEvent(QKeyEvent* e);

	virtual void dragEnterEvent(QDragEnterEvent *e);
    virtual void dragMoveEvent(QDragMoveEvent *e);
    virtual void dragLeaveEvent(QDragLeaveEvent *e);
    virtual void dropEvent(QDropEvent *e);

	virtual void contextMenuEvent(QContextMenuEvent *e);

private:
	AppMainWindow* _appWindow;
};

#endif // D3DWidget_h__
