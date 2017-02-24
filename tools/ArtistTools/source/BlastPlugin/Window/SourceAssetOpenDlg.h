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
	explicit SourceAssetOpenDlg(bool bOpenBlastFile = true, QWidget *parent = 0);
    ~SourceAssetOpenDlg();

    QString getFile();
    bool getSkinned();
    QVector3D getPosition();
    QVector3D getRotationAxis();
    double getRotationDegree();
	bool isAppend();

private slots:
    void on_btnOpenFile_clicked();

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::SourceAssetOpenDlg *ui;
	bool m_bOpenBlastFile;
};

#endif // SOURCEASSETOPENDLG_H
