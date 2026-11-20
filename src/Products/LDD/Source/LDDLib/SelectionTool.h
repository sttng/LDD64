// LEGO Digital Designer — restored source
// LEGO::SelectionTool — the selection tools of the scene control: a click selects (the filters
//   decide the parts, without filters the "toggle" tools toggle the clicked part), a rubber band
//   selects the parts inside it, the box of the part under the mouse is drawn (magenta) and its
//   developer tooltip shown, a drag picks the selection up (ClickStickPickupTool). Base of the
//   single selection, clone, delete and hide tools and of EditMode's filtered selection tools.
// LEGO::SelectionToolTooltipAugmentation — the tooltip of the scene control while a selection
//   tool is active: the brick description of the element under the mouse, in developer mode
//   only, while the preference BRICKTOOLTIP is on.
//
// TU 069 "SelectionTool.cpp" (Mac 0x15fa30-0x161f8b; Win 0x62834a-0x62b4a6, 0x6671cc-0x667a32).
// The object also holds ClickStickPickupTool's destructor and PickTool's base-object constructor
// (Mac 0x161a80).
// RTTI: SelectionTool : ClickStickPickupTool. Mac vtable 0x710110 (39 entries), sizeof 0x128
//   (Object +0x120); Win primary 0x9d0ee4 (29), ControlObserver-in 0x9d0e64, sizeof 0x13c.
//   SelectionTool::Filter : Object: Mac vtable none (abstract); Win vftable 0x9d03d0.
//   SelectionToolTooltipAugmentation : StandardTextTooltipAugmentation, PreferencesObserver
//   (+0x30): Mac vtable 0x71004c (10 entries), sizeof 0x4c; Win 0x9d7ce8.
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Preferences.h"
#include "StandardTextTooltipAugmentation.h"
#include "ClickStickPickupTool.h"

namespace LEGO {

class PrimitiveElement;
class Part;
class ClickStickTool;
class Render;
class RenderQueue;
class Mesh;
class BrickConnectSceneManager;

// The tooltip of the selection tools. Created by CreateSelectionToolTooltip (LDDUtilities TU 063,
// Mac 0x14d5a0).
class SelectionToolTooltipAugmentation : public StandardTextTooltipAugmentation,
                                         public PreferencesObserver {
public:
    // No control, empty text; observes the Preferences, reads BRICKTOOLTIP. Mac 0x15fa30,
    // Win 0x667898
    explicit SelectionToolTooltipAugmentation(BrickConnectSceneManager* sceneManager);
    virtual ~SelectionToolTooltipAugmentation() {}                          // Mac 0x161470/0x161510, Win sdd 0x667a32

    // The element whose description is shown (null: none). (inline in the original: +0x3c)
    void SetElement(PrimitiveElement* element) { m_element = element; }
    PrimitiveElement* GetElement() const { return m_element; }
    bool IsEnabled() const { return m_enabled; }

    // ---- TooltipAugmentation ----
    // At once (0). Mac 0x161550, Win 0x6c9dea (ICF)
    virtual UInt32 GetDelay();
    // Only while BRICKTOOLTIP is on and UpdateText has a text. Mac 0x15fea0, Win 0x667304
    virtual bool CreateTooltip(RefPointer<Control>& tooltip, ControlRefArray& extra);
    // While UpdateText has a text. Mac 0x15fed0, Win 0x66732d
    virtual bool UpdateTooltip(Control* tooltip, ControlRefArray& extra);

protected:
    // BRICKTOOLTIP (any case) is read again. Mac 0x15fc80 (thunk 0x15fd40), Win 0x667202
    virtual void OnPreferenceChanged(const String& key);
    // In developer mode (preference DeveloperMode) and with an element: the text becomes the
    // element's brick description; false otherwise. Mac 0x15fd60, Win 0x667256
    bool UpdateText();

    // ---- data (Mac offsets; PreferencesObserver +0x30, its subject +0x34) ----
    BrickConnectSceneManager* m_sceneManager;   // +0x38 (its database names the bricks)
    PrimitiveElement*         m_element;        // +0x3c
    bool                      m_enabled;        // +0x40 preference BRICKTOOLTIP
                                                // Object +0x44
};

class SelectionTool : public ClickStickPickupTool {
public:
    class Filter : public Object {
    public:
        Filter() {}                                                            // Win 0x627789
        virtual ~Filter() {}                                                   // Win 0x627749 / sdd 0x62776d
        // Mac slot 5 / Win slot 4: narrows (or replaces) `parts` given the picked element.
        virtual void Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const = 0;
    };

