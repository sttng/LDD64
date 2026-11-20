// LEGO Digital Designer — restored source
// LEGO::MaterialTool — see MaterialTool.h.
// TU 093 "MaterialTool.cpp": Mac 0x1a7fa0-0x1a903b, Win 0x67da26-0x67de95. Not code: the thunks,
// the destructor body (0x1a8420), the BasicArray<RefPointer<const AbstractBrick> > instance
// (0x1a8c40, 0x1a8f40), Mac 0x1a8fc0 = <iostream> + a header static.
#include "MaterialTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ControlManager.h"
#include "PrimitiveElement.h"
#include "Part.h"
#include "Brick.h"

namespace LEGO {

// Mac 0x1a7fa0, Win 0x67de95
MaterialTool::MaterialTool(bool withButton, CompositeBrickFilter* filter)
    : PickTool(false),
      m_control(0),
      m_sceneManager(0),
      m_filter(filter),
      m_matcher(0),
      m_withButton(withButton)
{
    LDDApplicationObserver::Attach(LDDApplication::GetInstance());
    SetCursor(0x6d);
}

// Mac 0x1a8590 / 0x1a8740 (body 0x1a8420), Win sdd 0x67dbd0
MaterialTool::~MaterialTool()
{
}

// Mac 0x1a8170, Win 0x67db12
bool MaterialTool::Create(BrickConnectSceneControl* sceneControl, ControlManager* manager,
                          BrickConnectSceneManager* sceneManager, CompositeBrickFilter* filter)
{
    m_sceneManager = sceneManager;
    m_control = new MaterialToolControl(manager, sceneManager, this, filter, 10, 8);
    return m_control->Create(sceneControl, 0);
}

// Mac 0x1a8270 (thunks 0x1a82e0 …), Win 0x67de76
void MaterialTool::OnFinalRelease()
{
    BrickConnectSceneControlTool::OnFinalRelease();
    LDDApplicationObserver::OnFinalRelease();
}

// Mac 0x1a8930, Win 0x67dd2a
void MaterialTool::OnPickClicked(bool flag)
{
    if (flag || m_picked == 0)
        return;
    BasicArray<RefPointer<const AbstractBrick> > matches;
    m_matcher->FindMatches(m_picked->GetPart()->GetBrick(), matches);
    MaterialControl::BrickArray bricks;
    bricks.Resize(matches.GetSize());
    for (UInt32 i = 0; i < matches.GetSize(); ++i)
        bricks[i] = static_cast<const BrickPalette::Brick*>(matches[i].Get());
    m_control->SetPickedElement(m_picked, bricks, m_withButton);
    m_control->SetPosition(Vector2(m_pressPosition.x + -10.0f, -10.0f + m_pressPosition.y));
    m_control->MoveInsideParent();
}

// Mac 0x1a8cb0, Win 0x67dc31
void MaterialTool::OnAttached(BrickConnectSceneControl* control)
{
    BrickConnectSceneControlTool::OnAttached(control);
    GetSceneControl()->ClearSelection();
    if (m_matcher)
        return;
    BrickPalette* palette = LDDApplication::GetInstance()->GetBrickPalette();
    m_matcher = new BrickMatcher(m_sceneManager->GetDatabase());
    for (BrickPalette::BrickSet::ConstIterator it = palette->GetBricksBegin();
         it != palette->GetBricksEnd(); ++it) {
        const BrickPalette::Brick* brick = (*it).Get();
        if (m_filter->Accept(brick))
            m_matcher->AddCandidate(brick);
    }
}

// Mac 0x1a8df0, Win 0x67db8d
void MaterialTool::OnDetached(BrickConnectSceneControl* control)
{
    m_control->SetShown(false);
    PickTool::OnDetached(control);
}

// Mac 0x1a8e20 (thunk 0x1a8e50), Win 0x67da26
void MaterialTool::OnMousePressed(Control* control, const Vector2& position, UInt32 button)
{
    m_pressPosition = position;
    PickTool::OnMousePressed(control, position, button);
}

// Mac 0x1a8e90, Win 0x67dbac
void MaterialTool::OnEvent19()
{
    PickTool::OnEvent19();
    m_control->SetShown(false);
}

// Mac 0x1a8ec0 (thunk 0x1a8f00), Win 0x67dbc3
void MaterialTool::OnBrandChanged(Brand)
{
    m_matcher = 0;
}

} // namespace LEGO
