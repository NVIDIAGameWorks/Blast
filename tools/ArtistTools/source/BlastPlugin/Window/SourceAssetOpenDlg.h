#ifndef SOURCEASSETOPENDLG_H
#define SOURCEASSETOPENDLG_H

#include <QtWidgets/QDialog>
#include <QtGui/QVector3D>

namespace Ui {
class SourceAssetOpenDlg;
}

class SourceAssetOpenDlg : public QDialog
{
    Q_OBJECT

public:
	explicit SourceAssetOpenDlg(int usefor, QWidget *parent = 0);
    ~SourceAssetOpenDlg();

	void setDefaultFile(const QString& fn);
    QString getFile();
    bool getSkinned();
    QVector3D getPosition();
    QVector3D getRotationAxis();
    double getRotationDegree();
	bool isAppend();
	bool isPreFractured();
	bool isAutoCompute();
	int  sceneUnitIndex();

private slots:
    void on_btnOpenFile_clicked();

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

	void on_checkBoxPreFractured_stateChanged(int arg1);
	void on_checkBoxAutoCompute_stateChanged(int arg1);

private:
    Ui::SourceAssetOpenDlg *ui;
	/*
	m_usefor:
	0 for open fbx file
	1 for open blast file
	2 for add BlastFamily
	*/
	int m_usefor;
};

#endif // SOURCEASSETOPENDLG_H
