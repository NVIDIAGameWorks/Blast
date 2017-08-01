#include "FractureSliceSettingsPanel.h"
#include "ui_FractureSliceSettingsPanel.h"
#include "SimpleScene.h"
#include "SampleManager.h"
#include <QtWidgets/QMessageBox>
#include "FractureGeneralPanel.h"
#include "BlastFamily.h"

FractureSliceSettingsPanel::FractureSliceSettingsPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FractureSliceSettingsPanel),
	_updateData(true),
	_generalPanel(nullptr)
{
    ui->setupUi(this);
}

FractureSliceSettingsPanel::~FractureSliceSettingsPanel()
{
    delete ui;
}

void FractureSliceSettingsPanel::updateValues()
{
	_updateData = false;
	BPPSlice* slice = _getBPPSlice();

	ui->spinBoxNumSlicesX->setValue(slice->numSlicesX);
	ui->spinBoxNumSlicesY->setValue(slice->numSlicesY);
	ui->spinBoxNumSlicesZ->setValue(slice->numSlicesZ);
	ui->spinBoxOffsetVariation->setValue(slice->offsetVariation);
	ui->spinBoxRotationVariation->setValue(slice->rotationVariation);
	ui->spinBoxNoiseAmplitude->setValue(slice->noiseAmplitude);
	ui->spinBoxNoiseFrequency->setValue(slice->noiseFrequency);
	ui->spinBoxNoiseOctaveNumber->setValue(slice->noiseOctaveNumber);
	ui->spinBoxNoiseSeed->setValue(slice->noiseSeed);
	ui->spinBoxSurfaceResolution->setValue(slice->surfaceResolution);
	_updateData = true;
}

void FractureSliceSettingsPanel::on_spinBoxNumSlicesX_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->numSlicesX = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNumSlicesY_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->numSlicesY = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNumSlicesZ_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->numSlicesZ = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxOffsetVariation_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->offsetVariation = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxRotationVariation_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->rotationVariation = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNoiseAmplitude_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->noiseAmplitude = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNoiseFrequency_valueChanged(double arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->noiseFrequency = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNoiseOctaveNumber_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->noiseOctaveNumber = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNoiseSeed_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->noiseSeed = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxSurfaceResolution_valueChanged(int arg1)
{
	if (!_updateData)
		return;

	BPPSlice* slice = _getBPPSlice();
	slice->surfaceResolution = arg1;
}

void FractureSliceSettingsPanel::on_btnApplyFracture_clicked()
{
	BPPSlice* slice = _getBPPSlice();
	SliceFractureExecutor executor;
	executor.setBPPSlice(slice);

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

BPPSlice* FractureSliceSettingsPanel::_getBPPSlice()
{
	BPPSlice* slice = nullptr;
	FracturePreset* preset = _generalPanel->getCurrentFracturePreset();
	if (nullptr != preset)
	{
		slice = &(preset->fracture.slice);
	}
	else
	{
		slice = &(BlastProject::ins().getParams().fracture.slice);
	}
	return slice;
}
