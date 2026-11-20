// LEGO Digital Designer — restored source
// TU 373 "LDDPostProcessor.cpp" (Mac 0x361940-0x362e2b; Win 0x658568-0x659a60). See
// LDDPostProcessor.h. The template instances of the TU — BasicArray<RefPointer<
// PostProcessingEffect> >::InsertRange (Mac 0x361e70, Win 0x65895f), BasicMap<String,
// RefPointer<Control> > (dtor Mac 0x362410 / 0x362730, node erase 0x362780, operator[]
// 0x362800 with its insert helpers 0x362940-0x362d10) — come from BasicArray.h / BasicMap.h.
#include "LDDPostProcessor.h"

#include "BrickConnectSceneControl.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "LDDWindowControl.h"
#include "StretchableButtonControl.h"
#include "RenderControlManager.h"
#include "PostProcessingEffect.h"
#include "Notifier.h"

namespace LEGO {

// Mac 0x361940
LDDPostProcessor::LDDPostProcessor(BrickConnectSceneControl* control, RenderControlManager* manager)
    : m_control(control), m_manager(manager)
{
}

// Mac 0x362440 / 0x362500 (body 0x362240), Win sdd 0x659a60
LDDPostProcessor::~LDDPostProcessor()
{
}

// Mac 0x361a50, Win 0x65962b
bool LDDPostProcessor::Initialize()
{
    m_dropDown = new DropDownControl(m_manager, false);
    if (!m_dropDown->Create(m_control, 0, Vector2(15.0f, 15.0f)))
        return false;
    m_window = new LDDWindowControl(m_manager);
    // NOTE (both builds): the result of the window's Create is not checked.
    m_window->Create(true, false);
    m_window->SetTitle(String("Post Processing Techniques"));
    m_window->SetContent(m_dropDown);
    m_window->SetContentSize(Vector2(m_dropDown->GetMinimumSize()));
    m_window->Setup(m_control, 0, 6, Rect(40.0f, 10.0f, 210.0f, 60.0f), 0, false);
    m_dropDown->SetPopupParent(m_control);
    Attach(m_dropDown);

    LDDApplication* application = LDDApplication::GetInstance();
    m_effects.InsertRange(m_effects.Begin(), application->GetPostProcessingEffects().Begin(),
                          LDDApplication::GetInstance()->GetPostProcessingEffects().End());
    m_effects.InsertRange(m_effects.End(), LDDApplication::GetInstance()->GetPostProcessingExtraEffects().Begin(),
                          LDDApplication::GetInstance()->GetPostProcessingExtraEffects().End());
    for (BasicArray<RefPointer<PostProcessingEffect> >::Iterator it = m_effects.Begin();
         it != m_effects.End(); ++it) {
        const String name = (*it)->GetName();
        RefPointer<Control> item = CreateDropDownItem(name, m_dropDown, m_manager);
        m_dropDown->AddItem(item);
        m_items[name] = item;
    }
    return true;
}

// Mac 0x3621e0
void LDDPostProcessor::SetShown(bool shown, const String& effect)
{
    m_dropDown->SetText(effect);
    m_window->SetShown(shown);
}

// Mac 0x362220
bool LDDPostProcessor::IsShown() const
{
    return m_window->GetStatus().m_visible;
}

// Mac 0x3625f0, Win 0x6592b3
void LDDPostProcessor::OnFinalRelease()
{
    for (UInt32 i = 0; i < m_subjects.GetSize(); ++i)
        static_cast<NotifierT<DropDownControl, DropDownControlObserver>*>(m_subjects[i])
            ->RemoveObserver(this);
    m_subjects.Clear();
    Object::OnFinalRelease();
}

// Mac 0x3626b0, Win 0x658568
void LDDPostProcessor::OnItemSelected(Control* item, UInt32 index)
{
    (void)item;
    m_dropDown->SetText(m_effects[index]->GetName());
    m_control->SetPostProcessor(m_effects[index]);
}

} // namespace LEGO
