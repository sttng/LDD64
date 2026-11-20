// Vortex 5.0.0 build 800 "vortexldd41" - VxIntersectResult (restored).
// Mac TU 498 0x45c140-0x45c44e (file guess Vx/VxIntersectResult.cpp), Win 0x723190-0x723560.
// The Contact accessors are V4.4's (src/Vx/VxIntersectResultContact.cpp, Mac TU 468).
#include "Vx/VxIntersect.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxAllocator.h"
#include "Vinc/VincIntersect.h"
#include "Vinc/VincModel.h"

namespace Vx {

// Mac D2 0x45c326 (D1 0x45c36e, D0 0x45c2d6), Win 0x723190 (sdd 0x723520)
VxIntersectResult::~VxIntersectResult()
{
    if (mOwnsResult) {
        vxFree(mResult->contacts);
        vxFree(mResult);
    }
}

// Mac 0x45c3d8, Win 0x723510
const char* VxIntersectResult::getClassName() const
{
    return "VxIntersectResult";
}

// Mac 0x45c140, Win 0x723290
VxReal VxIntersectResult::getTimeOfImpact() const
{
    if (mResult == 0)
        return 0;
    return mResult->time;
}

// Mac 0x45c1c6, Win 0x7234c0
VxIntersectResult::ContactIterator VxIntersectResult::contactBegin() const
{
    return ContactIterator(mResult->contacts, mResult);
}

// Mac 0x45c190, Win 0x7234e0
VxIntersectResult::ContactIterator VxIntersectResult::contactEnd() const
{
    return ContactIterator(mResult->contacts + mResult->contactCount, 0);
}

// Mac 0x45c1f4, Win 0x723450
VxIntersectResult::ContactIterator& VxIntersectResult::ContactIterator::operator++()
{
    ++mContact;
    return *this;
}

// Mac 0x45c200, Win 0x723460
VxIntersectResult::ContactIterator VxIntersectResult::ContactIterator::operator++(int)
{
    ContactIterator old(*this);
    ++*this;
    return old;
}

// Mac 0x45c22a, Win 0x7234a0 (operator== Win 0x723480)
bool VxIntersectResult::ContactIterator::operator!=(const ContactIterator& o) const
{
    return mContact != o.mContact;
}

// Mac 0x45c260, Win 0x723420
VxIntersectResult::Contact VxIntersectResult::ContactIterator::operator*() const
{
    return Contact(mContact, mResult->pair->model1, mResult->pair->model2);
}

// Mac 0x45c298, Win 0x7231c0
void VxIntersectResult::getCollisionGeometries(VxCollisionGeometry*& cg1, VxCollisionGeometry*& cg2) const
{
    if (mResult == 0)
        return;
    cg1 = (VxCollisionGeometry*)VincModelGetUserData(mResult->pair->model1);
    cg2 = (VxCollisionGeometry*)VincModelGetUserData(mResult->pair->model2);
}

// Mac 0x45c382, Win 0x723200
void VxIntersectResult::getCollisionSensors(VxCollisionSensor*& s1, VxCollisionSensor*& s2) const
{
    if (mResult == 0)
        return;
    VxCollisionGeometry* cg = (VxCollisionGeometry*)VincModelGetUserData(mResult->pair->model1);
    s1 = cg ? cg->mEntity : 0;
    cg = (VxCollisionGeometry*)VincModelGetUserData(mResult->pair->model2);
    s2 = cg ? cg->mEntity : 0;
}

// Win 0x7232b0
bool VxIntersectResult::getM108() const
{
    return mResult != 0 && mResult->m108 != 0;
}

// Win 0x7232d0
void VxIntersectResult::getNormal(VxVector3& n) const
{
    n.set(mResult->normal[0], mResult->normal[1], mResult->normal[2]);
}

// Win 0x723300
void VxIntersectResult::getContactGroupNormal(int group, VxVector3& n) const
{
    const VincContactGroup& g = mResult->groups[group];
    n.set(g.normal[0], g.normal[1], g.normal[2]);
}

// Win 0x723330
VxReal VxIntersectResult::getContactGroupM04(int group) const
{
    return mResult->groups[group].m04;
}

// Win 0x723350
int VxIntersectResult::getContactGroupContactCount(int group) const
{
    return mResult->groups[group].count;
}

// Win 0x723370 (the original sums even and odd groups separately, the total is the same)
int VxIntersectResult::getContactCountOfGroups(int groupCount) const
{
    int n = 0;
    for (int i = 0; i < groupCount; ++i)
        n += mResult->groups[i].count;
    return n;
}

// Win 0x7233d0
VxIntersectResult::Contact VxIntersectResult::getContact(int i) const
{
    return Contact(mResult->contacts + i, mResult->pair->model1, mResult->pair->model2);
}

} // namespace Vx
