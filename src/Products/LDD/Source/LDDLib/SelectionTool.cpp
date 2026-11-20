// LEGO Digital Designer — restored source
// LEGO::SelectionTool, LEGO::SelectionToolTooltipAugmentation — see SelectionTool.h.
// TU 069 "SelectionTool.cpp": Mac 0x15fa30-0x161f8b, Win 0x62834a-0x62b4a6 / 0x6671cc-0x667a32.
// Not code: the thunks, the BasicArray<RefPointer<Filter> > instance (0x1601b0, 0x161e90), the
// base-object constructor / destructor bodies with VTT (0x15ff00, 0x161a80, 0x161bd0), Mac
// 0x161f10 = <iostream> + a header static. ClickStickPickupTool's destructor (0x161620) is the
// header's inline one.
#include "SelectionTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ControlManager.h"
#include "Control.h"
#include "PrimitiveElement.h"
#include "Part.h"
#include "Color.h"
#include "Materials.h"
#include "RenderQueue.h"
#include "LDDUtilities.h"
#include "StringStream.h"

namespace LEGO {

namespace {

// The control is redrawn unless its manager is drawing (inline everywhere).
void RedrawControl(Control* control)
{
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

// std::find over the parts (inline).
bool ContainsPart(const BasicArray<RefPointer<Part> >& parts, const Part* part)
{
    for (UInt32 i = 0; i < parts.GetSize(); ++i) {
        if (parts[i].Get() == part)
            return true;
    }
    return false;
}

} // namespace

// ===========================================================================================
// SelectionToolTooltipAugmentation
// ===========================================================================================

// Mac 0x15fa30, Win 0x667898
SelectionToolTooltipAugmentation::SelectionToolTooltipAugmentation(BrickConnectSceneManager* sceneManager)
    : StandardTextTooltipAugmentation(0, String()),
      m_sceneManager(sceneManager),
      m_element(0),
      m_enabled(false)   // NOTE: not initialised by the original (only read from the preference)
{
    PreferencesObserver::Attach(Preferences::GetInstance());
    String value;
    if (Preferences::GetInstance()->GetValue(String("BRICKTOOLTIP"), value))
        FromString(m_enabled, value);
}

// Mac 0x161550, Win 0x6c9dea (ICF)
UInt32 SelectionToolTooltipAugmentation::GetDelay()
{
    return 0;
}

// Mac 0x15fc80 (thunk 0x15fd40), Win 0x667202
void SelectionToolTooltipAugmentation::OnPreferenceChanged(const String& key)
{
    if (!key.EqualsNoCase(String("BRICKTOOLTIP")))
        return;
    String value;
    if (PreferencesObserver::GetSubject()->GetValue(key, value))
        FromString(m_enabled, value);
}

// Mac 0x15fd60, Win 0x667256
bool SelectionToolTooltipAugmentation::UpdateText()
{
    bool developerMode = false;
    String value;
    if (Preferences::GetInstance()->GetValue(String("DeveloperMode"), value))
        FromString(developerMode, value);
    if (!developerMode || m_element == 0)
        return false;
    m_text = LDDUtilities::GetBrickDescription(m_sceneManager->GetDatabase(), m_element);
    return true;
}

// Mac 0x15fea0, Win 0x667304
bool SelectionToolTooltipAugmentation::CreateTooltip(RefPointer<Control>& tooltip, ControlRefArray& extra)
{
    if (m_enabled && UpdateText())
        return StandardTextTooltipAugmentation::CreateTooltip(tooltip, extra);
    return false;
}

// Mac 0x15fed0, Win 0x66732d
bool SelectionToolTooltipAugmentation::UpdateTooltip(Control* tooltip, ControlRefArray& extra)
{
    if (UpdateText())
        return StandardTextTooltipAugmentation::UpdateTooltip(tooltip, extra);
    return false;
}

// ===========================================================================================
// SelectionTool
// ===========================================================================================

// Mac 0x160210 (C1), 0x15ff00 (C2), Win 0x667bff
SelectionTool::SelectionTool(ClickStickTool* clickStickTool, bool toggle, bool replaceSelection)
    : ClickStickPickupTool(clickStickTool, replaceSelection),
      m_hover(0),
      m_boxMesh(0),
      m_tooltip(0),
      m_clearOnRelease(false),   // (not initialised by the original)
      m_toggle(toggle),
      m_hoverDirty(false)
{
    SetCursor(0x66);
}

// Mac 0x1617d0 / 0x161830 (body 0x161bd0), Win sdd 0x62b4a6
SelectionTool::~SelectionTool()
{
}

// Mac 0x1604f0, Win 0x667350
bool SelectionTool::Create(Render* render, BrickConnectSceneManager* sceneManager)
{
    SetActive(true);
    m_tooltip = CreateSelectionToolTooltip(sceneManager);
    m_boxMesh = CreateBoxMesh(render, Color::Magenta(), sceneManager->GetSilhouetteMaterial(), false);
    return m_boxMesh != 0;
}

// Mac 0x1605d0, Win 0x66776e
void SelectionTool::OnToolEvent24(const BasicArray<RefPointer<Part> >& parts)
{
    BasicArray<RefPointer<Part> > selection = GetSceneControl()->GetSelection();
    for (UInt32 i = 0; i < parts.GetSize(); ++i) {
        if (!ContainsPart(selection, parts[i]))
            selection.PushBack(parts[i]);
    }
    GetSceneControl()->SetSelection(selection);
}

// Mac 0x160850, Win 0x6677d5
void SelectionTool::OnAttached(BrickConnectSceneControl* control)
{
    m_tooltip->SetAnchor(control);
    m_tooltip->AttachTo(control);
    BrickConnectSceneControlTool::OnAttached(control);
    if (control->GetStatus().m_mouseInside)
        UpdateHover(control->GetManager()->GetMousePosition(control));
}

// Mac 0x1608c0, Win 0x667682
void SelectionTool::UpdateHover(const Vector2& position)
{
    BasicArray<RefPointer<Part> > filterParts;
    BasicArray<RefPointer<Part> > extraParts;
    PrimitiveElement* element =
        GetSceneControl()->PickElement(position, 0, true, 0, true, filterParts, extraParts);
    if (!m_hoverDirty && element == m_hover)
        return;
    m_hoverDirty = false;
    m_hover = element;
    m_tooltip->SetElement(element);
    m_tooltip->Refresh();
    RedrawControl(GetSceneControl());
    if (m_hover != 0)
        GetPartBoxTransform(m_hover->GetPart(), m_hoverTransform, true);
}

// Mac 0x160b30, Win 0x667430
void SelectionTool::OnDetached(BrickConnectSceneControl* control)
{
    if (m_hover != 0)
        RedrawControl(control);
    m_hover = 0;
    m_tooltip->SetElement(0);
    m_tooltip->Refresh();
    m_tooltip->Remove();
    m_savedSelection.Clear();
    PickTool::OnDetached(control);
}

// Mac 0x160bf0 (thunk 0x160c50), Win 0x6673ad
void SelectionTool::OnMouseLeave(Control*, const Vector2&)
{
    if (m_hover != 0)
        RedrawControl(GetSceneControl());
    m_hover = 0;
    m_tooltip->SetElement(0);
    m_tooltip->Refresh();
}

// Mac 0x160cb0 (thunk 0x160cd0), Win 0x667825
void SelectionTool::OnMouseMove(Control* control, const Vector2& position)
{
    UpdateHover(position);
    PickTool::OnMouseMove(control, position);
}

// Mac 0x160d10, Win 0x66747c
void SelectionTool::OnPickPressed(bool clear)
{
    if (m_picked == 0 || clear) {
        m_savedSelection.Clear();
        GetSceneControl()->SetSelection(m_savedSelection);
    }
    m_clearOnRelease = m_picked == 0;
}

// Mac 0x160db0, Win 0x6674bb
void SelectionTool::OnPickClicked(bool flag)
{
    if (flag)
        return;
    m_clearOnRelease = false;
    const BasicArray<RefPointer<Part> >& selection = GetSceneControl()->GetSelection();
    if (m_filters.IsEmpty()) {
        if (m_toggle)
            GetSceneControl()->ToggleSelection(m_picked->GetPart());
        return;
    }
    BasicArray<RefPointer<Part> > parts = selection;
    for (UInt32 i = 0; i < m_filters.GetSize(); ++i)
        m_filters[i]->Apply(m_picked, parts);
    GetSceneControl()->SetSelection(parts);
}

// Mac 0x160fb0, Win 0x66756e
void SelectionTool::OnRubberBandStarted()
{
    m_clearOnRelease = false;
    const BasicArray<RefPointer<Part> >& selection = GetSceneControl()->GetSelection();
    if (&selection != &m_savedSelection)
        m_savedSelection = selection;
    GetSceneControl()->SetShowSelectionBoxes(false);
}

// Mac 0x161060, Win 0x6675a2
void SelectionTool::OnRubberBandChanged()
{
    // NOTE: the parts selected before the band started are left out (both builds).
    BasicArray<RefPointer<Part> > parts;
    for (UInt32 i = 0; i < m_rubberBandParts.GetSize(); ++i) {
        if (!ContainsPart(m_savedSelection, m_rubberBandParts[i]))
            parts.PushBack(m_rubberBandParts[i]);
    }
    GetSceneControl()->SetSelection(parts);
}

// Mac 0x161020, Win 0x6671cc
void SelectionTool::OnRubberBandFinished()
{
    GetSceneControl()->SetShowSelectionBoxes(true);
    GetSceneControl()->UpdateSelectionMesh();
}

// Mac 0x161300, Win 0x6671ec
void SelectionTool::OnPickReleased()
{
    if (m_clearOnRelease)
        GetSceneControl()->ClearSelection();
}

// Mac 0x161330, Win 0x6673e0
void SelectionTool::OnEvent05(const Matrix3D& transform, Render*, RenderQueue* overlay, RenderQueue*)
{
    if (m_hover == 0)
        return;
    Matrix3D boxTransform;
    boxTransform.Multiply(transform, m_hoverTransform);
    overlay->Add(m_boxMesh, boxTransform, 0.0f, 0, 1.0f);
}

// Mac 0x161390, Win 0x667847
void SelectionTool::OnEvent18()
{
    BrickConnectSceneControl* control = GetSceneControl();
    if (control != 0 && control->GetStatus().m_mouseInside) {
        m_hoverDirty = true;
        UpdateHover(control->GetManager()->GetMousePosition(control));
    }
}

// Mac 0x161400, Win 0x667847 (ICF with OnEvent18)
void SelectionTool::OnEvent19()
{
    BrickConnectSceneControl* control = GetSceneControl();
    if (control != 0 && control->GetStatus().m_mouseInside) {
        m_hoverDirty = true;
        UpdateHover(control->GetManager()->GetMousePosition(control));
    }
}

// ===========================================================================================
// LDDUtilities (TU 063): emitted in LDDUtilities' object in the original; defined
// here so that the users of LDDUtilities do not need the tool classes. (name inferred)
// ===========================================================================================

// Mac 0x14d5a0 (Win inlined into SelectionTool::Create)
SelectionToolTooltipAugmentation* CreateSelectionToolTooltip(BrickConnectSceneManager* sceneManager)
{
    return new SelectionToolTooltipAugmentation(sceneManager);
}

} // namespace LEGO
