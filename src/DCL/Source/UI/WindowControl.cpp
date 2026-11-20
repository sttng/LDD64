// LEGO Digital Designer — restored source
// LEGO::WindowControl — TU 029 "WindowControl.cpp" (Mac 0xee970-0xf10fb, Win
// 0x50fcaa-0x510ab9).
// Not restored here: ObserverN<Control, ControlObserver>::Attach (Mac 0xef0f0, a template
// instance emitted here; ObserverN.h), the BasicArray<Control*> / construction-vtable helpers
// (0xf0e90, 0xf0b00-0xf0e10) and the text-module argument destructors (0xefaa0, 0xefad0, 0xf0e30,
// 0xf0e40).
#include "WindowControl.h"
#include "Render.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Font.h"
#include "TextLayout.h"

namespace LEGO {

namespace {

inline Real32 MaxOf(Real32 a, Real32 b) { return a <= b ? b : a; }

} // namespace

// Mac 0xee970 (C2), Win 0x50fce2
WindowControl::WindowControl(ControlManager* manager)
    : Control(manager),
      m_content(0),
      m_active(false),
      m_frameTexture(0),
      m_frameBuffer(0),
      m_frameOpaque(true),
      m_titleLayout(0),
      m_titleRect(0.0f, 0.0f, 0.0f, 0.0f),
      m_titleShadow(false),
      m_minimized(false),
      m_resizable(true),
      m_marginLeft(0.0f),
      m_marginTop(0.0f),
      m_marginRight(0.0f),
      m_marginBottom(0.0f),
      m_titleBarHeight(0.0f),
      m_borderWidth(0.0f),
      m_dragMode(kDragNone)
{
    m_dragHelper.m_resizable = true;
    m_dragHelper.m_movable = true;
    m_dragHelper.m_limited = true;
}

// Mac 0xf0920/0xf0980, Win 0x50fccc / 0x50fcaa
WindowControl::~WindowControl()
{
}

// Mac 0xf09f0
void WindowControl::OnFinalRelease()
{
    ObserverN<Control, ControlObserver>::OnFinalRelease();
    Observer<ControlManager, ControlManagerObserver>::OnFinalRelease();
    Control::OnFinalRelease();
}

// Mac 0xf0aa0, Win 0x51611f (ICF)
bool WindowControl::IsInside(const Vector2& /*point*/) const
{
    return true;
}

// Mac 0xeec70, Win 0x50fead
bool WindowControl::Initialize()
{
    SetInputFlags(m_inputFlags | 3);
    Observer<ControlManager, ControlManagerObserver>::Attach(m_manager);
    return true;
}

Real32 WindowControl::GetContentTop() const
{
    return MaxOf(m_marginTop, m_titleBarHeight);
}

// Win 0x510b55 (inlined twice on Mac)
void WindowControl::LayoutContent()
{
    ObserverN<Control, ControlObserver>::Detach(m_content);
    Rect rectangle(m_marginLeft, GetContentTop(), m_size.x - m_marginRight,
                   m_size.y - m_marginBottom);
    m_content->SetRectangle(rectangle);
    ObserverN<Control, ControlObserver>::Attach(m_content);
}

// Mac 0xeef10, Win 0x50fed1
void WindowControl::SetContent(Control* content)
{
    if (m_content == content)
        return;
    if (content) {
        if (!content->SetActive(true) || !content->SetEnabled(true) || !content->SetShown(true))
            return;
    }
    if (m_content) {
        m_content->SetParent(0, 0);
        m_content = 0;
        // NOTE: original quirk: the pointer is already null here (Mac 0xeefd0 / Win 0x50ff2a),
        // so the previous content is not detached.
        ObserverN<Control, ControlObserver>::Detach(m_content);
    }
    m_content = content;
    if (m_content) {
        LayoutContent();
        m_content->SetParent(this, this);
        ObserverN<Control, ControlObserver>::Attach(content);
    }
}

// Mac 0xef5e0, Win 0x50ff6a
bool WindowControl::SetTitle(const String& title)
{
    if (!m_titleLayout) {
        m_titleLayout = CreateTitleLayout();
        if (!m_titleLayout)
            return false;
    }
    m_titleLayout->SetText(title);
    return true;
}

// Mac 0xef670, Win 0x50ffa2
void WindowControl::SetResizable(bool resizable)
{
    m_resizable = resizable;
    if (!m_minimized)
        m_dragHelper.m_resizable = resizable;
}

// Size around the content; minimized: the title bar (at least 20 wide).
Vector2 WindowControl::GetFrameSize(bool minimized) const
{
    Vector2 contentSize = m_content ? m_content->GetSize() : Vector2(0.0f, 0.0f);
    Real32 width = m_marginLeft + m_marginRight + contentSize.x;
    Real32 height = GetContentTop() + m_marginBottom;
    if (minimized)
        return Vector2(MaxOf(width, 20.0f), height);
    return Vector2(width, height + contentSize.y);
}

// Mac 0xef690, Win 0x50ffbe
void WindowControl::Minimize()
{
    if (m_minimized)
        return;
    m_minimized = true;
    if (m_content)
        m_content->SetShown(false);
    m_dragHelper.m_resizable = false;
    SetSize(GetFrameSize(true));
    OnMinimized();
}

// Mac 0xef760, Win 0x51006b
void WindowControl::Restore()
{
    if (!m_minimized)
        return;
    m_minimized = false;
    if (m_content)
        m_content->SetShown(true);
    m_dragHelper.m_resizable = m_resizable;
    SetSize(GetFrameSize(false));
    OnRestored();
}

// Mac 0xef830, Win 0x510118
TextLayoutRender* WindowControl::CreateTitleLayout()
{
    ObjectFactory* factory = m_manager->GetObjectFactory();
    Font::SystemTypeFontArgs fontArgs;
    fontArgs.type = Font::kTitleBarFont;
    fontArgs.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    Object* object = factory->CreateObject(typeid(Font), fontArgs);
    if (!object)
        return 0;
    RefPointer<Font> font = dynamic_cast<Font*>(object);
    if (!font)
        return 0;

    TextLayout::CreateArgs layoutArgs;
    layoutArgs.text = String("");
    TextLayoutRender* layout = 0;
    object = factory->CreateObject(typeid(TextLayoutRender), layoutArgs);
    if (object) {
        layout = dynamic_cast<TextLayoutRender*>(object);
        if (layout) {
            layout->SetFont(font);
            layout->SetTruncate(true);
            layout->SetMaxWidth((UInt32)(Int64)m_titleRect.GetWidth());
        }
    }
    return layout;
}

// Mac 0xefae0, Win 0x510290
void WindowControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    if (!m_minimized) {
        if (oldSize != m_size) {
            const Vector2& minimum = m_dragHelper.m_minimumSize;
            if (m_size.x < minimum.x || m_size.y < minimum.y) {
                SetSize(Vector2(MaxOf(m_size.x, minimum.x), MaxOf(m_size.y, minimum.y)));
                return;
            }
            OnResized(oldSize);
        }
        if (m_content)
            LayoutContent();
    } else {
        Vector2 size = GetFrameSize(true);
        if (size != m_size) {
            SetSize(size);
            return;
        }
        if (oldSize != m_size)
            OnResized(oldSize);
    }
    if (m_titleLayout)
        m_titleLayout->SetMaxWidth((UInt32)(Int64)MaxOf(m_titleRect.GetWidth(), 0.0f));
    Control::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0xefee0, Win 0x5108e4
UInt32 WindowControl::HitTest(const Vector2& position) const
{
    Vector2 size(m_size);
    Real32 x = position.x;
    Real32 y = position.y;
    if (x < 0.0f || y < 0.0f || size.x <= x || size.y <= y)
        return kDragNone;

    typedef ControlDragHelper H;
    if (x < m_marginLeft) {
        if (y < m_borderWidth)
            return H::kLeft | H::kTop;
        return y < size.y - m_borderWidth ? (UInt32)H::kLeft : (UInt32)(H::kLeft | H::kBottom);
    }
    if (size.x - m_marginRight <= x) {
        if (y < m_borderWidth)
            return H::kTop | H::kRight;
        return y < size.y - m_borderWidth ? (UInt32)H::kRight : (UInt32)(H::kRight | H::kBottom);
    }
    if (y < m_marginTop) {
        if (x < m_borderWidth)
            return H::kLeft | H::kTop;
        return x < size.x - m_borderWidth ? (UInt32)H::kTop : (UInt32)(H::kTop | H::kRight);
    }
    if (y < size.y - m_marginBottom)
        return m_titleBarHeight <= y ? (UInt32)kDragNone : (UInt32)kDragMove;
    if (x < m_borderWidth)
        return H::kLeft | H::kBottom;
    return x < size.x - m_borderWidth ? (UInt32)H::kBottom : (UInt32)(H::kRight | H::kBottom);
}

namespace {

// A drag mode of ControlDragHelper (anything but kDragNone the hit test returns).
bool IsDragMode(UInt32 mode)
{
    switch (mode) {
    case 0x01: case 0x04: case 0x08: case 0x0c: case 0x10:
    case 0x18: case 0x20: case 0x24: case 0x30:
        return true;
    default:
        return false;
    }
}

} // namespace

// Mac 0xefd20, Win 0x510423
void WindowControl::OnMouseMove(const Vector2& position)
{
    if (IsDragMode(m_dragMode))
        m_dragHelper.Drag(m_dragMode, position, this);
    else if (m_dragMode == kDragNone)
        UpdateCursor(position);
    Control::OnMouseMove(position);
}

// Mac 0xefe40, Win 0x51048c
void WindowControl::OnMousePressed(const Vector2& position, UInt32 button)
{
    if (button == 0 && m_dragMode == kDragNone) {
        UInt32 mode = HitTest(position);
        if (mode != kDragNone) {
            m_dragMode = mode;
            m_dragHelper.m_grab = position;
            m_dragHelper.m_limits = Rect(m_parent->GetSize());
            UpdateCursor(position);
        }
    }
    Control::OnMousePressed(position, button);
}

// Mac 0xf0040, Win 0x5104fc
void WindowControl::OnMouseReleased(const Vector2& position, UInt32 button)
{
    if (button == 0 && IsDragMode(m_dragMode)) {
        m_dragMode = kDragNone;
        UpdateCursor(position);
    }
    Control::OnMouseReleased(position, button);
}

// Mac 0xf0700, Win 0x510ab9
void WindowControl::UpdateCursor(const Vector2& position)
{
    UInt32 mode = m_dragMode;
    UInt32 cursor;
    if (mode == kDragNone) {
        mode = HitTest(position);
        if (mode == kDragMove) {
            SetCursor(4, true);
            return;
        }
    } else if (mode == kDragMove) {
        SetCursor(5, true);
        return;
    }
    switch (mode) {
    case 0x04: case 0x0c: cursor = 0xc; break;
    case 0x08: case 0x18: cursor = 10; break;
    case 0x10: case 0x24: cursor = 0xd; break;
    case 0x20: case 0x30: cursor = 0xb; break;
    default: cursor = 1; break;
    }
    SetCursor(cursor, true);
}

// Mac 0xf0160, Win 0x510564
void WindowControl::Draw(ControlDrawArgs& args)
{
    Render* render = args.m_render;
    args.Setup(false);
    if (!m_content) {
        Vector2 size(m_size);
        Rect area(m_marginLeft, m_titleBarHeight, size.x - m_marginRight, size.y - m_marginBottom);
        Color white;
        args.DrawRect(area, white, false);
    }
    if (m_frameBuffer) {
        render->EnableBlend(!m_frameOpaque);
        render->SetTexture(m_frameTexture, 0);
        render->SetVertexBuffer(m_frameBuffer);
        render->DrawVertexBuffer();
        if (m_frameBuffer && m_titleLayout) {
            Vector2 position(m_titleRect.m_min);
            m_titleLayout->AlignInArea(m_titleRect.GetSize(), position);
            Rect visible(position, m_titleRect.GetSize());
            visible.Intersect(Rect(position, m_titleLayout->GetSize()));
            Rect clip = args.GetLocalClip();
            visible.Intersect(clip);
            if (visible.IsValid()) {
                Texture* texture = m_titleLayout->GetTexture();
                if (texture) {
                    if (m_titleShadow) {
                        Rect shadow(position + Vector2(1.0f, 1.0f), m_titleLayout->GetSize());
                        args.DrawTexture(shadow, texture, m_titleLayout->GetTexCoords(),
                                         m_titleShadowColor, true);
                    }
                    Rect text(position, m_titleLayout->GetSize());
                    args.DrawTexture(text, texture, m_titleLayout->GetTexCoords(), m_titleColor, true);
                }
            }
        }
    }
    m_status.m_invalid = false;
}

// Mac 0xf04d0 (thunk 0xf0590), Win 0x5107ee
void WindowControl::OnManagerKeyboardFocusChanged(bool /*flag*/, Control* /*previous*/,
                                                  Control* current, UInt32 /*modifiers*/)
{
    if (current == this && m_content) {
        m_content->SetKeyboardFocus(true);
        return;
    }
    Control* control = current;
    while (control != this && control)
        control = control->GetParent();
    if (!control) {
        if (!m_active)
            return;
        m_active = false;
        OnWindowDeactivated();
    } else {
        if (m_active)
            return;
        SetZOrder(0);
        m_active = true;
        OnWindowActivated();
    }
    if (!m_manager->IsUpdateLocked())
        Invalidate(false);
}

// Mac 0xf05c0 (thunk 0xf05e0), Win 0x510874
void WindowControl::OnManagerMouseOverChanged(Control* previous, Control* /*current*/)
{
    if (previous == this)
        m_dragMode = kDragNone;
}

// Mac 0xf0600 (thunk 0xf0680), Win 0x51088d
void WindowControl::OnRectangleChanged(Control* control, const Vector2& /*oldPosition*/,
                                       const Vector2& /*oldSize*/)
{
    if (!m_minimized && m_content == control) {
        const Vector2& size = control->GetSize();
        SetSize(Vector2(m_marginLeft + m_marginRight + size.x,
                        GetContentTop() + m_marginBottom + size.y));
    }
}

void WindowControl::OnResized(const Vector2& /*oldSize*/) {}         // Mac 0xf0ab0, Win 0x50fca7
void WindowControl::OnWindowActivated() {}                           // Mac 0xf0ac0, Win 0x512edf
void WindowControl::OnWindowDeactivated() {}                         // Mac 0xf0ad0, Win 0x512edf
void WindowControl::OnMinimized() {}                                 // Mac 0xf0ae0, Win 0x512edf
void WindowControl::OnRestored() {}                                  // Mac 0xf0af0, Win 0x512edf

} // namespace LEGO
