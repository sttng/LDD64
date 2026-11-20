// LEGO Digital Designer — restored source
// TU 047 "MenuItem.cpp": LEGO::Menu and LEGO::MenuItem (Mac 0x111280-0x111c6f, Win
// 0x432bc0-0x43310f). Compiler-generated, not written out: the BasicArray<RefPointer<MenuItem> >
// dtor (Mac 0x1112e0 / 0x111ac0, Win 0x4330e0), the virtual-base thunks Mac 0x111400 / 0x111460 /
// 0x1119e0 / 0x111a40, the base-object dtor body Mac 0x111b40, the <iostream> initialiser Mac
// 0x111c30.
#include "Menu.h"
#include "Command.h"

namespace LEGO {

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------

// Mac 0x111350 (D1) / 0x111420 (D0); Win 0x432d60, sdd 0x432d30: releases the items.
Menu::~Menu()
{
}

// ---------------------------------------------------------------------------
// MenuItem
// ---------------------------------------------------------------------------

// Mac 0x1115a0, Win 0x432e80
MenuItem::MenuItem()
    : m_key(0), m_modifiers(0)
{
}

// Mac 0x1114a0, Win 0x432dc0
MenuItem::MenuItem(const String& title, Command* command)
    : m_command(command), m_title(title), m_key(0), m_modifiers(0)
{
}

// Mac 0x111680, Win 0x432f20 (Win: shortcutCommand in EBX)
// NOTE: the Mac build uses `command` without a null check (Win checks); a null command would
// crash the Mac original.
MenuItem::MenuItem(const String& title, Command* command, UInt32 key, UInt32 modifiers,
                   Command* shortcutCommand)
    : m_command(command), m_shortcutCommand(shortcutCommand), m_title(title), m_key(key),
      m_modifiers(modifiers)
{
}

// Mac 0x1117a0, Win 0x433010
// NOTE: as above, Mac adds the reference to `submenu` without a null check.
MenuItem::MenuItem(const String& title, Menu* submenu)
    : m_submenu(submenu), m_title(title), m_key(0), m_modifiers(0)
{
}

// Mac 0x111910 (D1) / 0x111970 (D0), base body 0x111b40; Win 0x432bc0, sdd 0x432c70:
// title, then short-cut command, command, submenu released (reverse member order).
MenuItem::~MenuItem()
{
}

} // namespace LEGO
