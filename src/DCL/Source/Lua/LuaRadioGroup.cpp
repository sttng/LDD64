// LEGO Digital Designer — restored source
// TU 388 "LuaRadioGroup.cpp": Mac 0x378fb0-0x37a62b, Win 0x457900-0x458c9f.
#include "LuaRadioGroup.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "LuaToggleControl.h"
#include "XMLDOMNode.h"
#include "StringStream.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

LuaRadioGroup* CheckRadioGroup(lua_State* L)                                // Mac 0x37a2f0
{
    return CheckBoundObject<LuaRadioGroup>(LuaState::CastState(L), 1);
}

// RadioGroup.Create(self). Mac 0x379030
int LuaRadioGroupCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    RefPointer<LuaRadioGroup> object = new LuaRadioGroup;
    AttachLuaObject(self, object.Get());
    return 0;
}

// self:SetCurrent(index). Mac 0x379180, Win 0x458890
int LuaRadioGroupSetCurrent(lua_State* L)
{
    LuaRadioGroup* self = CheckRadioGroup(L);
    self->SetCurrent((UInt32)luaL_checkinteger(L, 2));
    return 0;
}

// self:GetCurrent(). Mac 0x3791d0
int LuaRadioGroupGetCurrent(lua_State* L)
{
    LuaRadioGroup* self = CheckRadioGroup(L);
    lua_pushnumber(L, (lua_Number)self->GetCurrent());
    return 1;
}

// self:GetCount(). Mac 0x379240
int LuaRadioGroupGetCount(lua_State* L)
{
    LuaRadioGroup* self = CheckRadioGroup(L);
    lua_pushnumber(L, (lua_Number)self->GetCount());
    return 1;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x752e4c
    { "Create", LuaRadioGroupCreate },
    { "SetCurrent", LuaRadioGroupSetCurrent },
    { "GetCurrent", LuaRadioGroupGetCurrent },
    { "GetCount", LuaRadioGroupGetCount },
    { 0, 0 }
};

} // namespace

LuaRadioGroup::~LuaRadioGroup()                                             // Mac 0x3794a0 (D2 0x3793b0), Win 0x457d60
{
}

// Mac 0x379720, Win 0x457e00
void LuaRadioGroup::OnFinalRelease()
{
    ToggleControlObserver::OnFinalRelease();
    LuaLayout::OnFinalRelease();
}

void LuaRadioGroup::UnbindLuaObject()                                       // Mac 0x379880, Win 0x457e10
{
    m_entries.Clear();
}

void LuaRadioGroup::ApplyCurrent()
{
    m_updating = true;
    for (UInt32 i = 0; i < m_entries.GetSize(); ++i) {
        m_entries[i].first->SetToggled(i == m_current);
        if (Control* shown = m_entries[i].second.Get())
            shown->SetShown(i == m_current);
    }
    m_updating = false;
}

// self:OnChanged(index) (CallLuaMethod<UInt32>, Mac 0x37a4b0). Mac 0x3792b0, Win 0x458b30
void LuaRadioGroup::SetCurrent(UInt32 index)
{
    if (m_current == index)
        return;
    m_current = index;
    ApplyCurrent();
    LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "OnChanged", false, 0, (unsigned int)index);
}

// Each <Button control="path" [show="path"]>: the toggle of the LuaToggleControl at `path` and
// the control at the show path join the group (observed, not untoggleable); the first entry
// shows its control and calls self:OnChanged(0). Then the toggles / shown controls follow the
// current index.
// NOTE: the original builds error texts ("LuaRadioGroup: 'Button' node under '<layout>' does not
// contain a control attribute." / "… which does not point to a ToggleControl." / "… which does
// not point to a Control.") and drops them (report compiled out in both builds); a show path
// that is not a control adds the toggle without one.
// Mac 0x379920, Win 0x457ea0
void LuaRadioGroup::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    LuaLayout::OnLayoutApplied(layout, args);
    DefineLayoutFunction(layout, "OnChanged", "index", args);
    bool added = false;
    const XMLDOMNode::NodeArray& children = layout->GetChildren();
    for (UInt32 i = 0; i < children.GetSize(); ++i) {
        const XMLDOMNode* node = children[i].Get();
        if (node->GetName() != String("Button"))
            continue;
        String path;
        if (!node->GetAttribute(String("control"), path))
            continue;
        ToggleControl* toggle = 0;
        {
            LuaObject object = LookupLuaObject(*args.m_table, path);
            if (BindableLuaObject* bound = GetBoundObject(object))
                if (LuaToggleControl* lua = dynamic_cast<LuaToggleControl*>(bound))
                    toggle = lua->GetToggle();
        }
        if (!toggle)
            continue;
        Control* shown = 0;
        {
            String showPath;
            if (node->GetAttribute(String("show"), showPath)) {
                LuaObject object = LookupLuaObject(*args.m_table, showPath);
                if (BindableLuaObject* bound = GetBoundObject(object))
                    if (LuaControl* lua = dynamic_cast<LuaControl*>(bound))
                        shown = lua->GetTargetControl();
            }
        }
        m_entries.PushBack(Entry(RefPointer<ToggleControl>(toggle), RefPointer<Control>(shown)));
        ToggleControlObserver::Attach(toggle);
        toggle->SetCanUntoggle(false);
        if (m_entries.GetSize() == 1) {
            if (shown)
                shown->SetShown(true);
            LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
            CallLuaMethod(self, "OnChanged", false, 0, (unsigned int)0);
        }
        added = true;
    }
    if (added)
        ApplyCurrent();
}

// Mac 0x37a260, Win 0x458420
void LuaRadioGroup::OnToggled(ToggleControl* control)
{
    if (m_updating)
        return;
    for (UInt32 i = 0; i < m_entries.GetSize(); ++i) {
        if (m_entries[i].first.Get() == control) {
            SetCurrent(i);
            return;
        }
    }
}

void RegisterLuaRadioGroup(LuaState* state)                                 // Mac 0x378fb0, Win 0x457c90
{
    LuaObject luaClass = CreateLuaClass(state, "RadioGroup", "Layout");
    luaClass.Register(kFunctions, 0);
}

} // namespace LEGO
