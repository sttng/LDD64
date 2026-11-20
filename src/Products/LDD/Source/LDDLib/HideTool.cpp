// LEGO Digital Designer — restored source
// LEGO::HideTool, LEGO::HideToolControl, LEGO::UndoHideTool — see HideTool.h.
// TU 292 "HideTool.cpp": Mac 0x29fdd0-0x2a17db, Win 0x67cbb9-0x67d54c. Not code: the thunks and
// the Observer<BrickConnectSceneControl, …> destructor copies (0x2a0f20-0x2a12d0), Mac 0x2a1760 =
// <iostream> + a header static.
#include "HideTool.h"
#include "BrickConnectSceneControl.h"
#include "RenderControlManager.h"
#include "ControlManager.h"
#include "PrimitiveElement.h"
#include "Part.h"
#include "Brick.h"
#include "Scene.h"
#include "TextLocale.h"
#include "TooltipManager.h"
#include "StandardTextTooltipAugmentation.h"
#include "LDDUtilities.h"

namespace LEGO {

// ===========================================================================================
// UndoHideTool
// ===========================================================================================

// Mac 0x2a02a0, Win 0x67cbb9
UndoHideTool::UndoHideTool(BrickConnectSceneControl* control,
                           const BasicArray<RefPointer<Brick> >& bricks, bool hide)
    : m_bricks(bricks), m_control(control), m_hide(hide)
{
}

// Mac 0x2a1540, Win 0x67cc8d
void UndoHideTool::Execute()
{
    if (!m_hide) {
        m_control->GetUndoManager()->AddAction(new UndoHideTool(m_control, m_bricks, true));
        m_control->ShowBricks(m_bricks);
    } else {
        m_control->GetUndoManager()->AddAction(new UndoHideTool(m_control, m_bricks, false));
        m_control->HideBricks(m_bricks);
    }
}

// ===========================================================================================
// HideToolControl
// ===========================================================================================

// Mac 0x29fdd0, Win 0x67d0f6
HideToolControl::HideToolControl(BrickConnectSceneControl* control)
    : ButtonControl(control->GetManager()), m_control(control)
{
    BrickConnectSceneControlObserver::Attach(control);
}

// Mac 0x29fed0 (thunks 0x29ff40, 0x29ffb0, 0x2a0020, 0x2a0090)
void HideToolControl::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    ButtonControl::OnFinalRelease();
}

// Mac 0x2a0110, Win 0x67cd4c
void HideToolControl::OnMouseClicked(const Vector2& position, UInt32 button)
{
    BasicArray<RefPointer<Brick> > bricks;
    m_control->GetHiddenBricks(bricks);
    m_control->GetUndoManager()->AddAction(new UndoHideTool(m_control, bricks, true));
    m_control->ShowBricks(bricks);
    Control::OnMouseClicked(position, button);
}

// Mac 0x2a0380 (thunk 0x2a0490), Win 0x67cdf4
void HideToolControl::OnEvent22(UInt32)
{
    BasicArray<RefPointer<Brick> > bricks;
    m_control->GetHiddenBricks(bricks);
    SetShown(!bricks.IsEmpty());
}

// ===========================================================================================
// HideTool
// ===========================================================================================

// Mac 0x2a04c0, Win 0x67ce53
HideTool::HideTool(ClickStickTool* clickStickTool)
    : SelectionTool(clickStickTool, true, true), m_control(0)
{
    SetCursor(0x73);
}

// Mac 0x2a1370 / 0x2a1430, Win sdd 0x67d024
HideTool::~HideTool()
{
}

// Mac 0x2a05b0, Win 0x67cf8a
void HideTool::Hide(const BasicArray<RefPointer<Part> >& parts)
{
    if (parts.IsEmpty())
        return;
    BasicArray<RefPointer<Brick> > bricks;
    Scene::GetBricks(parts, bricks);
    m_sceneControl->GetUndoManager()->AddAction(new UndoHideTool(m_sceneControl, bricks, false));
    m_sceneControl->HideBricks(bricks);
}

// Mac 0x2a05a0, Win 0x67d065
void HideTool::OnToolEvent24(const BasicArray<RefPointer<Part> >& parts)
{
    Hide(parts);
}

// Mac 0x2a1470, Win 0x5c8021 (ICF)
void HideTool::OnPickDragged()
{
}

// Mac 0x2a0750, Win 0x67d083
void HideTool::OnPickClicked(bool)
{
    // NOTE: the flag is not tested (both builds).
    BasicArray<RefPointer<Part> > parts;
    parts.PushBack(RefPointer<Part>(m_picked->GetPart()));
    Hide(parts);
}

// Mac 0x2a0720, Win 0x67d06a
void HideTool::OnPickReleased()
{
    Hide(GetSceneControl()->GetSelection());
}

// Mac 0x2a08b0, Win 0x67d299
void HideTool::OnAttached(BrickConnectSceneControl* control)
{
    SelectionTool::OnAttached(control);
    if (!m_control) {
        m_control = new HideToolControl(control);
        m_control->Create(control, 0, 6, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, true);
        m_control->SetAlignment(0x13, 0x13, 20.0f, -10.0f, 0);
        RefPointer<Texture> texture = LoadApplicationTexture(
            String("Graphics/EditScreen/UI02.png"),
            static_cast<RenderControlManager*>(control->GetManager())->GetRender());
        if (texture) {
            ButtonControl::StateDescription state;
            state.m_image = texture;
            state.m_transparent = true;
            state.m_imageRect.Set(Rect(Vector2(2.0f, 2.0f), Vector2(72.0f, 48.0f)));
            m_control->SetStateDescription(0, state);
            state.m_imageRect.Set(Rect(Vector2(2.0f, 51.0f), Vector2(72.0f, 48.0f)));
            m_control->SetStateDescription(1, state);
            state.m_imageRect.Set(Rect(Vector2(2.0f, 99.0f), Vector2(72.0f, 48.0f)));
            m_control->SetStateDescription(2, state);
        }
        CreateStandardTextTooltip(m_control, LocalizedString(String("HIDE TOOL UNHIDE TOOLTIP")))
            ->SetAnchor(control);
    }
    Hide(GetSceneControl()->GetSelection());
}

} // namespace LEGO
