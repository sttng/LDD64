// LEGO Digital Designer — restored source
// LEGO::PickTool — base of the tools that pick parts of the scene control with the left mouse
//   button (ClickStickPickupTool → SelectionTool …, MaterialTool): a press picks an element
//   (OnPickPressed), moving the mouse more than 3 x 1 pixels with an element starts a drag
//   (OnPickDragged, every move), without one a rubber band (the mouse is captured; the parts whose
//   elements are all inside the band, OnRubberBand*), a release on the pressed element is a click
//   (OnPickClicked), every release OnPickReleased. The band is drawn as a black rectangle.
//
// TU 068 "PickTool.cpp" (Mac 0x15e720-0x15fa2b; Win 0x6607bd-0x6611d9). Constructor inline (C2
// Mac 0x161a80, emitted in SelectionTool.cpp; MaterialTool Mac 0x1a7fa0), destructor body Mac
// 0x15f0d0 (D1 0x15f330, D0 0x15f470; Win sdd 0x6611b5 / 0x6611d9).
// RTTI: PickTool : BrickConnectSceneControlTool. Mac vtable 0x70fb4c (38 entries): BCSCTool's 27
//   with OnDetached 0x15e720, OnEvent07 0x15efc0, OnEvent19 0x15f0b0; new 27-33 (empty
//   0x15f5e0-0x15f640); Mac-only primary entries 34-37 of the ControlObserver overriders.
//   Win primary 0x9d7454 (29 entries: OnDetached 0x6609ed, OnEvent07 0x6607bd, OnEvent19
//   0x66087a, new 22-28 = Mac 27-33), ControlObserver-in 0x9d73d4 (OnMouseMove 0x660d9c,
//   OnMousePressed 0x660742, OnMouseReleased 0x660885, OnMouseCaptureLost 0x660905).
// Layout (Mac): BrickConnectSceneControlTool +0x00..+0x2b, members below, Object +0xa4
//   (sizeof 0xac). Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Scene.h"
#include "BrickConnectSceneControlTool.h"

namespace LEGO {

class Part;
class PrimitiveElement;
class Render;
struct ControlDrawArgs;

class PickTool : public BrickConnectSceneControlTool {
public:
    // (Mac C2 0x161a80) `cursorFlag`: BrickConnectSceneControlTool's.
    explicit PickTool(bool cursorFlag = false);
    virtual ~PickTool();                                                    // Mac 0x15f330/0x15f470, Win sdd 0x6611d9

    // ---- new virtuals (Mac slot / Win slot); all empty here ----
    // Button 1 went down (`clear` false; the picked element is m_picked, null on empty space).
    virtual void OnPickPressed(bool clear);                                 // 27/22 Mac 0x15f5e0
    // The mouse moves with the button held after a press on an element (every move once the
    // drag threshold was passed).
    virtual void OnPickDragged();                                           // 28/23 Mac 0x15f5f0
    // Released on the element that was pressed (`flag`: m_clickFlag, false).
    virtual void OnPickClicked(bool flag);                                  // 29/24 Mac 0x15f600
    // The rubber band starts (mouse captured) / its parts changed / it ended (capture lost).
    virtual void OnRubberBandStarted();                                     // 30/25 Mac 0x15f610
    virtual void OnRubberBandChanged();                                     // 31/26 Mac 0x15f620
    virtual void OnRubberBandFinished();                                    // 32/27 Mac 0x15f630
    // Button 1 went up (after OnPickClicked), or the tool left a pressed control.
    virtual void OnPickReleased();                                          // 33/28 Mac 0x15f640

    // ---- ControlObserver (Mac-only primary entries 34-37) ----
    // Button held (Control pressed-button bit 0): the band rectangle follows; with an element the
    // drag (threshold: width > 3 and height > 1, or width > 1 and height > 3), else the band
    // (capture, OnRubberBandStarted, SetActive(true)) and its frustum pick (parts whose elements
    // were all picked), OnRubberBandChanged. Mac 0x15e7d0 (thunk 0x15ed70), Win 0x660d9c
    virtual void OnMouseMove(Control* control, const Vector2& position);
    // Button 0 on the scene control: the band starts at `position`, the element under it
    // (PickElement with m_pickPoint, material slot, m_filterParts / m_extraParts),
    // OnPickPressed(false). Mac 0x15ed90 (thunk 0x15ee40), Win 0x660742
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);
    // Button 0 on the scene control: OnPickClicked when the element under `position` is still
    // the pressed one, or the band's capture ends; OnPickReleased. Mac 0x15ee60 (thunk 0x15ef20),
    // Win 0x660885
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button);
    // The band ends: OnRubberBandFinished, SetActive(false). Mac 0x15ef40 (thunk 0x15ef80),
    // Win 0x660905
    virtual void OnMouseCaptureLost(Control* control);

protected:
    // ---- BrickConnectSceneControlObserver ----
    // Forgets the pick, the band parts (OnPickReleased while pressed, capture released), then
    // BrickConnectSceneControlTool's. Mac 0x15e720, Win 0x6609ed
    virtual void OnDetached(BrickConnectSceneControl* control);
    using BrickConnectSceneControlTool::OnDetached;
    // The band rectangle (four black lines) while it is shown. Mac 0x15efc0, Win 0x6607bd
    virtual void OnEvent07(Render* render, ControlDrawArgs& args);
    // The scene structure changed: the picked element is forgotten. Mac 0x15f0b0, Win 0x66087a
    virtual void OnEvent19();

    // ---- data (Mac offsets) ----
    Vector2                       m_pressPosition;    // +0x2c
    Rect                          m_rubberBand;       // +0x34 press position .. mouse (normalized)
    bool                          m_rubberBandActive; // +0x44 the band is shown (mouse captured)
    bool                          m_dragging;         // +0x45
    bool                          m_pressed;          // +0x46
    bool                          m_clickFlag;        // +0x47 (always false; OnPickClicked's argument)
    PrimitiveElement*             m_picked;           // +0x48 element under the press
    Vector3                       m_pickPoint;        // +0x4c its hit point (world)
    Scene::PickResult             m_pickResult;       // +0x58 the band's frustum pick
    BasicArray<RefPointer<Part> > m_rubberBandParts;  // +0x74
    BasicArray<RefPointer<Part> > m_filterParts;      // +0x84 PickElement filter (empty)
    BasicArray<RefPointer<Part> > m_extraParts;       // +0x94 PickElement extra parts (empty)
                                                      // Object +0xa4
};

} // namespace LEGO
