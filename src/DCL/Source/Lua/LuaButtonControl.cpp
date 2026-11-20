// LEGO Digital Designer — restored source
// TU 314 "LuaButtonControl.cpp": Mac 0x2bb7a0-0x2bda2b, Win 0x4661c0-0x467caf.
#include "LuaButtonControl.h"
#include "LuaBindings.h"
#include "LuaCommandAdapter.h"
#include "LayoutResources.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "TextLocale.h"
#include "Color.h"
#include "Font.h"
#include "Texture.h"
#include "ButtonControl.h"
#include "BufferedAbleRenderControl.h"
#include "RenderControl.h"
#include "ControlManager.h"

using namespace LuaPlus;

namespace LEGO {

// ButtonControl.Create(self, parent). Mac 0x2bb8f0, Win 0x466a40
int LuaButtonControlCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<ButtonControl> control = new ButtonControl(manager);
    control->Initialize();
    RefPointer<LuaButtonControl> object = new LuaButtonControl;
    LuaObject parent(state, 2);
    LuaObject after(state);
    object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    return 0;
}

LuaButtonControl::LuaButtonControl()                                        // Mac 0x2bb7a0, Win 0x466920
    : m_button(0)
{
}

LuaButtonControl::~LuaButtonControl()                                       // Mac 0x2bbb80 / 0x2bbbe0, Win 0x4661c0
{
}

// Mac 0x2bc360 (thunks 0x2bc3b0 / 0x2bc410 / 0x2bc470 / 0x2bc4d0)
void LuaButtonControl::OnFinalRelease()
{
    CommandObserver::OnFinalRelease();
    LuaControl::OnFinalRelease();
}

// Mac 0x2bc530, Win 0x466e10
void LuaButtonControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    LuaControl::OnLayoutApplied(layout, args);
    String command;
    if (layout->GetAttribute(String("command"), command))
        ConnectLuaCommandObserver(*args.m_table, command, static_cast<CommandObserver*>(this));
    ApplyStateLayout(0, String("Up"), layout, args);
    ApplyStateLayout(1, String("Over"), layout, args);
    ApplyStateLayout(2, String("Down"), layout, args);
    ApplyStateLayout(3, String("Disabled"), layout, args);
}

// Mac 0x2bc740, Win 0x4677a0
void LuaButtonControl::ApplyStateLayout(UInt32 index, const String& name, const XMLDOMNode* layout,
                                        LuaLayoutArgs& args)
{
    const XMLDOMNode* node = layout->FindChild(name);
    if (!node)
        return;
    ButtonControl::StateDescription description(m_button->GetStateDescription(index));
    {
        String value;
        if (node->GetAttribute(String("blendColor"), value)) {
            UInt32 at = 0;
            FromStringOptional(description.m_color, value, at);
        }
    }
    RefPointer<Texture> image;
    Rect imageRect(0.0f, 0.0f, 0.0f, 0.0f);
    bool hasImage = false, hasRect = false;
    ParseLayoutImage(node, image, imageRect, hasImage, hasRect, m_button->GetManager());
    if (hasImage) {
        description.m_image = image;
        description.m_imageRect = imageRect;
    } else if (hasRect) {
        description.m_imageRect = imageRect;
    }
    node->GetAttribute(String("text"), description.m_text);
    if (node->GetAttribute(String("locText"), description.m_text))
        description.m_text = LocalizedString(description.m_text);
    RefPointer<Font> font;
    if (ParseLayoutFont(node, args.m_root, font, m_button->GetManager()))
        description.m_font = font;
    m_button->SetStateDescription(index, description);
}

