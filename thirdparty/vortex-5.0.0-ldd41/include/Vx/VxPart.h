// Vortex 5.0.0 build 800 "vortexldd41" - VxPart, VxMassProperties, VxPartMergeInfo (restored
// header).
//
// A VxPart is a VxCollisionSensor with an Mdt rigid body (mBody). Most setters forward to
// MdtBody (Mdt/MdtBody.h); the part's transform lives inside the MdtBody (mTransformPtr is
// set from MdtBodyGetTransformPtr, Mac 0x4689da -> 0x48ae1b).
// Sources: src/Vx/VxPart.cpp (Mac TU 508 0x467efe-0x46b68f, Win 0x71b7f0-0x71e930 + 0x730a50 /
// 0x730a80 / 0x731ac0), src/Vx/VxPartMergeInfo.cpp (Mac TU 509 0x46b690-0x46c87d, Win
// 0x777eb0-0x778000), src/Vx/VxMassProperties.cpp (Mac TU 503 0x45e31a-0x45ff0c, Win
// 0x761c40-0x762fe0).
#ifndef VX_VXPART_H
#define VX_VXPART_H

#include "Vx/VxEntity.h"
#include "Mdt/MdtTypes.h"

namespace Vx {

class VxPart;
class VxDynamicsResponseInput;
class VxGeometry;

// --------------------------------------------------------------------------------------
// VxMassProperties (Mac vtable 0x75e128, 6 slots; class id = VxBase's 0x101: slots 0/1 are
// VxBase's). Mac 0xec bytes (allocated with VxBase::operator new(0xec), Mac 0x4689da; deleted
// through slot 3 = VxBase::operator delete), Win 0x108. Also embedded in VxPartMergeInfo.
// Mass, centre-of-mass frame (relative to the part) and inertia tensor about it.
class VxMassProperties : public VxBase {
public:
    // C1 Mac 0x45e7b8 -> C2 0x45e732: keeps the entity's transform pointer, identity frame,
    // mass 0, inertia 0
    explicit VxMassProperties(VxEntity* entity);
    virtual ~VxMassProperties();                        // slot 2/3 Mac 0x45e62e (D2 0x45e604)/0x45e5cc, Win 0x762fe0
    virtual const char* getClassName() const;           // slot 4 Mac 0x45fe48, Win 0x762fd0 "VxMassProperties"

    void reset();                                       // Mac 0x45e674: identity frame, zero inertia (mass kept) (name inferred)
    void setMass(VxReal mass) { mMass = mass; }         // Mac 0x45e31a, Win 0x761f60 (out of line) (name inferred)
    VxReal getMass() const { return mMass; }
    void setInertiaTensor(const VxReal inertia[3][3]);  // Mac 0x45e872, Win 0x761fd0 (name inferred)
    void getInertiaTensor(VxReal inertia[3][3]) const;  // Mac 0x45e8f8, Win 0x761f70 (name inferred)
    void scaleInertiaTensor(VxReal s);                  // Mac 0x45f166, Win 0x762030 (name inferred)
    // Mac 0x45f14a -> 0x45eb0a, Win 0x762f60: clamps the principal moments (>= 1% of the largest,
    // triangle inequality) and rebuilds a symmetric tensor; false if nothing changed (name inferred)
    bool makeInertiaTensorValid();
    void setCOMTransform(const VxTransform& tm);        // Mac 0x45e4e4, Win 0x762180 (name inferred)
    // Mac 0x45e7d2, Win 0x7620c0: identity rotation with that translation (name inferred)
    void setCOMPosition(const VxVector3& p);
    void getCOMPosition(VxVector3& p) const;            // Mac 0x45e642, Win 0x762130 (name inferred)
    // Mac 0x45e32c, Win 0x761c40: COM position through the entity's transform (name inferred)
    void getCOMWorldPosition(VxVector3& p) const;
    void copy(const VxMassProperties& m);               // Mac 0x45e97e, Win 0x761e10 ("VxMassProperties::copy")
    // Mac 0x45fdf8 / 0x45fda8 (Win 0x762f70 / 0x762fa0) -> 0x45f2e4 (Win 0x762790): combines m,
    // placed by tm in this frame (parallel axis theorem); subtract uses -mass
    void addMassProperties(const VxMassProperties& m, const VxTransform& tm);
    void subtractMassProperties(const VxMassProperties& m, const VxTransform& tm);

