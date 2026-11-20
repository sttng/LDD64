// Vortex 5.0.0 build 800 "vortexldd41" - VxFluidInteractionUniverseUpdate: the universe update
// subscriber applying the fluid forces of the registered collision geometries (restored:
// Mac TU 486 0x44847e-0x453913). Member names inferred. Created lazily by
// VxUniverse::registerFluidInteraction (Mac 0x482722: new(0x24)). Fluid states come only from the
// Win-only VxUniverse::AddFluidState / RemoveFluidState (Win 0x70b020 / 0x708720); LDD makes none.
#ifndef VX_VXFLUIDINTERACTIONUNIVERSEUPDATE_H
#define VX_VXFLUIDINTERACTIONUNIVERSEUPDATE_H

#include "Vx/VxUniverse.h"

namespace Vx {

class VxCollisionGeometry;
class VxFluidState;

// Mac 0x24 bytes (the inline ctor zeroes both arrays). Mac vtable 0x75dd00: D1 0x4487c0 (D2
// 0x4486c8), D0 0x4485c8, 2 update 0x44856a, 3 = the base's 0x4853aa; Win 0x9f5bd4: scalar
// deleting dtor 0x70cc60 (D2 0x74b2d0), 1 update 0x74b200, 2 0x7076a0.
class VxFluidInteractionUniverseUpdate : public VxUniverse::UpdateSubscriber {
public:
    VxFluidInteractionUniverseUpdate() {}
    virtual ~VxFluidInteractionUniverseUpdate();
    // every geometry is offered to the fluid states (densest first) until one of them takes it
    virtual void update(VxUniverse* u);
    void addCollisionGeometry(VxCollisionGeometry* cg);    // Mac 0x4487d4, Win 0x74b5d0
    bool removeCollisionGeometry(VxCollisionGeometry* cg); // Mac 0x44848a, Win 0x74b540
    // Win only (dead-stripped on Mac):
    VxFluidState* getFluidState(int i) const;              // Win 0x74b2c0 (no range check)
    void addFluidState(VxFluidState* s);                   // Win 0x74b580: sorted by decreasing density, once
    bool removeFluidState(VxFluidState* s);                // Win 0x74b370
    void sortFluidStates();                                // Win 0x74b610 (std::sort, decreasing density)

    VxArray<VxCollisionGeometry*> mGeometries;  // +0x04
    VxArray<VxFluidState*> mFluidStates;        // +0x14 (owned: deleted by the dtor)
};

} // namespace Vx

#endif
