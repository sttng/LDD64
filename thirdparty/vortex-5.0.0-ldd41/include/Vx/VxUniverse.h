// Vortex 5.0.0 build 800 "vortexldd41" - VxUniverse (restored header:
// src/Vx/VxUniverse.cpp = Mac TU 528 0x4806ee-0x486cb1, Win 0x707650-0x70d7a0).
//
// A universe = root mechanism (all its parts/constraints) + Mdt world + Vinc space + the
// bridge (pair processing, response handlers, intersect-subscriber table). LDD creates one:
// new VxUniverse(10000, 100000, false) in VortexBrickPhysics (Mac TU 422 0x39ea10), adds it to
// VxFrame::instance().
//
// Stepping (VxFrame calls the VxStepperBase slots): beginStep (root preStep, "stepping" set),
// stepPhase1 = collision (Vinc space path update + VxBridge::update(dt), timed; the update
// subscribers; root step), stepDynamics (MdtWorldStep, timed), endStep (stepping cleared, root
// postStep, the bridge's user contacts disabled).
#ifndef VX_VXUNIVERSE_H
#define VX_VXUNIVERSE_H

#include "Vx/VxStepperBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxIntersect.h"
#include "Vx/VxVector3.h"
#include "Mdt/MdtTypes.h"
#include "Vinc/VincTypes.h"
#include <stdio.h>

namespace Vx {

class VxMechanism;
class VxAssemblyBase;
class VxDynamicsResponseInput;
class VxInternalIDIntersectSubscriber;
class VxBridge;
class VxVisitor;
class VxPart;
class VxCollisionSensor;
class VxContactMaterial;
class VxDisabledPairs;

// --------------------------------------------------------------------------------------
// VxUniverse (Mac vtable 0x75e9c8, 15 slots; Win 14; class id 0x106 Mac 0x4806ee)
// Mac 0x90 bytes, Win 0xb0 (new(0x90) Mac 0x480bec / new(0xb0) Win 0x7077c0).
class VxUniverse : public VxStepperBase, public VxEntityConstraintCollection<VxUniverse> {
public:
    // VxUniverse::IntersectSubscriber (LEGO LEGOUniverseCollector: Mac 0/1 D1/D0 (0x3ac390 empty,
    // 0x3b2da0), 2 = 0x3b2db0 empty, 3 = callback 0x3b2dc0; Win 3: sdd, 0x424ec0, 0x521b60)
    // (method names inferred). Events (VxBridge): 0 = the pair still touches, 1 = staying
    // pair not tested this step (result without contacts, response 0), 2 = first touch,
    // 3 = separated (slot 2). The event is the first argument (LEGOUniverseCollector Mac 0x3b2dc0
    // reads the result from [ebp+0x10]).
    class IntersectSubscriber {
    public:
        virtual ~IntersectSubscriber() {}                                    // slot 0/1
        virtual void notifyDisjoint(int /*event*/, VxIntersectResult* /*result*/) {} // slot 2
        virtual void notifyIntersect(int event, VxIntersectResult* result, VxDynamicsResponseInput* response) = 0; // slot 3
    };
    // VxUniverse::UpdateSubscriber (VxFluidInteractionUniverseUpdate: D1/D0, 2 = 0x44856a,
    // 3 = 0x4853aa default) (method names inferred)
    class UpdateSubscriber {
    public:
        virtual ~UpdateSubscriber() {}                  // slot 0/1
        virtual void update(VxUniverse* u) = 0;         // slot 2: every collision phase (Mac 0x4814c8)
        // slot 3 Mac 0x4853aa / Win 0x7076a0 (empty, out of line in VxUniverse.cpp): after
        // updateIntersections (Mac 0x4827ee)
        virtual void intersectionsUpdated();
        void vf03() { intersectionsUpdated(); }         // (earlier name)
    };
    // Iterator over the Mdt contacts of the world (MdtConstraint type kMdtTypeContact)
    // (class name inferred; Win strings show VxUniverse::DynamicPartIterator /
    // EnabledConstraintsIterator of the same family)
    class DynamicsContactIterator {
    public:
        explicit DynamicsContactIterator(MdtConstraintID c = 0) : mContact(c) {} // Mac 0x48084a (C2 0x48083c)
        DynamicsContactIterator& operator++();          // Mac 0x480aea: next enabled constraint of type 5
        VxDynamicsContact* operator*() const;           // Mac 0x480888
        bool operator!=(const DynamicsContactIterator& o) const; // Mac 0x480892
        MdtConstraintID mContact;
    };
    // Iterator over the parts of the enabled (dynamic, awake) Mdt bodies ("VxUniverse::
    // DynamicPartIterator" warning string, Win 0x708a70)
    class DynamicPartIterator {
    public:
        DynamicPartIterator() : mBody(0), mNext(0) {}   // Mac 0x4808f4 (C2 0x4808ca)
        // Mac 0x48094e (C2 0x480910): the part's body when it is dynamic in a universe
        explicit DynamicPartIterator(VxPart* part);
        DynamicPartIterator& operator++();              // Mac 0x4809a0, Win 0x708a70
        DynamicPartIterator operator++(int);            // Mac 0x480a32, Win 0x708af0
        VxPart* operator*() const;                      // Mac 0x480a7a (MdtBodyGetUserData)
        bool operator!=(const DynamicPartIterator& o) const { return mBody != o.mBody; } // Mac 0x480968
        MdtBodyID mBody;                                // +0x00
        MdtBodyID mNext;                                // +0x04
    };

