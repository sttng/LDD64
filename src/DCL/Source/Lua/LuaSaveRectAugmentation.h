// LEGO Digital Designer — restored source
// LEGO::LuaSaveRectAugmentation — Lua class "SaveRectAugmentation" (base "ControlAugmentation"),
// attached as "saveRect": keeps its control's rectangle in preference `preference` ("x y right
// bottom"); restored on attach, written on every change. Layout: saveSize, savePosition
// (default true), preference (empty = off). LDD: CameraControl (preference="CameraControlRect").
// TU 311 "LuaSaveRectAugmentation.cpp": Mac 0x2b97c0-0x2ba73b, Win 0x45c620-0x45cc7f.
// RTTI vmi {LuaControlAugmentation +0, ControlAugmentationControlObserver +0xc}, virtual base
// Object. Mac primary vtable 0x7468bc {D1 0x2b9e60, D0 0x2b9fe0, BindLuaObject 0x291a70,
// UnbindLuaObject 0x2ba380, OnLayoutApplied 0x2ba3a0, Attach 0x2b9b30, OnFinalRelease 0x2ba1b0,
// OnRectangleChanged 0x2ba560}; ctor Mac 0x2b97c0 (C1), Win 0x45c620; Win sdd 0x45c710, dtor
// 0x45c960, Attach 0x45c910, OnFinalRelease 0x45ca00, OnLayoutApplied 0x45ca10, Load 0x45cb70.
// Layout (Mac): LuaLayout +0x00..+0x0c, ControlAugmentationControlObserver +0x0c..+0x38, String
// preference +0x38, bool saveSize +0x48, bool savePosition +0x49, Object +0x4c (sizeof 0x54).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "LuaControlAugmentation.h"
#include "ControlAugmentationControlObserver.h"

namespace LEGO {

class LuaSaveRectAugmentation : public LuaControlAugmentation,
                                public ControlAugmentationControlObserver {
public:
    LuaSaveRectAugmentation() : m_saveSize(true), m_savePosition(true) {}  // Mac 0x2b97c0, Win 0x45c620
    virtual ~LuaSaveRectAugmentation() {}                                   // Mac 0x2b9e60/0x2b9fe0 (body 0x2b9dc0), Win 0x45c960 (sdd 0x45c710)

    // Detach(). Mac 0x2ba380
    virtual void UnbindLuaObject();
    // LuaLayout's, then saveSize, savePosition, preference (a new preference: Load when attached).
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2ba3a0, Win 0x45ca10
    // ControlAugmentationControlObserver::Attach, then Load when a preference is set.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x2b9b30 (thunk 0x2b9d30), Win 0x45c910

    const String& GetPreference() const { return m_preference; }
    bool GetSaveSize() const { return m_saveSize; }
    bool GetSavePosition() const { return m_savePosition; }

    // The preference's rectangle -> position (savePosition) / size (saveSize), then
    // MoveInsideParent; without a readable preference: Save. Mac 0x2b9bc0, Win 0x45cb70
    void Load();
    // Preference = the control's Rect(position, size) as text. Mac 0x2ba5a0
    void Save();

protected:
    // ControlAugmentationControlObserver's, then the Object one. Mac 0x2ba1b0, Win 0x45ca00
    virtual void OnFinalRelease();
    // The augmented control moved / resized: Save (when a preference is set).
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);                // Mac 0x2ba560 (thunk 0x2ba680)

private:
    String m_preference;     // Mac +0x38
    bool   m_saveSize;       // Mac +0x48 (true)
    bool   m_savePosition;   // Mac +0x49 (true)
};

// Registers SaveRectAugmentation (base ControlAugmentation) {Create}. Mac 0x2b98d0, Win 0x45c870.
// (name inferred)
void RegisterLuaSaveRectAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
