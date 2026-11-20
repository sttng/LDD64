// LEGO Digital Designer — restored source (Windows only)
// LEGO::PreferenceControlDropDown — see PreferenceControlDropDown.h (Win 0x651619-0x651869,
// 0x68a328-0x68a89d). Nothing is compiled on the Mac branch (no such class in LDD_mac).
#include "PreferenceControlDropDown.h"

#if defined(_WIN32)
#include "Control.h"
#include "Preferences.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "StretchableButtonControl.h"

namespace LEGO {

// Win 0x651619
PreferenceControlDropDown::PreferenceControlDropDown(ControlManager* manager, const String& key,
                                                     const String& label,
                                                     const BasicArray<Pair<String, String> >& items,
                                                     bool needsRestart)
    : PreferenceControl(manager, needsRestart), m_key(key), m_label(label), m_items(items),
      m_changed(false)
{
}

// Win 0x65177d (sdd 0x651869)
PreferenceControlDropDown::~PreferenceControlDropDown()
{
}

// Win 0x68a89d
bool PreferenceControlDropDown::Create(Control* parent)
{
    if (!RenderControl::Initialize())
        return false;
    Setup(parent, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    if (!CreateLabel() || !CreateDropDown(parent))
        return false;
    SetTransparent(true);
    return true;
}

// Win 0x68a396
bool PreferenceControlDropDown::CreateLabel()
{
    m_labelControl = new RenderControl(m_manager);
    if (!m_labelControl->Initialize())
        return false;
    m_labelControl->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_labelControl->SetFont(LDDApplication::GetInstance()->GetFont(3));
    m_labelControl->SetText(m_label);
    m_labelControl->ResizeHeightToFitText(135);
    m_labelControl->SetPosition(Vector2(0.0f, 0.0f));
    m_labelControl->SetAxisAlignment(1, 2, 2, 0.0f, 0);
    m_labelControl->SetTransparent(true);
    const Vector2 size(GetSize());
    const Real32 height = m_labelControl->GetSize().y;
    const Real32 right = m_labelControl->GetSize().x + m_labelControl->GetPosition().x;
    SetSize(Vector2(size.x < right ? right : size.x, size.y < height ? height : size.y));
    return true;
}

// Win 0x68a69e
bool PreferenceControlDropDown::CreateDropDown(Control* popupParent)
{
    if (m_items.GetSize() == 0)
        return false;
    m_dropDown = new DropDownControl(m_manager, true);
    if (!m_dropDown->Create(this, 0, Vector2(165.0f, 0.0f)))
        return false;
    {
        BasicArray<RefPointer<Control> > controls;
        for (UInt32 i = 0; i < m_items.GetSize(); ++i) {
            RefPointer<Control> item = CreateDropDownItem(m_items[i].second, m_dropDown.Get(),
                                                          m_manager);
            controls.PushBack(item);
        }
        m_dropDown->AddItems(controls);
    }
    m_dropDown->SetPopupParent(popupParent);
    DropDownControlObserver::Attach(m_dropDown.Get());
    String value;
    Preferences::GetInstance()->GetValue(m_key, value);
    SelectValue(value);
    const Vector2 size(GetSize());
    const Real32 height = m_dropDown->GetSize().y;
    const Real32 right = m_dropDown->GetSize().x + m_dropDown->GetPosition().x;
    SetSize(Vector2(size.x < right ? right : size.x, size.y < height ? height : size.y));
    return true;
}

// Win 0x68a4f4
void PreferenceControlDropDown::SelectValue(String value)
{
    UInt32 index = 0;
    for (UInt32 i = 0; i < m_items.GetSize(); ++i) {
        if (value == m_items[i].first)
            index = i;
    }
    m_dropDown->SelectItem(index);
}

// Win 0x68a568
void PreferenceControlDropDown::OnItemSelected(Control*, UInt32 index)
{
    m_dropDown->SetText(m_items[index].second);
    m_value = m_items[index].first;
    m_changed = m_remembered != m_value;
    Store();
}

// Win 0x68a5b4
void PreferenceControlDropDown::Revert()
{
    SelectValue(m_remembered);
}

// Win 0x65176e
bool PreferenceControlDropDown::IsChanged() const
{
    return m_changed;
}

// Win 0x68a346
void PreferenceControlDropDown::Store()
{
    Preferences::GetInstance()->SetValue(m_key, m_value);
}

// Win 0x68a328
void PreferenceControlDropDown::Remember()
{
    m_remembered = m_value;
    m_changed = false;
}

// Win 0x68a5e9
void PreferenceControlDropDown::ResetToDefault()
{
    Preferences::GetInstance()->RemoveValue(m_key);
    String value;
    Preferences::GetInstance()->GetValue(m_key, value);
    SelectValue(value);
}

void PreferenceControlDropDown::OnFinalRelease()
{
    DropDownControlObserver::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

} // namespace LEGO
#endif
