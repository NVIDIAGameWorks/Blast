#ifndef BlastToolbar_h__
#define BlastToolbar_h__

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMenu>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLineEdit>

#include "SlideSpinBox.h"

class BlastToolbar : public QDockWidget
{
	Q_OBJECT

public:
	BlastToolbar(QWidget* parent);

	void updateValues();

public slots:
		void on_btnOpenProject_clicked();
		void on_btnSaveProject_clicked();
		void on_btnExportAssets_clicked();

		void on_btnExportFilepath_clicked();
		void on_ssbiDepthPreview_valueChanged(int v);
		void on_cbExactCoverage_stateChanged(int state);

		void on_btnSelectTool_clicked();
		void on_pointselect_action();
		void on_rectselect_action();
		void on_drawselect_action();

		void on_btnPaintbrush_clicked();
		void on_btnFractureTool_clicked();
		void on_btnExplodedViewTool_clicked();
		void on_btnJointsTool_clicked();
		void on_btnFuseSelectedChunks_clicked();

		void on_btnReset_clicked();
		void on_btnSimulatePlay_clicked();
		void on_btnFrameStepForward_clicked();

		void on_btnBomb_clicked();
		void on_btnProjectile_clicked();
		void on_btnDropObject_clicked();

		void on_btnPreferences_clicked();

private:
	QHBoxLayout *hLayout;
	QFrame *fSeparate;

	QPushButton *btnOpenProject;
	QPushButton *btnSaveProject;
	QPushButton *btnExportAssets;
	
	QVBoxLayout *vLayoutExport;
	QHBoxLayout *hLayoutExport;	
	QLabel *lExportFilepath;
	QPushButton *btnExportFilepath;
	QLineEdit* leExportFilepath;

	QVBoxLayout *vLayoutDepthCoverage;
	QHBoxLayout *hlDepthPreview;
	QLabel *lbDepthPreview;
	SlideSpinBoxInt* ssbiDepthPreview;
	QHBoxLayout *hlExactCoverage;
	QLabel *lbExactCoverage;
	QCheckBox* cbExactCoverage;
	
	QPushButton *btnSelectTool;	
	QPushButton *btnPaintbrush;
	QPushButton *btnFractureTool;	
	QPushButton *btnExplodedViewTool;
	QPushButton *btnJointsTool;
	QPushButton *btnFuseSelectedChunks;
	
	QPushButton *btnReset;
	QPushButton *btnSimulatePlay;
	QPushButton *btnFrameStepForward;
	
	QPushButton *btnBomb;
	QPushButton *btnProjectile;
	QPushButton *btnDropObject;
	
	QPushButton *btnPreferences;
};
#endif // BlastToolbar_h__
