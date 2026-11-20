// Vortex 5.0.0 build 800 "vortexldd41" - VxEntity, VxCollisionSensor (restored header).
// Slot numbers are Mac (Itanium) indices; Win = Mac - 1 after the destructor (Mac 2/3, Win 2).
#ifndef VX_VXENTITY_H
#define VX_VXENTITY_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxTransform.h"
#include "Vx/VxVector3.h"
#include "Vx/VxSceneGraphInterface.h"

namespace Vx {

class VxUniverse;
class VxGeometry;
class VxCollisionGeometry;
class VxConstraint;
class VxAssemblyBase;
class VxMaterial;

// --------------------------------------------------------------------------------------
// VxEntity (Mac vtable 0x75dc48, 25 slots; Win 24; class id 0x100 Mac 0x446780)
// Mac 0x174 bytes, Win 0x190. src/Vx/VxEntity.cpp (Mac TU 484 0x44664e-0x447dd6,
// Win 0x7169b0-0x717cc0).
//
// An entity has a transform (mTransformPtr: its own mTransform, or the MdtBody's for a VxPart),
// a control type, an optional parent entity it follows through an AnimationDriver (children
// list mChildren), an optional scene-graph node and the universe it was added to.
class VxEntity : public VxBaseUserData {
public:
    // Control types (mControl). The glue gives parts 0 (anchor) or 2 (dynamic); VxPart::setControl
    // makes 0 and 1 kinematic (MdtBodySetAnimated) and 2 dynamic. While frozen, mInternalControl
    // is kControlFrozen (the constant Mac 0x643e70 = -1, read through 0x7024f0). (names inferred)
    enum ControlType {
        kControlFrozen = -1,
        kControlStatic = 0,
        kControlAnimated = 1,
        kControlDynamic = 2
    };

    // VxEntity::AnimationDriver (Mac typeinfo 0x75dcd8; Win vtable 0x9f6e74: dtor, pure drive):
    // moves an entity that follows a parent entity (setParent) (method name inferred)
    class AnimationDriver {
    public:
        virtual ~AnimationDriver() {}                   // Win 0x717c20 (shared)
        virtual void drive(VxEntity* parent, VxEntity* entity, VxReal dt) = 0; // slot 2 (Win 1)
    };
    // Mac vtable 0x75dcc0, Win 0x9f6e80: entity->moveTo(parent transform, dt)
    class FollowAnimationDriver : public AnimationDriver {
    public:
        virtual ~FollowAnimationDriver();               // Mac 0x447cf0/0x447cc6, Win 0x717c20
        virtual void drive(VxEntity* parent, VxEntity* entity, VxReal dt); // Mac 0x447d0a, Win 0x717a30
    };
    // Win vtable 0x9f6e94 (Mac: typeinfo only, the code was dead-stripped): moveTo(offset * parent
    // transform, dt). Win 0x108 bytes (Mac 0x104). setParent deletes a driver of this type when it
    // is replaced (Mac 0x447544: dynamic_cast to typeinfo 0x75dce4).
    class FollowWithOffsetAnimationDriver : public AnimationDriver {
    public:
        explicit FollowWithOffsetAnimationDriver(const VxTransform& offset); // Win 0x717a60
        virtual void drive(VxEntity* parent, VxEntity* entity, VxReal dt);  // Win 0x717be0
        VxTransform mOffset;    // Mac +0x04, Win +0x08 (name inferred)
        VxTransform mResult;    // Mac +0x84, Win +0x88 (offset * parent transform) (name inferred)
    };

