// LEGO Digital Designer — restored source
// LEGO::MaterialControl — the pop-up showing the colour variants of a brick: a frame
//   (LDDgraphics01 (73, 520), 11 px corners) around a matrix of 64 x 64 BricksControls,
//   `columns` per row, optionally a "more" button. A right drag turns every item's camera; a
//   click goes to the subclass (OnBricksControlClicked) and closes the pop-up, as do leaving it
//   and a modal control. Count badges follow the missing-brick counts (SHOWBRICKCOUNTS). Base of
//   PaletteMaterialControl and MaterialToolControl.
//
// Home TU 139 "MaterialControl.cpp" (Mac 0x1f40d0-0x1f729b; Win 0x66a362-0x66b321).
// RTTI vmi {StretchableControl, ControlObserver (Mac +0x210), ControlManagerObserver (+0x224),
//   PreferencesObserver (+0x22c), BrickConnectSceneControlObserver (+0x234)}, virtual base Object
//   (Mac +0x268, sizeof 0x270; Win +0x290).
// Constructor: Mac C2 0x1f40d0 (with VTT; inlined into the subclasses' constructors), Win 0x66a71d.
// Virtual slots: Mac 0x71ac1c (59 entries) = StretchableControl's 46 with OnFinalRelease 0x1f5c10,
//   new 46 Create 0x1f44c0, the Mac-only primary entries of the observer overriders 47-56,
//   57 OnBricksControlClicked (pure), 58 OnMoreClicked 0x1f7210 (empty).
//   Win: Create 0x66ad42, sdd 0x66a6fb; the observer overriders sit in the observer vtables.
// Names inferred except the RTTI class names and the preference keys.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Control.h"
#include "ControlManager.h"
#include "Preferences.h"
#include "StretchableControl.h"
#include "MatrixLayoutControl.h"
#include "ButtonControl.h"
#include "BrickFilter.h"
#include "BrickPalette.h"
#include "BrickConnectSceneControlObserver.h"
#include "BrickConnectSceneControl.h"

namespace LEGO {

class BricksControl;
class BrickConnectSceneManager;
class BrickConnectSceneControl;

class MaterialControl : public StretchableControl,
                        public ControlObserver,
                        public ControlManagerObserver,
                        public PreferencesObserver,
                        public BrickConnectSceneControlObserver {
public:
    typedef BasicArray<RefPointer<const BrickPalette::Brick> > BrickArray;

    // `rows` is stored only; `columns`: items per row (EditMode / MaterialTool pass 10, 8).
    // A new CompositeBrickFilter as the filter. Mac 0x1f40d0 (C2), Win 0x66a71d
    MaterialControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                    UInt32 rows, UInt32 columns);
    virtual ~MaterialControl();                                             // Mac 0x1f54b0/0x1f5820, Win sdd 0x66a6fb

    // The frame (StretchableControl::Create with the LDDgraphics01 rectangles), Setup below
    // `parent` behind `after` (hidden, enabled + active, no input), the empty item matrix, then
    // observes the manager, the Preferences and the scene control; transparent. False without
    // the texture or when a frame part fails. Mac 0x1f44c0, Win 0x66ad42
    virtual bool Create(Control* parent, Control* after);                   // 46/41
    using StretchableControl::Create;

    // The items: one BricksControl per brick the filter accepts (seen from `*camera` when
    // given), then the "more" button when `withMore`; the pop-up is sized to (min(n, columns) *
    // 64 + 9, rows * 64 + 14), shown, brought to the front, and the brick counts updated.
    // False (nothing shown) without items or when the button cannot be made.
    // Mac 0x1f5e40, Win 0x66b159
    bool SetBricks(BrickArray bricks, const Vector2* camera, bool withMore);
    // The filter the items must pass (and that counts them). Mac 0x1f5210 (BrickPaletteControl
    // Mac 0x68460)
    void SetBrickFilter(CompositeBrickFilter* filter);
    CompositeBrickFilter* GetBrickFilter() const { return m_filter.Get(); }

    // ---- ControlObserver ----
    using StretchableControl::OnMouseMove;
    using StretchableControl::OnMouseClicked;
    // Dragging an item with the right button (pressed-button bit 1): every item takes its camera.
    // Mac 0x1f65e0 (thunk 0x1f66c0)
    virtual void OnMouseMove(Control* control, const Vector2& position);    // 47/–
    // Button 0 on the "more" button: OnMoreClicked, on an item: OnBricksControlClicked; then the
    // clicked control is released later, the items go and the pop-up hides.
    // Mac 0x1f66f0 (thunk 0x1f6790)
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 48/–

