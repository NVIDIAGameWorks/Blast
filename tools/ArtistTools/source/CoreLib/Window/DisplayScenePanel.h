#ifndef DisplayScenePanel_h__
#define DisplayScenePanel_h__

#include <QtWidgets/QWidget>
#include "ui_DisplayScenePanel.h"

#include "corelib_global.h"

class DisplayScenePanel : public QWidget
{
	Q_OBJECT

public:
	DisplayScenePanel(QWidget* parent);

	public:
		void updateValues();

	public slots:
	CORELIB_EXPORT void on_btnVisualizeWind_stateChanged(int state);
	CORELIB_EXPORT void on_btnShowGrid_stateChanged(int state);
	CORELIB_EXPORT void on_btnShowAxis_stateChanged(int state);
	CORELIB_EXPORT void on_cbRenderType_currentIndexChanged(int index);
	CORELIB_EXPORT void on_btnShowWireframe_stateChanged(int state);
	CORELIB_EXPORT void on_btnShowHUD_stateChanged(int state);
	CORELIB_EXPORT void on_btnComputeStats_stateChanged(int state);
	CORELIB_EXPORT void on_btnComputeProfile_stateChanged(int state);
	CORELIB_EXPORT void on_btnUseLighting_stateChanged(int state);
	CORELIB_EXPORT void on_btnShowGraphicsMesh_stateChanged(int state);
	CORELIB_EXPORT void on_btnShowSkinnedOnly_stateChanged(int state);
	CORELIB_EXPORT void on_btnSkinningDQ_stateChanged(int state);
	CORELIB_EXPORT void on_checkBoxGizmoWithLocal_stateChanged(int state);
	CORELIB_EXPORT void on_checkBoxGizmoWithDepthTest_stateChanged(int state);
	CORELIB_EXPORT void on_checkBoxShowPlane_stateChanged(int state);


private:
	Ui::DisplayScenePanel ui;
};

#endif // DisplayScene_h__
