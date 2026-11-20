// LEGO Digital Designer — restored source
// TU 382 "KeyNames.cpp" (name inferred): the key-name table.
#include "KeyNames.h"
#include "LEGO/StdLib.h"

namespace LEGO {

// The one table (Mac 0x77f1d0, Win 0xb43fe0); static initialiser Mac 0x371ac0.
static StringToKey s_stringToKey;

bool StringToKey::Pred::operator()(const char* a, const char* b) const
{
    return StrICmp(a, b) < 0;
}

// Mac 0x36b4e0 (22 KB of inlined map::operator[]), Win 0x536930
StringToKey::StringToKey()
{
    // NOTE: key 0x1d (kKeyControl) is called "Command" on both platforms.
    m_names["Undefined"] = kKeyUndefined;
    m_names["Escape"] = kKeyEscape;
    m_names["1"] = kKey1;
    m_names["2"] = kKey2;
    m_names["3"] = kKey3;
    m_names["4"] = kKey4;
    m_names["5"] = kKey5;
    m_names["6"] = kKey6;
    m_names["7"] = kKey7;
    m_names["8"] = kKey8;
    m_names["9"] = kKey9;
    m_names["0"] = kKey0;
    m_names["Minus"] = kKeyMinus;
    m_names["-"] = kKeyMinus;
    m_names["Equal"] = kKeyEqual;
    m_names["="] = kKeyEqual;
    m_names["Backspace"] = kKeyBackspace;
    m_names["Tab"] = kKeyTab;
    m_names["Q"] = kKeyQ;
    m_names["W"] = kKeyW;
    m_names["E"] = kKeyE;
    m_names["R"] = kKeyR;
    m_names["T"] = kKeyT;
    m_names["Y"] = kKeyY;
    m_names["U"] = kKeyU;
    m_names["I"] = kKeyI;
    m_names["O"] = kKeyO;
    m_names["P"] = kKeyP;
    m_names["LeftBracket"] = kKeyLeftBracket;
    m_names["["] = kKeyLeftBracket;
    m_names["RightBracket"] = kKeyRightBracket;
    m_names["]"] = kKeyRightBracket;
    m_names["Return"] = kKeyReturn;
    m_names["Command"] = kKeyControl;
    m_names["A"] = kKeyA;
    m_names["S"] = kKeyS;
    m_names["D"] = kKeyD;
    m_names["F"] = kKeyF;
    m_names["G"] = kKeyG;
    m_names["H"] = kKeyH;
    m_names["J"] = kKeyJ;
    m_names["K"] = kKeyK;
    m_names["L"] = kKeyL;
    m_names["Semicolon"] = kKeySemicolon;
    m_names[";"] = kKeySemicolon;
    m_names["Apostrophe"] = kKeyApostrophe;
    m_names["'"] = kKeyApostrophe;
    m_names["Grave"] = kKeyGrave;
    m_names["`"] = kKeyGrave;
    m_names["Shift"] = kKeyShift;
    m_names["Backslash"] = kKeyBackslash;
    m_names["\\"] = kKeyBackslash;
    m_names["Z"] = kKeyZ;
    m_names["X"] = kKeyX;
    m_names["C"] = kKeyC;
    m_names["V"] = kKeyV;
    m_names["B"] = kKeyB;
    m_names["N"] = kKeyN;
    m_names["M"] = kKeyM;
    m_names["Comma"] = kKeyComma;
    m_names[","] = kKeyComma;
    m_names["Period"] = kKeyPeriod;
    m_names["."] = kKeyPeriod;
    m_names["Slash"] = kKeySlash;
    m_names["/"] = kKeySlash;
    m_names["NumpadMultiply"] = kKeyNumpadMultiply;
    m_names["Alt"] = kKeyAlt;
    m_names["Space"] = kKeySpace;
    m_names["CapsLock"] = kKeyCapsLock;
    m_names["F1"] = kKeyF1;
    m_names["F2"] = kKeyF2;
    m_names["F3"] = kKeyF3;
    m_names["F4"] = kKeyF4;
    m_names["F5"] = kKeyF5;
    m_names["F6"] = kKeyF6;
    m_names["F7"] = kKeyF7;
    m_names["F8"] = kKeyF8;
    m_names["F9"] = kKeyF9;
    m_names["F10"] = kKeyF10;
    m_names["NumLock"] = kKeyNumLock;
    m_names["ScrollLock"] = kKeyScrollLock;
    m_names["Numpad7"] = kKeyNumpad7;
    m_names["Numpad8"] = kKeyNumpad8;
    m_names["Numpad9"] = kKeyNumpad9;
    m_names["NumpadMinus"] = kKeyNumpadMinus;
    m_names["Numpad4"] = kKeyNumpad4;
    m_names["Numpad5"] = kKeyNumpad5;
    m_names["Numpad6"] = kKeyNumpad6;
    m_names["NumpadPlus"] = kKeyNumpadPlus;
    m_names["Numpad1"] = kKeyNumpad1;
    m_names["Numpad2"] = kKeyNumpad2;
    m_names["Numpad3"] = kKeyNumpad3;
    m_names["Numpad0"] = kKeyNumpad0;
    m_names["NumpadPeriod"] = kKeyNumpadPeriod;
    m_names["F11"] = kKeyF11;
    m_names["F12"] = kKeyF12;
    m_names["F13"] = kKeyF13;
    m_names["F14"] = kKeyF14;
    m_names["F15"] = kKeyF15;
    m_names["NumpadEqual"] = kKeyNumpadEqual;
    m_names["At"] = kKeyAt;
    m_names["@"] = kKeyAt;
    m_names["Colon"] = kKeyColon;
    m_names[":"] = kKeyColon;
    m_names["Underline"] = kKeyUnderline;
    m_names["_"] = kKeyUnderline;
    m_names["NumpadEnter"] = kKeyNumpadEnter;
    m_names["NumpadComma"] = kKeyNumpadComma;
    m_names["NumpadDivide"] = kKeyNumpadDivide;
    m_names["Home"] = kKeyHome;
    m_names["UpArrow"] = kKeyUpArrow;
    m_names["PageUp"] = kKeyPageUp;
    m_names["LeftArrow"] = kKeyLeftArrow;
    m_names["RightArrow"] = kKeyRightArrow;
    m_names["End"] = kKeyEnd;
    m_names["DownArrow"] = kKeyDownArrow;
    m_names["PageDown"] = kKeyPageDown;
    m_names["Insert"] = kKeyInsert;
    m_names["Delete"] = kKeyDelete;

    for (NameMap::ConstIterator it = m_names.Begin(); it != m_names.End(); ++it) {
        KeyMap::Iterator found = m_keys.Find(it->second);
        if (found == m_keys.End() || StrLen(found->second) <= StrLen(it->first))
            m_keys[it->second] = it->first;
    }
}

// Mac 0x36b360
StringToKey::~StringToKey()
{
}

// Mac 0x36b3f0
Key StringToKey::GetKey(const char* name)
{
    NameMap::ConstIterator it = s_stringToKey.m_names.Find(name);
    if (it == s_stringToKey.m_names.End())
        return kKeyUndefined;
    return it->second;
}

// Mac 0x36b480
const char* StringToKey::GetName(Key key)
{
    KeyMap::ConstIterator it = s_stringToKey.m_keys.Find(key);
    if (it == s_stringToKey.m_keys.End())
        return 0;
    return it->second;
}

} // namespace LEGO
