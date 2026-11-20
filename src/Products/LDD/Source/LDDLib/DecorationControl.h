// LEGO Digital Designer — restored source
// LEGO::DecorationControl — the decoration picker of the decoration tool: a pop-up (the
//   MaterialControl frame, LDDgraphics01 (73, 520)) with a MatrixLayoutControl of BricksControl
//   items, one per decoration the database maps to the picked part's (design, surface), each
//   showing a copy of the picked brick with that decoration, plus one without decoration; the
//   Lua function editMode:InjectScrollbarInPopup wraps the matrix in a scroll container (at most
//   6 rows are shown). Closing / camera sync as MaterialControl. Base of DecorationToolControl.
//
// Home TU 196 "DecorationControl.cpp" (Mac 0x247d10-0x24a32b; Win 0x697386-0x697b07, 0x6938ab).
// RTTI vmi {StretchableControl, ControlObserver (Mac +0x210), ControlManagerObserver (+0x224),
//   PreferencesObserver (+0x22c)}, virtual base Object (Mac +0x254, sizeof 0x25c; Win +0x280).
// Constructor: Mac C2 0x24a330 (TU 197, with VTT; inlined into DecorationToolControl's).
// Virtual slots: Mac 0x7332cc (56 entries) = StretchableControl's 46 with OnFinalRelease
//   0x249970, new 46 Create 0x247d10, 47 HasDecorations 0x248b40, the Mac-only primary entries
//   of the observer overriders 48-54, 55 OnBricksControlClicked (pure).
//   Win: Create 0x69777e, HasDecorations 0x697386, ShowDecorations 0x6973ee, ResetItems
//   0x696e74, sdd 0x6938ab.
// Names inferred except the RTTI class names, the preference key and the Lua names.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Control.h"
#include "ControlManager.h"
#include "Preferences.h"
#include "StretchableControl.h"
#include "MatrixLayoutControl.h"

namespace LEGO {

class BricksControl;
class BrickConnectSceneManager;
class PrimitiveElement;

class DecorationControl : public StretchableControl,
                          public ControlObserver,
                          public ControlManagerObserver,
                          public PreferencesObserver {
public:
    // `rows` is stored only; `columns`: items per row (DecorationTool passes 10, 8).
    // Mac 0x24a330 (C2), Win inlined
    DecorationControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                      UInt32 rows, UInt32 columns);
    virtual ~DecorationControl();                                           // Mac 0x2498a0/0x249900, Win sdd 0x6938ab

    // The frame (as MaterialControl::Create), Setup below `parent` behind `after` (hidden,
    // enabled + active), the empty item matrix, then observes the manager and the Preferences;
    // transparent. False without the texture or when a frame part fails. Mac 0x247d10, Win 0x69777e
    virtual bool Create(Control* parent, Control* after);                   // 46/41
    using StretchableControl::Create;
    // The database maps decorations to (design of `picked`'s primitive, `surface`).
    // Mac 0x248b40, Win 0x697386
    virtual bool HasDecorations(const PrimitiveElement* picked, UInt32 surface) const; // 47/42

    // One item per decoration of (design, `surface`) and the undecorated one, seen from the
    // scene camera's direction; with at least two items the pop-up is sized
    // (min(n, columns) * 64 + 30, min(rows, 6) * 64 + 14), shown and brought to the front (true).
    // Mac 0x248bd0, Win 0x6973ee
    bool ShowDecorations(const PrimitiveElement* picked, UInt32 surface);

    // ---- ControlObserver ----
    using StretchableControl::OnMouseMove;
    using StretchableControl::OnMouseClicked;
    // Right-button drag of an item: every item takes its camera. Mac 0x249270 (thunk 0x249350)
    virtual void OnMouseMove(Control* control, const Vector2& position);    // 48/–
    // Button 0 on an item: OnBricksControlClicked, release the control later, reset, hide.
    // Mac 0x249380 (thunk 0x249400)
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 49/–

    // ---- ControlManagerObserver (as MaterialControl) ----
    virtual void OnManagerMouseMove(const Vector2& position);               // 50/– Mac 0x249490 (thunk 0x249540)
    virtual void OnManagerMouseOverChanged(Control* previous, Control* current); // 51/– Mac 0x249560 (thunk 0x2495d0)
    virtual void OnManagerModalControl(bool modal, Control* control);       // 52/– Mac 0x249640 (thunk 0x249680)

    // ---- PreferencesObserver ----
    using ControlObserver::OnAttached;
    using ControlManagerObserver::OnAttached;
    using PreferencesObserver::OnAttached;
    using TextLayoutObserver::OnAttached;
    using TextureObserver::OnAttached;
    // ShowExtendedBrickToolTip re-read. Mac 0x2496c0 (thunk 0x2497b0)
    virtual void OnPreferenceChanged(const String& key);                    // 53/–
    // Mac 0x2497d0 (thunk 0x249880)
    virtual void OnAttached(Preferences* preferences);                      // 54/–

    // ---- new virtual ----
    // An item was clicked (button 0). DecorationToolControl 0x24a740
    virtual void OnBricksControlClicked(BricksControl* item) = 0;           // 55/–

    // (test / inspection accessors, not in the original)
    MatrixLayoutControl* GetItemLayout() const { return m_matrix.Get(); }
    Control*             GetScrollContainer() const { return m_scroll.Get(); }

protected:
    // The three observer parts leave their subjects, then RenderControl's. Mac 0x249970
    virtual void OnFinalRelease();

    // A new, empty item matrix (the old one released later, the old scroll container out of the
    // tree), handed to editMode:InjectScrollbarInPopup(self table, matrix table); its result
    // (a control) becomes the scroll container (margins -4, 10, -4, 10) and the matrix fills
    // it horizontally. Mac 0x2486a0, Win 0x696e74
    void ResetItems();

    // ---- data (Mac offsets / Win offsets) ----
    RefPointer<MatrixLayoutControl>  m_matrix;          // +0x234 / +0x25c
    RefPointer<Control>              m_scroll;          // +0x238 / +0x260
    UInt32                           m_rows;            // +0x23c (not used)
    UInt32                           m_columns;         // +0x240 / +0x268
    Vector2                          m_itemSize;        // +0x244 / +0x26c (64, 64)
    bool                             m_extendedTooltip; // +0x24c / +0x274 (passed on, not used)
    BrickConnectSceneManager*        m_sceneManager;    // +0x250 / +0x278
                                                        // Object +0x254 / +0x280
};

} // namespace LEGO
