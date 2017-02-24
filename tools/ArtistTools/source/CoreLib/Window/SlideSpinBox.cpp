#include "SlideSpinBox.h"

void SlideSpinBoxSlots::timeout()
{
	//qDebug("%s", __FUNCTION__);
	this->_cb->on_timeout();
}

void ConfigSpinBoxInt(SlideSpinBoxInt* spinBox, int minv, int maxv, int step)
{
	spinBox->blockSignals(true);
	spinBox->setRange(minv, maxv);
	spinBox->setSingleStep(step);
	spinBox->AutoConfigDragRange();
	spinBox->setKeyboardTracking(false);
	spinBox->blockSignals(false);
}
