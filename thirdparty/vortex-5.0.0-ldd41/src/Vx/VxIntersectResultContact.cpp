// Vortex 5.0.0 build 800 "vortexldd41" - VxIntersectResult::Contact accessors (restored;
// Mac TU 468 0x437bd4-0x437cd4; the rest of TU 468, the CRC table, is in
// VxID.cpp). Win inlines them. mContact is a VincContact (Vinc/VincIntersect.h).
#include "Vx/VxIntersect.h"
#include "Vinc/VincIntersect.h"
#include "Vinc/VincModel.h"

namespace Vx {

namespace {
inline const VincContact* contact(const VxIntersectResult::Contact* c) { return (const VincContact*)c->mContact; }
} // namespace

// Mac 0x437c16
void VxIntersectResult::Contact::getNormal(VxVector3& n) const
{
    const VincContact* c = contact(this);
    n.set(c->normal[0], c->normal[1], c->normal[2]);
}

// Mac 0x437c44
void VxIntersectResult::Contact::getPosition(VxVector3& p) const
{
    const VincContact* c = contact(this);
    p.set(c->position[0], c->position[1], c->position[2]);
}

// Mac 0x437c72
VxReal VxIntersectResult::Contact::getPenetration() const
{
    return -contact(this)->separation;
}

// Mac 0x437cd4
VxCollisionGeometry* VxIntersectResult::Contact::getCollisionGeometry1() const
{
    return (VxCollisionGeometry*)VincModelGetUserData(contact(this)->model1);
}

// Mac 0x437cbc
VxCollisionGeometry* VxIntersectResult::Contact::getCollisionGeometry2() const
{
    return (VxCollisionGeometry*)VincModelGetUserData(contact(this)->model2);
}

} // namespace Vx
