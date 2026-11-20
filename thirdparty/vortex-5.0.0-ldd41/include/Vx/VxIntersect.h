// Vortex 5.0.0 build 800 "vortexldd41" - intersection interfaces, results, line segment queries
// and dynamics contacts (restored header: VxIntersect.cpp = Mac TU 497,
// VxIntersectResult.cpp = Mac TU 498, VxLineSegIntersect.cpp = Mac TU 501).
//
// Five interfaces are implemented by LEGO classes (glue TUs 435-437); their slot layouts come
// from the LEGO vtables (Mac / Win classes.json):
//   VxIntersect::IntersectSubscriber  <- LEGOIndividualCollector, CastIndividualCollector
//   VxUniverse::IntersectSubscriber   <- LEGOUniverseCollector       (Vx/VxUniverse.h)
//   VxIntersectFilter                 <- LDDVortexCollisionFilter
//   VxLineSegIntersectSub             <- TerrainRayCastCollector
//   VxAssemblyBase::UpdateSubscriber  <- PointGravitationAction       (Vx/VxAssemblyBase.h)
//
// Notes:
//  * VxIntersect::IntersectSubscriber slots 0 / 1 take the other collision geometry and return
//    0 / 1 / 2 (see below); +0x08 is the subscriber's current geometry (written by
//    VxIntersect::intersect, Mac 0x45c075) - the class is 12 bytes, LEGO fields start at +0x0c.
//  * VxLineSegIntersectSub slot 0 is a filter on one geometry (TerrainRayCastCollector Mac
//    0x3b3b80 reads only [ebp+0xc]): 0 = test it, 2 = stop, else skip.
//  * VxIntersectResult: +0x04 "owns the result" flag, +0x08 the Vinc intersect result
//    (McdIntersectResult, Vinc/VincIntersect.h), not the pair.
//  * VxCollisionPairRequest (it owns a VincRequest record): Vx/VxCollisionPairRequest.h.
#ifndef VX_VXINTERSECT_H
#define VX_VXINTERSECT_H

#include "Vx/VxBase.h"
#include "Vx/VxVector3.h"
#include "Vx/VxCollisionPairRequest.h"
#include "Mdt/MdtTypes.h"
#include "Vinc/VincTypes.h"

struct McdIntersectResult;
struct McdContact;
struct McdModel;

namespace Vx {

class VxCollisionGeometry;
class VxCollisionSensor;
class VxIntersectResult;
class VxUniverse;

// --------------------------------------------------------------------------------------
// VxIntersect: the one-shot query of a sensor's geometries against the universe.
class VxIntersect {
public:
    // Mac 4 slots (LEGO subclasses: 0 = Mac 0x3b3710 "return 0" emitted inline in the LEGO TU,
    // 1 = the callback, 2/3 D1/D0); Win 3 (0 = 0x4f9670 shared, 1 callback, 2 sdd).
    // (method names inferred)
    class IntersectSubscriber {
    public:
        IntersectSubscriber() : mResult(0) {}          // Mac 0x45bfa8 (out of line: vtable 0x75dfc8), Win 0x71e960
        // slot 0: called for every candidate geometry `other` before the pair test (VxBridge
        // Mac 0x4335cc): 0 = test, 1 = skip this pair, 2 = stop the query
        virtual int filter(VxCollisionGeometry* /*other*/) { return 0; }
        // slot 1: the pair touches (contacts in mResult): 2 = stop, anything else = go on
        // (CastIndividualCollector Mac 0x3b3bc0 reads mResult and returns 2)
        virtual int notifyIntersect(VxCollisionGeometry* other) = 0;
        virtual ~IntersectSubscriber() {}               // slot 2/3

        VxIntersectResult* mResult;                     // +0x04 the bridge's result while testing
        VxCollisionGeometry* mCollisionGeometry;        // +0x08 the sensor geometry being tested (not set by the ctor)
    };

