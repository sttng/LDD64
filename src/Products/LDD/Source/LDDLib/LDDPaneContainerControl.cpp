// LEGO Digital Designer — restored source
// TU 287 "LDDPaneContainerControl.cpp": Mac 0x29aa60-0x29bb3b, Win 0x66d23b-0x66d874.
// Not code: Mac 0x29bac0 = <iostream> + a header static.
#include "LDDPaneContainerControl.h"
#include "LDDPaneToggleControl.h"
#include "AnimateShowHideControlCommand.h"
#include "SlidingHideAugmentation.h"
#include "RenderControlManager.h"
#include "LDDUtilities.h"
#include "Texture.h"

namespace LEGO {

namespace {

// Control::GetAugmentation<T> (template instance Mac 0x29b120, Win 0x66d4b5): the named
// augmentation as a T (dynamic_cast); false when there is none or it is no T.
template <class T>
bool GetAugmentationAs(Control* control, const String& name, RefPointer<T>& augmentation)
{
    RefPointer<ControlAugmentation> found;
    if (!control->GetAugmentation(name, found))
        return false;
    augmentation = found ? dynamic_cast<T*>(found.Get()) : 0;
    return !augmentation.IsNull();
}

} // namespace

// Mac 0x29aa60, Win 0x66d2e3
LDDPaneContainerControl::LDDPaneContainerControl(ControlManager* manager, Control* content,
                                                 bool observeContent)
    : Control(manager), m_grip(0), m_handle(0), m_content(content),
      m_observeContent(observeContent), m_dragging(false), m_dragOffset()
{
}

// Mac 0x29b370 (D1) / 0x29b4b0 (D0), Win sdd 0x66d463 (body 0x66d3c9)
LDDPaneContainerControl::~LDDPaneContainerControl()
{
}

// Mac 0x29b620 (thunks 0x29b690, 0x29b700)
void LDDPaneContainerControl::OnFinalRelease()
{
    ControlObserver::OnFinalRelease();
    Control::OnFinalRelease();
}

// Mac 0x29abe0, Win 0x66d525
bool LDDPaneContainerControl::Create(Control* parent, bool rightSide,
                                     AnimateShowHideControlCommand* command)
{
    ControlManager* manager = m_manager;
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
        static_cast<RenderControlManager*>(manager)->GetRender());
    if (!image)
        return false;
    if (!Setup(parent, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false))
        return false;

    // The grip strip along the content's edge.
    m_grip = new RenderControl(manager);
    m_grip->Initialize();
    m_grip->Setup(this, 0, 7, Rect(Vector2(5.0f, 9.0f)), 1, true);
    const UInt32 edge = rightSide * 2 + 1;   // 1 (near edge) or 3 (far edge)
    m_grip->SetAxisAlignment(0, edge, edge, 0.0f, 0);
    m_grip->SetAxisMargins(1, 0.0f, 0.0f, 0);
    const Rect gripLeft (299.0f, 190.0f, 312.0f, 192.0f);
    const Rect gripRight(319.0f, 190.0f, 332.0f, 192.0f);
    m_grip->SetImage(image, rightSide ? &gripRight : &gripLeft);

    // The show/hide handle, sliding the content.
    RefPointer<SlidingHideAugmentation> augmentation;
    if (!GetAugmentationAs(m_content, String("SlidingHideAugmentation"), augmentation))
        return false;
    m_handle = new LDDPaneToggleControl(manager, command, augmentation);
    m_handle->Create(this, 0, 7, rightSide);
    m_handle->SetAxisAlignment(0, edge, edge, 0.0f, 0);
    m_handle->SetAxisAlignment(1, 1, 1, -8.0f, 0);
    FitToChildren(true, true, true, true);

    if (m_observeContent) {
        if (m_content->GetStatus().m_visible) {
            ControlObserver::Attach(m_grip);
            m_grip->SetCursor(13, true);
        }
        ControlObserver::Attach(m_content);
    }
    return true;
}

// Mac 0x29b780 (thunk 0x29b7d0), Win 0x66d23b
void LDDPaneContainerControl::OnMousePressed(Control* control, const Vector2& position,
                                             UInt32 /*button*/)
{
    if (m_grip == control) {
        m_dragOffset = LocalToControl(control, position);
        m_dragging = true;
    }
}

// Mac 0x29b820 (thunk 0x29b840), Win 0x66d270
void LDDPaneContainerControl::OnMouseReleased(Control* control, const Vector2& /*position*/,
                                              UInt32 /*button*/)
{
    if (m_grip == control)
        m_dragging = false;
}

// Mac 0x29b860 (thunk 0x29b8f0), Win 0x66d280
void LDDPaneContainerControl::OnMouseMove(Control* control, const Vector2& position)
{
    if (m_dragging && m_grip == control) {
        Vector2 point = LocalToControl(m_content, position);
        m_content->SetSize(Vector2(point.x - m_dragOffset.x, point.y - m_dragOffset.y));
    }
}

// Mac 0x29b980 (thunk 0x29b9d0), Win 0x66d485
void LDDPaneContainerControl::OnShown(Control* control)
{
    if (m_content == control) {
        ControlObserver::Attach(m_grip);
        m_grip->SetCursor(13, true);
    }
}

// Mac 0x29ba20 (thunk 0x29ba70), Win 0x66d831
void LDDPaneContainerControl::OnHidden(Control* control)
{
    if (m_content == control) {
        ControlObserver::Detach(m_grip);
        m_grip->SetCursor(1, true);
    }
}

} // namespace LEGO