    VX_DECLARE_ALLOCATOR();                             // new Mac 0x480bec / Win 0x7077c0, delete Mac 0x480bd5 / Win 0x7077d0
    static int getStaticClassType();                    // Mac 0x4806ee: 0x106
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x482276
    // C1 Mac 0x484b72 -> C2 0x4848fa, Win 0x70b0d0 (glue: 10000, 100000, false). Creates the
    // root mechanism ("rootMechanism"), then initialize (argument names inferred)
    VxUniverse(int maxPartCount, int maxPairCount, bool flag);
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x4825fe, Win 0x70ae10
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x4806f8, Win 0x708bf0
    virtual ~VxUniverse();                              // slot 2/3 Mac 0x4848e6 (D2 0x484620)/0x484b9a, Win 0x70d640 (0x70ae40)
    virtual const char* getClassName() const;           // slot 4 Mac 0x4853b0, Win 0x70d130
    virtual void print(const char* fileName);           // slot 7 Mac 0x482b0a, Win 0x70b570 (6.8 KB)
    virtual void beginStep();                           // slot 8 Mac 0x48077c, Win 0x707e80
    virtual void stepPhase1();                          // slot 9 Mac 0x4814c8, Win 0x7096d0
    virtual void stepDynamics();                        // slot 10 Mac 0x481434, Win 0x707ea0
    virtual void stepPhase3();                          // slot 11 Mac 0x4853c2, Win 0x7076a0 (empty)
    virtual void endStep();                             // slot 12 Mac 0x4811da, Win 0x707f40
    virtual VxReal getTime1() const;                    // slot 13 Mac 0x480824, Win 0x708430: collision ms (+0x68)
    virtual VxReal getTime2() const;                    // slot 14 Mac 0x480830, Win 0x708440: dynamics ms (+0x70)
    void vf07() { print(0); }                           // (earlier name of slot 7)

    // --- assemblies (names inferred, cf. "VxAssemblyBase::addAssembly") ---
    bool addAssembly(VxAssemblyBase* a);                // Mac 0x480726, Win 0x707e20: root->addAssembly when a has no parent
    bool removeAssembly(VxAssemblyBase* a);             // Mac 0x480756, Win 0x707e50: root->removeAssembly
    // the root mechanism visited with slots 2 / 3 around it (Mac 0x480d42, Win 0x70ce80)
    int accept(VxVisitor* v);
    VxMechanism* getRootMechanism() const { return mRootMechanism; }
    bool isStepping() const { return mStepping; }

