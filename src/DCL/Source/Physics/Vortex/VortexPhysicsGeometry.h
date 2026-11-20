// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsGeometry — the Vortex side of every geometry of the Vortex physics (the
// second base, at +4, of VortexPhysicsGeometryBox / Sphere / Capsule / Cylinder / Configable /
// SampledHeightField): the Vx shape and the "safety margin" inflation the scene applies around a
// collision test. Header only; its two default bodies are emitted in
// VortexPhysicsGeometryConfigable.cpp (Mac TU 431). (names inferred)

// RTTI: class LEGO::VortexPhysicsGeometry : IUnknown. Win vtable 0xa2934c {3 pure + slot 3 pure,
//   0x40be80 (Inflate), 0x459790 (ResetSize), sdd 0x51e760}; Mac: the secondary vtables of the
//   six classes, the defaults Mac 0x3a78e0 / 0x3a78f0.
#pragma once
#include "LEGO/Types.h"
#include "BrickPhysics.h"

namespace Vx { class VxGeometry; }

namespace LEGO {

class VortexPhysicsGeometry : public IUnknown {
public:
    // The id the six geometries answer with this base (QueryInterface, function-local static
    // Mac 0x778ce8; the placed geometries / primitive elements query it, Mac 0x3a9600).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x257f5e43, 0xd549bd90, 0x025bddb3, 0x4840d717}};
        return id;
    }
    // The Vx shape (a Vx::VxGeometry, referenced by the geometry); NULL for a configurable
    // geometry (the placed geometries tell the two apart by it). The same signature as
    // PhysicsGeometrySampledHeightField slot 8: the height field overrides both with one function
    // (no own Mac primary slot, the secondary entry 0x3a7a20 is its thunk).
    virtual void* GetNativeGeometry() const = 0;                             // slot 3
    // Grows the shape by `amount` (each dimension that stays positive). Default: nothing
    // (Mac 0x3a78e0, Win 0x40be80).
    virtual void Inflate(Real32 /*amount*/) {}                               // slot 4
    // Back to the size the geometry was created with. Default: nothing (Mac 0x3a78f0,
    // Win 0x459790).
    virtual void ResetSize() {}                                              // slot 5
    virtual ~VortexPhysicsGeometry() {}                                      // slots 6 / 7 (Win sdd 0x51e760)
};

} // namespace LEGO
