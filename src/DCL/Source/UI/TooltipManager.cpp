// LEGO Digital Designer — restored source
// TU 049 "TooltipManager.cpp" (Mac 0x111cf0-0x11437b; Win 0x44b470-0x44b65f, 0x512ee0-0x5138f1):
// ControlAugmentation, ControlManagerAugmentation, TooltipAugmentation, TooltipManager (Trigger
// and BasicArray<RefPointer<Control> > are header templates emitted here on Mac).
#include "TooltipManager.h"
#include "ControlAugmentation.h"
#include "ControlManager.h"
#include "Control.h"
#include "Preferences.h"
#include "MathUtil.h"

namespace LEGO {

namespace {
const char* const kTooltipName = "tooltip";                 // the control augmentation's name
const char* const kTooltipManagerName = "TooltipManager";   // the manager augmentation's name
const char* const kShowTooltipsKey = "SHOWTOOLTIPS";        // preference
} // namespace

// ===========================================================================================
// ControlAugmentation
// ===========================================================================================

// Mac 0x111f10, Win 0x44b470
bool ControlAugmentation::Attach(Control* control, const String& name)
{
    if (m_control == control)
        return true;
    if (!control->AddAugmentation(name, this))
        return false;
    if (m_control)
        m_control->RemoveAugmentation(m_name);
    m_control = control;
    m_name = name;
    return true;
}

// Mac 0x111fc0, Win 0x44b4d0
void ControlAugmentation::Detach()
{
    if (Control* control = m_control) {
        m_control = 0;
        control->RemoveAugmentation(m_name);
    }
}

// Mac 0x113db0/0x113e40, Win 0x44b5b0 (sdd 0x44b630)
ControlAugmentation::~ControlAugmentation()
{
    if (m_control)
        m_control->RemoveAugmentation(m_name);
}

// ===========================================================================================
// ControlManagerAugmentation
// ===========================================================================================

// Mac 0x113ee0/0x113f40, Win 0x512f86 (sdd 0x512fd4)
ControlManagerAugmentation::~ControlManagerAugmentation()
{
}

// Mac 0x112360, Win 0x512f12
bool ControlManagerAugmentation::Attach(ControlManager* manager)
{
    if (m_manager == manager)
        return true;
    RefPointer<Object> keep(static_cast<Object*>(this));
    if (ControlManager* previous = m_manager) {
        m_manager = 0;
        previous->RemoveAugmentation(this);
        OnManagerDetached(previous);
    }
    if (!manager->AddAugmentation(this))
        return false;
    m_manager = manager;
    OnManagerAttached(manager);
    return true;
}

// Mac 0x112e10, Win 0x512fc9 (→ 0x5062d1)
void ControlManagerAugmentation::OnFinalRelease()
{
    if (ControlManager* manager = m_manager) {
        // NOTE: as in both originals the augmentation references itself here; with a manager the
        // manager's map still holds a reference, so this path is not normally reached.
        RefPointer<Object> keep(static_cast<Object*>(this));
        m_manager = 0;
        manager->RemoveAugmentation(this);
        OnManagerDetached(manager);
    }
    Object::OnFinalRelease();
}

// ===========================================================================================
// TooltipAugmentation
// ===========================================================================================

// Mac 0x1121f0/0x1122a0, Win 0x51313f (sdd 0x513115)
TooltipAugmentation::~TooltipAugmentation()
{
}

// Mac 0x111cf0, Win 0x512ffe
bool TooltipAugmentation::AttachTo(Control* control)
{
    RefPointer<ControlAugmentation> previous;
    control->GetAugmentation(String(kTooltipName), previous);
    if (previous)
        previous->Detach();
    if (!ControlAugmentation::Attach(control, String(kTooltipName)))
        return false;
    if (control->GetStatus().m_mouseInside) {
        // NOTE: both originals assume the manager has a TooltipManager (null is dereferenced).
        TooltipManager* manager = dynamic_cast<TooltipManager*>(
            control->GetManager()->GetAugmentation(String(kTooltipManagerName)));
        manager->SetAugmentation(this);
    }
    return true;
}

// Mac 0x111f80, Win 0x5130e5
void TooltipAugmentation::Remove()
{
    if (m_manager)
        m_manager->Hide();
    ControlAugmentation::Detach();
}

// Mac 0x111ff0, Win 0x5130fc
void TooltipAugmentation::Refresh()
{
    if (m_manager)
        m_manager->Refresh();
}

// ===========================================================================================
// TooltipManager
// ===========================================================================================

// Mac 0x112500, Win 0x513180
TooltipManager::TooltipManager(ControlManager* manager)
    : ControlManagerAugmentation(String(kTooltipManagerName)),
      m_value3c(0),
      m_showTooltips(true)
{
    ControlManagerAugmentation::Attach(manager);
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    Observer<Preferences, PreferencesObserver>::Attach(preferences.Get());
}

// Mac 0x112910/0x112af0 (body 0x112760), Win 0x513316 (sdd 0x5132ec)
TooltipManager::~TooltipManager()
{
}

// Mac 0x112d30, Win 0x5133c2
void TooltipManager::OnFinalRelease()
{
    Observer<Preferences, PreferencesObserver>::OnFinalRelease();
    Observer<Updater, UpdateObserver>::OnFinalRelease();
    Observer<ControlManager, ControlManagerObserver>::OnFinalRelease();
    ControlManagerAugmentation::OnFinalRelease();
}

// Mac 0x112f30, Win 0x5133e7
void TooltipManager::OnManagerAttached(ControlManager* manager)
{
    Observer<ControlManager, ControlManagerObserver>::Attach(manager);
}

// Mac 0x1131c0, Win 0x5133ef
void TooltipManager::OnManagerDetached(ControlManager* /*manager*/)
{
    Observer<ControlManager, ControlManagerObserver>::Detach();
}

// Mac 0x113310 (the augmentation map of the control, inlined find + dynamic_cast)
bool TooltipManager::FindTooltipAugmentation(Control* control, RefPointer<TooltipAugmentation>& augmentation)
{
    RefPointer<ControlAugmentation> found;
    if (!control->GetAugmentation(String(kTooltipName), found))
        return false;
    augmentation = dynamic_cast<TooltipAugmentation*>(found.Get());
    return augmentation.Get() != 0;
}

// (inlined: Mac 0x111cf0 / 0x113210, Win 0x5135c7)
void TooltipManager::SetAugmentation(TooltipAugmentation* augmentation)
{
    m_augmentation = augmentation;
    m_augmentation->m_manager = this;
    Refresh();
}

// Mac 0x113210 (thunk 0x113420), Win 0x5133fa
void TooltipManager::OnManagerMouseEnter(Control* control, const Vector2& /*position*/)
{
    RefPointer<TooltipAugmentation> augmentation;
    if (FindTooltipAugmentation(control, augmentation))
        SetAugmentation(augmentation.Get());
}

// Mac 0x113440 (thunk 0x113560), Win 0x5134d4
void TooltipManager::OnManagerMouseLeave(Control* /*control*/, const Vector2& /*position*/)
{
    Hide();
}

// Mac 0x113580 (thunk 0x113750), Win 0x5134df
void TooltipManager::OnManagerMouseMove(const Vector2& /*position*/)
{
    if (m_tooltip)
        Position();
}

// Mac 0x113770 (thunk 0x113780), Win 0x5134f2
void TooltipManager::OnDetached(ControlManager* /*manager*/)
{
    Hide();
}

// Mac 0x1137a0 (thunk 0x1138c0), Win 0x5134fd
void TooltipManager::OnTrigger()
{
    Show();
}

// Mac 0x1138e0 (thunk 0x1139d0), Win 0x513505
void TooltipManager::OnPreferenceChanged(const String& key)
{
    if (!key.EqualsNoCase(String(kShowTooltipsKey)))
        return;
    Preferences* preferences = Observer<Preferences, PreferencesObserver>::GetSubject();
    preferences->GetValue(String(kShowTooltipsKey), m_showTooltips);
    if (!m_showTooltips)
        Hide();
}

// Mac 0x1139f0 (thunk 0x113aa0), Win 0x513584
void TooltipManager::OnAttached(Preferences* preferences)
{
    preferences->GetValue(String(kShowTooltipsKey), m_showTooltips);
}

// (inlined: Mac 0x112010, 0x113450, 0x1137b0; Win 0x5136df)
void TooltipManager::DropTooltip()
{
    if (m_tooltip)
        m_tooltip->SetParent(0, 0);
    m_extra.Clear();
    m_tooltip = 0;
}

// Mac 0x112010, Win 0x5135e8
void TooltipManager::Refresh()
{
    if (!m_tooltip) {
        UInt32 delay = m_augmentation->GetDelay();
        if (delay == 0) {
            Show();
            return;
        }
        Trigger::Start(delay);
        return;
    }
    if (m_augmentation->UpdateTooltip(m_tooltip.Get(), m_extra)) {
        Position();
        return;
    }
    m_augmentation->DestroyTooltip(m_tooltip.Get(), m_extra);
    DropTooltip();
}

// Mac 0x1137b0, Win 0x513640
void TooltipManager::Show()
{
    Trigger::Stop();
    if (m_showTooltips && m_augmentation->CreateTooltip(m_tooltip, m_extra) && m_tooltip) {
        // Layer 2 (the sibling after layer 1 below the root).
        ControlManager* manager = Observer<ControlManager, ControlManagerObserver>::GetSubject();
        m_tooltip->SetParent(manager->GetLayer(1)->GetNextSibling(), 0);
        Position();
        m_tooltip->SetShown(true);
        return;
    }
    DropTooltip();
}

// Mac 0x113450 (thunk 0x113560 …), Win 0x51369f
void TooltipManager::Hide()
{
    if (!m_augmentation)
        return;
    Trigger::Stop();
    if (m_tooltip) {
        m_augmentation->DestroyTooltip(m_tooltip.Get(), m_extra);
        DropTooltip();
    }
    m_augmentation->m_manager = 0;
    m_augmentation = 0;
}

// Mac 0x1135a0, Win 0x513705
void TooltipManager::Position()
{
    ControlManager* manager = Observer<ControlManager, ControlManagerObserver>::GetSubject();
    Vector2 mouse(manager->GetMousePosition());
    Vector2 size(m_tooltip->GetSize());
    Rect area(0.0f, 0.0f, 0.0f, 0.0f);
    if (Control* anchor = m_augmentation->m_anchor)
        area.Set(Rect(anchor->GetAbsolutePosition(), anchor->GetSize()));
    else
        area.Set(Rect(manager->GetLayer(1)->GetPosition(), manager->GetLayer(1)->GetSize()));
    Vector2 position(mouse);
    position.y = (Real32)((EvalReal)position.y + 20.0);
    if (area.m_max.x < (EvalReal)mouse.x + size.x)
        position.x = (Real32)((EvalReal)area.m_max.x - size.x);
    // NOTE: original quirk — a tooltip sticking out on the left / top goes to 0, not to the
    // area's edge.
    if (position.x < area.m_min.x)
        position.x = 0.0f;
    if (area.GetHeight() < size.y) {
        position.y = 0.0f;
    } else {
        if (!((EvalReal)size.y + position.y <= area.m_max.y))
            position.y = (Real32)((EvalReal)mouse.y - size.y - 6.0);
        if (position.y < area.m_min.y)
            position.y = 0.0f;
    }
    m_tooltip->SetPosition(position);
}

} // namespace LEGO
