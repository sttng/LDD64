// LEGO Digital Designer — restored source
// TU 194 (Mac 0x2475e0-0x2476af, file name unknown: one function + the iostream initialiser).
// Called only by BrickConnectSceneControl's version dump (Mac 0x3b230). See Countries.h.
#include "Countries.h"

namespace LEGO {

// Mac 0x2475e0
void Countries::Format(String& out) const
{
    ConstIterator it = Begin();
    if (it == End())
        return;
    for (;;) {
        out << it->m_code << "|" << it->m_name;
        ++it;
        if (it == End())
            break;
        if (it != Begin())
            out << ",";
    }
}

} // namespace LEGO
