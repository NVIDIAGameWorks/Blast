#ifndef MATERIALASSIGNMENTSPANEL_H
#define MATERIALASSIGNMENTSPANEL_H

#include <QtWidgets/QWidget>

namespace Ui {
class MaterialAssignmentsPanel;
}

class MaterialAssignmentsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialAssignmentsPanel(QWidget *parent = 0);
    ~MaterialAssignmentsPanel();
	void updateValues();

	static MaterialAssignmentsPanel* ins();
	void getMaterialNameAndPaths(std::vector<std::string>& materialNames, std::vector<std::string>& materialPaths);

private slots:
    void on_comboBoxMaterialID1_currentIndexChanged(int index);

    void on_comboBoxMaterialID2_currentIndexChanged(int index);

    void on_comboBoxMaterialID3_currentIndexChanged(int index);

    void on_comboBoxMaterialID4_currentIndexChanged(int index);

private:
	void assignMaterialToMaterialID(int materialID, int materialIndex);

private:
    Ui::MaterialAssignmentsPanel *ui;
	int							_selectedGraphicsMesh;
	bool m_bValid;
};

#endif // MATERIALASSIGNMENTSPANEL_H
