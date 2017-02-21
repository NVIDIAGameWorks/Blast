#include <QtWidgets/QLabel>
#include <QtGui/QImage>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include "ExpandablePanel.h"
#include "Nv.h"

static QImage S_TriangleRight;
static QImage S_TriangleDown;

void InitTriangleResources(int w, int h)
{
	S_TriangleRight = QImage(w, h, QImage::Format_ARGB32);
	S_TriangleDown  = QImage(w, h, QImage::Format_ARGB32);

	S_TriangleRight.fill(QColor(0, 0, 0, 0));
	S_TriangleDown.fill(QColor(0, 0, 0, 0));

	QPainter painter(&S_TriangleRight);
	painter.setRenderHints(QPainter::Antialiasing,true);

	QPainterPath path;
	path.moveTo(0, 0);
	path.lineTo(w, h>>1);
	path.lineTo(0, h);
	path.lineTo(0, 0);
	painter.setPen(Qt::NoPen);
	painter.fillPath(path, QBrush(QColor(50, 50, 50)));

	// a painter cannot switch device?
	QPainter painter2(&S_TriangleDown);
	painter2.setRenderHint(QPainter::Antialiasing,true);
	path = QPainterPath();	// trick to clear up a path
	path.moveTo(0, 0);
	path.lineTo(w, 0);
	path.lineTo(w>>1, h);
	path.lineTo(0, 0);
	painter2.setPen(Qt::NoPen);
	painter2.fillPath(path, QBrush(QColor(50, 50, 50)));
}

class TitleLabelImpl : public TitleLabel
{
	enum
	{
		IMAGE_H = 10,
		IMAGE_W = 10,
		FIXED_H = 20,
	};

public:
	TitleLabelImpl(QWidget* parent, bool collapsed)
		: TitleLabel(parent)
		, _collapsed(collapsed)
	{
		QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		sizePolicy.setHorizontalStretch(0);
		sizePolicy.setVerticalStretch(0);
		sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
		this->setSizePolicy(sizePolicy);
		this->setMinimumSize(QSize(0, FIXED_H));
		this->setMaximumSize(QSize(16777215, FIXED_H));
		//this->setStyleSheet(QString::fromUtf8("background:rgb(219,219,219);border:1px solid rgb(185,185,185);"));
		this->setFrameShape(QFrame::NoFrame);
		this->setFrameShadow(QFrame::Plain);
		this->setTextFormat(Qt::AutoText);
		this->setScaledContents(false);
		this->setAlignment(Qt::AlignCenter);

		if(S_TriangleDown.isNull() || S_TriangleRight.isNull())
			InitTriangleResources(IMAGE_W, IMAGE_H);
	}

	void paintEvent(QPaintEvent * e)
	{
		QLabel::paintEvent(e);

		QPainter painter(this);
		painter.setRenderHints(QPainter::Antialiasing,true);

		const int L = 10;
		const int T = (FIXED_H - IMAGE_H)/2;
		QRect rc(L, T, IMAGE_W, IMAGE_H);

		// [later] transform the painter should also work;
		// but i don't get the expected results, so have to use two images
		if(_collapsed)
			painter.drawImage(rc, S_TriangleRight);
		else
			painter.drawImage(rc, S_TriangleDown);
		
	}

	bool Collapsed() {return _collapsed;}
	void SetCollapsed(bool b) 
	{
		_collapsed = b;
		this->update();
	}
	
protected:
	virtual void mousePressEvent(QMouseEvent* e)
	{
		if(e->buttons() == Qt::LeftButton)
		{
			_collapsed = !_collapsed;
			this->update();
			emit LPressSignal();
		}
	}

private:
	bool _collapsed;
};

ExpandablePanel::ExpandablePanel(QWidget* parent, bool collapsed)
	: QFrame(parent)
	, _pContent(NV_NULL)
{
	//_pContent = new QWidget(this);
	Init(collapsed);
}

ExpandablePanel::ExpandablePanel( QWidget* content, QWidget* parent /*= 0*/ )
	: QFrame(parent)
	, _pContent(content)
{
	Init();
}

ExpandablePanel::~ExpandablePanel()
{

}

void ExpandablePanel::SetCollapsed(bool b)
{
	if (_pTitle)
		_pTitle->SetCollapsed(b);
}

void ExpandablePanel::Init(bool collapsed)
{
	setAutoFillBackground(true);
	setFrameShape(QFrame::StyledPanel);
	setFrameShadow(QFrame::Sunken);
	_pMainLayout = new QVBoxLayout(this);
	_pMainLayout->setSpacing(2);
	_pMainLayout->setContentsMargins(0, 0, 0, 0);
	_pMainLayout->setObjectName(QString::fromUtf8("mainLayout"));

	_pTitle = new TitleLabelImpl(this, collapsed);
	_pTitle->setText("TitleLabel");
	_pMainLayout->addWidget(_pTitle);

	connect(_pTitle, SIGNAL(LPressSignal()), this, SLOT(TitlePress()));
	
	if(_pContent)
		_pMainLayout->addWidget(_pContent);
}

void ExpandablePanel::SetTitle( const QString& title )
{
	_pTitle->setText(title);
}

void ExpandablePanel::TitlePress()
{
	if(_pTitle->Collapsed())
	{
		_pContent->hide();
	}
	else	// expanded
	{
		_pContent->show();
	}
}

void ExpandablePanel::AddContent( QWidget* content )
{
	// only support one widget as content right now
	if (_pContent != NV_NULL) return;

	_pMainLayout->addWidget(content);
	_pContent = content;
}