    // ---- ControlManagerObserver ----
    // The mouse left the visible pop-up (and no control inside it holds the mouse): close.
    // Mac 0x1f67c0 (thunk 0x1f6870)
    virtual void OnManagerMouseMove(const Vector2& position);               // 49/–
    // Mac 0x1f6890 (thunk 0x1f6900)
    virtual void OnManagerMouseOverChanged(Control* previous, Control* current); // 50/–
    // Mac 0x1f6970 (thunk 0x1f69b0)
    virtual void OnManagerModalControl(bool modal, Control* control);       // 51/–

    // ---- PreferencesObserver ----
    using ControlObserver::OnAttached;
    using ControlManagerObserver::OnAttached;
    using PreferencesObserver::OnAttached;
    using TextLayoutObserver::OnAttached;
    using TextureObserver::OnAttached;
    // ShowExtendedBrickToolTip / SHOWBRICKCOUNTS re-read (the counts follow).
    // Mac 0x1f69f0 (thunk 0x1f6be0)
    virtual void OnPreferenceChanged(const String& key);                    // 52/–
    // Both keys read. Mac 0x1f6c00 (thunk 0x1f6d40)
    virtual void OnAttached(Preferences* preferences);                      // 53/–

    // ---- BrickConnectSceneControlObserver ----
    // The missing-brick counts changed: the count badges of the items (filter count - scene
    // count) while the scene shows missing bricks, SHOWBRICKCOUNTS is on and the filter counts.
    // Mac 0x1f6d60 (thunk 0x1f70d0), Win 0x66aa9a
    virtual void OnEvent16();                                               // 54/–
    // (empty overriders) Mac 0x1f70f0 / 0x1f7110 (thunks 0x1f7100 / 0x1f7120)
    virtual void OnAttached(BrickConnectSceneControl* control);             // 55/–
    virtual void OnDetached(BrickConnectSceneControl* control);             // 56/–
    using ControlObserver::OnDetached;
    using ControlManagerObserver::OnDetached;
    using PreferencesObserver::OnDetached;
    using TextLayoutObserver::OnDetached;
    using TextureObserver::OnDetached;

    // ---- new virtuals ----
    // An item was clicked (button 0). Mac pure, subclasses: PaletteMaterialControl 0x1f7520,
    // MaterialToolControl 0x1a9c30
    virtual void OnBricksControlClicked(BricksControl* item) = 0;           // 57/–
    // The "more" button was clicked. Mac 0x1f7210 (empty), MaterialToolControl 0x1a9cb0
    virtual void OnMoreClicked();                                           // 58/–

    // (test / inspection accessors, not in the original)
    MatrixLayoutControl* GetItemLayout() const { return m_matrix.Get(); }
    ButtonControl*       GetMoreButton() const { return m_more.Get(); }

protected:
    // The four observer parts leave their subjects, then RenderControl's. Mac 0x1f5c10
    virtual void OnFinalRelease();

    // A new, empty item matrix at (2, 3) (the old one is released later).
    // Mac 0x1f50f0, Win 0x66a362
    void ResetItems();
    // The "more" button ("Graphics/EditScreen/EditMode_Final.png" (193, 386, 48, 48), all four
    // states), 64 x 64, observed. Mac 0x1f61c0, Win 0x66a8e6
    bool CreateMoreButton();
    // Count badges off on every item. Mac 0x1f7130, Win 0x66acc1
    void HideCounts();
    // OnEvent16 (virtual) when the scene shows missing bricks, the counts are on and the filter
    // counts, else HideCounts. Win 0x66b10f (inlined on the Mac)
    void RefreshCounts();

    // ---- data (Mac offsets / Win offsets) ----
    RefPointer<MatrixLayoutControl>  m_matrix;          // +0x23c / +0x268
    UInt32                           m_rows;            // +0x240 / +0x26c (not used)
    UInt32                           m_columns;         // +0x244 / +0x270
    Vector2                          m_itemSize;        // +0x248 / +0x274 (64, 64)
    RefPointer<ButtonControl>        m_more;            // +0x250 / +0x27c
    RefPointer<CompositeBrickFilter> m_filter;          // +0x254 / +0x280
    BrickConnectSceneManager*        m_sceneManager;    // +0x258 / +0x284
    bool                             m_extendedTooltip; // +0x25c / +0x288 ShowExtendedBrickToolTip
    bool                             m_showCounts;      // +0x25d / +0x289 SHOWBRICKCOUNTS
                                                        // Object +0x268 / +0x290
};

} // namespace LEGO
