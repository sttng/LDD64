// LEGO Digital Designer — restored source
// TU 310 "LuaPopupAugmentation.cpp": Mac 0x2b8430-0x2b97bb, Win 0x45a5b0-0x45af4f.
// The Mac static initialiser 0x2b9740 is not code.
#include "LuaPopupAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "XMLDOMNode.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// PopupAugmentation.Create(self [, control]). Mac 0x2b85a0, Win 0x45a6d0
int LuaPopupAugmentationCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RefPointer<LuaPopupAugmentation> object;
    {
        LuaObject self = GetUnboundSelf(state);
        object = new LuaPopupAugmentation;
        AttachLuaObject(self, static_cast<LuaControlAugmentation*>(object.Get()));
    }
    Control* control = OptLuaControl(state, 2, 0);
    if (control)
        object->Attach(control, String("popup"));
    return 0;
}

// self:PopupChildAlign(reference, child). Mac 0x2b8780
int LuaPopupAugmentationPopupChildAlign(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    Control* popup = LuaPopupAugmentation::Check(state, 1)->GetControl();
    Control* reference = CheckLuaControl(state, 2);
    Control* child = CheckLuaControl(state, 3);
    popup->SetZOrder(0);
    popup->SetShown(true);
    Vector2 offset = child->LocalToControl(reference, Vector2(0.0f, 0.0f));
    popup->SetPosition(popup->GetPosition() + offset);
    popup->MoveInsideParent();
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x746440
    { "Create", LuaPopupAugmentationCreate },
    { "PopupChildAlign", LuaPopupAugmentationPopupChildAlign },
    { 0, 0 }
};

} // namespace

LuaPopupAugmentation* LuaPopupAugmentation::Check(LuaState* state, int narg) // Mac 0x2b95a0
{
    return CheckBoundObject<LuaPopupAugmentation>(state, narg);
}

void LuaPopupAugmentation::UnbindLuaObject()                                 // Mac 0x2b8e40
{
    Detach();
}

bool LuaPopupAugmentation::Attach(Control* control, const String& name)     // Mac 0x2b8850, Win 0x45aad0
{
    bool attached = ControlAugmentationControlObserver::Attach(control, name);
    m_popup = GetControl();
    if (m_popup)
        m_popup->SetShown(false);
    return attached;
}

void LuaPopupAugmentation::Detach()                                          // Mac 0x2b8990, Win 0x45ab20
{
    ControlManagerObserver::Detach();
    m_popup = 0;
    ControlAugmentationControlObserver::Detach();
}

void LuaPopupAugmentation::OnFinalRelease()                                  // Mac 0x2b8d10, Win 0x45ac30
{
    ControlManagerObserver::OnFinalRelease();
    ControlAugmentationControlObserver::OnFinalRelease();
    LuaControlAugmentation::OnFinalRelease();
}

void LuaPopupAugmentation::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2b8e50, Win 0x45ac70
{
    LuaLayout::OnLayoutApplied(layout, args);
    String path;
    if (!layout->GetAttribute(String("owner"), path))
        return;
    if (m_owner)
        StopObservingControl(m_owner);
    if (path.IsEmpty())
        return;
    LuaObject object = LookupLuaObject(*args.m_table, path);
    if (object.IsNil())
        return;   // NOTE: m_owner keeps the (no longer observed) previous owner (original)
    Control* owner = 0;
    if (BindableLuaObject* bound = GetBoundObject(object))
        if (LuaControl* control = dynamic_cast<LuaControl*>(bound))
            owner = control->GetTargetControl();
    m_owner = owner;
    // NOTE: the original uses the owner without a null check (a path to a non-control crashes);
    // fixed: skipped.
    if (!owner)
        return;
    ObserveControl(owner);
    if (m_popup && !owner->GetStatus().m_visible)
        m_popup->SetShown(false);
}

void LuaPopupAugmentation::OnShown(Control* control)                         // Mac 0x2b8ff0, Win 0x45add0
{
    if (m_popup != control)
        return;
    ControlManagerObserver::Attach(control->GetManager());
}

void LuaPopupAugmentation::OnHidden(Control* control)                        // Mac 0x2b9290, Win 0x45adf0
{
    if (!m_popup)
        return;
    if (m_owner == control)
        m_popup->SetShown(false);
    else if (m_popup == control)
        ControlManagerObserver::Detach();
}

void LuaPopupAugmentation::OnManagerMouseMove(const Vector2& position)      // Mac 0x2b9370, Win 0x45ae20
{
    if (!m_popup)
        return;
    Rect rectangle(m_popup->GetSize());
    Vector2 local(position);
    m_popup->ManagerToLocal(local);
    if (rectangle.Contains(local))
        return;
    Control* mouse = m_popup->GetManager()->GetMouseControl();
    if (mouse && mouse->GetManager()->GetMouseControl() == mouse &&
        (mouse->HasMouseCapture() || mouse->GetPressedButtons() != 0)) {
        if (mouse->IsDescendantOf(m_popup))
            return;
    }
    m_popup->SetShown(false);
}

void LuaPopupAugmentation::OnManagerMouseOverChanged(Control* /*previous*/, Control* current) // Mac 0x2b9440, Win 0x45aec0
{
    if (!m_popup)
        return;
    Rect rectangle(m_popup->GetSize());
    Vector2 mouse = m_popup->GetManager()->GetMousePosition(m_popup);
    if (rectangle.Contains(mouse))
        return;
    if (current && current->IsDescendantOf(m_popup))
        return;
    m_popup->SetShown(false);
}

void LuaPopupAugmentation::OnManagerModalControl(bool /*modal*/, Control* /*control*/) // Mac 0x2b9540
{
    if (m_popup)
        m_popup->SetShown(false);
}

void RegisterLuaPopupAugmentation(LuaState* state)                           // Mac 0x2b8520, Win 0x45aa20
{
    LuaObject luaClass = CreateLuaClass(state, "PopupAugmentation", "ControlAugmentation");
    luaClass.Register(kFunctions, 0);
}

} // namespace LEGO
