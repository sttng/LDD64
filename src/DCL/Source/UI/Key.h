// LEGO Digital Designer — restored source
// LEGO::Key — the platform-independent key code of the window's key messages (Window::HandleKey /
// HumanInput::OnKeyDown, MenuItem short-cuts, Window::IsKeyPressed). Values are DirectInput scan
// codes (DIK_*): Win maps virtual keys to them (table Win 0x40e770, Window.cpp), Mac maps them to
// key-equivalent characters (KeyToUniCharMapClass / KeyAndShiftToUniCharMapClass, WindowMac.mm;
// TU 154's key maps). Type name from the RTTI of BasicMap<LEGO::Key, unsigned short, ...>;
// enumerator names from the Lua key table strings (TU 382: "Escape", "Minus", … "NumpadEnter",
// "PageUp", "LeftArrow" …). Values unused by Window code are listed for completeness.
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

enum Key {
    kKeyUndefined     = 0x00,
    kKeyEscape        = 0x01,
    kKey1 = 0x02, kKey2 = 0x03, kKey3 = 0x04, kKey4 = 0x05, kKey5 = 0x06,
    kKey6 = 0x07, kKey7 = 0x08, kKey8 = 0x09, kKey9 = 0x0a, kKey0 = 0x0b,
    kKeyMinus         = 0x0c,
    kKeyEqual         = 0x0d,
    kKeyBackspace     = 0x0e,
    kKeyTab           = 0x0f,
    kKeyQ = 0x10, kKeyW = 0x11, kKeyE = 0x12, kKeyR = 0x13, kKeyT = 0x14,
    kKeyY = 0x15, kKeyU = 0x16, kKeyI = 0x17, kKeyO = 0x18, kKeyP = 0x19,
    kKeyLeftBracket   = 0x1a,
    kKeyRightBracket  = 0x1b,
    kKeyReturn        = 0x1c,
    kKeyControl       = 0x1d,
    kKeyA = 0x1e, kKeyS = 0x1f, kKeyD = 0x20, kKeyF = 0x21, kKeyG = 0x22,
    kKeyH = 0x23, kKeyJ = 0x24, kKeyK = 0x25, kKeyL = 0x26,
    kKeySemicolon     = 0x27,
    kKeyApostrophe    = 0x28,
    kKeyGrave         = 0x29,
    kKeyShift         = 0x2a,
    kKeyBackslash     = 0x2b,
    kKeyZ = 0x2c, kKeyX = 0x2d, kKeyC = 0x2e, kKeyV = 0x2f, kKeyB = 0x30,
    kKeyN = 0x31, kKeyM = 0x32,
    kKeyComma         = 0x33,
    kKeyPeriod        = 0x34,
    kKeySlash         = 0x35,
    kKeyRightShift    = 0x36,
    kKeyNumpadMultiply = 0x37,
    kKeyAlt           = 0x38,
    kKeySpace         = 0x39,
    kKeyCapsLock      = 0x3a,
    kKeyF1 = 0x3b, kKeyF2 = 0x3c, kKeyF3 = 0x3d, kKeyF4 = 0x3e, kKeyF5 = 0x3f,
    kKeyF6 = 0x40, kKeyF7 = 0x41, kKeyF8 = 0x42, kKeyF9 = 0x43, kKeyF10 = 0x44,
    kKeyNumLock       = 0x45,
    kKeyScrollLock    = 0x46,
    kKeyNumpad7 = 0x47, kKeyNumpad8 = 0x48, kKeyNumpad9 = 0x49,
    kKeyNumpadMinus   = 0x4a,
    kKeyNumpad4 = 0x4b, kKeyNumpad5 = 0x4c, kKeyNumpad6 = 0x4d,
    kKeyNumpadPlus    = 0x4e,
    kKeyNumpad1 = 0x4f, kKeyNumpad2 = 0x50, kKeyNumpad3 = 0x51, kKeyNumpad0 = 0x52,
    kKeyNumpadPeriod  = 0x53,
    kKeyF11 = 0x57, kKeyF12 = 0x58,
    kKeyF13 = 0x64, kKeyF14 = 0x65, kKeyF15 = 0x66,
    kKeyKana          = 0x70,
    kKeyConvert       = 0x79,
    kKeyNoConvert     = 0x7b,
    kKeyNumpadEqual   = 0x8d,
    kKeyCircumflex    = 0x90,
    kKeyAt            = 0x91,
    kKeyColon         = 0x92,
    kKeyUnderline     = 0x93,
    kKeyKanji         = 0x94,
    kKeyNumpadEnter   = 0x9c,
    kKeyNumpadComma   = 0xb3,
    kKeyNumpadDivide  = 0xb5,
    kKeyHome          = 0xc7,
    kKeyUpArrow       = 0xc8,
    kKeyPageUp        = 0xc9,
    kKeyLeftArrow     = 0xcb,
    kKeyRightArrow    = 0xcd,
    kKeyEnd           = 0xcf,
    kKeyDownArrow     = 0xd0,
    kKeyPageDown      = 0xd1,
    kKeyInsert        = 0xd2,
    kKeyDelete        = 0xd3,
    kKeyLeftWindows   = 0xdb,
    kKeyRightWindows  = 0xdc,
    kKeyApps          = 0xdd
};

} // namespace LEGO
