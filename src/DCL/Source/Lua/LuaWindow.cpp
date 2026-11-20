// LEGO Digital Designer — restored source
// TU 377 "LuaWindow.cpp": Mac 0x366b40-0x368a6f, Win 0x447000-0x44853f.
#include "LuaWindow.h"
#include "LuaBindings.h"
#include "AdapterBindableLuaObject.h"
#include "XMLDOMNode.h"
#include "BasicString.h"
#include "LEGO/StdLib.h"
#include "TextLocale.h"
#include "Application.h"
#include "Command.h"
#include "Menu.h"
#include "Key.h"
#include "KeyNames.h"

using namespace LuaPlus;

#if defined(LDD_LINUX)
extern bool g_linuxMenuAccessAsWindows;   // LuaPreferences.cpp
#endif

namespace LEGO {

namespace {

// The Command of a Lua table: the object of a Command adapter (LuaCommandAdapter), else a bound
// object that is itself a Command (LuaCommand). Win 0x446fc0 (inlined on Mac). (name inferred)
Command* GetLuaCommand(LuaObject& object)
{
    BindableLuaObject* bound = GetBoundObject(object);
    if (!bound)
        return 0;
    if (AdapterBindableLuaObjectBase<Command, BindableLuaObject>* adapter =
            dynamic_cast<AdapterBindableLuaObjectBase<Command, BindableLuaObject>*>(bound))
        return adapter->GetAdaptedObject();
    return dynamic_cast<Command*>(bound);
}

// `command` = the command at the Lua path of attribute `name` (from `table`, else the globals);
// untouched without the attribute (or an empty one). Mac 0x368530 (state in ECX), Win 0x447600
void GetMenuCommand(LuaState* state, const XMLDOMNode* node, const char* name,
                    RefPointer<Command>& command, LuaObject* table)
{
    String path;
    if (!node->GetAttribute(String(name), path) || path.IsEmpty())
        return;
    if (!table) {
        LuaObject globals = state->GetGlobals();
        LuaObject object = LookupLuaObject(globals, path);
        command = GetLuaCommand(object);
    } else {
        LuaObject object = LookupLuaObject(*table, path);
        command = GetLuaCommand(object);
    }
}

// Splits `text` at every character of `separators` (empty pieces included, a trailing empty
// piece dropped). Mac 0x3687e0, Win 0x448540. (name inferred)
void SplitAString(const AString& text, const AString& separators, BasicArray<AString>& out)
{
    out.Clear();
    UInt32 length = text.GetLength();
    UInt32 start = 0;
    const char* chars = text.GetBuffer();
    for (UInt32 i = 0; i < length; ++i) {
        bool separator = false;
        for (UInt32 j = 0; j < separators.GetLength(); ++j)
            if (separators.GetBuffer()[j] == chars[i])
                separator = true;
        if (separator) {
            out.PushBack(AString(chars + start, i - start));
            start = i + 1;
        }
    }
    if (start < length)
        out.PushBack(AString(chars + start, length - start));
}

// key="Ctrl, Shift, X": Control / Shift / Alt set modifier bits 1 / 2 / 4, any other name is
// the key. Win 0x447780 (inlined in Mac 0x3670f0)
void ParseMenuKey(const XMLDOMNode* node, UInt32& key, UInt32& modifiers)
{
    String text;
    if (!node->GetAttribute(String("key"), text) || text.IsEmpty())
        return;
    BasicArray<AString> names;
    SplitAString(text.ToUTF8(), AString(" ,"), names);
    for (UInt32 i = 0; i < names.GetSize(); ++i) {
        UInt32 k = StringToKey::GetKey(names[i].GetBuffer());
        if (k == 0)
            continue;
        if (k == kKeyControl)
            modifiers |= kModifierControl;
        else if (k == kKeyShift)
            modifiers |= kModifierShift;
        else if (k == kKeyAlt)
            modifiers |= kModifierAlt;
        else
            key = k;
    }
}

// <Access script="…">: the chunk called with the table returns true. (part of Mac 0x3670f0)
bool CheckMenuAccess(LuaState* state, const XMLDOMNode* node, LuaObject* table)
{
    String script;
    if (!node->GetAttribute(String("script"), script))
        script = node->GetText();
    if (script.IsEmpty())
        return false;
    AString chunk = script.ToUTF8();
    lua_State* L = state->GetCState();
    bool result = false;
    // NOTE: deliberate deviation (improvement): both originals pass the UTF-16 length of `script`
    // (Mac String::GetLength 0x288d60 at 0x36745a), which cuts a non-ASCII script short so its
    // menu items stay hidden; the UTF-8 byte count loads the whole script.
    if (state->LoadBuffer(chunk.GetBuffer(), chunk.GetLength(), chunk.GetBuffer())) {
        if (table)
            table->Push();
        else
            lua_pushnil(L);
#if defined(LDD_LINUX)
        // LDD64 Linux: the menus are the Windows build's (base.GetPlatformShortName() is "WIN" here)
        ::g_linuxMenuAccessAsWindows = true;
        bool called = state->PCall(1, 1);
        ::g_linuxMenuAccessAsWindows = false;
        if (called) {
#else
        if (state->PCall(1, 1)) {
#endif
            result = LuaToBoolean(state, -1);
            lua_pop(L, 1);
        }
    }
    return result;
}

// ---- Lua functions of the class "Window" -------------------------------------------------------

// self:ModalRun([func]): Application::RunModal(window, func ? reciever : null). Mac 0x366e00, Win 0x447340
int LuaWindowModalRun(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    Window* window = CheckLuaWindow(state, 1);
    RefPointer<LuaModalUpdateReciever> reciever;
    {
        LuaObject function(state, 2);
        if (function.IsFunction())
            reciever = new LuaModalUpdateReciever(function);
    }
    Application::GetInstance()->RunModal(window, reciever.Get());
    return 0;
}

// self:ModalStop(). Mac 0x366f40, Win 0x447440
int LuaWindowModalStop(lua_State* L)
{
    CheckLuaWindow(LuaState::CastState(L), 1);
    Application::GetInstance()->StopModal();
    return 0;
}

// self:SetModal([modal = true]). Mac 0x366f80, Win 0x447470
int LuaWindowSetModal(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    Window* window = CheckLuaWindow(state, 1);
    window->SetModalState(OptLuaBoolean(state, 2, true));
    return 0;
}

// self:IsModal(). Mac 0x366fe0, Win 0x4474b0
int LuaWindowIsModal(lua_State* L)
{
    Window* window = CheckLuaWindow(LuaState::CastState(L), 1);
    lua_pushboolean(L, window->IsModal());
    return 1;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x750540
    { "ModalRun", LuaWindowModalRun },
    { "ModalStop", LuaWindowModalStop },
    { "SetModal", LuaWindowSetModal },
    { "IsModal", LuaWindowIsModal },
    { 0, 0 }
};

} // namespace

// ================================================================================================

LuaWindow::LuaWindow(Window* window)                                        // Mac 0x366c80, Win 0x448200
    : AdapterBindableLuaObjectBase<Window, LuaLayout>(window)
{
}

LuaWindow::~LuaWindow()                                                     // Mac 0x368020, Win 0x447000
{
}

void LuaWindow::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x367da0, Win 0x448290
{
    if (const XMLDOMNode* menuNode = layout->FindChild(String("Menu"))) {
        RefPointer<Menu> menu;
        if (CreateLuaMenu(args.m_table->GetState(), menuNode, menu, args.m_table))
            m_object->SetMenu(menu.Get());
    }
    LuaLayout::OnLayoutApplied(layout, args);
}

LuaModalUpdateReciever::~LuaModalUpdateReciever()                         // Mac 0x368230, Win 0x447280
{
}

void LuaModalUpdateReciever::ModalUpdate()                                  // Mac 0x3682e0, Win 0x4472f0
{
    m_function.Push();
    m_function.GetState()->PCall(0, 0);
}

LuaObject CreateLuaWindow(Window* window, LuaState* state)                 // Mac 0x366b40, Win 0x447090
{
    {
        LuaObject existing = GetObjectMapEntry(state, window);
        if (!existing.IsNil())
            return LuaObject(state);
    }
    LuaObject table = CreateLuaInstance(state, "Window");
    RefPointer<LuaWindow> object = new LuaWindow(window);
    AttachLuaObject(table, object.Get());
    return table;
}

Window* CheckLuaWindow(LuaState* state, int narg)                           // Mac 0x368160, Win 0x448890
{
    Window* result;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        result = bound ? dynamic_cast<Window*>(bound) : 0;
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(Window).name());
    return result;
}

bool BuildLuaMenu(LuaState* state, const XMLDOMNode* node, Menu* menu, LuaObject* table) // Mac 0x3670f0, Win 0x447930
{
    const XMLDOMNode::NodeArray& children = node->GetChildren();
    for (UInt32 i = 0; i < children.GetSize(); ++i) {
        const XMLDOMNode* child = children[i].Get();
        if (child->GetName() == String("MenuItem")) {
            String text;
            bool hasText = child->GetAttribute(String("text"), text);
            if (!hasText) {
                String key;
                if (child->GetAttribute(String("locText"), key)) {
                    text = LocalizedString(key);
                    hasText = true;
                }
            }
            if (!hasText) {
                menu->AddItem(new MenuItem);                                  // separator
            } else if (child->GetChildren().IsEmpty()) {
                RefPointer<Command> command;
                RefPointer<Command> keyCommand;
                GetMenuCommand(state, child, "command", command, table);
                GetMenuCommand(state, child, "keyCommand", keyCommand, table);
                UInt32 key = 0, modifiers = 0;
                ParseMenuKey(child, key, modifiers);
                if (key == 0 || !command)
                    menu->AddItem(new MenuItem(text, command.Get()));
                else
                    menu->AddItem(new MenuItem(text, command.Get(), key, modifiers, keyCommand.Get()));
            } else {
                RefPointer<Menu> submenu = new Menu;
                if (!BuildLuaMenu(state, child, submenu.Get(), table))
                    return false;
                menu->AddItem(new MenuItem(text, submenu.Get()));
            }
        } else if (child->GetName() == String("Access")) {
            if (CheckMenuAccess(state, child, table) && !BuildLuaMenu(state, child, menu, table))
                return false;
        }
    }
    return true;
}

bool CreateLuaMenu(LuaState* state, const XMLDOMNode* node, RefPointer<Menu>& menu,
                   LuaObject* table)                                        // Mac 0x367040, Win 0x448110
{
    menu = new Menu;
    if (!BuildLuaMenu(state, node, menu.Get(), table)) {
        menu = 0;
        return false;
    }
    return true;
}

void RegisterLuaWindow(LuaState* state)                                     // Mac 0x366d30, Win 0x447500
{
    LuaObject luaClass = CreateLuaClass(state, "Window", "Layout");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
