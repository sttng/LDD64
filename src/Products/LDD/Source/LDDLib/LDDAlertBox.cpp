// LEGO Digital Designer — restored source
// TU 064 "LDDAlertBox.cpp": Mac 0x150650-0x1523cb, Win 0x64fe15-0x6507ff.
// Not code: construction-vtable / secondary-base thunks 0x151730-0x151d90, the inline
// StretchableButtonControl constructor copy 0x151e10 (C2; StretchableButtonControl.h),
// BasicArray<Control*> helpers 0x152020, 0x152350 = <iostream> + a header static.
#include "LDDAlertBox.h"
#include "LDDButtonControl.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "RenderControlManager.h"
#include "Application.h"
#include "Texture.h"

namespace LEGO {

// Mac 0x1515f0/0x151650, Win sdd 0x5fd92a
LDDAlertBox::~LDDAlertBox()
{
}

// Mac 0x1516c0
void LDDAlertBox::OnFinalRelease()
{
    ObserverN<Control, ControlObserver>::OnFinalRelease();
    ModalControl::OnFinalRelease();
}

// Mac 0x150650, Win 0x6503f8
bool LDDAlertBox::Initialize()
{
    if (!ModalControl::Initialize())
        return false;
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/General/Popup final.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;
    GetContent()->SetSize(Vector2(390.0f, 200.0f));

    m_icon = new RenderControl(m_manager);
    if (!m_icon->Initialize())
        return false;
    m_icon->Setup(GetContent(), 0, 7, Rect(Vector2(10.0f, 10.0f), Vector2()), 0, false);
    m_icon->SetImage(image, 0);

    m_title = new RenderControl(m_manager);
    if (!m_title->Initialize())
        return false;
    m_title->Setup(GetContent(), 0, 7,
                   Rect(79.0f, 10.0f, GetContent()->GetSize().x + -10.0f, 31.0f), 0x80000000u, false);
    m_title->SetTransparent(true);
    m_title->SetFont(LDDApplication::GetInstance()->GetFont(1));

    m_message = new RenderControl(m_manager);
    if (!m_message->Initialize())
        return false;
    m_message->Setup(GetContent(), 0, 7,
                     Rect(79.0f, 55.0f, GetContent()->GetSize().x + -10.0f, 55.0f), 0x80000000u, false);
    m_message->SetTransparent(true);
    m_message->SetFont(LDDApplication::GetInstance()->GetFont(7));

    m_button1 = new LDDButtonControl(m_manager);
    if (!CreateButton(m_button1))
        return false;
    m_button1->SetAlignment(0x33, 0x33, 10.0f, 10.0f, 0);
    m_button2 = new LDDButtonControl(m_manager);
    if (!CreateButton(m_button2))
        return false;
    m_button2->SetAlignment(0x33, 0x31, 10.0f, 0.0f, m_button1);
    m_button3 = new LDDButtonControl(m_manager);
    if (!CreateButton(m_button3))
        return false;
    m_button3->SetAlignment(0x31, 0x31, -10.0f, 10.0f, 0);

    if (!CreateCheckBox(m_manager, GetContent(), Vector2(), String(), 20.0f, m_checkBox,
                        m_checkBoxImage, m_checkBoxLabel))
        return false;
    m_checkBox->SetShown(false);
    m_checkBox->SetAxisAlignment(0, 1, 1, -10.0f, 0);
    m_checkBox->SetAxisAlignment(1, 3, 3, 10.0f, 0);
    return true;
}

// Mac 0x150f70 (Win inlined)
bool LDDAlertBox::CreateButton(LDDButtonControl* button)
{
    bool created = button->Create(String(""), GetContent(), Vector2());
    if (created)
        ControlObserver::Attach(button);
    return created;
}

// Mac 0x151030, Win 0x64fe2d
UInt32 LDDAlertBox::Run(UInt32 style, const String& message, const String& title,
                        const String& button1, const String& button2, const String& button3,
                        bool* dontShowAgain, const String& checkBoxText)
{
    // The icon of the style in Popup final.png (59 x 59).
    Rect iconRect(0.0f, 0.0f, 0.0f, 0.0f);
    switch (style) {
    case 0: iconRect.Set(Rect(Vector2(389.0f,  63.0f), Vector2(59.0f, 59.0f))); break;
    case 1: iconRect.Set(Rect(Vector2(453.0f,   0.0f), Vector2(59.0f, 59.0f))); break;
    case 2: iconRect.Set(Rect(Vector2(389.0f, 126.0f), Vector2(59.0f, 59.0f))); break;
    case 3: iconRect.Set(Rect(Vector2(389.0f,   0.0f), Vector2(59.0f, 59.0f))); break;
    default: break;
    }
    m_icon->SetImageRect(&iconRect);
    m_title->SetText(title);
    m_title->ResizeToFitText();
    m_message->SetPosition(Vector2(m_message->GetPosition().x,
        m_title->GetPosition().y + m_title->GetSize().y + 4.0f));
    m_message->SetText(message);
    m_message->ResizeToFitText();

    m_button1->SetTextForAllStates(button1);
    m_button2->SetShown(!button2.IsEmpty());
    m_button2->SetTextForAllStates(button2);
    m_button3->SetShown(!button3.IsEmpty());
    m_button3->SetTextForAllStates(button3);
    m_button1->ResizeHeightToFitText(300);
    m_button2->ResizeHeightToFitText(300);
    m_button3->ResizeHeightToFitText(300);

    if (dontShowAgain == 0 || checkBoxText.IsEmpty()) {
        m_checkBox->SetShown(false);
    } else {
        m_checkBox->SetTextForAllStates(checkBoxText);
        m_checkBoxLabel->ResizeHeightToFitText(0);
        m_checkBox->FitToChildren(true, true, true, true);
        m_checkBox->SetToggled(*dontShowAgain);
        m_checkBox->SetShown(true);
    }
    m_defaultButton = m_button1.Get();
    m_cancelButton = button2.IsEmpty() ? m_button1.Get() : m_button2.Get();

    Real32 textBottom = m_message->GetPosition().y + m_message->GetSize().y;
    if (textBottom <= 69.0f)
        textBottom = 69.0f;
    GetContent()->SetSize(Vector2(GetContent()->GetSize().x,
                                  textBottom + m_button1->GetSize().y + 20.0f));
    ShowModal();
    Application::GetInstance()->RunModal(m_manager->GetWindow(), 0);
    EndModal();
    if (dontShowAgain != 0 && !checkBoxText.IsEmpty())
        *dontShowAgain = m_checkBox->IsToggled();
    return m_result;
}

// Mac 0x1514d0 (thunk 0x151530), Win 0x6501c9
void LDDAlertBox::OnMouseClicked(Control* control, const Vector2& /*position*/, UInt32 /*button*/)
{
    if (control == m_button1)
        m_result = 0;
    else if (control == m_button2)
        m_result = 1;
    else if (control == m_button3)
        m_result = 2;
    Application::GetInstance()->StopModal();
}

// Mac 0x151580, Win 0x64fe15
void LDDAlertBox::OnAccept()
{
    m_result = 0;
    ModalControl::OnAccept();
    Application::GetInstance()->StopModal();
}

// Mac 0x1515b0, Win 0x650201
void LDDAlertBox::OnCancel()
{
    m_result = m_button2 == m_cancelButton ? 1 : 0;
    ModalControl::OnCancel();
    Application::GetInstance()->StopModal();
}

} // namespace LEGO
