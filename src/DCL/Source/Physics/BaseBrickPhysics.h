// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysics — engine-independent IUnknown part of the physics factory; derived by
// VortexBrickPhysics (Mac TU 422), which implements slots 3-22.
// BaseBrickPhysics.cpp (Mac TU 203, 0x251020-0x2511af; Win 0x5c4cb0-0x5c4d4f). Ctor inline
// (Mac 0x251100, emitted in this TU; Win inlined into VortexBrickPhysics).

// RTTI: class LEGO::BaseBrickPhysics : BrickPhysics : IUnknown.
//   Mac sizeof 8, vtable 0x7365a0 {QI 0x251020, AddRef 0x2510a0, Release 0x2510c0, 20 pure,
//       D1 0x251140, D0 0x251160}.
//   Win vtable 0xa2d4bc {0x5c4cb0, 0x5c7750, 0x5c4cf0, 20 pure, sdd 0x5c4d20}.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "BrickPhysics.h"

namespace LEGO {

class BaseBrickPhysics : public BrickPhysics {
public:
    BaseBrickPhysics() : m_refCount(1) {}                   // Mac 0x251100 (Win inlined)
    virtual ~BaseBrickPhysics() {}                          // Mac D1 0x251140 / D0 0x251160, Win sdd 0x5c4d20

    // Answers BrickPhysics::GetIID() only. Mac 0x251020, Win 0x5c4cb0
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x2510a0, Win 0x5c7750
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x2510c0, Win 0x5c4cf0 (deletes at 0)

    volatile Int32 m_refCount;                              // +0x04 (1 at construction)

private:
    BaseBrickPhysics(const BaseBrickPhysics&);
    BaseBrickPhysics& operator=(const BaseBrickPhysics&);
};

// Removes `value` from a pointer array by moving the last element into its place (order not
// kept); returns the position (End() when not found). Instances: Mac 0x2522a0
// (BaseBrickPhysicsConnectionHelper*), 0x253d90 (BaseBrickPhysicsPrimitiveElement*), 0x255530
// (BaseBrickPhysicsRigidElement*), 0x25a380 (BaseBrickPhysicsSnapshot*); Win 0x53e6c0 (one ICF
// body, after an inlined find). (name inferred: std::find + "erase by swap")
template <class T>
T** PhysicsArrayRemove(BasicArray<T*>& array, T* value)
{
    T** it = array.Begin();
    while (it != array.End() && *it != value)
        ++it;
    if (it != array.End()) {
        T** last = array.End() - 1;
        if (last != it)
            *it = *last;
        array.PopBack();
    }
    return it;
}

} // namespace LEGO
