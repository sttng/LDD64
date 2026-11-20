// LEGO Digital Designer — restored source
// TU 289 "SlidingHideAugmentation.cpp".
#include "SlidingHideAugmentation.h"
#include "Control.h"
#include "SystemTime.h"

namespace LEGO {

// Mac 0x29d1c0 (C2)
SlidingHideAugmentation::SlidingHideAugmentation()
    : m_alignment(1), m_axis(0), m_wasVisible(false), m_progress(0.0f), m_startTime(0),
      m_endTime(0)
{
}

// Mac 0x29dd80 / 0x29df00, Win 0x5157c8 / 0x5155f4
SlidingHideAugmentation::~SlidingHideAugmentation()
{
}

// Mac 0x29d2d0, Win 0x51561e
bool SlidingHideAugmentation::Attach(Control* control, const String& name)
{
    control->SetAxisAlignment(m_axis, m_alignment, m_alignment, 0.0f, 0);
    return ControlAugmentation::Attach(control, name);
}

// Mac 0x29d350, Win 0x51564d
void SlidingHideAugmentation::Detach()
{
    Stop();
    ControlAugmentation::Detach();
}

// Mac 0x29d390, Win 0x51565d
void SlidingHideAugmentation::Start(UInt64 duration)
{
    if (UpdateObserver::GetSubject())
        return;
    if (!GetControl())
        return;
    m_wasVisible = GetControl()->GetStatus().m_visible;
    GetControl()->SetShown(true);
    UpdateObserver::Attach(MainUpdater::GetInstance().Get());
    // NOTE: read after SetShown(true): 1 only when a parent keeps the control invisible.
    m_progress = GetControl()->GetStatus().m_visible ? 0.0f : 1.0f;
    m_startTime = (UInt64)GetMilliseconds();
    m_endTime = m_startTime + duration;

    NotifierT<SlidingHideAugmentation, SlidingHideAugmentationObserver>::Iterator it(*this);
    while (SlidingHideAugmentationObserver* observer = it.Next(m_observers))
        observer->OnSlideStarted(GetControl());
}

// Mac 0x29d930, Win 0x515747
void SlidingHideAugmentation::Stop()
{
    if (!GetControl())
        return;
    m_progress = m_wasVisible ? 1.0f : 0.0f;
    ApplyProgress();
    UpdateObserver::Detach();

    NotifierT<SlidingHideAugmentation, SlidingHideAugmentationObserver>::Iterator it(*this);
    while (SlidingHideAugmentationObserver* observer = it.Next(m_observers))
        observer->OnSlideStopped(GetControl());
}

// Mac 0x29dc00, Win 0x5158d1
void SlidingHideAugmentation::ApplyProgress()
{
    const Vector2& size = GetControl()->GetSize();
    Real32 offset = m_progress * (m_axis == 0 ? size.x : size.y);
    if (m_progress == 1.0f && m_wasVisible)
        GetControl()->SetShown(false);
    if (m_alignment != 1)
        offset = -offset;
    GetControl()->SetAxisAlignment(m_axis, m_alignment, m_alignment, offset, 0);
}

// Mac 0x29dcb0, Win 0x5157c0
bool SlidingHideAugmentation::IsSliding() const
{
    return UpdateObserver::GetSubject() != 0;
}

// Mac 0x29e240, Win 0x515844
void SlidingHideAugmentation::Update()
{
    UInt64 now = (UInt64)GetMilliseconds();
    if (now > m_endTime) {
        Stop();
        return;
    }
    // NOTE: both differences are taken as 32-bit unsigned values (Mac and Win alike).
    Real32 t = (Real32)(UInt32)(now - m_startTime) / (Real32)(UInt32)(m_endTime - m_startTime);
    if (t >= 1.0f)
        t = 1.0f;
    m_progress = t;
    if (!m_wasVisible)
        m_progress = 1.0f - t;
    ApplyProgress();
}

// Mac 0x29e0d0, Win 0x515b6d
void SlidingHideAugmentation::OnFinalRelease()
{
    Notifier<SlidingHideAugmentationObserver>::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
    ControlAugmentation::OnFinalRelease();
}

} // namespace LEGO
