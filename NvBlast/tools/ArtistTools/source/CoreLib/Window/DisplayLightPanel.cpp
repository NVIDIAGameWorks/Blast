#include "AppMainWindow.h"
#include "DisplayLightPanel.h"

#include "Light.h"
#include "GlobalSettings.h"

#include <QtWidgets/QColorDialog>
#include <QtCore/QFileInfo>

DisplayLightPanel::DisplayLightPanel(QWidget* parent)
	:
	QWidget(parent)
{
	ui.setupUi(this);
	_isUpdatingUI = false;

	ConfigSpinBox<SlideSpinBoxF>(ui.spinLightDistance,	0.0, 1000.0, 0.1);
	ConfigSpinBox<SlideSpinBoxF>(ui.spinLightIntensity,	0.0, 5.0, 0.01);

	ui.btnLightColorTex->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));
	ui.btnLightColorTex->setIconSize(QSize(12,12));
	ui.btnLightColorTexReload->setIcon(QIcon(":/AppMainWindow/images/Refresh_icon.png"));
	ui.btnLightColorTexClear->setIcon(QIcon(":/AppMainWindow/images/Remove_icon.png"));

	connect(ui.listSelectedLight, SIGNAL(itemSelectionChanged()), this, SLOT(onListSelectionChanged()));
}

void DisplayLightPanel::onListSelectionChanged()
{
	if (_isUpdatingUI)
		return;

	int numSelected = ui.listSelectedLight->selectedItems().count();

	// change selection only when there is something selected
	if (numSelected > 0)
	{
		int numItems = ui.listSelectedLight->count();
		{
			for (int i = 0; i < numItems; i++)
			{
				bool selected = ui.listSelectedLight->item(i)->isSelected();
				Light::GetLight(i)->m_selected = selected;
			}
		}
	}

	updateUI();
}

void DisplayLightPanel::on_cbShadowMapResolution_currentIndexChanged( int index )
{
	for (int i = 0; i < 4; i++)
	{
		Light* pLight = Light::GetLight(i);
		if (!pLight || !pLight->m_selected)
			continue;

		pLight->SetShadowMapResolution(index);
	}
}

void DisplayLightPanel::on_btnLightUseShadow_stateChanged(int state)
{
	for (int i = 0; i < 4; i++)
	{
		Light* pLight = Light::GetLight(i);
		if (!pLight || !pLight->m_selected)
			continue;

		pLight->m_useShadows = state;
	}
}

void DisplayLightPanel::on_btnVisualizeLight_stateChanged(int state)
{
	for (int i = 0; i < 4; i++)
	{
		Light* pLight = Light::GetLight(i);
		if (!pLight || !pLight->m_selected)
			continue;

		pLight->m_visualize = state;
	}
}

void DisplayLightPanel::on_btnLightEnable_stateChanged(int state)
{
	for (int i = 0; i < 4; i++)
	{
		Light* pLight = Light::GetLight(i);
		if (!pLight || !pLight->m_selected)
			continue;

		pLight->m_enable = state;
	}
}

void DisplayLightPanel::on_btnLinkLightEnable_stateChanged(int state)
{
	Light::SetLinkLightOption(state != 0 ? true : false);
}

void DisplayLightPanel::on_spinLightDistance_valueChanged(double v)
{
	for (int i = 0; i < 4; i++)
	{
		Light* pLight = Light::GetLight(i);
		if (!pLight || !pLight->m_selected)
			continue;
	
		pLight->SetDistance(v);
	}
}

void DisplayLightPanel::on_spinLightIntensity_valueChanged(double v)
{
	for (int i = 0; i < 4; i++)
	{
		Light* pLight = Light::GetLight(i);
		if (!pLight || !pLight->m_selected)
			continue;

		pLight->m_intensity = v;
	}
}

void DisplayLightPanel::on_btnVisualizeShadowMap_stateChanged(int state)
{
	GlobalSettings::Inst().m_visualizeShadowMap = state;
}

