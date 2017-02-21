#include "AppMainWindow.h"
#include "OutputWindow.h"

#include <QtGUI/QContextMenuEvent>

OutputWindow::OutputWindow(QWidget* parent)
	:
	QTextBrowser(parent)
{
	ui.setupUi(this);
}

void OutputWindow::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = createStandardContextMenu();
	
	QAction* act = new QAction("&Clear All", this);
	connect(act, SIGNAL(triggered()), this, SLOT(clear()));
	menu->addAction(act);
	menu->exec(event->globalPos());
	delete menu;
}