    const VxTransform* mTransformPtr;   // Mac +0x18, Win +0x30 (the entity's mTransformPtr) (name inferred)
    VxReal mMass;                       // Mac +0x1c, Win +0x38
    VxTransform mCOMTransform;          // Mac +0x24, Win +0x40 (COM frame in the part frame) (name inferred)
    VxReal mInertia[3][3];              // Mac +0xa4, Win +0xc0

private:
    void combine(const VxMassProperties& m, bool subtract, const VxTransform& tm); // Mac 0x45f2e4
    VxMassProperties(const VxMassProperties&);
    VxMassProperties& operator=(const VxMassProperties&);
};

// --------------------------------------------------------------------------------------
// The merge tree of parts (VxPart::setMergeParent): a merged part moves with its root part; the
// root's body carries the summed mass. No vtable, Mac 0x208 bytes, Win 0x220 (operator new(0x208)
// Mac 0x468d3e). (class and member names inferred)
class VxPartMergeInfo {
public:
    // the object at +0x204 (never set by the restored code; slot 3 asks for a separation)
    class SeparationTest {
    public:
        virtual ~SeparationTest() {}
        virtual void vf02() = 0;
        virtual bool shouldSeparate() = 0;              // Mac slot 3 (+0xc) (name inferred)
    };

    explicit VxPartMergeInfo(VxPart* part);             // C1 Mac 0x46bd3e -> C2 0x46ba86, Win 0x778000
    void addChild(VxPartMergeInfo* child);              // Mac 0x46b690
    void removeChild(VxPartMergeInfo* child);           // Mac 0x46b6b6
    // Mac 0x46b71c (Win 0x777f10) / 0x46b7bc (Win 0x777eb0): remove / add m (placed by tm) from
    // the summed mass properties up to the root; the root part's body gets them when apply
    void subtractMassProperties(VxMassProperties* m, const VxTransform* tm, bool apply);
    void addMassProperties(VxMassProperties* m, const VxTransform* tm, bool apply);
    void computeTransform(VxTransform& tm) const;       // Mac 0x46b852: relative transform * parent's
    void collectChildren(VxArray<VxPartMergeInfo*>& out) const;    // Mac 0x46bd58
    void collectDescendants(VxArray<VxPartMergeInfo*>& out) const; // Mac 0x46c714 (recursive)
    void updateRoot();                                  // Mac 0x46be4c (recursive)
    bool setParent(VxPartMergeInfo* parent);            // Mac 0x46c13a (always true)

    VxPart* mPart;                      // +0x00
    VxPartMergeInfo* mParent;           // +0x04
    VxPartMergeInfo* mRoot;             // +0x08 (this when not merged)
    VxPartMergeInfo* mFirstChild;       // +0x0c
    VxPartMergeInfo* mNext;             // +0x10
    VxPartMergeInfo* mPrevious;         // +0x14
    VxTransform mRelativeTransform;     // +0x18 relative to the parent part
    VxTransform mRootTransform;         // +0x98 relative to the root part
    VxMassProperties mMassProperties;   // +0x118 / Win +0x118 (this part + its merged children)
    SeparationTest* mSeparationTest;    // Mac +0x204, Win +0x220

private:
    void resetTransforms();
};

// --------------------------------------------------------------------------------------
// VxPart (Mac vtable 0x75e408, 41 slots; Win 0x9f7704, 40; class id 0x108 Mac 0x467f12)
// Mac 0x1dc bytes, Win 0x1f8 (glue: operator new(0x1dc) Mac 0x468237 / new(0x1f8) Win 0x71b8e0).
class VxPart : public VxCollisionSensor {
public:
    // The contact handler of the part's body (MdtBodySetM548 callback, Mac 0x4681ca): slot 2 gets
    // the part and the contact (class and method names inferred)
    class ContactHandler {
    public:
        virtual ~ContactHandler() {}
        virtual void handleContact(VxPart* part, VxDynamicsResponseInput* input) = 0;
    };

