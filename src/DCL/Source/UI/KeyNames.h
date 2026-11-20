// LEGO Digital Designer — restored source
// LEGO::StringToKey — key names <-> LEGO::Key codes for the Lua layer: LuaControlManager (TU 378)
// maps Lua key names to keys (GetKey), LuaControlManagerObserver (TU 319) passes key names to
// OnManagerKeyPressed / OnManagerKeyReleased (GetName).
// TU 382 "KeyNames.cpp" (name inferred): Mac 0x36b360-0x371b25, Win 0x536930 (ctor) and
// 0x537a70-0x537bff. One static instance (Mac 0x77f1d0, Win 0xb43fe0) built by the TU's static
// initialiser (Mac 0x371ac0).
// RTTI: BasicMap<char const*, Key, StringToKey::Pred> (Mac vtable 0x778588, Win sdd 0x537bc0)
// and BasicMap<Key, char const*, Less<Key> > (Mac 0x7785a0, Win sdd 0x537be0); the class holding
// both is assumed to be StringToKey (name from the nested Pred). Other names inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicMap.h"
#include "Key.h"

namespace LEGO {

class StringToKey {
public:
    // Case-insensitive name order (StrICmp, Mac 0x10b0b0).
    struct Pred {
        bool operator()(const char* a, const char* b) const;
    };

    typedef BasicMap<const char*, Key, Pred> NameMap;   // name -> key
    typedef BasicMap<Key, const char*>       KeyMap;    // key -> name

    // Fills the name table (119 names, some keys under two: "Minus" / "-" …); each key keeps its
    // longest name for GetName (equal lengths: the last in Pred order). Mac 0x36b4e0, Win 0x536930
    StringToKey();
    // Mac 0x36b360 (both maps), Win: map dtors 0x537a70 / 0x537b60
    ~StringToKey();

    // The key called `name` (case-insensitive); kKeyUndefined for unknown names.
    // Mac 0x36b3f0 (inlined on Win)
    static Key GetKey(const char* name);
    // The name of `key`; null for keys without a name. Mac 0x36b480 (inlined on Win)
    static const char* GetName(Key key);

    const NameMap& GetNameMap() const { return m_names; }
    const KeyMap&  GetKeyMap() const { return m_keys; }

private:
    NameMap m_names;   // Mac +0x00 (Win +0x00)
    KeyMap  m_keys;    // Mac +0x1c (Win +0x24)
};

} // namespace LEGO
