#ifndef CAMERABOOKMARKSDIALOG_H
#define CAMERABOOKMARKSDIALOG_H

#include <QtWidgets/QDialog>
#include "ui_CameraBookmarksDialog.h"

#include "corelib_global.h"

class CameraBookmarksDialog : public QDialog
{
    Q_OBJECT

public:
    CameraBookmarksDialog(QWidget *parent = 0);
    ~CameraBookmarksDialog();

private slots:
CORELIB_EXPORT void on_listWidgetBookmarks_itemSelectionChanged();
CORELIB_EXPORT void on_editBookmarkName_textChanged(QString val);
CORELIB_EXPORT void on_btnRename_clicked();
CORELIB_EXPORT void on_btnDelete_clicked();

private:
	void _fillListWidget();
	bool _isNameConflict(const QString& name);

private:
    Ui::CameraBookmarksDialog ui;
};

#endif // CAMERABOOKMARKSDIALOG_H