    // `toggle`: a click without filters toggles the clicked part (the multiple selection tool);
    // `replaceSelection`: ClickStickPickupTool's. The tool cursor is 0x66.
    // Mac 0x160210 (C1) / 0x15ff00 (C2), Win 0x667bff
    SelectionTool(ClickStickTool* clickStickTool, bool toggle, bool replaceSelection);
    virtual ~SelectionTool();                                               // Mac 0x1617d0/0x161830, Win sdd 0x62b4a6

    // Active; the tooltip (CreateSelectionToolTooltip) and the magenta box mesh (CreateBoxMesh
    // with the scene manager's silhouette material). False when the mesh fails.
    // Mac 0x1604f0, Win 0x667350
    bool Create(Render* render, BrickConnectSceneManager* sceneManager);

    // ---- BrickConnectSceneControlTool / PickTool (Mac slot / Win slot) ----
    // The parts of a group tree click are added to the selection. Mac 0x1605d0, Win 0x66776e
    virtual void OnToolEvent24(const BasicArray<RefPointer<Part> >& parts); // 24/21
    // Press on empty space (or `clear`): the selection is emptied (and again on release).
    // Mac 0x160d10, Win 0x66747c
    virtual void OnPickPressed(bool clear);                                 // 27/22
    // Click: the filters' parts become the selection; without filters the toggle tools toggle
    // the part. Mac 0x160db0, Win 0x6674bb
    virtual void OnPickClicked(bool flag);                                  // 29/24
    // The selection is saved, its boxes hidden. Mac 0x160fb0, Win 0x66756e
    virtual void OnRubberBandStarted();                                     // 30/25
    // The band's parts that were not selected before become the selection. Mac 0x161060,
    // Win 0x6675a2
    virtual void OnRubberBandChanged();                                     // 31/26
    // The selection boxes are shown again. Mac 0x161020, Win 0x6671cc
    virtual void OnRubberBandFinished();                                    // 32/27
    // A press on empty space empties the selection. Mac 0x161300, Win 0x6671ec
    virtual void OnPickReleased();                                          // 33/28

    // ---- ControlObserver ----
    // UpdateHover, then PickTool's. Mac 0x160cb0 (thunk 0x160cd0), Win 0x667825
    virtual void OnMouseMove(Control* control, const Vector2& position);
    // The hovered part goes. Mac 0x160bf0 (Mac-only primary entry 38; thunk 0x160c50),
    // Win 0x6673ad
    virtual void OnMouseLeave(Control* control, const Vector2& position);

    // The part under `position` (PickElement, element space, material slot): when it changed (or
    // m_hoverDirty), the tooltip follows, the control is redrawn and the box transform of the
    // part is computed (GetPartBoxTransform, grown). Mac 0x1608c0, Win 0x667682
    void UpdateHover(const Vector2& position);

protected:
    // ---- BrickConnectSceneControlObserver ----
    // The tooltip is anchored to / attached to the control; BCSCTool's; the hover follows the
    // mouse when it is inside. Mac 0x160850, Win 0x6677d5
    virtual void OnAttached(BrickConnectSceneControl* control);
    // No hover, the tooltip removed, the saved selection dropped; PickTool's. Mac 0x160b30,
    // Win 0x667430
    virtual void OnDetached(BrickConnectSceneControl* control);
    using PickTool::OnAttached;
    using PickTool::OnDetached;
    // The box mesh of the hovered part into the overlay queue. Mac 0x161330, Win 0x6673e0
    virtual void OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* overlay,
                           RenderQueue* scene);
    // The hover is refreshed (m_hoverDirty) while the mouse is inside. Mac 0x161390 / 0x161400,
    // Win 0x667847 (both)
    virtual void OnEvent18();
    virtual void OnEvent19();

    // Appends a filter (inline push_back, emitted in EditMode.cpp: Mac 0x192060). (name inferred)
    void AddFilter(Filter* filter) { m_filters.PushBack(RefPointer<Filter>(filter)); }

    // ---- data (Mac offsets) ----
    BasicArray<RefPointer<Filter> >              m_filters;          // +0xac
    PrimitiveElement*                            m_hover;            // +0xbc element under the mouse
    Matrix3D                                     m_hoverTransform;   // +0xc0 its part's box
    RefPointer<Mesh>                             m_boxMesh;          // +0x100
    RefPointer<SelectionToolTooltipAugmentation> m_tooltip;          // +0x104
    bool                                         m_clearOnRelease;   // +0x108
    BasicArray<RefPointer<Part> >                m_savedSelection;   // +0x10c
    bool                                         m_toggle;           // +0x11c
    bool                                         m_hoverDirty;       // +0x11d
                                                                     // Object +0x120
};

} // namespace LEGO
