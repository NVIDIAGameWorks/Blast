#include "MaterialLibraryPanel.h"
#include "ui_MaterialLibraryPanel.h"
#include <QtCore/QFileInfo>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include "QtUtil.h"
#include "AppMainWindow.h"
#include "SampleManager.h"
#include "MaterialAssignmentsPanel.h"
#include "FractureGeneralPanel.h"
#include "ResourceManager.h"
enum ETextureType
{
	eDiffuseTexture,
	eSpecularTexture,
	eNormalTexture
};

void OnTextureButtonClicked(BPPGraphicsMaterial& material, ETextureType t, QPushButton* pButton)
{
	QString texName = AppMainWindow::Inst().OpenTextureFile();

	QFileInfo fileInfo(texName);
	QByteArray ba = fileInfo.absoluteFilePath().toLocal8Bit();
	const char* filePath = (const char*)(ba);

	switch (t)
	{
	case eDiffuseTexture:
		copy(material.diffuseTextureFilePath, filePath);
		break;
	case eSpecularTexture:
		copy(material.specularTextureFilePath, filePath);
		break;
	case eNormalTexture:
		copy(material.normalTextureFilePath, filePath);
		break;
	}

	if (texName.isEmpty())
		pButton->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));
	else
		pButton->setIcon(QIcon(":/AppMainWindow/images/TextureIsUsed_icon.png"));

}

void OnTextureReload(BPPGraphicsMaterial& material, ETextureType t, QPushButton* pButton)
{
	QString texName;
	switch (t)
	{
	case eDiffuseTexture:
		texName = material.diffuseTextureFilePath.buf;
		// to do: reload texture
		break;
	case eSpecularTexture:
		texName = material.specularTextureFilePath.buf;
		// to do: reload texture
		break;
	case eNormalTexture:
		texName = material.normalTextureFilePath.buf;
		// to do: reload texture
		break;
	}

	if (texName.isEmpty())
		pButton->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));
	else
		pButton->setIcon(QIcon(":/AppMainWindow/images/TextureIsUsed_icon.png"));

}

void OnTextureClear(BPPGraphicsMaterial& material, ETextureType t, QPushButton* pButton)
{
	switch (t)
	{
	case eDiffuseTexture:
		freeString(material.diffuseTextureFilePath);
		// to do: clear texture
		break;
	case eSpecularTexture:
		freeString(material.specularTextureFilePath);
		// to do: clear texture
		break;
	case eNormalTexture:
		freeString(material.normalTextureFilePath);
		// to do: clear texture
		break;
	}

	pButton->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));

}

MaterialLibraryPanel* pMaterialLibraryPanel = nullptr;
MaterialLibraryPanel* MaterialLibraryPanel::ins()
{
	return pMaterialLibraryPanel;
}

MaterialLibraryPanel::MaterialLibraryPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MaterialLibraryPanel)
{
    ui->setupUi(this);

	pMaterialLibraryPanel = this;
}

MaterialLibraryPanel::~MaterialLibraryPanel()
{
    delete ui;
}

void MaterialLibraryPanel::updateValues()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPGraphicsMaterialArray& graphicsMaterialArray = projectParams.graphicsMaterials;

	ui->listWidget->clear();
	QStringList materialNames;
	int count = graphicsMaterialArray.arraySizes[0];
	for (int i = 0; i < count; ++i)
	{
		materialNames.append(graphicsMaterialArray.buf[i].name.buf);
	}
	ui->listWidget->addItems(materialNames);

	if (count > 0)
	{
		ui->btnModifyMat->setEnabled(true);
		ui->btnRemoveMat->setEnabled(true);
	}
	else
	{
		ui->btnModifyMat->setEnabled(false);
		ui->btnRemoveMat->setEnabled(false);
	}

	MaterialAssignmentsPanel::ins()->updateValues();
	FractureGeneralPanel::ins()->updateValues();
}

void MaterialLibraryPanel::on_btnAddMat_clicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input name for new graphics material:"),
		QLineEdit::Normal,
		"",
		&ok);
	bool nameExist = BlastProject::ins().isGraphicsMaterialNameExist(name.toUtf8().data());
	if (ok && !name.isEmpty() && !nameExist)
	{
		std::string strName = name.toUtf8().data();
		if (!BlastProject::ins().isGraphicsMaterialNameExist(strName.c_str()))
		{
			BlastProject::ins().addGraphicsMaterial(strName.c_str());
			updateValues();
			ui->listWidget->setCurrentRow(ui->listWidget->count() - 1);
		}
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && name.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the new graphics material!");
	}
}