// The button's state table is copied on first use and set back if it was copied.
// NOTE: blendColor (any non-empty value) copies the colour the control already has (from
// LuaControl::ApplyColorAttributes); the parsed value is not used. Mac 0x2bcbf0, Win 0x466fb0
void LuaButtonControl::ApplyRenderAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    LuaControl::ApplyRenderAttributes(layout, args);
    BasicArray<ButtonControl::StateDescription> states;
    {
        String value;
        if (layout->GetAttribute(String("cursor"), value)) {
            if (states.IsEmpty())
                m_button->GetStateDescriptions(states);
            UInt32 shape = GetTargetControl()->GetCursor();
            UInt32 at = 0;
            if (ParseCursorShapeOptional(shape, value, at)) {
                for (UInt32 i = 0; i < states.GetSize(); ++i)
                    states[i].m_cursor = shape;
            }
        }
    }
    {
        String color;
        bool found;
        {
            String value;
            UInt32 at = 0;
            found = layout->GetAttribute(String("blendColor"), value) && FromStringOptional(color, value, at);
        }
        if (found) {
            if (states.IsEmpty())
                m_button->GetStateDescriptions(states);
            Color current = GetBufferedControl()->GetColor();
            for (UInt32 i = 0; i < states.GetSize(); ++i)
                states[i].m_color = current;
        }
    }
    String text;
    if (layout->GetAttribute(String("text"), text)) {
        if (states.IsEmpty())
            m_button->GetStateDescriptions(states);
        for (UInt32 i = 0; i < states.GetSize(); ++i)
            states[i].m_text = text;
    }
    if (layout->GetAttribute(String("locText"), text)) {
        if (states.IsEmpty())
            m_button->GetStateDescriptions(states);
        text = LocalizedString(text);
        for (UInt32 i = 0; i < states.GetSize(); ++i)
            states[i].m_text = text;
    }
    {
        String value;
        if (layout->GetAttribute(String("font"), value)) {
            if (states.IsEmpty())
                m_button->GetStateDescriptions(states);
            RefPointer<Font> font;
            GetRenderControl()->GetFont(font);
            for (UInt32 i = 0; i < states.GetSize(); ++i)
                states[i].m_font = font;
        }
    }
    {
        String value;
        if (layout->GetAttribute(String("image"), value)) {
            if (states.IsEmpty())
                m_button->GetStateDescriptions(states);
            RefPointer<Texture> image;
            Rect imageRect(0.0f, 0.0f, 0.0f, 0.0f);
            GetRenderControl()->GetImage(image, imageRect);
            for (UInt32 i = 0; i < states.GetSize(); ++i) {
                states[i].m_image = image;
                states[i].m_imageRect = imageRect;
            }
        }
    }
    if (!states.IsEmpty())
        m_button->SetStateDescriptions(states);
}

// Mac 0x2bd600, Win 0x467bd0
bool LuaButtonControl::Attach(Control* control, const String& name)
{
    if (!LuaControl::Attach(control, name))
        return false;
    m_button = dynamic_cast<ButtonControl*>(GetTargetControl());
    if (Command* command = GetCommand())
        m_button->SetEnabled(command->GetEnable());
    return true;
}

void LuaButtonControl::Detach()                                             // Mac 0x2bd740, Win 0x467c40
{
    m_button = 0;
    LuaControl::Detach();
}

// Mac 0x2bd780, Win 0x467c50
void LuaButtonControl::OnMouseClicked(Control* control, const Vector2& position, UInt32 button)
{
    LuaControl::OnMouseClicked(control, position, button);
    if (button == 0 && GetTargetControl() == control) {
        if (Command* command = GetCommand())
            command->Do();
    }
}

void LuaButtonControl::OnAttached(Command* command)                         // Mac 0x2bd820, Win 0x467cb0
{
    if (m_button)
        m_button->SetEnabled(command->GetEnable());
}

void LuaButtonControl::OnEnableChanged(bool enabled)                        // Mac 0x2bd8a0, Win 0x467d00
{
    if (m_button)
        m_button->SetEnabled(enabled);
}

// Header inline in the original (Mac 0x38c990 copy in TU 404, Win 0x64c197).
ButtonControl* CheckLuaButtonControl(LuaState* state, int narg)
{
    ButtonControl* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaButtonControl* lua = bound ? dynamic_cast<LuaButtonControl*>(bound) : 0;
        if (lua)
            result = lua->GetButton();
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(ButtonControl).name());
    return result;
}

void RegisterLuaButtonControl(LuaState* state, ControlManager* manager)     // Mac 0x2bb820, Win 0x466c50
{
    LuaObject luaClass = CreateLuaClass(state, "ButtonControl", "Control");
    static const luaL_Reg kFunctions[] = { { 0, 0 } };                     // Mac 0x63bc70 (empty)
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaButtonControlCreate, 1);
}

} // namespace LEGO
