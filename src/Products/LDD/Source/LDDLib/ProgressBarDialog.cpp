// LEGO Digital Designer — restored source
// LEGO::ProgressBarDialog — see ProgressBarDialog.h. TU 122 (Mac 0x1d9000-0x1da3bb; Win
// 0x5fd572-0x5fd94c, 0x6508f5-0x650d23; static initialiser 0x1da340 = `#include <iostream>`).
#include "ProgressBarDialog.h"
#include "LDDButtonControl.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "RenderControlManager.h"
#include "Texture.h"
#include "TextLocale.h"
#include "Geometry.h"

namespace LEGO {

// Mac 0x1d9960 / 0x1d99c0 (body 0x1da1a0), Win sdd 0x5fd94c
ProgressBarDialog::~ProgressBarDialog()
{
}

// Mac 0x1d9000, Win 0x650abc
bool ProgressBarDialog::Initialize()
{
    if (!ModalControl::Initialize())
        return false;
    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/General/Popup final.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image.Get())
        return false;

    m_icon = new RenderControl(m_manager);
    if (!m_icon->Initialize())
        return false;
    m_icon->Setup(GetContent(), 0, 7, Rect(Vector2(10.0f, 10.0f), Vector2()), 0, false);
    Rect iconRect(Vector2(453.0f, 63.0f), Vector2(59.0f, 59.0f));
    m_icon->SetImage(image, &iconRect);

    m_progressBar = new ProgressBarControl(m_manager);
    if (!m_progressBar->Create(GetContent(), 0))
        return false;

    m_text = new RenderControl(m_manager);
    if (!m_text->Initialize())
        return false;
    m_text->Setup(GetContent(), 0, 7,
                  Rect(Vector2(m_icon->GetPosition().x + m_icon->GetSize().x + 10.0f,
                               m_icon->GetPosition().y),
                       m_progressBar->GetSize()),
                  0x80000000, false);
    m_text->SetTransparent(true);
    m_text->SetFont(LDDApplication::GetInstance()->GetFont(1));

    RefPointer<LDDButtonControl> button = new LDDButtonControl(m_manager);
    m_cancelButton = button.Get();
    if (!AddButton(button))
        return false;
    button->SetTextForAllStates(LocalizedString(String("CANCEL")));
    button->ResizeHeightToFitText(300);
    m_defaultButton = m_cancelButton;
    return true;
}

// Mac 0x1d96b0, Win 0x650a1d
bool ProgressBarDialog::AddButton(LDDButtonControl* button)
{
    bool created = button->Create(String(""), GetContent(), Vector2());
    if (created) {
        ControlObserver::Attach(button);
        button->SetAlignment(0x33, 0x33, 10.0f, 10.0f, 0);
    }
    return created;
}

// Mac 0x1d97a0, Win 0x6508f5
void ProgressBarDialog::SetMessage(const String& message)
{
    m_cancelled = false;
    m_text->SetText(message);
    m_text->ResizeToFitText();
    m_progressBar->SetPosition(Vector2(m_text->GetPosition().x,
                                       m_text->GetPosition().y + m_text->GetSize().y + 4.0f));
    Real32 bottom = m_progressBar->GetPosition().y + m_progressBar->GetSize().y;
    const Real32 iconBottom = m_icon->GetPosition().y + m_icon->GetSize().y;
    if (bottom <= iconBottom)
        bottom = iconBottom;
    GetContent()->SetSize(Vector2(m_text->GetPosition().x + m_text->GetSize().x + 10.0f,
                                  bottom + m_cancelButton->GetSize().y + 20.0f));
    ShowModal();
}

// Mac 0x1d98c0, Win 0x6509ed
void ProgressBarDialog::SetProgress(Real32 value, Real32 maximum)
{
    m_progressBar->SetMaximum(maximum);
    m_progressBar->SetValue(value);
}

// Mac 0x1d9aa0, Win 0x5fd572
bool ProgressBarDialog::IsCancelled() const
{
    return m_cancelled;
}

// Mac 0x1d9900 (thunk 0x1d9920), Win thunk 0x650a0d
void ProgressBarDialog::OnMouseClicked(Control* control, const Vector2&, UInt32)
{
    if (m_cancelButton.Get() == control)
        m_cancelled = true;
}

// Mac 0x1d9940
void ProgressBarDialog::OnAccept()
{
    m_cancelled = true;
}

// Mac 0x1d9950
void ProgressBarDialog::OnCancel()
{
    m_cancelled = true;
}

// Mac 0x1d9a30
void ProgressBarDialog::OnFinalRelease()
{
    ControlObserver::OnFinalRelease();
    ModalControl::OnFinalRelease();
}

} // namespace LEGO
