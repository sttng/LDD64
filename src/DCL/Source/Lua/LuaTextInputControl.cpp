// LEGO Digital Designer — restored source
// TU 390 "LuaTextInputControl.cpp": Mac 0x37ad70-0x37c54b, Win 0x45de70-0x45ed0f.
#include "LuaTextInputControl.h"
#include "LuaBindings.h"
#include "LayoutResources.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "Color.h"
#include "Font.h"
#include "ControlManager.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:SetText(text). Mac 0x37ae70
int LuaTextInputSetText(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    TextInputControl* control = CheckLuaTextInputControl(state, 1);
    control->SetText(CheckLuaString(state, 2));
    return 0;
}

// self:GetText(). Mac 0x37aef0
int LuaTextInputGetText(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    TextInputControl* control = CheckLuaTextInputControl(state, 1);
    PushLuaString(state, control->GetText());
    return 1;
}

// self:ResizeHeightToFitLines(lines). Mac 0x37af70
int LuaTextInputResizeHeightToFitLines(lua_State* L)
{
    TextInputControl* control = CheckLuaTextInputControl(LuaState::CastState(L), 1);
    control->SetLineCount((UInt32)luaL_checkinteger(L, 2));
    return 0;
}

// self:SetFont(name): the font `name` of the fonts layout (upvalue 1); an argument error
// "<name> does not specify a valid font description" without one. Mac 0x37afc0, Win 0x45e1e0
int LuaTextInputSetFont(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    TextInputControl* control = CheckLuaTextInputControl(state, 1);
    const XMLDOMNode* fonts = (const XMLDOMNode*)lua_touserdata(L, lua_upvalueindex(1));
    // (the argument error longjmps: the message is pushed first so no String is alive then; a null
    // RefPointer needs no dtor)
    RefPointer<Font> font;
    {
        String name = CheckLuaString(state, 2);
        LoadLayoutFont(name, fonts, font, control->GetManager());
        if (!font) {
            name << " does not specify a valid font description";
            AString message = name.ToUTF8();
            lua_pushstring(L, message.c_str() ? message.c_str() : "");
        }
    }
    if (!font)
        luaL_argerror(L, 2, lua_tostring(L, -1));
    control->SetFont(font.Get());
    return 0;
}

// TextInputControl.Create(self, parent): the control is bound only when its native field could
// be created (TextInputControl::Create). Mac 0x37b170
int LuaTextInputCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<TextInputControl> control = new TextInputControl(manager);
    if (control->Create()) {
        RefPointer<LuaTextInputControl> object = new LuaTextInputControl;
        LuaObject parent(state, 2);
        LuaObject after(state);
        object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    }
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x753674
    { "SetText", LuaTextInputSetText },
    { "GetText", LuaTextInputGetText },
    { "ResizeHeightToFitLines", LuaTextInputResizeHeightToFitLines },
    { 0, 0 }
};

} // namespace

LuaTextInputControl::~LuaTextInputControl()                                 // Mac 0x37b420, Win 0x45e440
{
}

// Mac 0x37b8c0
void LuaTextInputControl::OnFinalRelease()
{
    TextInputControlObserver::OnFinalRelease();
    LuaControl::OnFinalRelease();
}

// Mac 0x37bb00, Win 0x45e520
void LuaTextInputControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    LuaControl::OnLayoutApplied(layout, args);
    RefPointer<Font> font;
    if (ParseLayoutFont(layout, args.m_root, font, GetTargetControl()->GetManager()) && font)
        m_textInput->SetFont(font.Get());
    String text;
    if (layout->GetAttribute(String("text"), text))
        m_textInput->SetText(text);
    {
        UInt32 lines;
        String value;
        if (layout->GetAttribute(String("resizeHeightToFitLines"), value) && FromString(lines, value))
            m_textInput->SetLineCount(lines);
    }
    {
        TextInputControl::NumberType type;
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("mode"), value) && FromString(type, value, at))
            m_textInput->SetNumberType(type);
    }
    {
        Color color;
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("disabledColor"), value) && FromString(color, value, at))
            m_textInput->SetDisabledColor(color);
    }
    DefineLayoutFunction(layout, "OnTextEntered", "sender", args);
    DefineLayoutFunction(layout, "OnTextChanged", "sender", args);
}

// NOTE: the text input is observed without a null check (both builds). Mac 0x37beb0, Win 0x45e770
bool LuaTextInputControl::Attach(Control* control, const String& name)
{
    if (!LuaControl::Attach(control, name))
        return false;
    m_textInput = dynamic_cast<TextInputControl*>(GetTargetControl());
    TextInputControlObserver::Attach(m_textInput);
    return true;
}

void LuaTextInputControl::Detach()                                          // Mac 0x37bfb0, Win 0x45e7b0
{
    TextInputControlObserver::Detach(m_textInput);
    m_textInput = 0;
    LuaControl::Detach();
}

// self:OnTextEntered(self). Mac 0x37c020, Win 0x45e7e0
void LuaTextInputControl::OnTextEntered(TextInputControl* control)
{
    if (m_textInput != control)
        return;
    LuaObject sender = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
    LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "OnTextEntered", false, 0, sender);
}

// self:OnTextChanged(self). Mac 0x37c1e0, Win 0x45e860
void LuaTextInputControl::OnTextChanged(TextInputControl* control)
{
    if (m_textInput != control)
        return;
    LuaObject sender = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
    LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "OnTextChanged", false, 0, sender);
}

TextInputControl* CheckLuaTextInputControl(LuaState* state, int narg)      // Mac 0x37c300, Win 0x45e940
{
    TextInputControl* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaTextInputControl* lua = bound ? dynamic_cast<LuaTextInputControl*>(bound) : 0;
        if (lua)
            result = lua->GetTextInput();
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(TextInputControl).name());
    return result;
}

void RegisterLuaTextInputControl(LuaState* state, ControlManager* manager,
                                 const XMLDOMNode* fonts)                   // Mac 0x37ad70, Win 0x45e330
{
    LuaObject luaClass = CreateLuaClass(state, "TextInputControl", "Control");
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), (void*)fonts);
    luaClass.Register("SetFont", LuaTextInputSetFont, 1);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaTextInputCreate, 1);
}

} // namespace LEGO
