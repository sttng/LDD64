// LEGO Digital Designer — restored source
// TU 081 "FilterControl.cpp": Mac 0x16d290-0x16ee2b, Win 0x664c48-0x6652cc (+ 0x628328-0x629136).
// Not code: Mac 0x16edb0 = <iostream> + a header static.
#include "FilterControl.h"
#include "FilterControlManager.h"
#include "BoxFilterControl.h"
#include "RenderControlManager.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "TextLocale.h"
#include "Texture.h"
#include "MathUtil.h"

namespace LEGO {

// Mac 0x16e650 (D1) / 0x16e6b0 (D0), Win sdd 0x629136
FilterControl::~FilterControl()
{
}

// Mac 0x16db30 (thunks 0x16dbf0 … 0x16dc70)
void FilterControl::OnFinalRelease()
{
    ControlManagerObserver::OnFinalRelease();
    ControlObserver::OnFinalRelease();
    LDDPopUpControl::OnFinalRelease();
}

// Mac 0x16d290, Win 0x664dd7
bool FilterControl::Create(Control* parent, Control* after)
{
    if (!LDDPopUpControl::Create(parent, after))
        return false;
    ControlManager* manager = m_manager;
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/Icon_general02.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;

    m_layout = new MatrixLayoutControl(manager);
    m_layout->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_layout->SetAxisAlignment(1, 1, 1, -15.0f, 0);
    m_layout->SetAxisMargins(0, -15.0f, 15.0f, 0);

    m_clearButton = new ButtonControl(m_manager);
    if (!m_clearButton->Create(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, true))
        return false;
    ButtonControl::StateDescription description;
    description.m_transparent = m_transparent;
    description.m_image = image;
    const Vector2 size(130.0f, 55.0f);
    description.m_imageRect.Set(Rect(Vector2(3.0f, 358.0f), size));
    m_clearButton->SetStateDescription(0, description);
    description.m_imageRect.Set(Rect(Vector2(141.0f, 358.0f), size));
    m_clearButton->SetStateDescription(1, description);
    description.m_imageRect.Set(Rect(Vector2(279.0f, 358.0f), size));
    m_clearButton->SetStateDescription(2, description);
    description.m_imageRect.Set(Rect(Vector2(417.0f, 358.0f), size));
    m_clearButton->SetStateDescription(3, description);
    CreateStandardTextTooltip(m_clearButton, LocalizedString(String("REMOVEBOXFILTER")));
    LDDApplication::GetInstance()->AddButtonSound(m_clearButton);
    m_clearButton->SetAlignment(0x32, 0x32, 0.0f, 15.0f, 0);
    ControlObserver::Attach(m_clearButton);
    return true;
}

// Mac 0x16d960, Win 0x665154
void FilterControl::SetItems(const BasicArray<RefPointer<Control> >& items, const Vector2& spacing)
{
    m_layout->RemoveAllControls();
    for (BasicArray<RefPointer<Control> >::ConstIterator it = items.Begin(); it != items.End(); ++it) {
        Control* item = *it;
        item->SetParent(m_layout, 0);
        ControlObserver::Attach(item);
        item->SetInputFlags(item->GetInputFlags() | 1);
        m_layout->AddControl(item);
    }
    Real32 cellWidth = spacing.x;
    if (Abs(cellWidth) < 1e-05f)
        cellWidth = m_layout->GetSize().x;
    m_layout->SetCellSize(Vector2(cellWidth, spacing.y));

    Real32 width = m_layout->GetSize().x;
    if (width <= m_clearButton->GetSize().x)
        width = m_clearButton->GetSize().x;
    SetSize(Vector2(width + 30.0f,
                    m_layout->GetSize().y + m_clearButton->GetSize().y + 45.0f));

    // NOTE: dereferences the first item without an emptiness check (both builds).
    if (items[0]) {
        FilterItem* first = dynamic_cast<FilterItem*>(items[0].Get());
        if (first)
            m_defaultFilter = first->GetFilter();
    }
}

// Mac 0x16dd30, Win 0x664d40
void FilterControl::SelectItem(Control* item)
{
    FilterItem* filterItem = dynamic_cast<FilterItem*>(item);
    if (!filterItem)
        return;
    for (MatrixLayoutControl::ControlIterator it = m_layout->BeginControls();
         it != m_layout->EndControls(); ++it)
        static_cast<ToggleControl*>(it->Get())->SetToggled(false);
    m_filterManager->AddFilter(filterItem->GetFilter());
    static_cast<ToggleControl*>(item)->SetToggled(true);
    SetShown(false);
}

// Mac 0x16dc90 (thunk 0x16de10), Win 0x664d93
void FilterControl::OnMouseClicked(Control* control, const Vector2& /*position*/, UInt32 button)
{
    if (button != 0)
        return;
    for (MatrixLayoutControl::ControlIterator it = m_layout->BeginControls();
         it != m_layout->EndControls(); ++it)
        static_cast<ToggleControl*>(it->Get())->SetToggled(false);
    if (m_clearButton != control) {
        SelectItem(control);
        return;
    }
    m_filterManager->RemoveFilter(m_defaultFilter);
    SetShown(false);
}

// Mac 0x16de40, Win 0x6652b5
void FilterControl::OnShown()
{
    m_mouseInside = false;
    ControlManagerObserver::Attach(m_manager);
}

// Mac 0x16e0d0, Win 0x664401
void FilterControl::OnHidden()
{
    ControlManagerObserver::Detach();
}

// Mac 0x16e130 (thunk 0x16e180), Win 0x664c48
void FilterControl::OnManagerMouseEnter(Control* control, const Vector2& /*position*/)
{
    for (; control; control = control->GetParent()) {
        if (control == this) {
            m_mouseInside = true;
            return;
        }
    }
    if (m_mouseInside) {
        m_mouseInside = false;
        SetShown(false);
    }
}

// Mac 0x16e1e0 (thunk 0x16e230), Win 0x664c81
void FilterControl::OnManagerMouseDown(const Vector2& position, UInt32 /*button*/)
{
    for (Control* control = m_manager->GetControlAt(position); control; control = control->GetParent())
        if (control == this)
            return;
    SetShown(false);
}

} // namespace LEGO
