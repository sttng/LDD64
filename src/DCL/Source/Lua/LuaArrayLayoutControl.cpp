// LEGO Digital Designer — restored source
// TU 344 "LuaArrayLayoutControl.cpp": Mac 0x328320-0x32956b, Win 0x463ef0-0x464a4f.
#include "LuaArrayLayoutControl.h"
#include "LuaBindings.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "ControlManager.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

ArrayLayoutControl* CheckArray(lua_State* L)
{
    return CheckLuaArrayLayoutControl(LuaState::CastState(L), 1);
}

// self:GetColumnMode() -> whether the array is one row. Mac 0x3283f0
int LuaArrayGetColumnMode(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    lua_pushboolean(L, array->IsHorizontal());
    return 1;
}

// self:SetColumnMode([row = true]). Mac 0x328440
int LuaArraySetColumnMode(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    array->SetHorizontal(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:GetAutoSizeMode(). NOTE: pushes the mode as a boolean (both builds). Mac 0x3284a0
int LuaArrayGetAutoSizeMode(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    lua_pushboolean(L, array->GetResizeMode());
    return 1;
}

// self:SetAutoSizeMode([mode = 1]) (0 off, 1 single, 2 both). Mac 0x3284f0, Win 0x4641d0
int LuaArraySetAutoSizeMode(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    UInt32 mode = (UInt32)luaL_optinteger(L, 2, 1);
    if (mode > 2)
        luaL_argerror(L, 2, "invalid auto size mode");
    array->SetResizeMode((ArrayLayoutControl::ResizeMode)mode);
    return 0;
}

// self:GetReverseMode(). Mac 0x328570
int LuaArrayGetReverseMode(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    lua_pushboolean(L, array->IsReversed());
    return 1;
}

// self:SetReverseMode([reverse = true]). Mac 0x3285c0
int LuaArraySetReverseMode(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    array->SetReversed(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:GetSpacing(). Mac 0x328620
int LuaArrayGetSpacing(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    lua_pushnumber(L, array->GetSpacing());
    return 1;
}

// self:SetSpacing([spacing = 0]). Mac 0x328670
int LuaArraySetSpacing(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    array->SetSpacing((Real32)luaL_optnumber(L, 2, 0));
    return 0;
}

// self:GetNumberOfControls(). Mac 0x3286c0
int LuaArrayGetNumberOfControls(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    lua_pushnumber(L, (lua_Number)array->GetControlCount());
    return 1;
}

// self:AddControl(control). Mac 0x328720
int LuaArrayAddControl(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    Control* control = CheckLuaControl(LuaState::CastState(L), 2);
    array->AddControl(control);
    return 0;
}

// self:InsertControl(control, index). Mac 0x328770
int LuaArrayInsertControl(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    Control* control = CheckLuaControl(LuaState::CastState(L), 2);
    UInt32 index = (UInt32)luaL_checkinteger(L, 3);
    array->InsertControl(control, index);
    return 0;
}

// self:RemoveControl(control). Mac 0x3287d0
int LuaArrayRemoveControl(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    Control* control = CheckLuaControl(LuaState::CastState(L), 2);
    array->RemoveControl(control);
    return 0;
}

// self:RemoveControls(). Mac 0x328820
int LuaArrayRemoveControls(lua_State* L)
{
    ArrayLayoutControl* array = CheckArray(L);
    array->RemoveAllControls();
    return 0;
}

// ArrayControl.Create(self, parent). Mac 0x328850
int LuaArrayCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<ArrayLayoutControl> control = new ArrayLayoutControl(manager);
    RefPointer<LuaArrayLayoutControl> object = new LuaArrayLayoutControl;
    LuaObject parent(state, 2);
    LuaObject after(state);
    object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x74bcc0
    { "GetColumnMode", LuaArrayGetColumnMode },
    { "SetColumnMode", LuaArraySetColumnMode },
    { "GetAutoSizeMode", LuaArrayGetAutoSizeMode },
    { "SetAutoSizeMode", LuaArraySetAutoSizeMode },
    { "GetReverseMode", LuaArrayGetReverseMode },
    { "SetReverseMode", LuaArraySetReverseMode },
    { "GetSpacing", LuaArrayGetSpacing },
    { "SetSpacing", LuaArraySetSpacing },
    { "GetNumberOfControls", LuaArrayGetNumberOfControls },
    { "AddControl", LuaArrayAddControl },
    { "InsertControl", LuaArrayInsertControl },
    { "RemoveControl", LuaArrayRemoveControl },
    { "RemoveControls", LuaArrayRemoveControls },
    { 0, 0 }
};

} // namespace

LuaArrayLayoutControl::~LuaArrayLayoutControl()                             // Mac 0x328ac0, Win 0x464670
{
}

// Mac 0x328ee0, Win 0x4646f0
void LuaArrayLayoutControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    {
        ArrayLayoutControl::ResizeMode mode;
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("autoSizeMode"), value) && FromString(mode, value, at))
            m_array->SetResizeMode(mode);
    }
    LuaControl::OnLayoutApplied(layout, args);
    {
        bool row;
        String value;
        if (layout->GetAttribute(String("columnMode"), value) && FromString(row, value))
            m_array->SetHorizontal(row);
    }
    {
        bool reverse;
        String value;
        if (layout->GetAttribute(String("reverseMode"), value) && FromString(reverse, value))
            m_array->SetReversed(reverse);
    }
    {
        Real32 spacing;
        String value;
        if (layout->GetAttribute(String("spacing"), value) && FromString(spacing, value))
            m_array->SetSpacing(spacing);
    }
    // The controls of the <Content> children join the array unless already in it.
    // NOTE: a child that is not a Lua control adds a null control (both builds).
    const XMLDOMNode* content = layout->FindChild(String("Content"));
    if (!content)
        return;
    const XMLDOMNode::NodeArray& children = content->GetChildren();
    for (UInt32 i = 0; i < children.GetSize(); ++i) {
        LuaObject table = CreateChild(children[i].Get(), args);
        if (table.IsNil())
            continue;
        Control* control = 0;
        if (BindableLuaObject* bound = GetBoundObject(table))
            if (LuaControl* object = dynamic_cast<LuaControl*>(bound))
                control = object->GetTargetControl();
        ArrayLayoutControl::ControlIterator it = m_array->BeginControls();
        while (it != m_array->EndControls() && it->Get() != control)
            ++it;
        if (it == m_array->EndControls())
            m_array->AddControl(control);
    }
}

// Mac 0x329300, Win 0x464990
bool LuaArrayLayoutControl::Attach(Control* control, const String& name)
{
    if (!LuaControl::Attach(control, name))
        return false;
    m_array = dynamic_cast<ArrayLayoutControl*>(GetTargetControl());
    return true;
}

void LuaArrayLayoutControl::Detach()                                        // Mac 0x3293e0, Win 0x4649d0
{
    m_array = 0;
    LuaControl::Detach();
}

ArrayLayoutControl* CheckLuaArrayLayoutControl(LuaState* state, int narg)  // Mac 0x329420, Win 0x4649e0
{
    ArrayLayoutControl* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaArrayLayoutControl* lua = bound ? dynamic_cast<LuaArrayLayoutControl*>(bound) : 0;
        if (lua)
            result = lua->GetArray();
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(ArrayLayoutControl).name());
    return result;
}

void RegisterLuaArrayLayoutControl(LuaState* state, ControlManager* manager) // Mac 0x328320, Win 0x4644d0
{
    LuaObject luaClass = CreateLuaClass(state, "ArrayControl", "Control");
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaArrayCreate, 1);
}

} // namespace LEGO
