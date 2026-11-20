// LEGO Digital Designer — restored source
// BricksControlItemPressedNotifier.cpp — TU 076 (Mac 0x16b6b0-0x16b9ab: one function and the
// <iostream> initialiser; file name inferred). Win 0x6910f8.
#include "BricksControlItemPressedNotifier.h"

namespace LEGO {

// Mac 0x16b6b0, Win 0x6910f8
void BricksControlItemPressedNotifier::NotifyBrickPressed(PrimitiveElementBrick* brick)
{
    Iterator it(*this);
    while (BricksControlItemPressedObserver* observer = it.Next(m_observers))
        observer->OnBrickPressed(brick);
}

} // namespace LEGO
