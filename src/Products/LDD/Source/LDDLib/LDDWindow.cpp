// LEGO Digital Designer — restored source
// LDDWindow.cpp — Windows part of LEGO::LDDWindow (TU 155; the Mac part is LDDWindow.mm).
#if defined(_WIN32)
#include "LDDWindow.h"

namespace LEGO {

// Win 0x5fbcaa
bool LDDWindow::CreateContext()
{
    return OpenGLWindow::CreateContext();
}

} // namespace LEGO
#endif // _WIN32
