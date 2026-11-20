// LEGO Digital Designer — restored source
// LEGO::ButtonControl, LEGO::NonTransparentSelectionMask — TU 028 "ButtonControl.cpp"
// (Mac 0xeb1e0-0xee96b, Win 0x516111-0x516b71).
//
// Not restored here (other classes' inline / template bodies the TU emits): RenderControl's
// dtor, OnFinalRelease and observer overriders (Mac 0xec2b0-0xec730, RenderControl.cpp), the
// TextLayoutObserver / TextureObserver / Observer<> dtors (0xec880-0xecde0), the
// BasicArray<StateDescription> template members (0xeb290, 0xed3c0-0xedb40, 0xede60 insert,
// 0xee5b0 assign) and the construction-vtable helpers.
#include "ButtonControl.h"
#include "ControlManager.h"
#include "ImageLoader.h"
#include "Font.h"

namespace LEGO {

// ===========================================================================================
// NonTransparentSelectionMask
// ===========================================================================================

// Mac 0xed2c0, Win 0x516111
Vector2 NonTransparentSelectionMask::GetSize() const
{
    return Vector2();
}

// Mac 0xed2e0, Win 0x51611f
bool NonTransparentSelectionMask::Contains(const Vector2& /*point*/) const
{
    return true;
}

// ===========================================================================================
// ButtonControl::StateDescription
// ===========================================================================================

// Mac 0xed1e0, Win 0x5161ec
ButtonControl::StateDescription::StateDescription()
    : m_image(0),
      m_imageRect(0.0f, 0.0f, 0.0f, 0.0f),
      m_color(1.0f, 1.0f, 1.0f, 1.0f),
      m_transparent(true),
      m_font(0),
      m_cursor(1)
{
}

ButtonControl::StateDescription::StateDescription(const StateDescription& other)
    : m_image(other.m_image),
      m_imageRect(other.m_imageRect),
      m_color(other.m_color),
      m_transparent(other.m_transparent),
      m_font(other.m_font),
      m_text(other.m_text),
      m_cursor(other.m_cursor)
{
}

// Member-wise (inlined in Mac 0xebc50; Win 0x516588).
ButtonControl::StateDescription& ButtonControl::StateDescription::operator=(const StateDescription& other)
{
    m_image = other.m_image;
    m_imageRect = other.m_imageRect;
    m_color = other.m_color;
    m_transparent = other.m_transparent;
    m_font = other.m_font;
    m_text = other.m_text;
    m_cursor = other.m_cursor;
    return *this;
}

// Mac 0xeddc0 (text, font, image)
ButtonControl::StateDescription::~StateDescription()
{
}

// ===========================================================================================
// ButtonControl
// ===========================================================================================

// Mac 0xeb560 (C2 0xeb1e0), Win 0x51631d
ButtonControl::ButtonControl(ControlManager* manager)
    : RenderControl(manager),
      m_state(STATE_DISABLED),
      m_imageTarget(0),
      m_textTarget(0)
{
    m_stateMap[0] = 0;
    m_stateMap[1] = 1;
    m_stateMap[2] = 2;
    m_stateMap[3] = 3;
}

// Mac 0xece40/0xecea0, Win 0x516255 / 0x5162e5
ButtonControl::~ButtonControl()
{
}

// Win 0x5167f0 (inlined in every Mac handler)
void ButtonControl::SetState(UInt32 state)
{
    if (m_state != state) {
        m_state = state;
        UpdateState();
    }
}

// The resize to `count` entries the Initialize functions inline (Mac std::vector::resize with a
// default StateDescription; Win 0x5169d4).
void ButtonControl::EnsureStateDescriptions(UInt32 count)
{
    if (m_states.GetSize() < count)
        m_states.Resize(count, StateDescription());
}

// Mac 0xeb650, Win 0x516421
bool ButtonControl::Initialize()
{
    EnsureStateDescriptions(4);
    if (!RenderControl::Initialize())
        return false;
    m_states[STATE_UP].m_cursor = 1;
    m_states[STATE_OVER].m_cursor = 2;
    m_states[STATE_DOWN].m_cursor = 3;
    m_states[STATE_DISABLED].m_cursor = 1;
    SetSelectionMask(new NonTransparentSelectionMask());
    SetInputFlags(m_inputFlags | 1);
    SetTransparent(true);
    return true;
}

// Mac 0xeb8f0, Win 0x5164e7
bool ButtonControl::Create(Control* parent, Control* after, UInt32 flags, const Rect& rectangle,
                           UInt32 inputFlags, bool hitTestable)
{
    if (!Initialize())
        return false;
    return Setup(parent, after, flags, rectangle, inputFlags, hitTestable);
}

// Mac 0xeb920, Win 0x516517
bool ButtonControl::Create(const char* imageBaseName, Control* parent, Control* after, UInt32 flags,
                           const Rect& rectangle, UInt32 inputFlags, bool hitTestable)
{
    bool created = Create(parent, after, flags, rectangle, inputFlags, hitTestable);
    if (created)
        SetImages(imageBaseName);
    return created;
}

// Mac 0xeb990, Win 0x5165da
void ButtonControl::SetImages(const char* imageBaseName)
{
    static const char* const kSuffixes[4] = { ".up", ".over", ".down", ".disabled" };
    for (UInt32 i = 0; i < 4; ++i) {
        ImageLoader* loader = ImageLoader::GetInstance();
        String name(imageBaseName);
        StateDescription& description = m_states[i];
        description.m_image = loader->GetImage(name << kSuffixes[i], description.m_imageRect);
    }
    UpdateState();
}

// Mac 0xebc50, Win 0x51654c (assignment Win 0x516588)
void ButtonControl::SetStateDescription(UInt32 index, const StateDescription& description)
{
    m_states[index] = description;
    if (m_stateMap[m_state] == index)
        UpdateState();
}

// Mac 0xebd70
ButtonControl::StateDescription& ButtonControl::GetStateDescription(UInt32 index)
{
    return m_states[index];
}

// Mac 0xebd90
void ButtonControl::GetStateDescriptions(BasicArray<StateDescription>& descriptions) const
{
    if (&m_states != &descriptions)
        descriptions = m_states;
}

// Mac 0xebde0
void ButtonControl::SetStateDescriptions(const BasicArray<StateDescription>& descriptions)
{
    if (&m_states != &descriptions)
        m_states = descriptions;
    UpdateState();
}

// Mac 0xebe30, Win 0x51675a
void ButtonControl::SetTextForAllStates(const String& text)
{
    for (StateDescription* d = m_states.Begin(); d != m_states.End(); ++d)
        d->m_text = text;
    UpdateState();
}

// Mac 0xebe80, Win 0x516790
void ButtonControl::SetFontForAllStates(Font* font)
{
    for (StateDescription* d = m_states.Begin(); d != m_states.End(); ++d)
        d->m_font = font;
    UpdateState();
}

// Mac 0xebf00, Win 0x5167c6
void ButtonControl::SetStateMapping(UInt32 state, UInt32 index)
{
    if (m_stateMap[state] != index) {
        m_stateMap[state] = index;
        if (m_state == state)
            UpdateState();
    }
}

// Mac 0xebf40
UInt32 ButtonControl::GetStateMapping(UInt32 state) const
{
    return m_stateMap[state];
}

// Mac 0xebf60
UInt32 ButtonControl::GetState() const
{
    return m_state;
}

// Mac 0xebf70, Win 0x516805
void ButtonControl::UpdateState()
{
    StateDescription& description = m_states[m_stateMap[m_state]];

    RenderControl* imageTarget = m_imageTarget ? m_imageTarget.Get() : this;
    imageTarget->SetImage(description.m_image, &description.m_imageRect);
    imageTarget->SetColor(description.m_color);
    imageTarget->SetTransparent(description.m_transparent);

    RenderControl* textTarget = m_textTarget ? m_textTarget.Get() : this;
    if (!description.m_font) {
        // NOTE: original quirk: the text (and the cursor) only change when the description or
        // the target has a font; a null font is then handed to SetFont.
        RefPointer<Font> font;
        textTarget->GetFont(font);
        if (!font)
            return;
    }
    textTarget->SetFont(description.m_font);
    textTarget->SetText(description.m_text);
    SetCursor(description.m_cursor, false);
}

// Mac 0xec0d0, Win 0x5168e5
void ButtonControl::OnHidden()
{
    if (m_status.m_enabledInTree)
        SetState(STATE_UP);
    Control::OnHidden();
}

// Mac 0xec110, Win 0x516900
void ButtonControl::OnEnabled()
{
    SetState(m_status.m_mouseInside ? STATE_OVER : STATE_UP);
    Control::OnEnabled();
}

// Mac 0xec160, Win 0x51691d
void ButtonControl::OnDisabled()
{
    SetState(STATE_DISABLED);
    Control::OnDisabled();
}

// Mac 0xec190, Win 0x516930
void ButtonControl::OnMouseEnter(const Vector2& position)
{
    if (m_value124 & 1)
        SetState(STATE_DOWN);
    else if (m_status.m_enabledInTree)
        SetState(STATE_OVER);
    Control::OnMouseEnter(position);
}

// Mac 0xec1f0, Win 0x516960
void ButtonControl::OnMouseLeave(const Vector2& position)
{
    if (!m_status.m_enabledInTree)
        return;
    SetState(STATE_UP);
    Control::OnMouseLeave(position);
}

// Mac 0xec230, Win 0x516982
void ButtonControl::OnMousePressed(const Vector2& position, UInt32 button)
{
    if (button == 0)
        SetState(STATE_DOWN);
    Control::OnMousePressed(position, button);
}

// Mac 0xec270, Win 0x5169a7
void ButtonControl::OnMouseReleased(const Vector2& position, UInt32 button)
{
    if (button == 0 && m_status.m_mouseInside)
        SetState(STATE_OVER);
    Control::OnMouseReleased(position, button);
}

} // namespace LEGO