    static int getStaticClassType();                    // Mac 0x446780: 0x100
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x447090: {0x100, 0x198, 0x101}
    VxEntity();                                         // Mac 0x446eee
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x447418, Win 0x717620
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x44678a, Win 0x717070
    virtual ~VxEntity();                                // slot 2/3 Mac 0x447896 (D2 0x44774e)/0x447600, Win 0x717cc0 (D2 0x7177d0)
    virtual const char* getClassName() const;           // slot 4 Mac 0x447ba8, Win 0x717c50 "VxEntity"
    virtual void setName(const char* name);             // slot 5 Mac 0x446bde, Win 0x7169b0 (indexed in sNameMap)
    virtual bool isPart() const = 0;                    // slot 6 (VxCollisionSensor 0x43b2a4 false, VxPart 0x46b560 true) (name inferred)
    virtual void setControl(int control) = 0;           // slot 7 (VxPart Mac 0x46a9aa; glue: part->setControl(2))
    virtual void freeze(bool b);                        // slot 8 Mac 0x447be0, Win 0x717c80
    // slots 9/10: inline (out-of-line copies Mac 0x43b25e / 0x43b268, emitted in VxCollisionSensor's TU)
    virtual int getConstraintCount() const { return 0; }                // slot 9 (name inferred)
    virtual VxConstraint* getConstraint(int /*i*/) const { return 0; }  // slot 10 (name inferred)
    virtual int getCollisionGeometryCount() const;      // slot 11 Mac 0x447bba returns 0 (name inferred)
    virtual VxCollisionGeometry* getCollisionGeometry(int i) const; // slot 12 Mac 0x447bc4 returns 0 (name inferred)
    virtual bool getBoundingBox(VxVector3& min, VxVector3& max) const = 0;  // slot 13 (CS Mac 0x43a46e) (name inferred)
    virtual bool getBoundingSphere(VxVector3& center, VxReal& radius) const = 0; // slot 14 (CS Mac 0x43a644) (name inferred)
    virtual void setTransform(const VxTransform& tm);   // slot 15 Mac 0x446ae0, Win 0x717c60
    // slot 16: inline (out-of-line Mac 0x43b272, Win 0x716a50): setTransform(tm) (VxPart: MdtBodyMoveTo) (name inferred)
    virtual void moveTo(const VxTransform& tm, VxReal /*dt*/) { setTransform(tm); }
    // slot 17 Mac 0x447bce (empty); VxCollisionSensor 0x439fe0 updates every collision geometry
    // (cg slot 21) (name inferred)
    virtual void updateCollisionGeometryTransforms();
    // slots 18-20: response model of the entity (the universe's response table key) (names inferred)
    virtual void registerResponseModel() = 0;           // slot 18 (CS Mac 0x43b378)
    virtual int getResponseModel() const = 0;           // slot 19 (CS Mac 0x43b2ae)
    virtual bool setResponseModel(int model) = 0;       // slot 20 (CS Mac 0x43a156)
    virtual void setUniverse(VxUniverse* u);            // slot 21 Mac 0x44681e, Win 0x716ff0 (name inferred)
    virtual void removeFromUniverse();                  // slot 22 Mac 0x447bd4 (empty) (name inferred)
    virtual void addToUniverse(VxUniverse* u);          // slot 23 Mac 0x447bda (empty) (name inferred)
    // slot 24: inline (out-of-line Mac 0x43b28c, empty); VxPart 0x4686e8 follows its merge parent
    // (name inferred)
    virtual void updateFromMergeParent() {}

    // --- names ---
    // Mac 0x446bac, Win 0x7169d0: VxBase::findFromName in sNameMap (0 when the map is empty)
    static VxEntity* findFromName(const char* name);
    static VxBase::NameMap* sNameMap;                   // Mac 0x77931c (name inferred)

    // --- transform ---
    void getTransform(VxTransform& tm) const;           // Mac 0x4478aa, Win 0x716fd0 (copies *mTransformPtr)
    const VxReal* getPositionPtr() const;               // Mac 0x446c58, Win 0x716a70: &(*mTransformPtr)[3][0] (name inferred)
    void getPosition(VxVector3& p) const;               // Mac 0x4467ae, Win 0x716a80 (name inferred)
    // Mac 0x446c0e, Win 0x716c70: Euler angles of the transform in the given order (Mac 0x447df8)
    // (name inferred)
    void getEulerAngles(VxVector3& angles, int order) const;
    void setScale(const VxVector3& s);                  // Mac 0x4467da (name inferred)

    // --- control ---
    void setInternalControl(int control);               // Mac 0x44693a, Win 0x717340 (name inferred)
    void updateAnimationMode();                         // Mac 0x4468a6 (name inferred)