    VX_DECLARE_ALLOCATOR();                             // new Mac 0x468237 / Win 0x71b8e0, delete Mac 0x468294 / Win 0x707f90
    static int getStaticClassType();                    // Mac 0x467f12, Win 0x71b910: 0x108
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x46acd6
    VxPart();                                           // C1 Mac 0x46b1ae -> C2 0x46b0a4, Win 0x71e560
    explicit VxPart(VxReal mass);                       // Win 0x71e620 (Mac dead-stripped)
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x46b05e, Win 0x71e530
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x467f1c, Win 0x71c7a0
    virtual ~VxPart();                                  // slot 2/3 Mac 0x46b384 (D2 0x46b1c2)/0x46b398, Win 0x71e930 (D2 0x71d480)
    virtual const char* getClassName() const;           // slot 4 Mac 0x46b56a, Win 0x71e920 "VxPart"
    virtual void setName(const char* name);             // slot 5 Mac 0x4680ee, Win 0x71c580 (also names the MdtBody)
    virtual bool isPart() const;                        // slot 6 Mac 0x46b560 (true)
    virtual void setControl(int control);               // slot 7 Mac 0x46a9aa, Win 0x71dd60
    virtual void freeze(bool b);                        // slot 8 Mac 0x4699e0, Win 0x71c9e0
    virtual int getConstraintCount() const;             // slot 9 Mac 0x467f32, Win 0x71c880
    virtual VxConstraint* getConstraint(int i) const;   // slot 10 Mac 0x467f4a, Win 0x71d390
    virtual void setTransform(const VxTransform& tm);   // slot 15 Mac 0x46931c, Win 0x71cbb0
    // slot 16 Mac 0x468b76, Win 0x71bf60: in a universe and not frozen with dt != 0:
    // MdtBodyMoveTo + slot 17; otherwise setTransform (frozen: VxInfo first) (name inferred)
    virtual void moveTo(const VxTransform& tm, VxReal dt);
    virtual void removeFromUniverse();                  // slot 22 Mac 0x468ff2, Win 0x71c890
    virtual void addToUniverse(VxUniverse* u);          // slot 23 Mac 0x469bc2, Win 0x71c940
    virtual void updateFromMergeParent();               // slot 24 Mac 0x4686e8, Win 0x71c710
    virtual void addCollisionGeometry(VxCollisionGeometry* cg); // slot 25 Mac 0x46a5b4, Win 0x71dc50
    virtual bool removeCollisionGeometry(int index);    // slot 27 Mac 0x46a52a, Win 0x71dcf0
    using VxCollisionSensor::removeCollisionGeometry;
    using VxCollisionSensor::addGeometry;
    virtual void setLinearVelocity(const VxVector3& v); // slot 29 Mac 0x46886e, Win 0x71ba00
    virtual void setAngularVelocity(const VxVector3& w); // slot 30 Mac 0x4687f8, Win 0x71ba60
    virtual void setLinearVelocity(VxReal x, VxReal y, VxReal z);  // slot 31 Mac 0x468834, Win 0x71ba30
    virtual void setAngularVelocity(VxReal x, VxReal y, VxReal z); // slot 32 Mac 0x4687be, Win 0x71ba90
    virtual void getLinearVelocity(VxReal& x, VxReal& y, VxReal& z) const;  // slot 33 Mac 0x467f6e, Win 0x71bac0
    virtual void getAngularVelocity(VxReal& x, VxReal& y, VxReal& z) const; // slot 34 Mac 0x467fb0, Win 0x71bb00
    virtual void getLinearVelocity(VxVector3& v) const;  // slot 35 Mac 0x46879e, Win 0x71bb40
    virtual void getAngularVelocity(VxVector3& w) const; // slot 36 Mac 0x46877e, Win 0x71bb60
    // slot 39 Mac 0x46a7cc, Win 0x71e720: dynamic part: new collision geometry with density =
    // mass / volume (the part's mass grows by mass when it is user-set); the relative transform
    // is ignored then (NOTE: original); otherwise addGeometry(geometry, material, relative, 1.0)
    // (name inferred)
    virtual VxCollisionGeometry* addGeometryByMass(VxGeometry* geometry, VxMaterial* material,
                                                   const VxTransform* relative, VxReal mass);
    // slot 40 Mac 0x468b2a, Win 0x71bb80: dynamic parts only, MdtBody +0x404 (Kea acceleration)
    // (name inferred)
    virtual void getLinearAcceleration(VxVector3& a) const;