    // Mac 0x45bfea, Win 0x71e970 ("VxIntersect::intersect: collision geometry is NULL",
    // "...requires the sensor to be added to the universe"): every collision geometry of
    // `sensor` against the universe's broad-phase pairs, through the universe's VxBridge
    // (VxBridge::intersect); `update` first refreshes the pairs (VxBridge::updatePairs).
    // True when a pair touched.
    static bool intersect(VxCollisionSensor* sensor, IntersectSubscriber* subscriber, bool update);
};

// --------------------------------------------------------------------------------------
// VxIntersectResult (Mac 3 slots: D1, D0, getClassName; Win 2): a view of a Vinc intersect
// result; built on the stack by VxBridge around its own result (not owned). 0x0c bytes.
class VxIntersectResult {
public:
    // One contact as the glue sees it (12 bytes, built by value by ContactIterator::operator*:
    // the contact and the pair's two models) (class name inferred)
    class Contact {
    public:
        // Mac C1 0x437bee -> C2 0x437bd4 (out of line in TU 468; inline here)
        Contact(McdContact* c, McdModel* m1, McdModel* m2) : mContact(c), m04(m1), m08(m2) {}
        void getNormal(VxVector3& n) const;            // Mac 0x437c16: contact +0x18
        void getPosition(VxVector3& p) const;          // Mac 0x437c44: contact +0x00
        VxReal getPenetration() const;                 // Mac 0x437c72: -(contact +0x30)
        VxCollisionGeometry* getCollisionGeometry1() const; // Mac 0x437cd4: model at contact +0x44 (Vinc 0x4c1989)
        VxCollisionGeometry* getCollisionGeometry2() const; // Mac 0x437cbc: model at contact +0x48
        void* mContact;                                 // +0x00 Vinc contact (0x4c bytes on Mac)
        void* m04;                                      // +0x04 the pair's model 1
        void* m08;                                      // +0x08 the pair's model 2
    };
    class ContactIterator {
    public:
        // Mac C1 0x45c16e -> C2 0x45c15a
        ContactIterator(McdContact* contact = 0, McdIntersectResult* result = 0) : mContact(contact), mResult(result) {}
        ContactIterator operator++(int);                // Mac 0x45c200 (returns the old position; ++ = Mac 0x45c1f4)
        ContactIterator& operator++();                  // Mac 0x45c1f4
        bool operator!=(const ContactIterator& o) const; // Mac 0x45c22a (compares the contacts)
        Contact operator*() const;                      // Mac 0x45c260
        McdContact* mContact;                           // +0x00
        McdIntersectResult* mResult;                    // +0x04 (0 for end())
    };

    explicit VxIntersectResult(McdIntersectResult* r = 0, bool owns = false) : mOwnsResult(owns), mResult(r) {}
    // slot 0/1 Mac 0x45c36e (D2 0x45c326)/0x45c2d6, Win 0x723190 (D1) / 0x723520 (sdd): an owned
    // result is freed with its contact array (vxFree)
    virtual ~VxIntersectResult();
    virtual const char* getClassName() const;           // slot 2 Mac 0x45c3d8, Win 0x723510 "VxIntersectResult"

    ContactIterator contactBegin() const;               // Mac 0x45c1c6 (name inferred)
    ContactIterator contactEnd() const;                 // Mac 0x45c190 (name inferred)
    VxReal getTimeOfImpact() const;                     // Mac 0x45c140: result +0x10c, 0 without result (name inferred)
    // Mac 0x45c298, Win 0x7231c0: the user data of the pair's two models (name inferred)
    void getCollisionGeometries(VxCollisionGeometry*& cg1, VxCollisionGeometry*& cg2) const;
    // Mac 0x45c382, Win 0x723200: their entities (0 for a geometry without one) (name inferred)
    void getCollisionSensors(VxCollisionSensor*& s1, VxCollisionSensor*& s2) const;
    McdIntersectResult* getResult() const { return mResult; }
    // Win-only accessors (dead-stripped on the Mac; Win 0x7232b0-0x7233d0) (names inferred)
    bool getM108() const;                               // Win 0x7232b0: result +0x108 != 0 (name unknown)
    void getNormal(VxVector3& n) const;                 // Win 0x7232d0: the average normal
    void getContactGroupNormal(int group, VxVector3& n) const; // Win 0x723300
    VxReal getContactGroupM04(int group) const;         // Win 0x723330 (name unknown)
    int getContactGroupContactCount(int group) const;   // Win 0x723350
    int getContactCountOfGroups(int groupCount) const;  // Win 0x723370: groups [0, groupCount)
    Contact getContact(int i) const;                    // Win 0x7233d0

