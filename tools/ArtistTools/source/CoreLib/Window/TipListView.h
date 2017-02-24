#ifndef TipListView_h__
#define TipListView_h__

#include <QtWidgets/QWidget>
#include <QtWidgets/QListWidget>
#include <QtCore/QEvent>

#include "corelib_global.h"

class TipListView : public QListWidget
{
	Q_OBJECT

public:
	TipListView(QWidget* parent = 0);

	void setTips(QStringList& tips);

protected:
	bool event(QEvent *);
private:

	QStringList mTips;

};
#endif // TipListView_h__
