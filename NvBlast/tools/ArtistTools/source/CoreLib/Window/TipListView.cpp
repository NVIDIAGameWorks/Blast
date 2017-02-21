#include "TipListView.h"
#include <QtWidgets/QToolTip>
#include <QtCore/QEvent>
#include <QtGUI/QEvent.h>


TipListView::TipListView(QWidget* parent)
	:QListWidget(parent)
{
}

void TipListView::setTips(QStringList& tips)
{
	mTips = tips;
}

bool TipListView::event(QEvent *evt)
{
	if (evt->type() == QEvent::ToolTip)
	{
		if (mTips.size() > 0)
		{
			QHelpEvent *helpEvent = (QHelpEvent *)(evt);
			const QPoint& pos = helpEvent->pos();
			QListWidgetItem* pItem = itemAt(pos);
			int index = 0, num = count();
			for (int i = 0; i < num; ++i)
			{
				QListWidgetItem* pi = item(i);
				if (pi == pItem && mTips.size() > i)
				{
					QToolTip::showText(helpEvent->globalPos(), mTips[i]);
					return true;
				}
			}
		}
		//QToolTip::hideText();
		//evt->ignore();
		//return true;
	}
	return QListWidget::event(evt);
}
