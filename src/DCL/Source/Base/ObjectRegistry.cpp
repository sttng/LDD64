// LEGO Digital Designer — restored source
// TU "ObjectRegistry.cpp"  Mac 0xaed0-0xb37f, Win 0x42ce30-0x42d14f
// Get/Register/UnregisterAll are inline in ObjectRegistry.h. Templates emitted here: registry
// map dtors (Mac 0xb220/0xb250, Win 0x42d0d0/0x42d130), _M_erase (Mac 0xb2a0),
// RefPointer<ObjectRegistry> dtor (Mac 0xb050, singleton atexit).
// NOTE: the Mac static initialiser (0xb300) also zero-fills a 24-byte header-defined static
// (0x01000000 in the third dword pair) found in most TUs; not part of this module.
#include "ObjectRegistry.h"

namespace LEGO {

// Mac 0xaf70 (complete-object ctor), Win 0x42ce30
ObjectRegistry::ObjectRegistry()
{
}

// Mac 0xb080 (D1), 0xb110 (D0), virtual thunks 0xb180/0xb1a0; Win 0x42ced0, 0x42cf60 (sdd),
// 0x42d410 (vtordisp thunk)
ObjectRegistry::~ObjectRegistry()
{
}

// Mac 0xaed0, Win 0x42cf90
ObjectRegistry* ObjectRegistry::GetInstance()
{
    static RefPointer<ObjectRegistry> s_instance(new ObjectRegistry);
    return s_instance;
}

// Mac 0xb150 (virtual thunk 0xb1e0), Win 0x42cf50 (vtordisp thunk 0x42d400)
void ObjectRegistry::OnFinalRelease()
{
    UnregisterAll();
    Object::OnFinalRelease();
}

} // namespace LEGO
