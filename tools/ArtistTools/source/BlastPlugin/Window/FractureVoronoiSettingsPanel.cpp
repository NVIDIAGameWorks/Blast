#include "FractureVoronoiSettingsPanel.h"
#include "ui_FractureVoronoiSettingsPanel.h"
#include "ProjectParams.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFileInfo>
#include "AppMainWindow.h"
#include "SimpleScene.h"
#include "SampleManager.h"
#include "FractureGeneralPanel.h"
#include "BlastFamily.h"

FractureVoronoiSettingsPanel::FractureVoronoiSettingsPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FractureVoronoiSettingsPanel),
	_updateData(true),
	_generalPanel(nullptr)
{
    ui->setupUi(this);

    ui->groupBoxVisualizers->hide();
}

FractureVoronoiSettingsPanel::~FractureVoronoiSettingsPanel()
{
    delete ui;
}

void FractureVoronoiSettingsPanel::updateValues()
{
	_updateData = false;
	BPPVoronoi* voronoi = _getBPPVoronoi();

	ui->comboBoxSiteGeneration->setCurrentIndex(voronoi->siteGeneration);
	_showCurrentSiteGenerationUI();
	ui->spinBoxNumberOfSites->setValue(voronoi->numSites);

	ui->spinBoxNumberOfClusters->setValue(voronoi->numberOfClusters);
	ui->spinBoxSitesPerCluster->setValue(voronoi->sitesPerCluster);
	ui->spinBoxClusterRadius->setValue(voronoi->clusterRadius);

	_updateData = true;
}

void FractureVoronoiSettingsPanel::on_checkBoxGridPreview_stateChanged(int arg1)
{

}

void FractureVoronoiSettingsPanel::on_checkBoxFracturePreview_stateChanged(int arg1)
{

}

void FractureVoronoiSettingsPanel::on_checkBoxCutterMesh_stateChanged(int arg1)
{

}

void FractureVoronoiSettingsPanel::on_comboBoxSiteGeneration_currentIndexChanged(int index)
{
	if (!_updateData)
		return;

	BPPVoronoi* voronoi = _getBPPVoronoi();
	voronoi->siteGeneration = index;
	_showCurrentSiteGenerationUI();
}

void FractureVoronoiSettingsPanel::on_spinBoxNumberOfSites_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPVoronoi* voronoi = _getBPPVoronoi();
	voronoi->numSites = arg1;
}

void FractureVoronoiSettingsPanel::on_spinBoxNumberOfClusters_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPVoronoi* voronoi = _getBPPVoronoi();
	voronoi->numberOfClusters = arg1;
}

void FractureVoronoiSettingsPanel::on_spinBoxSitesPerCluster_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPVoronoi* voronoi = _getBPPVoronoi();
	voronoi->sitesPerCluster = arg1;
}

void FractureVoronoiSettingsPanel::on_spinBoxClusterRadius_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPVoronoi* voronoi = _getBPPVoronoi();
	voronoi->clusterRadius = arg1;
}

void FractureVoronoiSettingsPanel::on_btnApplyFracture_clicked()
{
	BPPVoronoi* voronoi = _getBPPVoronoi();
	VoronoiFractureExecutor executor;
	executor.setBPPVoronoi(voronoi);

	SampleManager* pSampleManager = SampleManager::ins();
	std::map<BlastAsset*, std::vector<BlastFamily*>>& AssetFamiliesMap = pSampleManager->getAssetFamiliesMap();
	std::map<BlastAsset*, AssetList::ModelAsset>& AssetDescMap = pSampleManager->getAssetDescMap();
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPGraphicsMaterialArray& theArray = projectParams.graphicsMaterials;
	BPPFractureGeneral& fractureGeneral = projectParams.fracture.general;
	int32_t nMaterialIndex = fractureGeneral.applyMaterial;
	std::string materialName = "";
	RenderMaterial* pRenderMaterial = nullptr;
	if (nMaterialIndex > 0)
	{
		BPPGraphicsMaterial& item = theArray.buf[nMaterialIndex - 1];
		materialName = item.name.buf;
		pRenderMaterial = pSampleManager->getRenderMaterial(materialName);
	}

	std::map<BlastAsset*, std::vector<uint32_t>> selectedChunks = SampleManager::ins()->getSelectedChunks();
	std::map<BlastAsset*, std::vector<uint32_t>>::iterator itrAssetSelectedChunks = selectedChunks.begin();
	for (; itrAssetSelectedChunks != selectedChunks.end(); itrAssetSelectedChunks++)
	{
		if (itrAssetSelectedChunks->second.size() == 0)
		{
			continue;
		}

		if (pRenderMaterial != nullptr)
		{
			BlastAsset* pBlastAsset = itrAssetSelectedChunks->first;
			std::vector<BlastFamily*> families = AssetFamiliesMap[pBlastAsset];
			int familySize = families.size();
			for (int fs = 0; fs < familySize; fs++)
			{
				BlastFamily* pBlastFamily = families[fs];
				pBlastFamily->setMaterial(pRenderMaterial, false);

				AssetList::ModelAsset modelAsset = AssetDescMap[pBlastAsset];
				int assetID = BlastProject::ins().getAssetIDByName(modelAsset.name.c_str());
				BPPAssetInstance* instance = BlastProject::ins().getAssetInstance(assetID, fs);
				copy(instance->inMaterial, materialName.c_str());
			}
		}

		executor.setSourceAsset(itrAssetSelectedChunks->first);
		executor.setTargetChunks(itrAssetSelectedChunks->second);
		executor.execute();
	}
}

BPPVoronoi* FractureVoronoiSettingsPanel::_getBPPVoronoi()
{
	BPPVoronoi* voronoi = nullptr;
	FracturePreset* preset = _generalPanel->getCurrentFracturePreset();
	if (nullptr != preset)
	{
		voronoi = &(preset->fracture.voronoi);
	}
	else
	{
		voronoi = &(BlastProject::ins().getParams().fracture.voronoi);
	}
	return voronoi;
}

void FractureVoronoiSettingsPanel::_showCurrentSiteGenerationUI()
{
	ui->widgetUniform->hide();
	ui->widgetClusters->hide();
	BPPVoronoi* voronoi = _getBPPVoronoi();
	switch(voronoi->siteGeneration)
	{
	case 0:
		ui->widgetUniform->show();
		break;
	case 1:
		ui->widgetClusters->show();
		break;
	}
}
