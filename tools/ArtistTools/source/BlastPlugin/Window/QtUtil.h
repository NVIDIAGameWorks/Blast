#ifndef QtUtil_h__
#define QtUtil_h__

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtWidgets/QColorDialog>
#include <QtGui/QPalette>
#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include "corelib_global.h"

// utility functions to quickly set and update UI

void setStyledToolTip(QPushButton *pButton, const char *tooltip);
void pickColor(atcore_float4& color);
void setButtonColor(QPushButton *button, float r, float g, float b);

void setFocusColor(QWidget* qWidget, bool sameAsDefault, bool sameForAllAssets);

void updateColorButton(QPushButton* pButton, int paramID, QLabel* label = 0);

void setTextureButtons(QPushButton *pButton, QPushButton *pReload, QPushButton *pClear);
void updateTextureButton(QPushButton* pButton, const QString& texturePath);

void setClearButtonIcon(QPushButton *pButton);

//bool LoadHairTexture(NvHair::TextureType::Enum textureType);
//bool ReloadHairTexture(NvHair::TextureType::Enum textureType);
//bool ClearHairTexture(NvHair::TextureType::Enum textureType);

#endif
