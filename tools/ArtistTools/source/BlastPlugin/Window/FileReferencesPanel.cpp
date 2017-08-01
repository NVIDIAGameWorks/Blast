#include "FileReferencesPanel.h"
#include "ui_FileReferencesPanel.h"
#include "AppMainWindow.h"
#include <QtWidgets/QFileDialog>
#include "ProjectParams.h"
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include "GlobalSettings.h"
#include "SampleManager.h"
#include "ViewerOutput.h"

FileReferencesPanel* gFileReferencesPanel = nullptr;
FileReferencesPanel* FileReferencesPanel::ins()
{
	return gFileReferencesPanel;
}

FileReferencesPanel::FileReferencesPanel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FileReferencesPanel)
{
    ui->setupUi(this);
	ui->lineEditFBXSourceAsset->setReadOnly(true);

	ui->lineEditFBX->setText("New.fbx");
	ui->lineEditObj->setText("New.obj");
	ui->lineEditCollision->setText("New.repx");
	ui->lineEditLLAsset->setText("Newll.blast");
	ui->lineEditTKAsset->setText("Newtk.blast");
	ui->lineEditBPXA->setText("New.blast");
	bValid = false;
	ui->checkBoxFBX->setChecked(false);
	ui->checkBoxObj->setChecked(false);
	ui->checkBoxCollision->setChecked(false);
	ui->checkBoxLLAsset->setChecked(false);
	ui->checkBoxTKAsset->setChecked(false);
	ui->checkBoxBPXA->setChecked(false);
	bValid = true;

	gFileReferencesPanel = this;

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

	ui->lineEditFBX->setText("");
	ui->lineEditObj->setText("");
	ui->lineEditCollision->setText("");
	ui->lineEditLLAsset->setText("");
	ui->lineEditTKAsset->setText("");
	ui->lineEditBPXA->setText("");
	bValid = false;
	ui->checkBoxFBX->setChecked(false);
	ui->checkBoxObj->setChecked(false);
	ui->checkBoxCollision->setChecked(false);
	ui->checkBoxLLAsset->setChecked(false);
	ui->checkBoxTKAsset->setChecked(false);
	ui->checkBoxBPXA->setChecked(false);
	bValid = true;

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager == nullptr)
	{
		return;
	}

	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = AssetDescMap.find(pBlastAsset);
	if (itADM  == AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset modelAsset = itADM->second;
	if (modelAsset.name.empty())
	{
		return;
	}

	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	BPPAsset asset;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		asset = assetArray.buf[aaas];
		std::string assetname = asset.name;
		if (assetname == modelAsset.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return;
	}

	QFileInfo fileInfo(modelAsset.name.c_str());

	if (asset.fbx.buf != nullptr)
		ui->lineEditFBX->setText(asset.fbx.buf);
	else
	{
		ui->lineEditFBX->setText(fileInfo.baseName() + "_New.fbx");
	}

	if (asset.obj.buf != nullptr)
		ui->lineEditObj->setText(asset.obj.buf);
	else
	{
		ui->lineEditObj->setText(fileInfo.baseName() + "_New.obj");
	}

	if (asset.collision.buf != nullptr)
		ui->lineEditCollision->setText(asset.collision.buf);
	else
	{
		ui->lineEditCollision->setText(fileInfo.baseName() + "_New.repx");
	}

	if (asset.llasset.buf != nullptr)
		ui->lineEditLLAsset->setText(asset.llasset.buf);
	else
	{
		ui->lineEditLLAsset->setText(fileInfo.baseName() + "_Newll.blast");
	}

	if (asset.tkasset.buf != nullptr)
		ui->lineEditTKAsset->setText(asset.tkasset.buf);
	else
	{
		ui->lineEditTKAsset->setText(fileInfo.baseName() + "_Newtk.blast");
	}

	if (asset.bpxa.buf != nullptr)
		ui->lineEditBPXA->setText(asset.bpxa.buf);
	else
	{
		ui->lineEditBPXA->setText(fileInfo.baseName() + "_New.blast");
	}

	bValid = false;
	ui->checkBoxFBX->setChecked(asset.exportFBX);
	ui->checkBoxObj->setChecked(asset.exportOBJ);
	ui->checkBoxCollision->setChecked(asset.exportCollision);
	ui->checkBoxLLAsset->setChecked(asset.exportLLAsset);
	ui->checkBoxTKAsset->setChecked(asset.exportTKAsset);
	ui->checkBoxBPXA->setChecked(asset.exportBPXA);
	bValid = true;
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
	if (!bValid)
	{
		return;
	}

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager == nullptr)
	{
		return;
	}

	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = AssetDescMap.find(pBlastAsset);
	if (itADM == AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset modelAsset = itADM->second;
	if (modelAsset.name.empty())
	{
		return;
	}

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		std::string assetname = assetArray.buf[aaas].name;
		if (assetname == modelAsset.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return;
	}

	BPPAsset& asset = assetArray.buf[aaas];
	asset.exportFBX = ui->checkBoxFBX->isChecked();
}

