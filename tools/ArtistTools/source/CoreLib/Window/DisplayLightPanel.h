#ifndef DisplayLightPanel_h__
#define DisplayLightPanel_h__

#include <QtWidgets/QWidget>
#include "ui_DisplayLightPanel.h"

#include "corelib_global.h"

class DisplayLightPanel : public QWidget
{
	Q_OBJECT

public:
	DisplayLightPanel(QWidget* parent);

	public:
		CORELIB_EXPORT void updateValues();

	public slots:
	CORELIB_EXPORT void onListSelectionChanged();

	CORELIB_EXPORT void on_spinLightDistance_valueChanged(double v);
	CORELIB_EXPORT void on_spinLightIntensity_valueChanged(double v);

	CORELIB_EXPORT void on_btnLightUseShadow_stateChanged(int state);
	CORELIB_EXPORT void on_btnVisualizeLight_stateChanged(int state);
	CORELIB_EXPORT void on_btnLightEnable_stateChanged(int state);
	CORELIB_EXPORT void on_btnLinkLightEnable_stateChanged(int state);

	CORELIB_EXPORT void on_btnLightColor_clicked();
	CORELIB_EXPORT void on_btnLightColorTex_clicked();
	CORELIB_EXPORT void on_btnLightColorTexReload_clicked();
	CORELIB_EXPORT void on_btnLightColorTexClear_clicked();

	CORELIB_EXPORT void on_btnVisualizeShadowMap_stateChanged(int state);

	CORELIB_EXPORT void on_cbShadowMapResolution_currentIndexChanged(int index);

private:
	Ui::DisplayLightPanel ui;
	bool _isUpdatingUI;
	void setButtonColor(QPushButton *button, float r, float g, float b);

	void updateUI();
};

#endif
