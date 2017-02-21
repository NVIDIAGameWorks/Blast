#ifndef SlideSpinBox_h__
#define SlideSpinBox_h__

#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionSpinBox>
#include <QtWidgets/QAbstractSpinBox>
#include <QtGui/QMouseEvent>
#include <QtCore/QTimer>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QSpinBox>

#include "corelib_global.h"

/*
	A Lesson: never use a template to sub-class QWidgets!
	Qt doesn't moc template widget classes;
	So we have to use a specific 'slots-callback' pair to let the moc system work...
 */
class SlideSpinBoxCallback
{
public:
	virtual void on_timeout() = 0;
};

class SlideSpinBoxSlots : public QObject
{
	Q_OBJECT

public:
	SlideSpinBoxSlots(SlideSpinBoxCallback* cb)
		: _cb (cb)
	{}

	public slots:
		CORELIB_EXPORT void timeout();

private:
	SlideSpinBoxCallback* _cb;
};

template<typename SpinWidget, typename ValueType>
class SlideSpinBox : public SpinWidget, public SlideSpinBoxCallback
{

public:
	void on_timeout()
	{
		//qDebug("%s", __FUNCTION__);
		EmitDragValueChanged();
	}

public:
	typedef ValueType value_type;

	SlideSpinBox(QWidget* parent = 0)
		: SpinWidget(parent)
		, _dragRange(DEFAULT_DRAG_RANGE)
	{
		setAccelerated(true);

		ResetCustState();

		_pSlots = new SlideSpinBoxSlots(this);
		
		_dragTimer.setParent(this);
		_dragTimer.setInterval(DRAG_TIMER_INTERVAL);
		_dragTimer.setSingleShot(true);
		connect(&_dragTimer, SIGNAL(timeout()), _pSlots, SLOT(timeout()));
	}

	~SlideSpinBox()
	{
		delete _pSlots;
	}

	void SetDragRange(int range)
	{
		if(range < MIN_DRAG_RANGE) 
			range = MIN_DRAG_RANGE;
		else if(range > MAX_DRAG_RANGE)
			range = MAX_DRAG_RANGE;
		
		_dragRange = range;
	}

	void AutoConfigDragRange()
	{
		ValueType minv = this->minimum();
		ValueType maxv = this->maximum();

		double vrange = maxv - minv;
		double step = 1.0;

		if(vrange <= 1.0)
		{
			step = 0.01;
		}
		else if(vrange <= 10.0)
		{
			step = 0.1;
		}

		double pixelError = 2.0;
		double dragRange = vrange * pixelError / step;

		SetDragRange((int)dragRange);
	}

protected:
	void mouseMoveEvent ( QMouseEvent * event )
	{
		if(!_dragBegin)
		{
			QAbstractSpinBox::mouseMoveEvent(event);
			return;
		}

		if(!_dragging)
		{
			_dragging = true;
			_cursor = this->cursor();

			this->setCursor(Qt::SizeVerCursor);
		}

		//if(_dragging)
		//{
		//	_dragTimer.start();	// always restart, we only need the timeout event;
		//}

		event->accept();
		DragValue(event->pos());
	}

	void mousePressEvent( QMouseEvent * event )
	{
		if(event->button() == Qt::LeftButton)
		{
			SetupCustState(event);
		}
		
		if(_dragging)
			blockSignals(true);
		QAbstractSpinBox::mousePressEvent(event);
		if(_dragging)
			blockSignals(false);
	}

	void mouseReleaseEvent( QMouseEvent * event )
	{
		if(_dragging)
		{
			setCursor(_cursor);
			_dragTimer.stop();
		}

		QAbstractSpinBox::mouseReleaseEvent(event);
		//if(_dragBegin)
		//	EmitDragValueChanged();

		ResetCustState();
	}

	void timerEvent(QTimerEvent *event)
	{
		if(_dragging)
		{
			// stop timer event to disable acceleration/auto-stepping
			event->ignore();
			return;
		}

		event->accept();

		if(_dragBegin) blockSignals(true);
		QAbstractSpinBox::timerEvent(event);
		if(_dragBegin) blockSignals(false);
	}

private:
	enum
	{
		MIN_DRAG_RANGE = 50,
		MAX_DRAG_RANGE = 500,

		DEFAULT_DRAG_RANGE = 100,

		DRAG_TIMER_INTERVAL= 30,	// in msec
	};

	bool SetupCustState(QMouseEvent* event)
	{
		QStyleOptionSpinBox opt;
		this->initStyleOption(&opt);
		QStyle::SubControl hitControl = this->style()->hitTestComplexControl(QStyle::CC_SpinBox, &opt, event->pos());
		if(hitControl != QStyle::SC_SpinBoxUp && hitControl != QStyle::SC_SpinBoxDown)
			return false;

		_pressPosition = event->pos();
		_dragging	= false;
		_dragBegin	= true;
		_pressValue	= this->value();
		_dragValue  = this->value();

		return true;
	}

	void ResetCustState()
	{
		_dragBegin = false;
		_dragging = false;
	}
	
	void DragValue(const QPoint& pos)
	{
		ValueType valueRange = this->maximum() - this->minimum();

		double dh = (double)(_pressPosition.y() - pos.y());
		double dv = valueRange * dh / (double)_dragRange;
		ValueType v = _pressValue + (ValueType)dv;

		//blockSignals(true);
		setValue(v);
		//blockSignals(false);
	}

	void EmitDragValueChanged()
	{
		//if(this->value() != _pressValue)	// value are always bound by qt (and decimal settings)
		if(this->value() != _dragValue)
		{	
			_dragValue = this->value();
			emit valueChanged(this->value());
		}
	}

	int		_dragRange;

	QPoint	_pressPosition;
	ValueType	_pressValue;
	ValueType	_dragValue;

	bool	_dragging;
	bool	_dragBegin;
	QCursor _cursor;

	QTimer _dragTimer;
	SlideSpinBoxSlots* _pSlots;
};

typedef SlideSpinBox<QDoubleSpinBox, double> SlideSpinBoxF;
typedef SlideSpinBox<QSpinBox, int> SlideSpinBoxInt;

template<typename CustSpinBox>
class ConfigSpinBox
{
public:
	typedef typename CustSpinBox::value_type ValueType;

	ConfigSpinBox(CustSpinBox* spinBox, ValueType minv, ValueType maxv, ValueType step = (ValueType)1)
	{
		spinBox->blockSignals(true);
		spinBox->setRange(minv, maxv);
		spinBox->setSingleStep(step);
		spinBox->AutoConfigDragRange();
		spinBox->setKeyboardTracking(false);
		SpecificConfig<ValueType>(spinBox);
		spinBox->blockSignals(false);
	}

	template<typename T>
	void SpecificConfig(CustSpinBox* spinBox){}

	template<>
	void SpecificConfig<double>(CustSpinBox* spinBox)
	{
		spinBox->setDecimals(2.0);
	}
};

#include "corelib_global.h"
CORELIB_EXPORT void ConfigSpinBoxInt(SlideSpinBoxInt* spinBox, int minv, int maxv, int step = (int)1);

#endif // SlideSpinBox_h__
