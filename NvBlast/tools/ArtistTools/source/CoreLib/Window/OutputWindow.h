#ifndef OutputWindow_h__
#define OutputWindow_h__

#include <QtWidgets/QTextBrowser>
#include "ui_OutputWindow.h"

#include "UIGlobal.h"

class CORELIB_EXPORT OutputWindow : public QTextBrowser
{
	Q_OBJECT

public:
	OutputWindow(QWidget* parent);

	void contextMenuEvent(QContextMenuEvent *event);

private:
	Ui::OutputWindow ui;
};

#endif // DisplayScene_h__
