#ifndef ExpandablePanel_h__
#define ExpandablePanel_h__

#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>

#include "corelib_global.h"

class TitleLabelImpl;
class QString;
class QVBoxLayout;

class TitleLabel : public QLabel
{
	Q_OBJECT

public:
	TitleLabel(QWidget* parent):QLabel(parent){}

signals:
	void LPressSignal();
};

class ExpandablePanel : public QFrame
{
	Q_OBJECT

public:
	CORELIB_EXPORT explicit ExpandablePanel(QWidget* parent, bool collapsed = false);
	explicit ExpandablePanel(QWidget* content, QWidget* parent);
	virtual ~ExpandablePanel();

	CORELIB_EXPORT void SetTitle(const QString& title);
	void SetCollapsed(bool b);
	QWidget* Content() {return _pContent;}
	CORELIB_EXPORT void AddContent(QWidget* content);

public slots:
CORELIB_EXPORT void TitlePress();

private:
	void Init(bool collapsed = false);

private:
	QVBoxLayout* _pMainLayout;
	TitleLabelImpl* _pTitle;
	QWidget* _pContent;
};

#endif // ExpandablePanel_h__
