// LEGO Digital Designer — restored source
// LEGO::VortexShapeCollisionFilter — the shapes a primitive / scenery element does not collide
// with (flexible connections, PhysicsPrimitiveElement slots 9 / 10); LEGO::LDDVortexCollisionFilter
// — the Vx::VxIntersectFilter every LEGO collision model carries (VxCollisionGeometry::unk_437f26).
// Original object LDDVortexCollisionFilter.cpp (Mac TU 436); the shape filter's Add / Remove are in
// VortexShapeCollisionFilter.cpp. Every name is inferred.

// RTTI: class LEGO::VortexShapeCollisionFilter (no base, no vtable). sizeof 0x20 (both builds):
//   VortexPhysicsPrimitiveElement +0x64, VortexPhysicsSceneryElement +0x1c.
// RTTI: class LEGO::LDDVortexCollisionFilter : Vx::VxIntersectFilter. Mac vtable 0x779038
//   {D1 0x3b4350, D0 0x3b4990, accept 0x3b49a0}; Win 0xa2a7e8 {sdd 0x52d950, accept 0x52d850}.
//   The global instance: Mac 0x77cdbc (initialiser in TU 436: vtable only), Win 0xb43f98
//   (initialiser 0x52d7e0: vtable, +4 = 0, +8 = 0).
#pragma once
#include "LEGO/Types.h"
#include "Vx/VxIntersect.h"
#include "BrickPhysics.h"

namespace LEGO {

class VortexShapeCollisionFilter {
public:
    // the slots are not initialised by the original (only the first m_count are read); fixed:
    // null (low)
    VortexShapeCollisionFilter() : m_count(0)
    {
        for (UInt32 i = 0; i < 7; ++i)
            m_filters[i] = 0;
    }

    // Appends `filter`. NOTE: no bounds check (the original's; 7 entries). Mac 0x3b4360 (Win
    // inlined into VortexPhysicsPrimitiveElement 0x52b2e0)
    void Add(VortexShapeCollisionFilter* filter);
    // Removes `filter` (the last entry takes its place). Mac 0x3b4380 (Win inlined 0x52b320)
    void Remove(VortexShapeCollisionFilter* filter);

    VortexShapeCollisionFilter* m_filters[7];             // +0x00 (not initialised)
    UInt32                      m_count;                  // +0x1c
};

class LDDVortexCollisionFilter : public Vx::VxIntersectFilter {
public:
    // m_mode, set by VortexPhysicsScene around its queries
    enum Mode { ModeDefault = 0, ModeTerrain = 1, ModeCallback = 2 };

    LDDVortexCollisionFilter() : m_mode(ModeDefault), m_callback(0) {}
    virtual ~LDDVortexCollisionFilter() {}                // Mac D1 0x3b4350 / D0 0x3b4990, Win sdd 0x52d950

    // ModeCallback: two brick geometries of one element never collide, other brick pairs only
    // when the callback answers Collision; then (as in ModeDefault) the flexible-connection
    // filters. ModeTerrain: only pairs with the terrain, no filters. Mac 0x3b49a0, Win 0x52d850
    virtual bool accept(Vx::VxCollisionGeometry* a, Vx::VxCollisionGeometry* b);

    UInt32                    m_mode;      // +0x04
    PhysicsCollisionCallback* m_callback;  // +0x08 (ModeCallback; VortexPhysicsScene::TestElementCollision)
};

// The filter given to every LEGO collision model. Mac 0x77cdbc (non-lazy pointer 0x7024e8),
// Win 0xb43f98.
extern LDDVortexCollisionFilter g_lddVortexCollisionFilter;

} // namespace LEGO