    // --- parent / animation driver ---
    // Mac 0x447544: leaves the old parent, deletes the old driver if it is a
    // FollowWithOffsetAnimationDriver, refuses a parent cycle (both cleared) (name inferred)
    void setParent(AnimationDriver* driver, VxEntity* parent);
    void addChild(VxEntity* child);                     // Mac 0x44745e (name inferred)
    void removeChild(VxEntity* child);                  // Mac 0x446956 (name inferred)
    int getChildCount() const;                          // Mac 0x44680c (name inferred)
    // Mac 0x4469b6: a static entity with a parent and a driver is moved by the driver (universe
    // time step), then the same for every child (name inferred)
    void applyAnimationDriver();
    void applyAnimationDriverIfMode0();                 // Mac 0x446a42 (name inferred)
    void applyAnimationDriverIfNotMode2();              // Mac 0x446c66 (name inferred)
    // Mac 0x446c82: slot 24, an animated entity moves to its user transform, then the driver
    // when the animation mode is 1 (name inferred)
    void updateBeforeStep();

    // --- scene-graph node ---
    void setNodeParent(VxEntity* e) { mNodeParent = e; } // Mac 0x4467a0 (out of line) (name inferred)
    // Mac 0x446d6e: binds a node (0 = unbind); mode 1 pushes the transform to the node, otherwise
    // the entity takes the node's transform; true if the binding changed (name inferred)
    bool setNode(const VxNode& node, int mode);
    void updateFromNode();                              // Mac 0x446cda (name inferred)
    void updateNode();                                  // Mac 0x447a94, Win 0x7171f0 (name inferred)
    // Mac 0x447960, Win 0x716f60: the transform relative to mNodeParent / mNodeOffset (name inferred)
    void getNodeTransform(VxTransform& tm) const;

    VxUniverse* getUniverse() const { return mUniverse; }
    VxEntity* getParent() const { return mParent; }

    static FollowAnimationDriver sFollowAnimationDriver; // Mac 0x77cdd8 (vptr only) (name inferred)

    VxEntity* mParent;                  // Mac +0x20, Win +0x38 (name inferred)
    AnimationDriver* mAnimationDriver;  // Mac +0x24, Win +0x3c (name inferred)
    VxArray<VxEntity*> mChildren;       // Mac +0x28, Win +0x40 (name inferred)
    VxNode mNode;                       // Mac +0x38, Win +0x50 (name inferred)
    VxAssemblyBase* mAssembly;          // Mac +0x3c, Win +0x54 (VxPart::setMergeParent messages) (name inferred)
    VxUniverse* mUniverse;              // Mac +0x40, Win +0x58 (slot 21; "while stepping" check reads universe +0x54)
    VxEntity* mNodeParent;              // Mac +0x44, Win +0x5c (name inferred)
    VxTransform* mTransformPtr;         // Mac +0x48, Win +0x60 (= &mTransform after construction)
    VxTransform* mNodeOffset;           // Mac +0x4c, Win +0x64 (MeMemoryAPI block, freed by the dtor) (name inferred)
    VxTransform mUserTransform;         // Mac +0x50, Win +0x68: the last transform given to setTransform (name inferred)
    VxVector3 mScale;                   // Mac +0xd0, Win +0xe8 (1,1,1) (name inferred)
    int mControl;                       // Mac +0xe8, Win +0x100 (ControlType, = 1)
    VxTransform mTransform;             // Mac +0xec, Win +0x108 (identity)
    int mAnimationMode;                 // Mac +0x16c, Win +0x188 (0/1/2, updateAnimationMode; not initialised by the ctor) (name inferred)
    int mInternalControl;               // Mac +0x170, Win +0x18c: control actually applied (kControlFrozen while frozen) (name inferred)
};

// --------------------------------------------------------------------------------------
// VxCollisionSensor (Mac vtable 0x75d608, 39 slots; Win 38; class id 0x107)
// Mac 0x1bc bytes, Win 0x1d8. src/Vx/VxCollisionSensor.cpp (Mac TU 470
// 0x439a4c-0x43b519, Win 0x718700-0x7198b0).
// An entity without dynamics that owns collision geometries (VxPart derives from it). In a
// universe, its enabled geometries' models are in the universe's space, and the pairs between
// geometries of the same entity are disabled in that space. The models' velocity pointers are
// the sensor's velocities (a sensor moves only by setTransform; the velocities feed swept tests).
// Mac slot numbers; MSVC groups same-name overloads in reverse order (Win: removeCollisionGeometry
// (cg) before (int), setLinearVelocity(x, y, z) before (v), getLinearVelocity(v) before (x, y, z)).
class VxCollisionSensor : public VxEntity {
public:
    // The response model a new sensor gets (Mac *0x7025b8 -> 0x64432c = 1; Win: the constant)
    // and the "enabled" collision-geometry flag mask (Mac *0x7025bc -> 0x643d34 = 2) (names inferred)
    enum { kDefaultResponseModel = 1, kEnabledFlagMask = 2 };

