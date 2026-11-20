// LEGO Digital Designer — restored source
// TU 294 "SimpleSoundChangedNotifier.cpp" (Mac 0x2a2660-0x2a344f, Win 0x659a7f-0x65a12a): the
// non-template functions (the rest is template code, e.g. NotifierT::OnFinalRelease 0x2a2f90).
// See SimpleSoundChangedNotifier.h.
#include "SimpleSoundChangedNotifier.h"

namespace LEGO {

// Mac 0x2a2700 (D1), 0x2a27b0 (D0), thunks 0x2a2750 / 0x2a2820; Win 0x659cc3, 0x659e34
SimpleSoundChangedObserver::~SimpleSoundChangedObserver()
{
}

// Mac 0x2a2660, Win 0x659a7f
void SimpleSoundChangedObserver::OnSoundChanged(const RefPointer<SimpleSound>& /*sound*/, bool /*play*/)
{
}

// Mac 0x2a2c90 (D1), 0x2a2d80 (D0), thunks 0x2a2d00 / 0x2a2e00; Win 0x659f98, 0x65a00b
SimpleSoundChangedNotifier::~SimpleSoundChangedNotifier()
{
}

// Mac 0x2a28a0, Win 0x659cd6
void SimpleSoundChangedNotifier::NotifySoundChanged(const RefPointer<SimpleSound>& sound, bool play)
{
    Iterator pass(*this);
    while (SimpleSoundChangedObserver* observer = pass.Next(m_observers)) {
        RefPointer<SimpleSound> copy(sound);
        observer->OnSoundChanged(copy, play);
    }
}

} // namespace LEGO
