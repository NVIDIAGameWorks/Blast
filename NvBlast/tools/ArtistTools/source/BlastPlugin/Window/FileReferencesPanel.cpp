#include "FileReferencesPanel.h"
#include "ui_FileReferencesPanel.h"
#include "AppMainWindow.h"
#include <QtWidgets/QFileDialog>
#include "ProjectParams.h"
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include "GlobalSettings.h"

FileReferencesPanel::FileReferencesPanel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FileReferencesPanel)
	, _saveFBX(true)
	, _saveBlast(true)
	, _saveCollision(true)
{
    ui->setupUi(this);
	ui->lineEditFBXSourceAsset->setReadOnly(true);

	ui->checkBoxFBX->setChecked(_saveFBX);
	ui->checkBoxBlast->setChecked(_saveBlast);
	ui->checkBoxCollision->setChecked(_saveCollision);

	updateValues();
}

FileReferencesPanel::~FileReferencesPanel()
{
    delete ui;
}

void FileReferencesPanel::updateValues()
{
	AppMainWindow& window = AppMainWindow::Inst();
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPFileReferences& fileReferences = projectParams.blast.fileReferences;
	if (fileReferences.fbxSourceAsset.buf != nullptr)
		ui->lineEditFBXSourceAsset->setText(fileReferences.fbxSourceAsset.buf);
	else
		ui->lineEditFBXSourceAsset->setText("");

	GlobalSettings& globalSettings = GlobalSettings::Inst();
	QString projectFileName = globalSettings.m_projectFileName.c_str();

	if (projectFileName.isEmpty())
	{
		ui->lineEditFBX->setText("New.fbx");
		ui->lineEditBlast->setText("New.Blast");
		ui->lineEditCollision->setText("New.repx");
	}
	else
	{
		QFileInfo fileInfo(projectFileName);

		if (fileReferences.fbx.buf != nullptr)
			ui->lineEditFBX->setText(fileReferences.fbx.buf);
		else
		{
			ui->lineEditFBX->setText(fileInfo.baseName() + " New.fbx");
		}

		if (fileReferences.blast.buf != nullptr)
			ui->lineEditBlast->setText(fileReferences.blast.buf);
		else
		{
			ui->lineEditBlast->setText(fileInfo.baseName() + " New.Blast");
		}

		if (fileReferences.collision.buf != nullptr)
			ui->lineEditCollision->setText(fileReferences.collision.buf);
		else
		{
			ui->lineEditCollision->setText(fileInfo.baseName() + " New.repX");
		}
	}
}

void FileReferencesPanel::on_btnOpenFile_clicked()
{
	AppMainWindow& window = AppMainWindow::Inst();
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPFileReferences& fileReferences = projectParams.blast.fileReferences;
	const char* fbxSourceAsset = fileReferences.fbxSourceAsset.buf;
	QString lastDir = (fbxSourceAsset != nullptr ? fbxSourceAsset : window._lastFilePath);
	QString fileName = QFileDialog::getOpenFileName(&window, "Open FBX File", lastDir, "FBX File (*.FBX)");

	ui->lineEditFBXSourceAsset->setText(fileName);
}

void FileReferencesPanel::on_btnReload_clicked()
{

}

void FileReferencesPanel::on_btnRemove_clicked()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPFileReferences& fileReferences = projectParams.blast.fileReferences;
	if (fileReferences.fbxSourceAsset.buf != nullptr)
	{
		ui->lineEditFBXSourceAsset->setText("");
		freeString(fileReferences.fbxSourceAsset);
		// to do: remove source fbx file
	}
}

void FileReferencesPanel::on_checkBoxFBX_stateChanged(int arg1)
{
	_saveFBX = (arg1 == 0 ? false : true);
}

void FileReferencesPanel::on_checkBoxBlast_stateChanged(int arg1)
{
	_saveBlast = (arg1 == 0 ? false : true);
}

void FileReferencesPanel::on_checkBoxCollision_stateChanged(int arg1)
{
	_saveCollision = (arg1 == 0 ? false : true);
}

void FileReferencesPanel::on_btnSave_clicked()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPFileReferences& fileReferences = projectParams.blast.fileReferences;

	copy(fileReferences.fbxSourceAsset, ui->lineEditFBXSourceAsset->text().toUtf8().data());

	if (_saveFBX)
	{
		copy(fileReferences.fbx, ui->lineEditFBX->text().toUtf8().data());
		// to do: save fbx file
	}

	if (_saveBlast)
	{
		copy(fileReferences.blast, ui->lineEditBlast->text().toUtf8().data());
		// to do: save blast file
	}

	if (_saveCollision)
	{
		copy(fileReferences.collision, ui->lineEditCollision->text().toUtf8().data());
		// to do: save collision file
	}
}
