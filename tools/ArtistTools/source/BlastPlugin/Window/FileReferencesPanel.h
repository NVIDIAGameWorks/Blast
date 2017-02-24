#ifndef FILEREFERENCESPANEL_H
#define FILEREFERENCESPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class FileReferencesPanel;
}

class FileReferencesPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FileReferencesPanel(QWidget *parent = 0);
    ~FileReferencesPanel();
	void updateValues();

private slots:
    void on_btnOpenFile_clicked();

    void on_btnReload_clicked();

    void on_btnRemove_clicked();

    void on_checkBoxFBX_stateChanged(int arg1);

    void on_checkBoxBlast_stateChanged(int arg1);

    void on_checkBoxCollision_stateChanged(int arg1);

    void on_btnSave_clicked();

private:
    Ui::FileReferencesPanel *ui;
	bool					_saveFBX;
	bool					_saveBlast;
	bool					_saveCollision;
};

#endif // FILEREFERENCESPANEL_H