void MaterialLibraryPanel::on_btnModifyMat_clicked()
{
	QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
	if (items.size() == 0)
	{
		SampleManager::ins()->output("please select a material first !");
		return;
	}
	QByteArray tmp = items.at(0)->text().toUtf8();
	const char* oldName = tmp.data();

	bool ok = false;
	QString newName = QInputDialog::getText(this,
		tr("Blast Tool"),
		tr("Please input new name for the selected graphics material:"),
		QLineEdit::Normal,
		oldName,
		&ok);
	bool nameExist = BlastProject::ins().isGraphicsMaterialNameExist(newName.toUtf8().data());
	if (ok && !newName.isEmpty() && !nameExist)
	{
		std::string strOldName = oldName;
		std::string strNewName = newName.toUtf8().data();

		SampleManager::ins()->renameRenderMaterial(strOldName, strNewName);

		BlastProject::ins().renameGraphicsMaterial(oldName, newName.toUtf8().data());
		updateValues();
	}
	else if (ok && nameExist)
	{
		QMessageBox::warning(this, "Blast Tool", "The name you input is already exist!");
	}
	else if (ok && newName.isEmpty())
	{
		QMessageBox::warning(this, "Blast Tool", "You need input a name for the selected graphics material!");
	}
}

void MaterialLibraryPanel::on_btnRemoveMat_clicked()
{
	QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
	if (items.size() == 0)
	{
		SampleManager::ins()->output("please select a material first !");
		return;
	}
	QByteArray tem = items.at(0)->text().toUtf8();

	std::string strName = tem.data();

	SampleManager::ins()->removeRenderMaterial(strName);

	BlastProject::ins().removeGraphicsMaterial(strName.c_str());
	updateValues();
	ui->listWidget->setCurrentRow(ui->listWidget->count() - 1);
}

void MaterialLibraryPanel::on_listWidget_currentRowChanged(int currentRow)
{
	_refreshMaterialValues(currentRow);
}

void MaterialLibraryPanel::on_btnDiffuseColor_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		atcore_float4* color = (atcore_float4*)&(material->diffuseColor);
		pickColor(*color);
		setButtonColor(ui->btnDiffuseColor, color->x, color->y, color->z);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();

		BlastProject::ins().reloadDiffuseColor(strName.c_str(), color->x, color->y, color->z);

		SampleManager::ins()->reloadRenderMaterial(strName, color->x, color->y, color->z);
	}
}

void MaterialLibraryPanel::on_btnDiffuseColorTex_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureButtonClicked(*material, eDiffuseTexture, ui->btnDiffuseColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();
		QString qTexture = material->diffuseTextureFilePath.buf;
		std::string strTexture = qTexture.toUtf8().data();

		BlastProject::ins().reloadDiffuseTexture(strName.c_str(), strTexture.c_str());

		SampleManager::ins()->reloadRenderMaterial(strName, strTexture);
	}
}

void MaterialLibraryPanel::on_btnDiffuseColorTexReload_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureReload(*material, eDiffuseTexture, ui->btnDiffuseColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();
		QString qTexture = material->diffuseTextureFilePath.buf;
		std::string strTexture = qTexture.toUtf8().data();
		
		SampleManager::ins()->reloadRenderMaterial(strName, "");
		ResourceManager::ins()->releaseTexture(strTexture.c_str());
		SampleManager::ins()->reloadRenderMaterial(strName, strTexture);
	}
}

void MaterialLibraryPanel::on_btnDiffuseColorTexClear_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureClear(*material, eDiffuseTexture, ui->btnDiffuseColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();

		BlastProject::ins().reloadDiffuseTexture(strName.c_str(), "");
		
		SampleManager::ins()->reloadRenderMaterial(strName, "");
	}
}

void MaterialLibraryPanel::on_btnSpecularColor_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		atcore_float4* color = (atcore_float4*)&(material->specularColor);
		pickColor(*color);
		setButtonColor(ui->btnSpecularColor, color->x, color->y, color->z);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();

		BlastProject::ins().reloadSpecularColor(strName.c_str(), color->x, color->y, color->z);

		SampleManager::ins()->reloadRenderMaterial(strName, color->x, color->y, color->z, false);
	}
}

void MaterialLibraryPanel::on_btnSpecularColorTex_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureButtonClicked(*material, eSpecularTexture, ui->btnSpecularColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();
		QString qTexture = material->specularTextureFilePath.buf;
		std::string strTexture = qTexture.toUtf8().data();

		BlastProject::ins().reloadSpecularTexture(strName.c_str(), strTexture.c_str());

		SampleManager::ins()->reloadRenderMaterial(strName, strTexture, RenderMaterial::TT_Specular);
	}
}

