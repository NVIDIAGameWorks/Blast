#include "FractureSliceSettingsPanel.h"
#include "ui_FractureSliceSettingsPanel.h"
#include "ProjectParams.h"
#include "SimpleScene.h"
#include "SampleManager.h"
#include <QtWidgets/QMessageBox>

FractureSliceSettingsPanel::FractureSliceSettingsPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FractureSliceSettingsPanel)
{
    ui->setupUi(this);
}

FractureSliceSettingsPanel::~FractureSliceSettingsPanel()
{
    delete ui;
}

void FractureSliceSettingsPanel::updateValues()
{
	BPPSlice& slice = BlastProject::ins().getParams().fracture.slice;

	ui->spinBoxNumSlices->setValue(slice.numSlices);
	ui->spinBoxOffsetVariation->setValue(slice.offsetVariation);
	ui->spinBoxRotationVariation->setValue(slice.rotationVariation);
	ui->spinBoxNoiseAmplitude->setValue(slice.noiseAmplitude);
	ui->spinBoxNoiseFrequency->setValue(slice.noiseFrequency);
	ui->spinBoxNoiseSeed->setValue(slice.noiseSeed);
}

void FractureSliceSettingsPanel::on_spinBoxNumSlices_valueChanged(int arg1)
{
	BPPSlice& slice = BlastProject::ins().getParams().fracture.slice;
	slice.numSlices = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxOffsetVariation_valueChanged(double arg1)
{
	BPPSlice& slice = BlastProject::ins().getParams().fracture.slice;
	slice.offsetVariation = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxRotationVariation_valueChanged(double arg1)
{
	BPPSlice& slice = BlastProject::ins().getParams().fracture.slice;
	slice.rotationVariation = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNoiseAmplitude_valueChanged(double arg1)
{
	BPPSlice& slice = BlastProject::ins().getParams().fracture.slice;
	slice.noiseAmplitude = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNoiseFrequency_valueChanged(double arg1)
{
	BPPSlice& slice = BlastProject::ins().getParams().fracture.slice;
	slice.noiseFrequency = arg1;
}

void FractureSliceSettingsPanel::on_spinBoxNoiseSeed_valueChanged(int arg1)
{
	BPPSlice& slice = BlastProject::ins().getParams().fracture.slice;
	slice.noiseSeed = arg1;
}

void FractureSliceSettingsPanel::on_btnApplyFracture_clicked()
{
	BPPSlice& slice = BlastProject::ins().getParams().fracture.slice;
	SliceFractureExecutor executor;
	executor.applyNoise(slice.noiseAmplitude, slice.noiseAmplitude, 0, 0, 0, slice.noiseSeed);
	executor.applyConfig(slice.numSlices, slice.numSlices, slice.numSlices, slice.offsetVariation, slice.rotationVariation);

	bool multiplyChunksSelected = false;
	std::map<BlastAsset*, std::vector<uint32_t>> selectedChunks = SampleManager::ins()->getSelectedChunks();
	std::map<BlastAsset*, std::vector<uint32_t>>::iterator itrAssetSelectedChunks = selectedChunks.begin();

	if (selectedChunks.size() > 1)
	{
		multiplyChunksSelected = true;
	}
	else if (selectedChunks.size() == 1 && itrAssetSelectedChunks->second.size() > 1)
	{
		multiplyChunksSelected = true;
	}
	else if((selectedChunks.size() == 1 && itrAssetSelectedChunks->second.size() == 0) || (selectedChunks.size() == 0))
	{
		return;
	}

	if (multiplyChunksSelected)
	{
		QMessageBox::warning(NULL, "Blast Tool", "Now, this tool can only fracture one chunk!");
		return;
	}

	executor.setSourceAsset(itrAssetSelectedChunks->first);
	executor.setTargetChunk(itrAssetSelectedChunks->second.at(0));
	executor.execute();

	//VoronoiFractureExecutor executor;
	//executor.setTargetChunk(0);
	//executor.execute();
}