void DisplayLightPanel::setButtonColor(QPushButton *button, float r, float g, float b)
{
	QColor specColor;
	specColor.setRgbF(r,g,b);
	QString specBtnStyle = QString("background-color: rgb(%1,%2,%3);")
		.arg(specColor.red())
		.arg(specColor.green())
		.arg(specColor.blue());
	button->setStyleSheet(specBtnStyle);
}


static bool getNewColor(atcore_float3& color)
{
	QColor currentColor;
	currentColor.setRgbF(color.x, color.y, color.z);

	QColor newColor = QColorDialog::getColor(currentColor, NV_NULL);
	if(newColor.isValid())
	{
		qreal r,g,b;
		newColor.getRgbF(&r, &g, &b);

		color.x = r;
		color.y = g;
		color.z = b;
		return true;
	}

	return false;
}

void DisplayLightPanel::on_btnLightColor_clicked()
{
	Light* pLight = Light::GetFirstSelectedLight();

	atcore_float3 color = pLight->m_color;

	if (getNewColor(color))
		setButtonColor(ui.btnLightColor, color.x, color.y, color.z);

	for (int i = 0; i < 4; i++)
	{
		Light* pLight = Light::GetLight(i);
		if (!pLight || !pLight->m_selected)
			continue;

		pLight->m_color = color;
	}
}

inline void SetTextureIcon(QPushButton* pButton, bool enabled)
{
	if (enabled)
		pButton->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));
	else
		pButton->setIcon(QIcon(":/AppMainWindow/images/TextureIsUsed_icon.png"));
}

void DisplayLightPanel::on_btnLightColorTex_clicked()
{
	QString texName = AppMainWindow::Inst().OpenTextureFile();
	QFileInfo fileInfo(texName);
	QByteArray ba = fileInfo.absoluteFilePath().toLocal8Bit();

	if (Light::SetEnvTextureFromFilePath((const char*)ba))
		SetTextureIcon(ui.btnLightColorTex, Light::GetEnvTextureFilePath().empty());
}

void DisplayLightPanel::on_btnLightColorTexReload_clicked()
{
	std::string path = Light::GetEnvTextureFilePath();
	if (Light::SetEnvTextureFromFilePath(path.c_str()))
		SetTextureIcon(ui.btnLightColorTex, Light::GetEnvTextureFilePath().empty());
}

void DisplayLightPanel::on_btnLightColorTexClear_clicked()
{
	Light::SetEnvTextureFromFilePath(nullptr);
	SetTextureIcon(ui.btnLightColorTex, Light::GetEnvTextureFilePath().empty());
}

void DisplayLightPanel::updateUI()
{
	Light* pLight = Light::GetFirstSelectedLight();
	if (!pLight)
		return;

	atcore_float3& color = pLight->m_color;
	setButtonColor(ui.btnLightColor, color.x, color.y, color.z);

	ui.btnVisualizeLight->setChecked(pLight->m_visualize);
		
	ui.spinLightDistance->setValue(pLight->GetDistance());
	ui.spinLightIntensity->setValue(pLight->m_intensity);

	ui.btnLightUseShadow->setChecked(pLight->m_useShadows);
	ui.btnLightEnable->setChecked(pLight->m_enable);

	ui.btnVisualizeShadowMap->setChecked(GlobalSettings::Inst().m_visualizeShadowMap);

	ui.cbShadowMapResolution->setCurrentIndex(pLight->m_shadowMapResolutionIndex);

	ui.btnLinkLightEnable->setChecked(Light::GetLinkLightOption());

	SetTextureIcon(ui.btnLightColorTex, Light::GetEnvTextureFilePath().empty());
}

void DisplayLightPanel::updateValues()
{
	_isUpdatingUI = true;

	ui.listSelectedLight->clear();
	for (int i = 0; i < 4; i++)
	{
		Light* pLight = Light::GetLight(i);
		if (!pLight)
			continue;

		const char* lightName = pLight->m_name.c_str();
		bool selected = pLight->m_selected;

		ui.listSelectedLight->addItem(lightName);
		ui.listSelectedLight->item(i)->setSelected(selected);
	}
	ui.listSelectedLight->setSelectionMode(QAbstractItemView::ExtendedSelection);

	updateUI();

	_isUpdatingUI = false;
}
