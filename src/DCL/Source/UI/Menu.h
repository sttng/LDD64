// LEGO Digital Designer — restored source
// LEGO::Menu — a menu (menu bar or submenu): an ordered list of MenuItems. LEGO::MenuItem — a
// separator, a submenu, or a Command with an optional short-cut (key + modifiers) and an optional
// separate short-cut command.
// TU 047 "MenuItem.cpp" (file name guessed from the home classes): Mac 0x111280-0x111c6f, Win
// 0x432bc0-0x43310f. Built by LuaWindow (Mac 0x3670f0) and Mode (Mac 0x196ec0), handed to
// Window::SetMenu (native menu bar: Win HMENU, Mac NSMenu of LEGOMenuItemWrapper items; key
// short-cuts). Directory UI/ is a guess (next to Window).
//
// RTTI (both builds): `virtual Object` base, no other bases.
// Menu     Mac vtable 0x70ad7c {D1 0x111350, D0 0x111420}, Object-in 0x70ad9c (Object +0x14);
//          Win vftable 0xa20044 {sdd 0x432d30, OnFinalRelease, AddRef, Release}, vbptr +0,
//          Object +0x14.
// MenuItem Mac vtable 0x7641bc {D1 0x111910, D0 0x111970}, Object-in 0x7641dc (Object +0x28);
//          Win vftable 0xa20028 {sdd 0x432c70, …}, vbptr +0, Object +0x1c.
// All member names are inferred (no symbols, no strings).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"

namespace LEGO {

class Command;
class MenuItem;

// Short-cut modifier bits of MenuItem::GetModifiers / Window key messages (Win: GetKeyState
// of VK_CONTROL / VK_SHIFT / VK_MENU, Win 0x40ce90; Mac: NSCommandKeyMask / NSShiftKeyMask /
// NSAlternateKeyMask in Window::BuildMenu). (names inferred)
enum KeyModifier {
    kModifierControl = 1,     // Win Ctrl ("MENUCTRL+"), Mac Command
    kModifierShift   = 2,     // "MENUSHIFT+"
    kModifierAlt     = 4      // "MENUALT+", Mac Option
};

// ===========================================================================================
// Menu — Mac sizeof 0x18 (+Object): vptr, BasicArray<RefPointer<MenuItem>> +0x04.
//        Win: vbptr +0, BasicArray +0x04 (vptr +4, begin +8, end +0xc, capacity +0x10).
// ===========================================================================================
class Menu : public virtual Object {
public:
    Menu() {}                                                         // Mac 0x111280, Win 0x432ca0
    virtual ~Menu();                                                  // Mac 0x111350/0x111420, Win 0x432d60 (sdd 0x432d30)

    // Appends an item (inlined into the menu builders, e.g. Mac 0x3670f0). (name inferred)
    void AddItem(MenuItem* item) { m_items.PushBack(RefPointer<MenuItem>(item)); }

    const BasicArray<RefPointer<MenuItem> >& GetItems() const { return m_items; }   // (inlined)

private:
    BasicArray<RefPointer<MenuItem> > m_items;   // Mac +0x04, Win +0x04

    Menu(const Menu&);
    Menu& operator=(const Menu&);
};

// ===========================================================================================
// MenuItem — Mac: vptr, submenu +0x04, command +0x08, short-cut command +0x0c, title String
//            +0x10, key +0x20, modifiers +0x24, Object +0x28.
//            Win: vbptr +0, submenu +0x04, command +0x08, short-cut command +0x0c, title
//            +0x10, key +0x14, modifiers +0x18, Object +0x1c.
// An item without command (or with an empty title) and without submenu is a separator.
// ===========================================================================================
class MenuItem : public virtual Object {
public:
    // Separator.
    MenuItem();                                                       // Mac 0x1115a0, Win 0x432e80
    // Command item without short-cut.
    MenuItem(const String& title, Command* command);                  // Mac 0x1114a0, Win 0x432dc0
    // Command item with the short-cut `key` (LEGO::Key) + `modifiers` (KeyModifier bits); the
    // short-cut runs `shortcutCommand` when given, else `command`. (argument names inferred)
    MenuItem(const String& title, Command* command, UInt32 key, UInt32 modifiers,
             Command* shortcutCommand);                               // Mac 0x111680, Win 0x432f20
    // Submenu.
    MenuItem(const String& title, Menu* submenu);                     // Mac 0x1117a0, Win 0x433010
    virtual ~MenuItem();                                              // Mac 0x111910/0x111970, Win 0x432bc0 (sdd 0x432c70)

    Menu*         GetSubMenu() const { return m_submenu.Get(); }      // Mac 0x1118a0
    Command*      GetCommand() const { return m_command.Get(); }      // Mac 0x1118b0
    // The command a key short-cut runs: the short-cut command if set, else the command.
    Command*      GetShortcutCommand() const                          // Mac 0x1118c0
    {
        return m_shortcutCommand.Get() ? m_shortcutCommand.Get() : m_command.Get();
    }
    const String& GetTitle() const { return m_title; }                // Mac 0x1118e0
    UInt32        GetKey() const { return m_key; }                    // Mac 0x1118f0
    UInt32        GetModifiers() const { return m_modifiers; }        // Mac 0x111900

private:
    RefPointer<Menu>    m_submenu;           // Mac +0x04, Win +0x04
    RefPointer<Command> m_command;           // Mac +0x08, Win +0x08
    RefPointer<Command> m_shortcutCommand;   // Mac +0x0c, Win +0x0c
    String              m_title;             // Mac +0x10, Win +0x10
    UInt32              m_key;               // Mac +0x20, Win +0x14
    UInt32              m_modifiers;         // Mac +0x24, Win +0x18

    MenuItem(const MenuItem&);
    MenuItem& operator=(const MenuItem&);
};

} // namespace LEGO