void FileReferencesPanel::on_checkBoxObj_stateChanged(int arg1)
{
	if (!bValid)
	{
		return;
	}

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager == nullptr)
	{
		return;
	}

	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = AssetDescMap.find(pBlastAsset);
	if (itADM == AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset modelAsset = itADM->second;
	if (modelAsset.name.empty())
	{
		return;
	}

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		std::string assetname = assetArray.buf[aaas].name;
		if (assetname == modelAsset.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return;
	}

	BPPAsset& asset = assetArray.buf[aaas];
	asset.exportOBJ = ui->checkBoxObj->isChecked();
}

void FileReferencesPanel::on_checkBoxCollision_stateChanged(int arg1)
{
	if (!bValid)
	{
		return;
	}

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager == nullptr)
	{
		return;
	}

	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = AssetDescMap.find(pBlastAsset);
	if (itADM == AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset modelAsset = itADM->second;
	if (modelAsset.name.empty())
	{
		return;
	}

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		std::string assetname = assetArray.buf[aaas].name;
		if (assetname == modelAsset.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return;
	}

	BPPAsset& asset = assetArray.buf[aaas];
	asset.exportCollision = ui->checkBoxCollision->isChecked();
}

void FileReferencesPanel::on_checkBoxLLAsset_stateChanged(int arg1)
{
	if (!bValid)
	{
		return;
	}

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager == nullptr)
	{
		return;
	}

	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = AssetDescMap.find(pBlastAsset);
	if (itADM == AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset modelAsset = itADM->second;
	if (modelAsset.name.empty())
	{
		return;
	}

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		std::string assetname = assetArray.buf[aaas].name;
		if (assetname == modelAsset.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return;
	}

	BPPAsset& asset = assetArray.buf[aaas];
	asset.exportLLAsset = ui->checkBoxLLAsset->isChecked();
}
	
void FileReferencesPanel::on_checkBoxTKAsset_stateChanged(int arg1)
{
	if (!bValid)
	{
		return;
	}

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager == nullptr)
	{
		return;
	}

	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = AssetDescMap.find(pBlastAsset);
	if (itADM == AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset modelAsset = itADM->second;
	if (modelAsset.name.empty())
	{
		return;
	}

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		std::string assetname = assetArray.buf[aaas].name;
		if (assetname == modelAsset.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return;
	}

	BPPAsset& asset = assetArray.buf[aaas];
	asset.exportTKAsset = ui->checkBoxTKAsset->isChecked();
}
	
void FileReferencesPanel::on_checkBoxBPXA_stateChanged(int arg1)
{
	if (!bValid)
	{
		return;
	}

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager == nullptr)
	{
		return;
	}

	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = AssetDescMap.find(pBlastAsset);
	if (itADM == AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset modelAsset = itADM->second;
	if (modelAsset.name.empty())
	{
		return;
	}

	BPParams& projectParams = BlastProject::ins().getParams();
	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		std::string assetname = assetArray.buf[aaas].name;
		if (assetname == modelAsset.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return;
	}

	BPPAsset& asset = assetArray.buf[aaas];
	asset.exportBPXA = ui->checkBoxBPXA->isChecked();

	if (asset.exportBPXA)
	{
		bValid = false;
		ui->checkBoxObj->setChecked(true);
		asset.exportOBJ = true;
		bValid = true;
	}
}

void FileReferencesPanel::on_btnSave_clicked()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPFileReferences& fileReferences = projectParams.blast.fileReferences;

	copy(fileReferences.fbxSourceAsset, ui->lineEditFBXSourceAsset->text().toUtf8().data());

	SampleManager* pSampleManager = SampleManager::ins();
	if (pSampleManager == nullptr)
	{
		return;
	}

	BlastAsset* pBlastAsset = pSampleManager->getCurBlastAsset();
	if (pBlastAsset == nullptr)
	{
		return;
	}

	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	std::map<BlastAsset*, AssetList::ModelAsset>::iterator itADM = AssetDescMap.find(pBlastAsset);
	if (itADM == AssetDescMap.end())
	{
		return;
	}

	AssetList::ModelAsset modelAsset = itADM->second;
	if (modelAsset.name.empty())
	{
		return;
	}

	BPPAssetArray& assetArray = projectParams.blast.blastAssets;
	int aaas = 0;
	for (; aaas < assetArray.arraySizes[0]; aaas++)
	{
		std::string assetname = assetArray.buf[aaas].name;
		if (assetname == modelAsset.name)
			break;
	}
	if (aaas == assetArray.arraySizes[0])
	{
		return;
	}

	BPPAsset& asset = assetArray.buf[aaas];

	copy(asset.fbx, ui->lineEditFBX->text().toUtf8().data());
	asset.exportFBX = ui->checkBoxFBX->isChecked();

	copy(asset.obj, ui->lineEditObj->text().toUtf8().data());
	asset.exportOBJ = ui->checkBoxObj->isChecked();

	copy(asset.collision, ui->lineEditCollision->text().toUtf8().data());
	asset.exportCollision = ui->checkBoxCollision->isChecked();

	copy(asset.llasset, ui->lineEditLLAsset->text().toUtf8().data());
	asset.exportLLAsset = ui->checkBoxLLAsset->isChecked();

	copy(asset.tkasset, ui->lineEditTKAsset->text().toUtf8().data());
	asset.exportTKAsset = ui->checkBoxTKAsset->isChecked();

	copy(asset.bpxa, ui->lineEditBPXA->text().toUtf8().data());
	asset.exportBPXA = ui->checkBoxBPXA->isChecked();

	SampleManager::ins()->exportAsset();
}
