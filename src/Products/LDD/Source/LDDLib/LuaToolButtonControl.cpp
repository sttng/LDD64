// LEGO Digital Designer — restored source
// TU 281 "LuaToolButtonControl.cpp": Mac 0x293a20-0x294ffb, Win 0x635965-0x635e8f.
#include "LuaToolButtonControl.h"
#include "LuaBindings.h"
#include "LuaTool.h"
#include "ButtonControl.h"
#include "ControlManager.h"
#include "XMLDOMNode.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:GetTool() -> the Tool table of the button's tool (nil without one).
int LuaToolButtonControlGetTool(lua_State* L)                                // Mac 0x294020, Win 0x635c35
{
    LuaState* state = LuaState::CastState(L);
    LuaToolButtonControl* control = CheckBoundObject<LuaToolButtonControl>(state, 1); // (Mac 0x294eb0)
    LuaObject tool = GetObjectMapEntry(state, control->GetTool());
    tool.Push();
    return 1;
}

const luaL_Reg kFunctions[] = {                                             // Mac 0x73fba0
    { "GetTool", LuaToolButtonControlGetTool },
    { 0, 0 }
};

} // namespace

int LuaToolButtonControlCreate(lua_State* L)                                 // Mac 0x293a20, Win 0x635c88
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = static_cast<ControlManager*>(lua_touserdata(L, lua_upvalueindex(1)));
    BrickConnectSceneManager* sceneManager =
        static_cast<BrickConnectSceneManager*>(lua_touserdata(L, lua_upvalueindex(2)));
    RefPointer<ButtonControl> button(new ButtonControl(manager));
    button->Initialize();
    RefPointer<LuaToolButtonControl> object(new LuaToolButtonControl);
    {
        LuaObject parent(state, 2);
        LuaObject after(state);
        object->Initialize(self, parent, after, button.Get(), 7, Rect(0, 0, 0, 0), 0, false);
        object->BrickConnectSceneManagerObserver::Attach(sceneManager);
    }
    return 0;
}

LuaToolButtonControl::~LuaToolButtonControl()                                // Mac 0x294180 (D2 0x2940b0)
{
    if (m_tool)
        m_tool->Release();
}

void LuaToolButtonControl::UpdateButtonStates(BrickConnectSceneControlTool* active) // (inline), Win 0x635991
{
    if (m_tool == active) {
        GetButton()->SetStateMapping(0, 2);
        GetButton()->SetStateMapping(1, 2);
    } else {
        GetButton()->SetStateMapping(0, 0);
        GetButton()->SetStateMapping(1, 1);
    }
}

void LuaToolButtonControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x294970, Win 0x635a3a
{
    LuaButtonControl::OnLayoutApplied(layout, args);
    DefineLayoutFunction(layout, "OnToolChanged", "oldTool, newTool", args);
    String toolPath;
    if (!layout->GetAttribute(String("tool"), toolPath))
        return;
    if (m_tool) {
        BrickConnectSceneControlTool* old = m_tool;
        m_tool = 0;
        old->Release();
    }
    if (!toolPath.IsEmpty()) {
        LuaObject table = LookupLuaObject(*args.m_table, toolPath);
        if (!table.IsNil()) {
            BindableLuaObject* bound = GetBoundObject(table);
            LuaTool* luaTool = bound ? dynamic_cast<LuaTool*>(bound) : 0;
            BrickConnectSceneControlTool* tool = luaTool ? luaTool->GetAdaptedObject() : 0;
            BrickConnectSceneControlTool* old = m_tool;
            if (old != tool) {
                m_tool = tool;
                if (tool)
                    tool->AddRef();
                if (old)
                    old->Release();
            }
        }
    }
    // NOTE: neither the button nor the tool is checked here (unlike OnAttached; both builds).
    if (BrickConnectSceneManager* manager = BrickConnectSceneManagerObserver::GetSubject())
        UpdateButtonStates(manager->GetActiveTool());
}

void LuaToolButtonControl::OnFinalRelease()                                  // Mac 0x294740
{
    BrickConnectSceneManagerObserver::OnFinalRelease();                     // (Detach, Object's)
    LuaButtonControl::OnFinalRelease();
}

void LuaToolButtonControl::OnMouseClicked(Control* control, const Vector2& position, UInt32 button) // Mac 0x294bd0, Win 0x635965
{
    LuaButtonControl::OnMouseClicked(control, position, button);
    BrickConnectSceneManager* manager = BrickConnectSceneManagerObserver::GetSubject();
    if (m_tool && manager)
        manager->SetActiveTool(m_tool);
}

void LuaToolButtonControl::OnAttached(BrickConnectSceneManager* manager)     // Mac 0x294c50, Win 0x635b2a
{
    if (GetButton() && m_tool)
        UpdateButtonStates(manager->GetActiveTool());
}

void LuaToolButtonControl::OnActiveToolChanged(BrickConnectSceneControlTool* previous,
                                               BrickConnectSceneControlTool* current) // Mac 0x294d10, Win 0x635b48
{
    if (GetButton() && m_tool)
        UpdateButtonStates(current);
    LuaObject previousTable = GetObjectMapEntry(m_luaState, previous);
    LuaObject currentTable = GetObjectMapEntry(m_luaState, current);
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnToolChanged", false, 0,
                  previousTable, currentTable);                            // (Mac 0x2929a0)
}

void RegisterLuaToolButtonControl(LuaState* state, ControlManager* controlManager,
                                  BrickConnectSceneManager* sceneManager)   // Mac 0x293f50, Win 0x635df1
{
    LuaObject luaClass = CreateLuaClass(state, "ToolButtonControl", "ButtonControl");
    luaClass.Register(kFunctions, 0);
    lua_pushlightuserdata(state->GetCState(), controlManager);
    lua_pushlightuserdata(state->GetCState(), sceneManager);
    luaClass.Register("Create", LuaToolButtonControlCreate, 2);
}

} // namespace LEGO