void MaterialLibraryPanel::on_btnSpecularColorTexReload_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureReload(*material, eSpecularTexture, ui->btnSpecularColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();
		QString qTexture = material->specularTextureFilePath.buf;
		std::string strTexture = qTexture.toUtf8().data();

		SampleManager::ins()->reloadRenderMaterial(strName, "", RenderMaterial::TT_Specular);
		ResourceManager::ins()->releaseTexture(strTexture.c_str());
		SampleManager::ins()->reloadRenderMaterial(strName, strTexture, RenderMaterial::TT_Specular);
	}
}

void MaterialLibraryPanel::on_btnSpecularColorTexClear_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureClear(*material, eSpecularTexture, ui->btnSpecularColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();

		BlastProject::ins().reloadSpecularTexture(strName.c_str(), "");

		SampleManager::ins()->reloadRenderMaterial(strName, "", RenderMaterial::TT_Specular);
	}
}

void MaterialLibraryPanel::on_spinSpecularShin_valueChanged(double arg1)
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		float specularShininess = (float)arg1;

		material->specularShininess = specularShininess;

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();
		SampleManager::ins()->reloadRenderMaterial(strName, specularShininess);
	}
}

void MaterialLibraryPanel::on_btnNormalColorTex_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureButtonClicked(*material, eNormalTexture, ui->btnNormalColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();
		QString qTexture = material->normalTextureFilePath.buf;
		std::string strTexture = qTexture.toUtf8().data();

		BlastProject::ins().reloadNormalTexture(strName.c_str(), strTexture.c_str());

		SampleManager::ins()->reloadRenderMaterial(strName, strTexture, RenderMaterial::TT_Normal);
	}
}

void MaterialLibraryPanel::on_btnNormalColorTexReload_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureReload(*material, eNormalTexture, ui->btnNormalColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();
		QString qTexture = material->normalTextureFilePath.buf;
		std::string strTexture = qTexture.toUtf8().data();

		SampleManager::ins()->reloadRenderMaterial(strName, "", RenderMaterial::TT_Normal);
		ResourceManager::ins()->releaseTexture(strTexture.c_str());
		SampleManager::ins()->reloadRenderMaterial(strName, strTexture, RenderMaterial::TT_Normal);
	}
}

void MaterialLibraryPanel::on_btnNormalColorTexClear_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureClear(*material, eNormalTexture, ui->btnNormalColorTex);

		QString qName = material->name.buf;
		std::string strName = qName.toUtf8().data();

		BlastProject::ins().reloadNormalTexture(strName.c_str(), "");

		SampleManager::ins()->reloadRenderMaterial(strName, "", RenderMaterial::TT_Normal);
	}
}

void MaterialLibraryPanel::_refreshMaterialValues(int idx)
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPGraphicsMaterialArray& graphicsMaterialArray = projectParams.graphicsMaterials;
	int count = graphicsMaterialArray.arraySizes[0];
	if (idx >= 0 && idx < count)
	{
		const BPPGraphicsMaterial& material = graphicsMaterialArray.buf[idx];

		ui->spinSpecularShin->setValue(material.specularShininess);

		const nvidia::NvVec4& diffuseColor = material.diffuseColor;
		const nvidia::NvVec4& specularColor = material.specularColor;
		setButtonColor(ui->btnDiffuseColor, diffuseColor.x, diffuseColor.y, diffuseColor.z);
		setButtonColor(ui->btnSpecularColor, specularColor.x, specularColor.y, specularColor.z);

		updateTextureButton(ui->btnDiffuseColorTex, material.diffuseTextureFilePath.buf);
		updateTextureButton(ui->btnSpecularColorTex, material.specularTextureFilePath.buf);
		updateTextureButton(ui->btnNormalColorTex, material.normalTextureFilePath.buf);
	}
	else
	{
		ui->spinSpecularShin->setValue(0.0f);

		setButtonColor(ui->btnDiffuseColor, 0, 0, 0);
		setButtonColor(ui->btnSpecularColor, 0, 0, 0);
		updateTextureButton(ui->btnDiffuseColorTex, "");
		updateTextureButton(ui->btnSpecularColorTex, "");
		updateTextureButton(ui->btnNormalColorTex, "");
	}
}

BPPGraphicsMaterial* MaterialLibraryPanel::_getSelectedMaterial()
{
	BPParams& projectParams = BlastProject::ins().getParams();
	BPPGraphicsMaterialArray& graphicsMaterialArray = projectParams.graphicsMaterials;

	int idx = ui->listWidget->currentRow();
	int count = graphicsMaterialArray.arraySizes[0];
	if (idx < 0 || idx > count)
		return nullptr;

	return &(graphicsMaterialArray.buf[idx]);
}
