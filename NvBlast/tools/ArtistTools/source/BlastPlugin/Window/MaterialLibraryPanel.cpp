#include "MaterialLibraryPanel.h"
#include "ui_MaterialLibraryPanel.h"
#include <QtCore/QFileInfo>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include "QtUtil.h"
#include "AppMainWindow.h"
#include "SimpleScene.h"
#include "RenderMaterial.h"
#include "ResourceManager.h"
#include "SampleManager.h"
#include "MaterialAssignmentsPanel.h"
#include "Renderable.h"
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

	SimpleScene::Inst()->SetFurModified(true);
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

	SimpleScene::Inst()->SetFurModified(true);
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

	SimpleScene::Inst()->SetFurModified(true);
}

MaterialLibraryPanel* pMaterialLibraryPanel = nullptr;
MaterialLibraryPanel* MaterialLibraryPanel::ins()
{
	return pMaterialLibraryPanel;
}

void MaterialLibraryPanel::addMaterial(std::string materialName, std::string diffuseTexture)
{
	if (!BlastProject::ins().isGraphicsMaterialNameExist(materialName.c_str()))
	{
		BlastProject::ins().addGraphicsMaterial(materialName.c_str(), diffuseTexture.c_str());
		updateValues();
		ui->listWidget->setCurrentRow(ui->listWidget->count() - 1);
	}
}

void MaterialLibraryPanel::removeMaterial(std::string name)
{
	BlastProject::ins().removeGraphicsMaterial(name.c_str());
	updateValues();
	ui->listWidget->setCurrentRow(ui->listWidget->count() - 1);
}

void MaterialLibraryPanel::deleteMaterials()
{
	std::vector<std::string>::iterator itStr;
	std::vector<Renderable*>::iterator itRenderable;

	for (itStr = m_NeedDeleteRenderMaterials.begin(); itStr != m_NeedDeleteRenderMaterials.end(); itStr++)
	{
		std::string materialName = *itStr;
		RenderMaterial* pRenderMaterial = m_RenderMaterialMap[materialName];
		std::vector<Renderable*>& renderables = pRenderMaterial->getRelatedRenderables();
		for (itRenderable = renderables.begin(); itRenderable != renderables.end(); itRenderable++)
		{
			Renderable* pRenderable = *itRenderable;
			pRenderable->setMaterial(*RenderMaterial::getDefaultRenderMaterial());
		}

		std::map<std::string, RenderMaterial*>::iterator it = m_RenderMaterialMap.find(materialName);
		if (it != m_RenderMaterialMap.end())
		{
			m_RenderMaterialMap.erase(it);
			MaterialLibraryPanel::ins()->removeMaterial(materialName);
		}
	}

	m_NeedDeleteRenderMaterials.clear();
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
		ResourceManager* pResourceManager = ResourceManager::ins();
		std::string strName = name.toUtf8().data();
		RenderMaterial* pRenderMaterial = new RenderMaterial(strName.c_str(), *pResourceManager, "model_simple_textured_ex");
		m_RenderMaterialMap[strName] = pRenderMaterial;
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

		std::map<std::string, RenderMaterial*>::iterator it = m_RenderMaterialMap.find(strOldName);
		if (it != m_RenderMaterialMap.end())
		{
			RenderMaterial* pRenderMaterial = it->second;
			m_RenderMaterialMap.erase(it);
			m_RenderMaterialMap[strNewName] = pRenderMaterial;
		}

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
	std::map<std::string, RenderMaterial*>::iterator it = m_RenderMaterialMap.find(strName);
	if (it != m_RenderMaterialMap.end())
	{
		m_NeedDeleteRenderMaterials.push_back(it->second->getMaterialName());
	}
	else
	{
		SampleManager::ins()->deleteRenderMaterial(strName);
	}
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
	}
}

void MaterialLibraryPanel::on_btnDiffuseColorTex_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureButtonClicked(*material, eDiffuseTexture, ui->btnDiffuseColorTex);
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

		std::map<std::string, RenderMaterial*>& renderMaterials = SampleManager::ins()->getRenderMaterials();
		std::map<std::string, RenderMaterial*>::iterator it = renderMaterials.find(strName);
		if (it != renderMaterials.end())
		{
			it->second->setTextureFileName(strTexture);
		}
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
		QString qTexture = material->diffuseTextureFilePath.buf;
		std::string strTexture = qTexture.toUtf8().data();

		std::map<std::string, RenderMaterial*>& renderMaterials = SampleManager::ins()->getRenderMaterials();
		std::map<std::string, RenderMaterial*>::iterator it = renderMaterials.find(strName);
		if (it != renderMaterials.end())
		{
			it->second->setTextureFileName(strTexture);
		}
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
	}
}

void MaterialLibraryPanel::on_btnSpecularColorTex_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureButtonClicked(*material, eSpecularTexture, ui->btnSpecularColorTex);
	}
}

void MaterialLibraryPanel::on_btnSpecularColorTexReload_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureReload(*material, eSpecularTexture, ui->btnSpecularColorTex);
	}
}

void MaterialLibraryPanel::on_btnSpecularColorTexClear_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureClear(*material, eSpecularTexture, ui->btnSpecularColorTex);
	}
}

void MaterialLibraryPanel::on_spinSpecularShin_valueChanged(double arg1)
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		material->specularShininess = (float)arg1;
	}
}

void MaterialLibraryPanel::on_btnNormalColorTex_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureButtonClicked(*material, eNormalTexture, ui->btnNormalColorTex);
	}
}

void MaterialLibraryPanel::on_btnNormalColorTexReload_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureReload(*material, eNormalTexture, ui->btnNormalColorTex);
	}
}

void MaterialLibraryPanel::on_btnNormalColorTexClear_clicked()
{
	BPPGraphicsMaterial* material = _getSelectedMaterial();
	if (material)
	{
		OnTextureClear(*material, eNormalTexture, ui->btnNormalColorTex);
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