    // --- dynamics (Mdt world) (names inferred) ---
    void setGravity(VxReal x, VxReal y, VxReal z);      // Mac 0x481204, Win 0x707da0 (MdtWorldSetGravity)
    void setAutoDisable(bool b);                        // Mac 0x481252, Win 0x707c90 (MdtWorldSetAutoDisable)
    void unk_481252(bool b) { setAutoDisable(b); }      // (earlier name)
    // Mac 0x480c7e (through the inline wrapper Mac 0x422f86): MdtWorldInsertBody; warns while
    // stepping ("VxUniverse insert part: cannot be called while stepping ...")
    void insertBody(MdtBodyID body);
    // Mac 0x481270: MdtWorldGetGrouping(world, grouping); new groupings copy grouping 0's parameters
    MdtGrouping* getGrouping(int grouping);
    int getKeaM2c() const;                              // Mac 0x481320 (MdtWorldGetKeaM2c)
    int getMaxConstraintsPerLevel() const;              // Mac 0x481336 (MdtWorldGetMaxConstraintsPerLevel)
    int getPartitionMaxDepth() const;                   // Mac 0x48134c, Win 0x707a10: -1 when the partition optimisation is off
    void setMaxPartitionsPerUnit(int n);                // Mac 0x48070e, Win 0x707aa0 (world +0x320 when n > 0)
    // MdtWorld body callbacks (world +0x2d0 / +0x2cc): the part's enabled collision geometries
    // are frozen (Mac 0x481380, Win 0x707700) / unfrozen (Mac 0x4813da, Win 0x707760) in the space
    static void MEAPI bodyDisabledCallback(MdtBodyID body);
    static void MEAPI bodyEnabledCallback(MdtBodyID body);
    DynamicPartIterator dynamicPartBegin();             // Mac 0x480a9c (enabled bodies of the world)
    DynamicPartIterator dynamicPartEnd();               // Mac 0x4808f4 (inline construction)
    DynamicsContactIterator dynamicsContactBegin();     // Mac 0x480b32 (name inferred)
    DynamicsContactIterator dynamicsContactEnd();       // Mac 0x480864 (name inferred)

    // --- collision (Vinc space, bridge) (names inferred) ---
    int getCollisionGeometryCount() const;              // Mac 0x48123c, Win 0x707ce0 (VincSpaceGetModelCount)
    // Mac 0x484e6c, Win 0x70a650 ("VxUniverse insert collision geometry: cannot be called while
    // stepping"): into the space, then the collection subscribers get event 5; through the
    // out-of-line wrapper Mac 0x422fba / Win 0x7501c0
    void insertCollisionModel(McdModelID model);
    // Mac 0x4850e2, Win 0x70a6b0: out of the space, pairs refreshed, event 6; wrapper Mac
    // 0x422fa0, Win 0x7501e0
    void removeCollisionModel(McdModelID model);
    // Mac 0x482722, Win 0x70af80: (un)registers a collision geometry with a fluid force function
    void registerFluidInteraction(VxCollisionGeometry* cg, bool add);
    void addUpdateSubscriber(UpdateSubscriber* s);      // Mac 0x482644, Win 0x787720 (push_back)
    // Mac 0x4827ee, Win 0x70ac10: the pair / contact processing of a step without stepping: the
    // awake dynamic parts that are not enabled are enabled for it, the space path updated, the
    // bridge updated (dt 0; subscriber notification off unless `notify`) and its pairs
    // refreshed, the parts disabled again, then update subscriber slot 3
    void updateIntersections(bool notify);
    void unk_4827ee(bool b) { updateIntersections(b); } // (earlier name)
    void setSleepingPairsIntersect(bool b);             // Mac 0x4808b8, Win 0x7086d0 (bridge +0x16)
    void unk_4808b8(bool b) { setSleepingPairsIntersect(b); } // (earlier name)
    // Mac 0x48079c, Win 0x707f60: bridge slot 2 (type1, type2, event 0..3, subscriber, position in
    // the list) (glue: (0, 0, 2, collector, 0))
    void addIntersectSubscriber(int type1, int type2, int event, IntersectSubscriber* s, unsigned int index);
    // Mac 0x4807d6: bridge slot 3
    bool removeIntersectSubscriber(int type1, int type2, int event, IntersectSubscriber* s);
    // Mac 0x480ce8, Win 0x708f90: grows the type table if needed, then VxBridge::getPairRequest
    VxCollisionPairRequest* getCollisionPairRequest(int type1, int type2);
    // Mac 0x48105a, Win 0x707fd0 (lazy, +0x80): subscribed to events 2 / 0 / 3 of (0, 0) and (0, 1)
    VxInternalIDIntersectSubscriber* getInternalIDIntersectSubscriber();
    int getMaxCollisionGeometryType() const;            // Mac 0x4808a8, Win 0x708f80: bridge type count - 1
    void setCollisionGeometryTypeCount(int count);      // Mac 0x480cca, Win 0x708600
    void registerResponseModel(int model);              // Mac 0x480c38 (VxBridge::registerResponseModel)
    bool canChangeResponseModel(int oldModel, int newModel) const; // Mac 0x480c56, Win 0x7086a0
    // VxBridge::updateCollisionGeometryType (a wrapper of the restoration; the original calls the
    // bridge directly, Mac 0x4346c8 / Win 0x741030)
    void updateCollisionGeometryType(VxCollisionGeometry* cg);
    // pair intersection switches (Vinc space pair flags). The (cg, cg) version needs both
    // geometries enabled, in the same space, on different entities.
    bool enablePairIntersect(VxCollisionGeometry* cg0, VxCollisionGeometry* cg1);   // Mac 0x480d8c, Win 0x708330
    void enablePairIntersect(VxCollisionGeometry* cg, VxCollisionSensor* sensor);   // Mac 0x480df4, Win 0x708d40
    void enablePairIntersect(VxCollisionSensor* s0, VxCollisionSensor* s1);         // Mac 0x480e4c, Win 0x709640
    // Mac 0x480eaa, Win 0x7082a0 ("VxUniverse::disablePairIntersect: both collision gemoetry
    // must be added ...", at most 30 times)
    bool disablePairIntersect(VxCollisionGeometry* cg0, VxCollisionGeometry* cg1);
    void disablePairIntersect(VxCollisionGeometry* cg, VxCollisionSensor* sensor);  // Mac 0x480f68, Win 0x708c80
    void disablePairIntersect(VxCollisionSensor* s0, VxCollisionSensor* s1);        // Mac 0x480fe0, Win 0x7095d0
    // Mac 0x481b72, Win 0x708dc0: the universe's disabled pairs, then (with `visit`) every
    // assembly's (VxAssemblyBase::applyDisabledCollisions)
    void applyDisabledCollisions(bool visit);