    // --- mass ---
    VxReal getMass() const;                              // Mac 0x467ff2, Win 0x71bbe0: dynamic ? mass : 0 (name inferred)
    void setMass(VxReal mass);                           // Mac 0x46a670, Win 0x71e060 ("VxPart::setMass")
    void setMassAndInertia(VxReal mass, const VxReal inertia[3][3]); // Win 0x71cb00 (Mac dead-stripped) (name inferred)
    VxMassProperties* getMassProperties() const { return mMassProperties; } // Mac 0x468010 (out of line)
    // Mac 0x468adc, Win 0x71bd40: dynamic parts: COM position in world (name inferred)
    void getCOMPosition(VxVector3& p) const;
    // Mac 0x46a4e8, Win 0x714600: dynamic part with geometry: clears the three mUser* flags and
    // recomputes the mass properties from the collision geometries (name inferred)
    void updateMassPropertiesFromGeometry();
    void computeMassProperties();                        // Mac 0x46a0a4, Win 0x71d9e0 (name inferred)
    void applyMassProperties(VxMassProperties* mp);      // Mac 0x46a066, Win 0x71d9a0 (name inferred)
    void applyCenterOfMass(VxMassProperties* mp);        // Mac 0x469dd4, Win 0x71d6f0 (name inferred)
    void applyMass(VxMassProperties* mp);                // Mac 0x468564, Win 0x730a50 (name inferred)
    void applyInertia(VxMassProperties* mp);             // Mac 0x46853e, Win 0x730a80 (name inferred)
    void updateCenterOfRotation();                       // Mac 0x46831a, Win 0x71c440 (name inferred)
    // Mac 0x46837e, Win 0x71a430: re-expresses c's attachments on this body after a new COM frame
    // (name inferred)
    void updateConstraintAttachments(VxConstraint* c, const VxTransform& comTransform);
    // flags written directly by the glue after setMass (Mac 0x3abd90: +0x1d6 = 1, +0x1d4/0x1d5 = 1)
    void setUserMassFlags(bool mass, bool com, bool inertia)
    {
        mUserMass = mass;
        mUserCOM = com;
        mUserInertia = inertia;
    }

    // --- dynamics ---
    void resetForces();                                  // Mac 0x4689c0, Win 0x521370 (MdtBodyResetForces)
    void addForce(const VxVector3& f);                   // Mac 0x469d20 (name inferred)
    void addForceAtPosition(const VxVector3& f, const VxVector3& p); // Mac 0x469248 (name inferred)
    void addTorque(const VxVector3& t);                  // Mac 0x469c76 (name inferred)
    void addImpulse(const VxVector3& i);                 // Mac 0x469194, Win 0x71d3c0 (name inferred)
    void addImpulseAtPosition(const VxVector3& i, const VxVector3& p); // Mac 0x4690c0, Win 0x71cfe0 (name inferred)
    // Mac 0x468c72, Win 0x71c7c0: (merge root) dynamic: enable / disable the body, kinematic:
    // wake it up (name inferred)
    void enableDynamics(bool enable);
    bool isEnabled() const;                              // Mac 0x4688ee, Win 0x71b9b0 (name inferred)
    int getEnabledTime() const;                          // Mac 0x4688aa (name inferred)
    void setEnabledTime(int t);                          // Mac 0x4688c4, Win 0x719ec0 (dynamic parts) (name inferred)
    void getVelocityAtPoint(const VxVector3& p, VxVector3& v) const; // Mac 0x4685b2 (name inferred)
    void getAngularAcceleration(VxVector3& a) const;     // Mac 0x46858c: of the merge root body (name inferred)
    void getFrameLinearAcceleration(VxVector3& a) const; // Mac 0x468c16 (name inferred)
    const VxReal* getLinearVelocityPtr() const;          // Mac 0x4686ce (name inferred)
    const VxReal* getAngularVelocityPtr() const;         // Mac 0x4686b4 (name inferred)
    VxReal getLinearVelocityDamping() const;             // Mac 0x4685f4, Win 0x71c020 (name inferred)
    VxReal getAngularVelocityDamping() const;            // Mac 0x4685da, Win 0x71c030 (name inferred)
    void setLinearVelocityDamping(VxReal d);             // Mac 0x46860e, Win 0x71bfe0 (name inferred)
    void setAngularVelocityDamping(VxReal d);            // Mac 0x468b06, Win 0x71c000 (name inferred)
    VxReal getAutoDisableVelocityThreshold() const;      // Mac 0x46869a (name inferred)
    VxReal getAutoDisableAngularVelocityThreshold() const; // Mac 0x468666 (name inferred)
    VxReal getAutoDisableAccelerationThreshold() const;  // Mac 0x468680 (name inferred)
    VxReal getAutoDisableAngularAccelerationThreshold() const; // Mac 0x46864c (name inferred)
    int getAutoDisableAliveWindow() const;               // Mac 0x468632 (name inferred)
    // Mac 0x468a90: MdtBodyGetDefaultAutoDisableParams (name inferred)
    static void getDefaultAutoDisableParameters(VxReal& velocity, VxReal& acceleration,
                                                VxReal& angularVelocity, VxReal& angularAcceleration,
                                                int& aliveWindow);
    int getGrouping() const;                             // Mac 0x4680d4, Win 0x731ac0 (name inferred)
    void setGrouping(int grouping);                      // Mac 0x468128, Win 0x71c530 (name inferred)
    void setContactHandler(ContactHandler* h);           // Mac 0x468168: deletes the old one (name inferred)
    ContactHandler* getContactHandler() const;           // Mac 0x46801e (name inferred)

