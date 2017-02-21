#include "QtUtil.h"

#include <QtWidgets/QLabel>
#include "AppMainWindow.h"

#include "SimpleScene.h"

//////////////////////////////////////////////////////////
void setStyledToolTip(QPushButton *pButton, const char *tooltip)
{
	char styledString[1024];
	sprintf(styledString, "<span style=\"color:black;\">%s</span>", tooltip);
	pButton->setToolTip(styledString);
}

//////////////////////////////////////////////////////////
QString addStar(QString text, bool add)
{
	QByteArray ba = text.toUtf8();

	const char* in = ba.data();
	char out[1024];

	int i = 0;
	for (i = 0; i < strlen(in); i++)
	{
		if (in[i] == '*')
			break;
		out[i] = in[i];
	}
	out[i] = 0;

	QString newtext;
	if (add)
		newtext = QString((const char*)out) + QString("*");
	else
		newtext = QString((const char*)out) ;
	return newtext;
}

//////////////////////////////////////////////////////////
void setFocusColor(QWidget* qWidget, bool sameAsDefault, bool sameForAllAssets)
{
	if (!qWidget)
		return;

	QString sameStyle = QString("font: ; color: rgb(150,150,150);") ;
	QString differentStyle = QString("font: bold; color: rgb(255,55,55);");
	QString style = (sameForAllAssets) ? sameStyle : differentStyle;

	qWidget->setStyleSheet(style);

	QLabel* label = dynamic_cast<QLabel*>(qWidget);
	if (label)
	{
		QString newtext = addStar(label->text(), !sameAsDefault);

		label->setFrameStyle(0);
		label->setText(newtext);
	}
}

//////////////////////////////////////////////////////////
void pickColor(atcore_float4& color)
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
	}
}

//////////////////////////////////////////////////////////
void setButtonColor(QPushButton *button, float r, float g, float b)
{
	QColor specColor;
	specColor.setRgbF(r,g,b);
	QString specBtnStyle = QString("background-color: rgb(%1,%2,%3);")
		.arg(specColor.red())
		.arg(specColor.green())
		.arg(specColor.blue());

	button->setStyleSheet(specBtnStyle);
}


/////////////////////////////////////////////////////////////////////////////////////
void updateColorButton(QPushButton* button, int paramID, QLabel* label)
{
	//atcore_float4 v;
	//SimpleScene::Inst()->GetFurCharacter().GetHairParam(paramID, &v);

	//setButtonColor(button, v.x, v.y, v.z);

	//if (label)
	//	setFocusColor(label, paramID);
}

/////////////////////////////////////////////////////////////////////////////////////
void setClearButtonIcon(QPushButton *pButton)
{
	pButton->setIcon(QIcon(":/AppMainWindow/images/Remove_icon.png"));
}

/////////////////////////////////////////////////////////////////////////////////////
void setTextureButtons(QPushButton *pTex, QPushButton *pReload, QPushButton *pClear)
{
	pTex->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));
	pReload->setIcon(QIcon(":/AppMainWindow/images/Refresh_icon.png"));
	pClear->setIcon(QIcon(":/AppMainWindow/images/Remove_icon.png"));

	pTex->setIconSize(QSize(12,12));
	pReload->setIconSize(QSize(12,12));
	pClear->setIconSize(QSize(12,12));
}

/////////////////////////////////////////////////////////////////////////////////////
void updateTextureButton(QPushButton* pButton, const QString& texturePath)
{
	if (!texturePath.isEmpty())		setStyledToolTip(pButton, texturePath.toUtf8().data());

	bool isTextureUsed = true;
	QIcon notUsedIcon = QIcon(":/AppMainWindow/images/TextureEnabled_icon.png");
	QIcon isUsedIcon = QIcon(":/AppMainWindow/images/TextureIsUsed_icon.png");
	QIcon disabledIcon = QIcon(":/AppMainWindow/images/TextureDisabled_icon.png");

	pButton->setIcon(!texturePath.isEmpty() ? isUsedIcon : notUsedIcon);
}

///////////////////////////////////////////////////////////////////////////////////////
//bool LoadHairTexture(NvHair::TextureType::Enum textureType)
//{
//	QString texName = AppMainWindow::Inst().OpenTextureFile();
//	return SimpleScene::Inst()->GetFurCharacter().LoadHairTexture(textureType, texName.toLocal8Bit());
//}
//
///////////////////////////////////////////////////////////////////////////////////////
//bool ReloadHairTexture(NvHair::TextureType::Enum textureType)
//{
//	return SimpleScene::Inst()->GetFurCharacter().ReloadHairTexture(textureType);
//}
//
///////////////////////////////////////////////////////////////////////////////////////
//bool ClearHairTexture(NvHair::TextureType::Enum textureType)
//{
//	return SimpleScene::Inst()->GetFurCharacter().ClearHairTexture(textureType);
//}
