// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsPrimitiveElement — the engine-independent part of a primitive element
// (the collision geometry of one bone): IUnknown, the owning body, the user data and the mass
// properties. VortexPhysicsPrimitiveElement (Mac TU 433) implements slots 3-5 and 9-11.
// BaseBrickPhysicsPrimitiveElement.cpp (Mac TU 209, 0x253790-0x25390f; Win 0x5c7710 QI, 0x5c7750
// AddRef (ICF), 0x5c7770 Release, 0x52f4c0 / 0x46ee40 / 0x52aee0 (ICF accessors), sdd 0x52af00).
// Ctor inline (Mac: into the VortexPhysicsPrimitiveElement ctor 0x3a9600).

// RTTI: class LEGO::BaseBrickPhysicsPrimitiveElement : PhysicsPrimitiveElement : IUnknown.
//   Mac vtable 0x736828 {0x253790, 0x253810, 0x253830, 3 pure, 0x253870, 0x253880, 0x253890,
//       3 pure, D1 0x2538a0, D0 0x2538c0}.
//   Win vtable 0xa2a344 {0x5c7710, 0x5c7750, 0x5c7770, 3 pure, 0x52f4c0, 0x46ee40, 0x52aee0,
//       3 pure, sdd 0x52af00}.
#pragma once
#include "LEGO/Types.h"
#include "BrickPhysics.h"

namespace LEGO {

class BaseBrickPhysicsRigidElement;

class BaseBrickPhysicsPrimitiveElement : public PhysicsPrimitiveElement {
public:
    // Mac inlined into 0x3a9600 (refcount 1, no body / user data, a copy of `properties`)
    // (the former m_unknown64[7] / m_unknown80 (+0x64 / +0x80, "0") are the
    // VortexShapeCollisionFilter base of VortexPhysicsPrimitiveElement at +0x64; this class is 0x64
    // bytes, RTTI offset 100 of the second base.)
    explicit BaseBrickPhysicsPrimitiveElement(const PhysicsMassProperties& properties)
        : m_refCount(1), m_owner(0), m_userData(0), m_massProperties(properties)
    {
    }
    virtual ~BaseBrickPhysicsPrimitiveElement() {}         // Mac D1 0x2538a0 / D0 0x2538c0, Win sdd 0x52af00

    // Answers PhysicsPrimitiveElement::GetIID() only. Mac 0x253790, Win 0x5c7710
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x253810, Win 0x5c7750
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x253830, Win 0x5c7770
    virtual const PhysicsMassProperties& GetMassProperties() const; // Mac 0x253870, Win 0x52f4c0
    virtual PrimitiveElement* GetUserData() const;          // Mac 0x253880, Win 0x46ee40
    virtual void SetUserData(PrimitiveElement* element);    // Mac 0x253890, Win 0x52aee0

    // ---- data -------------------------------------------------------------------------------
    volatile Int32                 m_refCount;      // +0x04 1
    BaseBrickPhysicsRigidElement*  m_owner;         // +0x08 the body (BaseBrickPhysicsRigidElement::AddPrimitiveElement)
    PrimitiveElement*              m_userData;      // +0x0c
    PhysicsMassProperties          m_massProperties; // +0x10 (sizeof 0x64)

private:
    BaseBrickPhysicsPrimitiveElement(const BaseBrickPhysicsPrimitiveElement&);
    BaseBrickPhysicsPrimitiveElement& operator=(const BaseBrickPhysicsPrimitiveElement&);
};

} // namespace LEGO