    VX_DECLARE_ALLOCATOR();                             // delete Mac 0x439ec5 / Win 0x718850 (no Vortex code news a sensor)
    static int getStaticClassType();                    // Mac 0x439a4c: 0x107
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x43ae90, Win 0x719680 ({0x107, 0x100, 0x198, 0x101})
    VxCollisionSensor();                                // Mac 0x43ab54, Win 0x719340
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x43b218, Win 0x719700
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x439a56, Win 0x718af0
    // slot 2/3 Mac 0x43ab40 (D2 0x43aa50)/0x43a95a, Win 0x7198b0 (D2 0x7193f0): every collision
    // geometry leaves the space and is deleted
    virtual ~VxCollisionSensor();
    virtual const char* getClassName() const;           // slot 4 Mac 0x43b292, Win 0x719860
    virtual void setName(const char* name);             // slot 5 Mac 0x43a102, Win 0x718b10 (+ model names)
    virtual bool isPart() const;                        // slot 6 Mac 0x43b2a4, Win 0x719870 (false)
    virtual void setControl(int control);               // slot 7 Mac 0x43a3e2, Win 0x718700 (0 / 1 only)
    virtual void freeze(bool b);                        // slot 8 Mac 0x439bf8, Win 0x719270
    virtual int getCollisionGeometryCount() const;      // slot 11 Mac 0x43b2bc, Win 0x719890
    // slot 12 Mac 0x439ace, Win 0x719250. NOTE: no range check (the original's).
    virtual VxCollisionGeometry* getCollisionGeometry(int i) const;
    // slot 13 Mac 0x43a46e, Win 0x718f80: union of the boxes of the enabled (mode 0) geometries;
    // false (box = position) without one (name inferred)
    virtual bool getBoundingBox(VxVector3& min, VxVector3& max) const;
    // slot 14 Mac 0x43a644, Win 0x7190b0: sphere around the geometries' spheres (name inferred)
    virtual bool getBoundingSphere(VxVector3& center, VxReal& radius) const;
    virtual void setTransform(const VxTransform& tm);   // slot 15 Mac 0x43a36a, Win 0x7192c0
    virtual void updateCollisionGeometryTransforms();   // slot 17 Mac 0x439fe0, Win 0x718c40
    virtual void registerResponseModel();               // slot 18 Mac 0x43b378, Win 0x7198a0 (name inferred)
    virtual int getResponseModel() const;               // slot 19 Mac 0x43b2ae, Win 0x719880 (name inferred)
    // slot 20 Mac 0x43a156, Win 0x718f10: only when the universe's response table accepts the
    // change; pushed to every model (m80) (name inferred)
    virtual bool setResponseModel(int model);
    virtual void removeFromUniverse();                  // slot 22 Mac 0x43a2e6, Win 0x718ca0
    virtual void addToUniverse(VxUniverse* u);          // slot 23 Mac 0x43a1ca, Win 0x718d00
    // slot 25 Mac 0x43ac42, Win 0x719730: takes cg (from its previous sensor), model transform /
    // velocities / response model / name, space entry and sweep state (name inferred)
    virtual void addCollisionGeometry(VxCollisionGeometry* cg);
    // slot 26 Mac 0x439de0, Win 0x718760: new VxCollisionGeometry(geometry) with material,
    // optional relative transform and density, added by slot 25 (name inferred)
    virtual VxCollisionGeometry* addGeometry(VxGeometry* geometry, VxMaterial* material,
                                             const VxTransform* relative, VxReal density);
    virtual bool removeCollisionGeometry(int index);    // slot 27 Mac 0x439d56, Win 0x719600 (name inferred)
    // slot 28 Mac 0x43a5c8, Win 0x7194b0: a child of a composite is removed from the composite
    // ("VxCollisionSensor::removeCollisionGeometry: A VxCompositeCollisionGeometry has 0 children
    // and has been disabled.") (name inferred)
    virtual bool removeCollisionGeometry(VxCollisionGeometry* cg);
    virtual void setLinearVelocity(const VxVector3& v); // slot 29 Mac 0x43a8ae, Win 0x7189b0 (name inferred)
    virtual void setAngularVelocity(const VxVector3& w); // slot 30 Mac 0x439a9c, Win 0x7189e0 (name inferred)
    virtual void setLinearVelocity(VxReal x, VxReal y, VxReal z);  // slot 31 Mac 0x439a6c, Win 0x718950
    virtual void setAngularVelocity(VxReal x, VxReal y, VxReal z); // slot 32 Mac 0x43a598, Win 0x718980
    virtual void getLinearVelocity(VxReal& x, VxReal& y, VxReal& z) const;  // slot 33 Mac 0x439f12, Win 0x718a70
    virtual void getAngularVelocity(VxReal& x, VxReal& y, VxReal& z) const; // slot 34 Mac 0x439edc, Win 0x718aa0
    virtual void getLinearVelocity(VxVector3& v) const;  // slot 35 Mac 0x439f7a, Win 0x718a10
    virtual void getAngularVelocity(VxVector3& w) const; // slot 36 Mac 0x439f48, Win 0x718a40
    // slot 37 Mac 0x439fac, Win 0x718c70: VxCollisionGeometry::update() on every cg (name inferred)
    virtual void updateCollisionGeometries();
    // slot 38 Mac 0x43a074, Win 0x7188c0: cg's enabled flag changed: its model enters the space
    // (pairs with the other enabled geometries of this entity disabled) or leaves it (name inferred)
    virtual void collisionGeometryEnableChanged(VxCollisionGeometry* cg);