    // --- Win-only API (dead-stripped on the Mac) ---
    // Win 0x708b80: VxBridge::setContactBufferSize unless stepping or a zero argument
    void setContactBufferSize(int contactCount, int bufferCount);
    void getContactBufferSize(int& contactCount, int& bufferCount) const; // Win 0x708bc0
    // Win 0x708760: a user contact between two parts (one at least dynamic and in this universe;
    // the default contact material when material is 0)
    VxDynamicsContact* createUserDynamicsContact(VxPart* p0, VxPart* p1, VxContactMaterial* material,
                                                 const VxVector3& position, const VxVector3& normal, VxReal penetration);
    void deleteUserDynamicsContact(VxDynamicsContact* c); // Win 0x708830

    // the root assembly connection + world / space / bridge creation (Mac 0x481c06, Win 0x709180)
    // (name inferred)
    void initialize(int maxPartCount, int maxPairCount);

    // (VxEntityConstraintCollection<VxUniverse> at Mac +0x34, Win +0x50)
    VxMechanism* mRootMechanism;    // Mac +0x40, Win +0x5c
    VxArray<UpdateSubscriber*> m44; // Mac +0x44, Win +0x60 (update subscribers)
    bool mStepping;                 // Mac +0x54, Win +0x70 (checked by VxPart before freeze/remove)
    int m58;                        // Mac +0x58, Win +0x74 (maxPartCount rounded up to even) (name unknown)
    MdtWorldID mWorld;              // Mac +0x5c, Win +0x78
    McdSpaceID mSpace;              // Mac +0x60, Win +0x7c
    VxBridge* mIntersectManager;    // Mac +0x64, Win +0x80 (Vx/VxBridge.h; new(0x64)) (name inferred)
    VxReal mTime1;                  // Mac +0x68, Win +0x88 (collision ms)
    VxReal mTime2;                  // Mac +0x70, Win +0x90 (dynamics ms)
    UpdateSubscriber* m78;          // Mac +0x78, Win +0x98 (VxFluidInteractionUniverseUpdate, lazy)
    bool mFlag;                     // Mac +0x7c, Win +0x9c (ctor argument 3: VincSpaceAxisSortCreate's flag)
    VxInternalIDIntersectSubscriber* mInternalIDSubscriber; // Mac +0x80, Win +0xa0
    IntersectSubscriber* m84;       // Mac +0x84, Win +0xa4 (deleted by the dtor; never set) (type unknown)
    VxDisabledPairs* m88;           // Mac +0x88, Win +0xa8 (new(0x30), Mac 0x422bfe)
    bool m8c;                       // Mac +0x8c, Win +0xac (= false, not read)
};

} // namespace Vx

#endif
