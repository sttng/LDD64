// LEGO Digital Designer — restored source
// TU 138 "MaterialPickerAugmentation.cpp": Mac 0x1f3190-0x1f40cb, Win 0x69181f-0x691c1b.
// Not code: the destructor body 0x1f3f40, construction-vtable thunks (0x1f3c50-0x1f3f20);
// Mac 0x1f4020 = <iostream> + a header static + kMaterialPickerAugmentationName.
#include "MaterialPickerAugmentation.h"
#include "PaletteMaterialControl.h"
#include "BricksControl.h"
#include "RenderControl.h"
#include "ControlManager.h"
#include "SystemTime.h"
#include "Texture.h"

namespace LEGO {

const String kMaterialPickerAugmentationName("Material picker");

// Mac 0x1f3190
MaterialPickerAugmentation::MaterialPickerAugmentation(PaletteMaterialControl* materials,
                                                       Control* control, Texture* texture)
    : m_materials(materials), m_badge(), m_start(0), m_delay(500), m_enabled(false)
{
    m_badge = new RenderControl(control->GetManager());
    m_badge->Initialize();
    m_badge->Setup(control, 0, 6, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_badge->SetAlignment(0x13, 0x13, 0.0f, -2.0f, 0);
    static const Rect s_badgeRect(Vector2(406.0f, 175.0f), Vector2(16.0f, 16.0f));
    m_badge->SetImage(texture, &s_badgeRect);
    Attach(control, kMaterialPickerAugmentationName);
}

// Mac 0x1f3500, Win 0x6918bd
bool MaterialPickerAugmentation::Attach(Control* control, const String& name)
{
    if (!dynamic_cast<BricksControl*>(control))
        return false;
    if (!ControlAugmentation::Attach(control, name))
        return false;
    ObserveControl(control);
    UpdateObserver::Detach();
    return true;
}

// Mac 0x1f3490
void MaterialPickerAugmentation::SetShowVariants(bool show)
{
    if (m_enabled == show)
        return;
    m_enabled = show;
    m_badge->SetShown(show);
    if (!m_enabled)
        UpdateObserver::Detach();
}

// Mac 0x1f35e0 (thunk 0x1f3730), Win 0x6918f3
void MaterialPickerAugmentation::Update()
{
    UInt64 now = (UInt64)GetMilliseconds();
    if (GetControl()->GetPressedButtons() != 0) {
        m_start = now;
        return;
    }
    if (now - m_start > m_delay) {
        ShowVariants();
        UpdateObserver::Detach();
    }
}

// Mac 0x1f3660, Win 0x69181f
void MaterialPickerAugmentation::ShowVariants()
{
    Control* control = GetControl();
    if (!control)
        return;
    BricksControl* item = dynamic_cast<BricksControl*>(control);
    if (!item)
        return;
    Vector2 position = item->LocalToControl(m_materials->GetParent(), Vector2(0.0f, 0.0f));
    m_materials->ShowVariants(item->GetPaletteBrick(), &item->GetCamera());
    m_materials->SetPosition(position);
    m_materials->MoveInsideParent();
}

// Mac 0x1f37b0 (thunk 0x1f3a30), Win 0x691be9
void MaterialPickerAugmentation::OnMouseEnter(Control* control, const Vector2&)
{
    if (!m_enabled)
        return;
    UpdateObserver::Attach(static_cast<Updater*>(control->GetManager()));
    m_start = (UInt64)GetMilliseconds();
}

// Mac 0x1f3a50 (thunk 0x1f3aa0), Win 0x691944
void MaterialPickerAugmentation::OnMouseLeave(Control*, const Vector2&)
{
    UpdateObserver::Detach();
}

// Mac 0x1f3bc0
void MaterialPickerAugmentation::OnFinalRelease()
{
    UpdateObserver::OnFinalRelease();
    ControlAugmentationControlObserver::OnFinalRelease();
    Object::OnFinalRelease();
}

} // namespace LEGO
