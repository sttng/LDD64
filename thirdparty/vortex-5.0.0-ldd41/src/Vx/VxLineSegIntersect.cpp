// Vortex 5.0.0 build 800 "vortexldd41" - line segment queries against a universe (restored).
// Mac TU 501 0x45d3d4-0x45d881 (file guess: the TU of
// (anonymous namespace)::LineSegIntersectVisitor), Win 0x71ac00-0x71b100.
// NOTE: both functions below start with a guarded function-local static of an empty type
// (Mac guard 0x779380 + atexit 0x45d3d4 (empty), Win guard 0xaf14c0) that nothing reads; it is
// not reproduced.
// Not restored: the Win-only VxLineSegArrayIntersect (0x71ada0, see Vx/VxIntersect.h).
#include "Vx/VxIntersect.h"
#include "Vx/VxVisitor.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxEntity.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vinc/VincIntersect.h"
#include "Vinc/VincModel.h"

#include <float.h>
#include <string.h>

namespace Vx {

namespace {

// Mac vtable 0x75e088 (9 slots: D1 0x45d434, D0 0x45d3fc, 2-5 / 7 / 8 VxVisitor's, 6 0x45d72a);
// Win: ctor 0x71af90, slot 5 0x71b020. Mac 0x30 bytes, Win 0x38.
class LineSegIntersectVisitor : public VxVisitor {
public:
    // Win 0x71af90 (inlined on the Mac)
    LineSegIntersectVisitor(const VxVector3* from, const VxVector3& to, bool firstHit, VxLineSegResult* result,
                            VxLineSegIntersectSub* subscriber)
        : mFound(false), mSubscriber(subscriber), mResult(result), mFrom(from), mTo(to), mFirstHit(firstHit)
    {
#if defined(_WIN32)
        if (mResult == 0)
            VxFatalError(0, "LineSegIntersectVisitor::LineSegIntersectVisitor : Invalide mOutOverlap pointer.\n");
#endif
    }
    virtual ~LineSegIntersectVisitor() {}               // Mac D1 0x45d434, D0 0x45d3fc, Win 0x728d60
    virtual int visitEntity(VxEntity* e);               // slot 6 Mac 0x45d72a, Win 0x71b020

    bool mFound;                        // +0x04
    VxLineSegIntersectSub* mSubscriber; // +0x08
    VxLineSegResult* mResult;           // +0x0c
    const VxVector3* mFrom;             // +0x10
    VxVector3 mTo;                      // Mac +0x14, Win +0x18 (shortened to every hit)
    bool mFirstHit;                     // Mac +0x2c, Win +0x30
};

// Mac 0x45d5b7, Win 0x71ac00: one collision geometry; `result` is read (the previous hit
// as start values for Vinc) and, on a hit, overwritten
bool intersectCollisionGeometry(VxCollisionGeometry* cg, const VxVector3* from, const VxVector3* to, bool firstHit,
                                VxLineSegResult* result)
{
    VincLineSegResult r;
    // NOTE: without a result the original leaves the Vinc result uninitialised
    memset(&r, 0, sizeof(r));
    r.element = -1;
    if (result != 0) {
        r.model = result->mCollisionGeometry != 0 ? result->mCollisionGeometry->mModel : 0;
        r.distance = result->mDistance;
        r.normal[0] = result->mNormal[0];
        r.normal[1] = result->mNormal[1];
        r.normal[2] = result->mNormal[2];
        r.position[0] = result->mPosition[0];
        r.position[1] = result->mPosition[1];
        r.position[2] = result->mPosition[2];
        r.element = result->mElement;
    }
    r.firstHit = firstHit;
    if (VincLineSegIntersect(cg->mModel, from->data(), to->data(), &r) == 0)
        return false;
    if (result != 0) {
        VincModelID hit = r.m3c != 0 ? r.m3c : r.model;
        result->mCollisionGeometry = (VxCollisionGeometry*)VincModelGetUserData(hit);
        result->mDistance = r.distance;
        result->mNormal.set(r.normal[0], r.normal[1], r.normal[2]);
        result->mPosition.set(r.position[0], r.position[1], r.position[2]);
        result->mElement = r.element;
    }
    return true;
}

// Mac 0x45d72a, Win 0x71b020
int LineSegIntersectVisitor::visitEntity(VxEntity* e)
{
    // NOTE: the original asks slot 6 twice and accepts 0 or 1 - always true for isPart()
    if (e->isPart() == 0 || e->isPart() == 1) {
        for (int i = 0; i < e->getCollisionGeometryCount(); ++i) {
            VxCollisionGeometry* cg = e->getCollisionGeometry(i);
            int f = mSubscriber != 0 ? mSubscriber->filter(cg) : 0;
            if (f == 0) {
                if (intersectCollisionGeometry(cg, mFrom, &mTo, mFirstHit, mResult)) {
                    if (&mResult->mPosition != &mTo)
                        mTo = mResult->mPosition;
                    mFound = true;
                    if (mFirstHit)
                        return 2;
                }
            } else if (f == 2) {
                return 2;
            }
        }
    }
    return 0;
}

} // namespace

// Mac 0x45d45e, Win 0x71acb0
bool VxLineSegIntersect(VxUniverse* universe, const VxVector3& from, const VxVector3& to, bool firstHit,
                        VxLineSegResult* result, VxLineSegIntersectSub* subscriber)
{
    VxLineSegResult none;
    none.mCollisionGeometry = 0;
    none.mDistance = DBL_MAX;
    none.mElement = -1;
    LineSegIntersectVisitor v(&from, to, firstHit, result != 0 ? result : &none, subscriber);
    universe->accept(&v);
    return v.mFound;
}

} // namespace Vx
