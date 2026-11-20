// LEGO Digital Designer — restored source
// LEGO::LDDPostProcessor — the developer window of the scene control that chooses the
//   post-processing effect: an LDDWindowControl "Post Processing Techniques" (hidden at first)
//   holding a DropDownControl with one item per effect (LDDApplication's effects, then the
//   debug effects); a choice is shown in the drop-down and set on the scene control
//   (BrickConnectSceneControl::SetPostProcessor). BrickConnectSceneControl::Initialize
//   (Mac 0x32b90) creates it (+0x50c) and calls Initialize; the debug key 't' shows / hides it.
//
// TU 373 "LDDPostProcessor.cpp" (Mac 0x361940-0x362e2b; Win 0x658568-0x659a60).
// Vtables: Mac 0x74ff1c {D1 0x362440, D0 0x362500, OnFinalRelease 0x3625f0, OnAttached
// 0x220fd0, OnDetached 0x220fe0, OnItemSelected 0x3626b0, Initialize 0x361a50}, Object-in
// 0x74ff50; Win 0x9d6a88 {OnAttached / OnDetached 0x65a12a, OnItemSelected 0x658568,
// Initialize 0x65962b}, Object-in 0x9d6a74 {sdd 0x6599bf, OnFinalRelease 0x6599c7, …}.
// Mac layout: DropDownControlObserver (vptr, subjects +0x04), BrickConnectSceneControl* +0x14,
// RenderControlManager* +0x18, UIntVector2 +0x1c (not used), window +0x24, drop-down +0x28,
// effects +0x2c, items by name +0x3c, Object +0x58 (sizeof 0x60). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "DropDownControl.h"
#include "LDDWindowControl.h"
#include "PostProcessingEffect.h"

namespace LEGO {

class BrickConnectSceneControl;
class RenderControlManager;
class Control;

class LDDPostProcessor : public DropDownControlObserver {
public:
    // Mac 0x361940 (Win inlined in the scene control)
    LDDPostProcessor(BrickConnectSceneControl* control, RenderControlManager* manager);
    virtual ~LDDPostProcessor();                                            // Mac 0x362440/0x362500 (body 0x362240), Win sdd 0x659a60
    // The drop-down shows the chosen effect's name and the scene control uses the effect.
    virtual void OnItemSelected(Control* item, UInt32 index);               // Mac 0x3626b0, Win 0x658568
    // Builds the window and the drop-down (at (15, 15); the window at (40, 10) .. (210, 60),
    // hidden) below the scene control and one item per effect. False when the drop-down or
    // the window cannot be created.
    virtual bool Initialize();                                              // 6 Mac 0x361a50, Win 0x65962b

    // Shows `effect` in the drop-down and shows / hides the window. Mac 0x3621e0 (Win inlined)
    void SetShown(bool shown, const String& effect);
    // The window is visible. Mac 0x362220 (Win inlined)
    bool IsShown() const;

protected:
    // Leaves every drop-down it observes (without OnDetached), then Object::OnFinalRelease.
    virtual void OnFinalRelease();                                          // Mac 0x3625f0, Win 0x6592b3

    BrickConnectSceneControl*                     m_control;     // +0x14
    RenderControlManager*                         m_manager;     // +0x18
    UIntVector2                                   m_vector1c;    // +0x1c (not used)
    RefPointer<LDDWindowControl>                  m_window;      // +0x24
    RefPointer<DropDownControl>                   m_dropDown;    // +0x28
    BasicArray<RefPointer<PostProcessingEffect> > m_effects;     // +0x2c
    BasicMap<String, RefPointer<Control> >        m_items;       // +0x3c the drop-down items by effect name

private:
    LDDPostProcessor(const LDDPostProcessor&);
    LDDPostProcessor& operator=(const LDDPostProcessor&);
};

} // namespace LEGO