    // Mac 0x43a014, Win 0x718b60: sets mSweepEnabled and pushes it to every cg unless frozen
    // (cg slot 31) (name inferred; the glue calls it with true before setLinearVelocity on a
    // hit-test sensor)
    void setSweepEnabled(bool b);
    // Mac 0x439b28, Win 0x718bb0: per enabled collision geometry: frozen -> update, space reset
    // + freeze, sweep off; otherwise unfreeze and the sensor's sweep flag (called by freeze and
    // VxPart::enableDynamics) (name inferred)
    void updateCollisionGeometryStates();
    // Mac 0x439c66, Win 0x719520: removes entry `index`; in a universe an enabled model leaves
    // the space (removeFromSpace) or stays with its pairs to this entity's geometries
    // re-enabled; the cg loses its entity. Returns true. (name inferred)
    bool removeCollisionGeometry(int index, bool removeFromSpace);
    // Mac 0x43a8e0, Win 0x719620: the same by pointer (composite children: see slot 28) (name inferred)
    bool removeCollisionGeometry(VxCollisionGeometry* cg, bool removeFromSpace);
    // LDD64: pair group mode, for a sensor that only gains geometries and then loses them all: each
    // new geometry disables its pairs with the others as before, those with a record one by one,
    // the rest as one group (CxSmallSort::groupJoin) instead of O(n^2) steps. The same pair states.
    // Anything else (re-enabling, moving a geometry) ends the mode for the geometries after it.
    void setPairGroup(bool on);
    // in pair group mode with every geometry enabled (VxPart::addCollisionGeometry: nothing to do
    // for the disabled ones)
    bool pairGroupAllEnabled() const;

    VxArray<VxCollisionGeometry*> mCollisionGeometries; // Mac +0x174, Win +0x190
    int mResponseModel;         // Mac +0x184, Win +0x1a0 (slots 18-20) (name inferred)
    bool mSweepEnabled;         // Mac +0x188, Win +0x1a4 (setSweepEnabled) (name inferred)
    VxVector3 mLinearVelocity;  // Mac +0x18c, Win +0x1a8
    VxVector3 mAngularVelocity; // Mac +0x1a4, Win +0x1c0
};

} // namespace Vx

#endif
