// LEGO Digital Designer — restored source
// TU 320 "LuaConsole.cpp": Mac 0x2c98c0-0x2cb14b, Win 0x456630-0x4573bf.
#include "LuaConsole.h"
#include "LuaBindings.h"
#include "ControlManager.h"
#include "SliderAugmentation.h"
#include "ObjectFactory.h"
#include "Font.h"
#include "Color.h"

#include <typeinfo>

using namespace LuaPlus;

namespace LEGO {

namespace {

// _ERRORREPORTER while a console line runs: the message goes to the console (upvalue).
// Mac 0x2ca8b0, Win 0x456dc0
int LuaConsoleErrorReporter(lua_State* L)
{
    LuaConsole* console = (LuaConsole*)lua_touserdata(L, lua_upvalueindex(1));
    console->AddOutputLine(String(lua_tostring(L, 1)));
    return 0;
}

} // namespace

LuaConsole::LuaConsole(ControlManager* manager, ContainerControl* container, LuaState* state) // Mac 0x2c98c0
    : BufferedAbleRenderControl(manager), m_container(container), m_state(state)
{
}

LuaConsole::~LuaConsole()                                                   // Mac 0x2caab0, Win 0x456630
{
}

bool LuaConsole::Initialize()                                               // Mac 0x2c9af0, Win 0x4568d0
{
    Font::SystemTypeFontArgs args;
    args.type = Font::kSystemFont;
    args.size = 15;
    args.color = Color::Black();
    Object* created = GetManager()->GetObjectFactory()->CreateObject(typeid(Font), args);
    Font* font = created ? dynamic_cast<Font*>(created) : 0;
    if (!font)
        return false;
    RefPointer<Font> fontReference(font);

    m_input = new TextInputControl(GetManager());
    m_input->Create();
    m_input->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_input->SetFont(font);
    m_input->SetText(String());
    m_input->SetLineCount(1);
    m_input->SetAxisMargins(0, 0.0f, 0.0f, 0);
    m_input->SetAxisAlignment(1, 3, 3, 0.0f, 0);
    m_input->SetEnabled(true);
    m_input->SetShown(true);
    TextInputControlObserver::Attach(m_input.Get());
    ControlObserver::Attach(m_input.Get());

    m_output = new TextOutputControl(GetManager());
    m_output->Initialize();
    m_output->Setup(0, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_output->SetFont(font);
    m_output->SetAxisMargins(0, 0.0f, 0.0f, 0);

    Real32 inputHeight = m_input->GetSize().y;
    SetMinimumSize(Vector2(10.0f, inputHeight + inputHeight));

    m_container->SetContent(m_output.Get());
    m_container->SetScrollAlways(true, false);
    m_container->SetAxisMargins(0, 0.0f, 0.0f, 0);
    m_container->SetAxisAlignments(1, 1, 1, 0.0f, 0, 3, 1, 0.0f, m_input.Get());
    m_container->SetParent(this, 0);

    SetShown(true);
    SetEnabled(true);
    SetActive(true);
    return true;
}

void LuaConsole::AddOutputLine(const String& text)                          // Win 0x456d50
{
    m_output->AddLine(text);
    m_container->GetSlider(true)->SetValue((Real64)m_output->GetSize().y);
}

void LuaConsole::OnFinalRelease()                                           // Mac 0x2ca060, Win 0x456da0
{
    ControlObserver::OnFinalRelease();
    TextInputControlObserver::OnFinalRelease();
    BufferedAbleRenderControl::OnFinalRelease();
}

void LuaConsole::OnShown(Control* control)                                  // Mac 0x2ca180, Win 0x457120
{
    if (m_input.Get() == control)
        control->SetKeyboardFocus(true);
}

void LuaConsole::OnTextEntered(TextInputControl* control)                   // Mac 0x2ca1e0, Win 0x457150
{
    String text = control->GetText();
    control->SetText(String(""));
    if (text.IsEmpty())
        return;
    AddOutputLine(String(">") << text);

    LuaObject globals = m_state->GetGlobals();
    if (text.GetAt(0) == '/') {
        // "/p expr" (or "/p"): print the value of a Lua path.
        UInt32 length = text.GetLength();
        if ((length > 2 && text.GetAt(1) == 'p' && text.GetAt(2) == ' ') ||
            (length == 2 && text.GetAt(1) == 'p')) {
            String path(text);
            path.Erase(0, 3);
            path.Trim();
            LuaObject object = LookupLuaObject(globals, path);
            String out;
            DumpLuaValue(object, out, true);
            AddOutputLine(out);
        }
    } else {
        LuaObject reporter = globals.RawGetByName("_ERRORREPORTER");
        lua_pushlightuserdata(m_state->GetCState(), this);
        globals.Register("_ERRORREPORTER", LuaConsoleErrorReporter, 1);
        if (m_state->LoadWBuffer((const lua_WChar*)text.GetBuffer(), text.GetLength(), 0))
            m_state->PCall(0, 0);
        globals.SetObject("_ERRORREPORTER", reporter);                    // Mac 0x2ca970 (LuaObject::Set)
    }
}

void DumpLuaValue(LuaObject& object, String& out, bool fields)              // Mac 0x2ca5e0, Win 0x456e80
{
    switch (object.Type()) {
    case LUA_TNIL:
        out.Append("nil");
        break;
    case LUA_TBOOLEAN:
        out.Append(object.GetBoolean() ? "true" : "false");
        break;
    case LUA_TLIGHTUSERDATA:
        out.Append("[Light user data]");
        break;
    case LUA_TNUMBER: {
        String number;
        number << (Real64)object.GetNumber();
        out.Append(number);
        break;
    }
    case LUA_TSTRING:
    case LUA_TWSTRING:
        out.Append(1, '"');
        out.Append(LuaToString(object));
        out.Append(1, '"');
        break;
    case LUA_TTABLE: {
        out.Append("[Table]");
        if (BindableLuaObject* bound = GetBoundObject(object)) {
            out.Append(" - bound to [");
            out.Append(typeid(*bound).name());
            out.Append("]");
        }
        if (!fields)
            return;
        for (LuaTableIterator it(object, true); it.IsValid(); it.Next()) {
            out.Append("\r\n");
            out.Append(2, ' ');
            DumpLuaValue(it.GetKey(), out, false);
            out.Append(" = ");
            DumpLuaValue(it.GetValue(), out, false);
        }
        break;
    }
    case LUA_TFUNCTION:
        out.Append(object.IsCFunction() ? "[C Function]" : "[Lua Function]");
        break;
    case LUA_TUSERDATA:
        out.Append("[User data]");
        break;
    case LUA_TTHREAD:
        out.Append("[Thread]");
        break;
    default:
        break;
    }
}

} // namespace LEGO
