#ifndef GRAPHEDITORTESTAPP_H
#define GRAPHEDITORTESTAPP_H

#include <QtWidgets/QWidget>

namespace nvidia {
namespace CurveEditor {

class CurveEditorMainWindow;
class CurveAttributeBase;
class CurveAttribute;
class ColorAttribute;

} // namespace CurveEditor
} // namespace nvidia

using namespace nvidia::CurveEditor;
 
namespace Ui
{
    class CurveEditorTestAppClass;
}

class CurveEditorTestApp : public QWidget
{
    Q_OBJECT

public:
    CurveEditorTestApp(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~CurveEditorTestApp();

private slots:
	void on_btnCurveEditor_clicked(void);
    void onCurveAttributeChanged(nvidia::CurveEditor::CurveAttribute* attribute);
    void onColorAttributeChanged(nvidia::CurveEditor::ColorAttribute* attribute);
    void onReloadColorAttributeTexture(nvidia::CurveEditor::ColorAttribute* attribute, bool reloadColorTex, int selectedCtrlPntIndex);

private:
    void _fillCurveAttributes();
    void _fillColorAttributes();

private:
    Ui::CurveEditorTestAppClass*        ui;
	CurveEditorMainWindow*				_curveEditor;
    std::vector<CurveAttributeBase*>    _curveAttributes;
    std::vector<ColorAttribute*>        _colorAttributes;
};

#endif // GRAPHEDITORTESTAPP_H
