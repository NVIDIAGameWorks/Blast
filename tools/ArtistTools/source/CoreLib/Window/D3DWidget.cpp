#include <QtGui/QResizeEvent>
#include <QtGui/QMouseEvent>
#include <QtCore/QTimer>
#include <QtCore/QMimeData>

#include "D3DWidget.h"
#include "AppMainWindow.h"
#include "SimpleScene.h"

D3DWidget::D3DWidget( QWidget* parent )
	: QWidget(parent, Qt::MSWindowsOwnDC)	// Same settings as used in Qt's QGLWidget
{
	// same settings as used in QGLWidget to avoid 'white flickering'
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NoSystemBackground);

	// to get rid of 'black flickering'
	setAttribute(Qt::WA_OpaquePaintEvent, true);

	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	setSizePolicy(sizePolicy);

	_appWindow = qobject_cast<AppMainWindow*>(parent);
	this->setAcceptDrops(true);

	setMouseTracking(true);
}

void D3DWidget::dragEnterEvent(QDragEnterEvent *e)
{
	CoreLib::Inst()->D3DWidget_dragEnterEvent(e);
	e->acceptProposedAction();
}

void D3DWidget::dragMoveEvent(QDragMoveEvent *e)
{
	CoreLib::Inst()->D3DWidget_dragMoveEvent(e);
	e->acceptProposedAction();
}

void D3DWidget::dragLeaveEvent(QDragLeaveEvent *e)
{
	CoreLib::Inst()->D3DWidget_dragLeaveEvent(e);
	//e->acceptProposedAction();
}

void D3DWidget::dropEvent(QDropEvent *e)
{
	CoreLib::Inst()->D3DWidget_dropEvent(e);

	const QMimeData* data = e->mimeData();
	QString name = data->objectName();

	bool hasUrls = data->hasUrls();
	if (!hasUrls)
		return;

	QList<QUrl> urlList = data->urls();
    QStringList fileNames;
    for (int i = 0; i < urlList.size() && i < 32; ++i) {
		fileNames.append(urlList.at(i).toLocalFile());
    }
	
	e->acceptProposedAction();

	AppMainWindow::Inst().processDragAndDrop(fileNames);
}

void D3DWidget::paintEvent( QPaintEvent* e )
{
	SimpleScene::Inst()->Draw();
}

void D3DWidget::Shutdown()
{
	SimpleScene::Inst()->Shutdown();
}

void D3DWidget::Timeout()
{
	SimpleScene::Inst()->Timeout();
}

void D3DWidget::resizeEvent( QResizeEvent* e )
{
	int w = e->size().width();
	int h = e->size().height();
	// resize calls D3D11RenderWindow::Resize
	SimpleScene::Inst()->Resize(w,h);
	// D3DWidget_resizeEvent calls resize in DeviceManager
	CoreLib::Inst()->D3DWidget_resizeEvent(e);
}

void D3DWidget::mouseMoveEvent( QMouseEvent* e )
{
	atcore_float2 pos = gfsdk_makeFloat2(e->x(), e->y());
	SimpleScene::Inst()->onMouseMove(pos);

	Q_ASSERT(_appWindow != NV_NULL);
	char mode = _appWindow->TestMouseScheme(e->modifiers(), e->buttons());
	CoreLib::Inst()->D3DWidget_mouseMoveEvent(e);
	if (!e->isAccepted())
	{
		return;
	}
	if(mode == 0) return;
	SimpleScene::Inst()->Drag(mode);
}

void D3DWidget::wheelEvent(QWheelEvent* e)
{
	SimpleScene::Inst()->onMouseWheel(e->delta());
	SimpleScene::Inst()->WheelZoom();

	CoreLib::Inst()->D3DWidget_wheelEvent(e);
}

void D3DWidget::mousePressEvent( QMouseEvent* e )
{
	atcore_float2 pos = gfsdk_makeFloat2(e->x(), e->y());
	SimpleScene::Inst()->onMouseDown(pos);

	CoreLib::Inst()->D3DWidget_mousePressEvent(e);
}

void D3DWidget::mouseReleaseEvent( QMouseEvent* e )
{
	atcore_float2 pos = gfsdk_makeFloat2(e->x(), e->y());
	SimpleScene::Inst()->onMouseUp(pos);

	CoreLib::Inst()->D3DWidget_mouseReleaseEvent(e);
}

void D3DWidget::keyPressEvent( QKeyEvent* e )
{
	CoreLib::Inst()->D3DWidget_keyPressEvent(e);
}

void D3DWidget::keyReleaseEvent( QKeyEvent* e )
{
	CoreLib::Inst()->D3DWidget_keyReleaseEvent(e);
}

void D3DWidget::contextMenuEvent(QContextMenuEvent *e)
{
	CoreLib::Inst()->D3DWidget_contextMenuEvent(e);
}

