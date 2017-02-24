#ifndef MATERIALLIBRARYPANEL_H
#define MATERIALLIBRARYPANEL_H

#include <QtWidgets/QWidget>
#include "ProjectParams.h"

class RenderMaterial;

namespace Ui {
class MaterialLibraryPanel;
}

class MaterialLibraryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialLibraryPanel(QWidget *parent = 0);
    ~MaterialLibraryPanel();
	void updateValues();

	static MaterialLibraryPanel* ins();
	void addMaterial(std::string materialName, std::string diffuseTexture);
	void removeMaterial(std::string name);
	std::map<std::string, RenderMaterial*>& getRenderMaterials(){ return m_RenderMaterialMap; }
	void deleteMaterials();
	void deleteMaterialMap();

private slots:
    void on_btnAddMat_clicked();

    void on_btnModifyMat_clicked();

    void on_btnRemoveMat_clicked();

    void on_listWidget_currentRowChanged(int currentRow);

    void on_btnDiffuseColor_clicked();

    void on_btnDiffuseColorTex_clicked();

    void on_btnDiffuseColorTexReload_clicked();

    void on_btnDiffuseColorTexClear_clicked();

    void on_btnSpecularColor_clicked();

    void on_btnSpecularColorTex_clicked();

    void on_btnSpecularColorTexReload_clicked();

    void on_btnSpecularColorTexClear_clicked();

	void on_spinSpecularShin_valueChanged(double arg1);

    void on_btnNormalColorTex_clicked();

    void on_btnNormalColorTexReload_clicked();

    void on_btnNormalColorTexClear_clicked();

private:
	void _refreshMaterialValues(int idx);
	BPPGraphicsMaterial* _getSelectedMaterial();

	std::map<std::string, RenderMaterial*> m_RenderMaterialMap;
	std::vector<std::string> m_NeedDeleteRenderMaterials;

private:
    Ui::MaterialLibraryPanel *ui;
};

#endif // MATERIALLIBRARYPANEL_H