    bool mOwnsResult;                                   // +0x04 (name inferred)
    McdIntersectResult* mResult;                        // +0x08 (name inferred)
};

// --------------------------------------------------------------------------------------
// VxIntersectFilter (LEGO LDDVortexCollisionFilter: Mac 0 = D1 0x3b4350, 1 = D0 0x3b4990,
// 2 = filter 0x3b49a0; Win 0 = sdd, 1 = filter 0x52d850). Set on a collision geometry
// (VxCollisionGeometry::unk_437f26 -> model ma4); VxBridge asks both geometries' filters.
// (method name inferred)
class VxIntersectFilter {
public:
    virtual ~VxIntersectFilter() {}                     // slot 0/1
    virtual bool accept(VxCollisionGeometry* a, VxCollisionGeometry* b) = 0; // slot 2
};

// --------------------------------------------------------------------------------------
// VxLineSegIntersectSub (LEGO TerrainRayCastCollector: Mac 0 = filter 0x3b3b80, 1/2 = D1/D0;
// Win 0 = filter 0x523b90, 1 = sdd) (method name inferred)
class VxLineSegIntersectSub {
public:
    // 0 = test the geometry, 2 = stop the query, anything else = skip it
    virtual int filter(VxCollisionGeometry* cg) = 0;    // slot 0
    virtual ~VxLineSegIntersectSub() {}                 // slot 1/2
};

// The line segment hit ("outOverlap" in the Win strings; Mac 0x40, Win 0x48 bytes)
// (struct and member names inferred)
struct VxLineSegResult {
    VxCollisionGeometry* mCollisionGeometry;            // +0x00 the geometry hit
    VxVector3 mPosition;                                // Mac +0x04, Win +0x08
    VxVector3 mNormal;                                  // Mac +0x1c, Win +0x20
    VxReal mDistance;                                   // Mac +0x34, Win +0x38 (DBL_MAX: nothing hit)
    int mElement;                                       // Mac +0x3c, Win +0x40 (-1)
};

// Mac 0x45d45e (Win: inlined into the glue; out of line Win 0x71acb0): the first hit (or, with
// firstHit = false, the nearest one) of the segment from -> to against every collision geometry
// of the universe's entities (a LineSegIntersectVisitor walks the universe); `result` may be 0;
// geometries the subscriber filters out are skipped. True when something was hit. (name inferred)
bool VxLineSegIntersect(VxUniverse* universe, const VxVector3& from, const VxVector3& to, bool firstHit,
                        VxLineSegResult* result, VxLineSegIntersectSub* subscriber);
// NOT RESTORED (Win-only, dead-stripped on the Mac, never reached by LDD): Win 0x71ada0
// VxLineSegArrayIntersect(universe, from[], to[], count, firstHit, results[], subscriber)
// ("VxLineSegArrayIntersect : outOverlap can't be NULL.") with its subscriber class
// VxLineSegIntersectArraySub: per segment it first re-tests the previous hit geometry (height
// fields through Win 0x759850), then runs VxLineSegIntersect with a subscriber that skips it.

// --------------------------------------------------------------------------------------
// VxDynamicsContact: the Vx view of an Mdt contact - the object pointer IS the MdtContactID
// (Mac 0x44609c passes `this` straight to Mdt 0x48f407) (class name inferred).
class VxDynamicsContact {
public:
    VxReal getPenetration() const;                      // Mac 0x44609c -> MdtContact +0xb50 (name inferred)
    bool setEnabled(bool enable);                       // Mac 0x44604c, Win 0x7251e0 (name inferred)
    MdtContactID getMdtContact() const { return (MdtContactID)this; }
};

} // namespace Vx

#endif
