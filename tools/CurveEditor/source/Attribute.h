#ifndef ATTRIBUTE_H__
#define ATTRIBUTE_H__

#include "Curve.h"
#include <vector>

namespace nvidia {
namespace CurveEditor {

class CurveEditorMainWindow;

enum CurveAttributeType
{
    eSingleAttr,
    eGroupAttr,
    eColorAttr,
};

class CURVEEDITOR_EXPORT CurveAttributeBase
{
    friend class CurveEditorMainWindow;
public:
    std::string getName() const  { return _name; }
    CurveAttributeType getType() const { return _type; }

    inline bool canMoveControlPointHorizontally() { return _canMoveControlPointHorizontally;  }
    inline bool canAddRemoveControlPoint() {   return _canAddRemoveControlPoint;   }
    inline bool canChangeTangentType() {   return _canChangeTangentType;   }

protected:
    CurveAttributeBase(const std::string& name, CurveAttributeType type, bool canMoveControlPointHorizontally = false, bool canAddRemoveControlPoint = false, bool canChangeTangentType = false)
        : _name(name)
        , _type(type)
        , _canMoveControlPointHorizontally(canMoveControlPointHorizontally)
        , _canAddRemoveControlPoint(canAddRemoveControlPoint)
        , _canChangeTangentType(canChangeTangentType)
    {
    }

protected:
    std::string         _name;
    CurveAttributeType  _type;
    bool                _canMoveControlPointHorizontally;
    bool                _canAddRemoveControlPoint;
    bool                _canChangeTangentType;
};

class CURVEEDITOR_EXPORT CurveAttribute : public CurveAttributeBase
{
public:
    CurveAttribute()
        : CurveAttributeBase("", eSingleAttr)
    {
    }

    CurveAttribute(const std::string& name, bool canMoveControlPointHorizontally = false, bool canAddRemoveControlPoint = false, bool canChangeTangentType = false)
        : CurveAttributeBase(name, eSingleAttr, canMoveControlPointHorizontally, canAddRemoveControlPoint, canChangeTangentType)
    {
    }

    CurveAttribute(const CurveAttribute& attr)
        : CurveAttributeBase(attr._name, eSingleAttr)
    {
    }

    ~CurveAttribute()
    {
    }

    QColor  color;
    Curve   curve;
};

class CURVEEDITOR_EXPORT CurveAttributeGroup : public CurveAttributeBase
{
public:
    CurveAttributeGroup(std::string name, bool canMoveControlPointHorizontally = false, bool canAddRemoveControlPoint = false, bool canChangeTangentType = false)
        : CurveAttributeBase(name, eGroupAttr, canMoveControlPointHorizontally, canAddRemoveControlPoint, canChangeTangentType)
    {
    }

    std::vector<CurveAttribute*> attributes;
};

class CURVEEDITOR_EXPORT ColorAttribute : public CurveAttributeBase
{
    friend class CurveEditor;
public:
    ColorAttribute()
        : CurveAttributeBase("", eSingleAttr)
        , useAlphaFromColor(false)
    {
    }

    ColorAttribute(const std::string& name, bool canMoveControlPointHorizontally = false, bool canAddRemoveControlPoint = false, bool canChangeTangentType = false)
        : CurveAttributeBase(name, eColorAttr, canMoveControlPointHorizontally, canAddRemoveControlPoint, canChangeTangentType)
        , useAlphaFromColor(false)
    {
    }

    ColorCurve  colorCurve;
    ColorCurve  alphaCurve;
    bool        useAlphaFromColor;
};

} // namespace CurveEditor
} // namespace nvidia

#endif // ATTRIBUTE_H__