    // --- constraints (VxConstraint::setPart, V4.3) ---
    void addConstraint(VxConstraint* c);                 // Mac 0x46ab92, Win 0x71e8c0 (name inferred)
    // Mac 0x469afa: removes c when it uses this part exactly once (name inferred)
    void removeConstraint(VxConstraint* c);

    // --- merging ---
    bool setMergeParent(VxPart* parent);                 // Mac 0x468d3e, Win 0x71d180 ("VxPart::setMergeParent")
    VxPart* getMergeParent() const;                      // Mac 0x468032, Win 0x71c690 (name inferred)
    VxPart* getMergeRoot();                              // Mac 0x468068, Win 0x71c6c0 (name inferred)
    bool isMergeRoot() const;                            // Mac 0x468080 (name inferred)
    const VxTransform& getMergeRootTransform() const;    // Mac 0x468054 (NOTE: needs merge info) (name inferred)
    void separateMergedParts();                          // Mac 0x468f9c, Win 0x71b920 (name inferred)
    MdtBody** getMergeRootBodyPtr() const;               // Mac 0x4680a0, Win 0x71c700: &mBody->self (name inferred)

    MdtBodyID getMdtBody() const { return mBody; }

    static bool sSweepByDefault;        // Mac 0x779420, Win 0xaf14c4 (never set by LDD) (name inferred)

    MdtBodyID mBody;                    // Mac +0x1bc, Win +0x1d8 (MdtBodyCreateFromPool, Mac 0x48c91f)
    VxArray<VxConstraint*> mConstraints; // Mac +0x1c0, Win +0x1dc (slots 9/10) (name inferred)
    VxMassProperties* mMassProperties;  // Mac +0x1d0, Win +0x1ec
    bool mUserMass;                     // Mac +0x1d4, Win +0x1f0 (setMass sets it) (name inferred)
    bool mUserCOM;                      // Mac +0x1d5, Win +0x1f1 (computeMassProperties keeps the COM) (name inferred)
    bool mUserInertia;                  // Mac +0x1d6, Win +0x1f2 (... keeps the inertia) (name inferred)
    VxPartMergeInfo* mMergeInfo;        // Mac +0x1d8, Win +0x1f4 ("VxPart::setMergeParent")

private:
    void init();                                         // Mac 0x4689da, Win 0x71d8c0 (name inferred)
    static void contactCallback(MdtBody* body, MdtConstraint* contact); // Mac 0x4681ca (name inferred)
    // the part that takes this part's forces: separates from a merge parent whose test asks for
    // it, otherwise the merge root (0 = the root, already resolved) (inlined helper)
    VxPart* forceTarget();
};

} // namespace Vx

#endif
