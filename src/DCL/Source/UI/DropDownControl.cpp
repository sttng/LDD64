// LEGO Digital Designer — restored source
// TU 169 (Mac 0x229100-0x22b96b) and the ctor of TU 157 (Mac 0x220600).
#include "DropDownControl.h"
#include "RenderControlManager.h"
#include "ButtonControl.h"
#include "Texture.h"
#include "Color.h"
#include "LEGO/String.h"
#include "MathUtil.h"
#include "LDDUtilities.h"
#include "LDDApplication.h"

namespace LEGO {

typedef NotifierT<DropDownControl, DropDownControlObserver> DropDownNotifier;

// Mac 0x220600 (C1)
DropDownControl::DropDownControl(ControlManager* manager, bool notifyAlways)
    : RenderControl(manager), m_selected(-1), m_notifyAlways(notifyAlways)
{
}

// Mac 0x22abc0 (D1), 0x22b3f0 (D2 body), 0x22ac20 (D0) and thunks 0x22ad90-0x22b3d0;
// Win sdd 0x659103
DropDownControl::~DropDownControl()
{
}

// Mac 0x229100, Win 0x68bc73
bool DropDownControl::Create(Control* parent, Control* after, const Vector2& position)
{
    if (!RenderControl::Initialize())
        return false;
    // The third argument is the position (Mac 0x229147 / Win 0x68bcae: Rect(position, Vector2());
    // the size follows from label and button)
    Setup(parent, after, 7, Rect(position, Vector2()), 0, false);
    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/LDDgraphics01.png"), static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image.Get())
        return false;

    // the label: a frame image with the choice's text, centred
    m_label = new RenderControl(m_manager);
    if (!m_label->Initialize())
        return false;
    m_label->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0x40000001, false);
    Rect labelImage(Vector2(45.0f, 437.0f), Vector2(6.0f, 22.0f));
    m_label->SetImage(image, &labelImage);
    m_label->SetSize(Vector2(125.0f, 22.0f));
    m_label->SetFrame(Vector3(4.0f, 1.0f, 1.0f), Vector3(22.0f, 0.0f, 0.0f), true);
    m_label->SetDrawAsFrame(true);
    m_label->SetFont(LDDApplication::GetInstance()->GetFont(3));
    m_label->SetHorizontalAlignment(2);
    m_label->SetVerticalAlignment(2);
    ControlObserver::Attach(m_label);

    // the arrow button: four state images
    m_button = new ButtonControl(m_manager);
    if (!m_button->Initialize())
        return false;
    m_button->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    ButtonControl::StateDescription description;
    description.m_image = image;
    description.m_transparent = true;
    static const Real32 kButtonImages[4][2] = {{44.0f, 367.0f}, {64.0f, 367.0f},
                                               {44.0f, 391.0f}, {64.0f, 391.0f}};
    for (UInt32 i = 0; i < 4; ++i) {
        description.m_imageRect = Rect(Vector2(kButtonImages[i][0], kButtonImages[i][1]),
                                       Vector2(16.0f, 22.0f));
        m_button->SetStateDescription(i, description);
    }
    m_button->SetAlignment(0x13, 0x13, 0.0f, 0.0f, 0);   // right end, vertically on top
    ControlObserver::Attach(m_button);

    // the pop-up frame (not shown), below the button's right edge
    m_popup = new RenderControl(m_manager);
    m_popup->Initialize();
    Rect popupImage(Vector2(45.0f, 520.0f), Vector2(24.0f, 178.0f));
    m_popup->SetImage(image, &popupImage);
    m_popup->SetFrame(Vector3(7.0f, 0.0f, 17.0f), Vector3(12.0f, 0.0f, 12.0f), true);
    m_popup->SetDrawAsFrame(true);
    m_popup->Setup(this, 0, 6, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_popup->SetAxisAlignment(0, 3, 3, -7.0f, m_button);
    m_popup->SetAxisAlignment(1, 1, 3, 0.0f, m_button);
    ControlManagerObserver::Attach(m_manager);
    SetSize(Vector2(m_label->GetSize().x + m_button->GetSize().x, m_button->GetSize().y));
    return true;
}

// Mac 0x229eb0
void DropDownControl::LayoutItems()
{
    Real32 width = 0.0f;
    Real32 height = 0.0f;
    for (UInt32 i = 0; i < m_items.GetSize(); ++i) {
        const Vector2& size = m_items[i]->GetSize();
        width = size.x <= width ? width : size.x;
        height = size.y <= height ? height : size.y;
    }
    UInt32 column = 0;
    UInt32 row = 0;
    for (UInt32 i = 0; i < m_items.GetSize(); ++i) {
        Vector2 position((Real32)(EvalReal((Real32)(double)column) * width + 5.0f),
                         (Real32)(EvalReal((Real32)(double)row) * height + 5.0f));
        m_items[i]->SetPosition(position);
        ++row;
        if (row > 7) {
            row = 0;
            ++column;
        }
    }
    m_popup->FitToChildren(false, false, true, true);
    const Vector2& size = m_popup->GetSize();
    m_popup->SetSize(Vector2(size.x + 12.0f, 12.0f + size.y));
}

// Mac 0x22a0a0
void DropDownControl::AddItem(Control* item)
{
    m_items.PushBack(RefPointer<Control>(item));
    item->SetParent(m_popup, 0);
    ControlObserver::Attach(item);
    LayoutItems();
}

// Mac 0x22a160
bool DropDownControl::SetText(const String& text)
{
    return m_label->SetText(text);
}

// Mac 0x22a190, Win 0x68b8b2
void DropDownControl::SetPopupParent(Control* parent)
{
    m_popup->SetParent(parent, 0);
}

// Mac 0x22a1c0 (thunk 0x22a520), Win 0x68ba5a
void DropDownControl::OnMousePressed(Control* control, const Vector2&, UInt32)
{
    for (UInt32 i = 0; i < m_items.GetSize(); ++i) {
        if (m_items[i] != control)
            continue;
        m_popup->SetShown(false);
        if (m_selected == (Int32)i && !m_notifyAlways)
            return;
        m_selected = (Int32)i;
        DropDownNotifier::Iterator it(*this);
        while (DropDownControlObserver* observer = it.Next(DropDownNotifier::m_observers))
            observer->OnItemSelected(control, (UInt32)m_selected);
        return;
    }
    if (control == (Control*)m_button || control == (Control*)m_popup || control == (Control*)m_label) {
        m_popup->SetZOrder(0);
        m_popup->SetShown(!m_popup->GetStatus().m_visible);
    }
}

// Mac 0x22a540 (thunk 0x22a6d0), Win 0x68bb22
void DropDownControl::OnManagerMouseDown(const Vector2& position, UInt32)
{
    if (!m_popup->GetStatus().m_visible)
        return;
    Control* control = m_manager->GetControlAt(position);
    if (control == (Control*)m_button || control == (Control*)m_popup || control == (Control*)m_label)
        return;
    for (; control; control = control->GetParent()) {
        for (UInt32 i = 0; i < m_items.GetSize(); ++i)
            if (m_items[i] == control)
                return;
    }
    m_popup->SetShown(false);
}

// Mac 0x22ac90
void DropDownControl::OnFinalRelease()
{
    ControlManagerObserver::OnFinalRelease();
    ControlObserver::OnFinalRelease();
    DropDownNotifier::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

} // namespace LEGO
