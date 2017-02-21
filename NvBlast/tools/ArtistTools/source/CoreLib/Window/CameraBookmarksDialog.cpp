#include "CameraBookmarksDialog.h"
#include "SimpleScene.h"
#include "AppMainWindow.h"
#include <QtWidgets/QMessageBox>

CameraBookmarksDialog::CameraBookmarksDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

	ui.buttonBox->button(QDialogButtonBox::Ok)->setFixedWidth(64);
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setFixedWidth(64);
	ui.buttonBox->button(QDialogButtonBox::Ok)->setFixedHeight(23);
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setFixedHeight(23);
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);

	QObject::connect(ui.buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
	QObject::connect(ui.buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

	ui.editBookmarkName->setEnabled(false);
	ui.btnRename->setEnabled(false);
	ui.btnDelete->setEnabled(false);

	_fillListWidget();
}

CameraBookmarksDialog::~CameraBookmarksDialog()
{

}

void CameraBookmarksDialog::on_listWidgetBookmarks_itemSelectionChanged()
{
	if (ui.listWidgetBookmarks->selectedItems().size() == 0)
	{
		ui.editBookmarkName->setEnabled(false);
		ui.btnRename->setEnabled(false);
		ui.btnDelete->setEnabled(false);
	}
	else
	{
		QListWidgetItem* item = ui.listWidgetBookmarks->selectedItems().at(0);
		ui.editBookmarkName->setEnabled(true);
		ui.btnRename->setEnabled(true);
		ui.btnDelete->setEnabled(true);

		ui.editBookmarkName->setText(item->text());
	}
}

void CameraBookmarksDialog::on_editBookmarkName_textChanged(QString val)
{

}

void CameraBookmarksDialog::on_btnRename_clicked()
{
	if (ui.editBookmarkName->text().isEmpty())
	{
		QListWidgetItem* item = ui.listWidgetBookmarks->selectedItems().at(0);
		ui.editBookmarkName->setText(item->text());
	}
	else
	{
		if (_isNameConflict(ui.editBookmarkName->text()))
		{
			QMessageBox::warning(this, tr("Warning"), tr("The new name is conflict with name of other camera bookmark!"));
			return;
		}

		QListWidgetItem* item = ui.listWidgetBookmarks->selectedItems().at(0);
		SimpleScene::Inst()->renameBookmark(item->text(), ui.editBookmarkName->text());
		AppMainWindow::Inst().renameBookmark(item->text(), ui.editBookmarkName->text());
		item->setText(ui.editBookmarkName->text());
	}

}

void CameraBookmarksDialog::on_btnDelete_clicked()
{
	QListWidgetItem* item = ui.listWidgetBookmarks->selectedItems().at(0);
	SimpleScene::Inst()->removeBookmark(item->text());
	AppMainWindow::Inst().removeBookmark(item->text());
	ui.listWidgetBookmarks->takeItem(ui.listWidgetBookmarks->row(item));
	delete item;

}

void CameraBookmarksDialog::_fillListWidget()
{
	QList<QString> bookmarks = SimpleScene::Inst()->getBookmarkNames();
	ui.listWidgetBookmarks->addItems(bookmarks);
}

bool CameraBookmarksDialog::_isNameConflict(const QString& name)
{
	QList<QString> bookmarks = SimpleScene::Inst()->getBookmarkNames();
	int bookmarkCount = bookmarks.size();
	for (int i = 0; i < bookmarkCount; ++i)
	{
		if (bookmarks.at(i) == name)
			return true;
	}

	return false;
}