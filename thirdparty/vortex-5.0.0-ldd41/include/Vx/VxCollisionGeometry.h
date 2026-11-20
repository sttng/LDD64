// Vortex 5.0.0 build 800 "vortexldd41" - VxCollisionGeometry, VxCompositeCollisionGeometry
// (restored header: VxCollisionGeometry.cpp = Mac TU 469,
// VxCompositeCollisionGeometry.cpp = Mac TU 471).
//
// A collision geometry = one VxGeometry (shape, refcounted) placed on an entity (a
// VxCollisionSensor or VxPart), with its own Vinc collision model (mModel, Vinc/VincModel.h):
// the model's transform pointer is the entity's transform (or relTM * entity transform when a
// relative transform is set), its velocity pointers the sensor's velocities, its user data
// this object. Composite geometries own a Vinc aggregate model (VincAggregateModelCreate) whose
// elements are the models of child collision geometries.
//
// API notes (intersection / response and the glue):
//  * VincModelGetUserData(model) is the VxCollisionGeometry; the model's state (getCollisionMode,
//    VincModelGetState) is 2 for a disabled child of a composite;
//  * flags (mFlags, initial 3): bit 0 = contributes mass (density != 0), bit 1 = enabled (the
//    model is in the universe's space while the entity is in a universe), bit 2 = has a fluid
//    interaction force function;
//  * the model's m7c is the geometry type ID (0; 1 for height fields / triangle meshes /
//    terrains; 5 for composites) that indexes the universe's pair-request table, m80 the
//    entity's response model (VxCollisionSensor::getResponseModel), ma4 a user value
//    (unk_437f26), ma8 the entity name (VxCollisionSensor::setName);
//  * Mac slot numbers; MSVC groups same-name overloads in reverse order (Win: getMassProperties
//    raw before the VxMassProperties one, getMaterial(int) before getMaterial(); composite
//    removeCollisionGeometry(cg) before (int)).
#ifndef VX_VXCOLLISIONGEOMETRY_H
#define VX_VXCOLLISIONGEOMETRY_H

#include "Vx/VxBase.h"
#include "Vx/VxTransform.h"
#include "Vx/VxVector3.h"
#include "Vx/VxSceneGraphInterface.h"
#include "Vinc/VincTypes.h"

namespace Vx {

class VxGeometry;
class VxCollisionSensor;
class VxMassProperties;
class VxMaterial;
class VxIntersectFilter;
class VxCollisionGeometry;

// Fluid interaction force callback (slot 23 calls it for dynamic parts; Win cdecl). The
// defaults per shape live in the fluid module (Mac TU 486). (names inferred)
typedef bool (*VxFluidInteractionForceFn)(VxCollisionGeometry* cg, void* fluidState);

// Fluid interaction parameters (operator new(0x40), slots 27-29) (names inferred)
struct VxFluidInteractionData {
    VxFluidInteractionData()
    {
        m00.setZero();
        m18.set(1.0, 1.0, 1.0);
        m30 = 0.0;
        m38 = 1.0;
    }
    VxVector3 m00;      // +0x00 (0, 0, 0)
    VxVector3 m18;      // +0x18 (1, 1, 1)
    VxReal m30;         // +0x30 (0)
    VxReal m38;         // +0x38 (1)
};

// --------------------------------------------------------------------------------------
// VxCollisionGeometry (Mac vtable 0x75d568, 32 slots; Win 31; class id 0x161)
// Mac 0x44 bytes, Win 0x5c (glue: new(0x44) Mac 0x4387bf / new(0x5c) Win 0x70fb50).
class VxCollisionGeometry : public VxBaseUserData {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x4387bf / Win 0x70fb50, delete Mac 0x4387a8 / Win 0x70fb60
    // getStaticClassType Mac 0x437d52 (0x161); types Mac 0x4395a0, Win 0x711060; isOfType slot 0
    // Mac 0x439928, Win 0x7110e0; isOfClassType slot 1 Mac 0x437d5c, Win 0x7104d0
    VX_DECLARE_CLASS_TYPES();
    // C1 Mac 0x438744 -> C2 0x43864c, Win 0x7104f0: addRef's the shape; with a Vinc geometry a
    // Vinc model (identity transform, user data this) and the default geometry type ID
    explicit VxCollisionGeometry(VxGeometry* geometry);
    VxCollisionGeometry();                              // Mac 0x4385dc, Win 0x710600 (no model: the composite's base)
    virtual ~VxCollisionGeometry();                     // slot 2/3 Mac 0x439440 (D2 0x439318)/0x4391e8, Win 0x7111a0 (D2 0x710e00)
    virtual const char* getClassName() const;           // slot 4 Mac 0x43996e, Win 0x711160
    virtual void setName(const char* name);             // slot 5 Mac 0x4399b6, Win 0x70f990 (name map sNameMap)
    // slot 6 Mac 0x439036, Win 0x710f00: a copy on the same shape (or a clone of it), with the
    // density, material, response model, relative transform, flags, collision mode, fluid
    // data and function (name inferred)
    virtual VxCollisionGeometry* clone(bool cloneGeometry);
    virtual void prepare();                             // slot 7 Mac 0x437d72, Win 0x70fbf0: shape slot 8 (name inferred)
    // slot 8 Mac 0x438d06, Win 0x710890 ("VxCompositeCollisionGeometry::replaceGeometry")
    virtual void replaceGeometry(VxGeometry* geometry);
    virtual VxGeometry* getGeometry();                  // slot 9 Mac 0x439980, Win 0x711170
    virtual const VxGeometry* getGeometry() const;      // slot 10 Mac 0x43998c, Win 0x711170
    virtual void setMaterial(VxMaterial* material);     // slot 11 Mac 0x437d88, Win 0x70fcb0 (name inferred)
    virtual VxMaterial* getMaterial() const;            // slot 12 Mac 0x437d96, Win 0x70fcc0 (name inferred)
    // slot 13 Mac 0x438b0a, Win 0x7109d0: the shape's per-element material when the index is
    // valid, else getMaterial() (name inferred)
    virtual VxMaterial* getMaterial(int index) const;
    virtual bool setDensity(VxReal density);            // slot 14 Mac 0x438a6c, Win 0x710640: flag bit 0 = density != 0 (name inferred)
    virtual VxReal getDensity() const;                  // slot 15 Mac 0x43854a, Win 0x70fca0 (name inferred)
    // slot 16 Mac 0x438190, Win 0x710030: slot 17 into a VxMassProperties (name inferred)
    virtual bool getMassProperties(VxMassProperties& mp, VxReal* volume);
    // slot 17 Mac 0x438218, Win 0x70ffd0: VincModelGetMassProperties (mass, inertia, centre of
    // mass = relTM translation, volume) (name inferred)
    virtual bool getMassProperties(VxReal& mass, VxReal inertia[3][3], VxVector3& com, VxReal* volume);
    // slot 18 Mac 0x437dbc, Win 0x710090: slot 17 with the inertia scaled to `mass`
    // (name inferred)
    virtual bool getScaledMassProperties(VxReal mass, VxVector3& com, VxReal inertia[3][3], VxReal* volume);
    // slot 19 Mac 0x43834e, Win 0x710190: the model's centre of rotation in world coordinates
    // (transform position - R^T-rotated model offset) (name inferred)
    virtual void getCenterOfRotation(VxVector3& p) const;
    // slot 20 Mac 0x4382b8, Win 0x710140: the transform after dt at the model's velocities
    // (MeMatrix4TMUpdateFromVelocitiesCOM, epsilon 1e-4) (name inferred)
    virtual void predictTransform(VxTransform& out, VxReal dt) const;
    // slot 21 Mac 0x437fb8, Win 0x7103f0: VincModelUpdateTransform when a model exists (name inferred)
    virtual void updateCollisionModel();
    virtual McdGeometryID getMcdGeometry() const;       // slot 22 Mac 0x438afc, Win 0x710400 (name inferred)
    // slot 23 Mac 0x437ec4, Win 0x7102c0: the force function, only for a dynamic entity
    // (internal control 2) (name inferred)
    virtual bool applyFluidInteractionForce(void* fluidState);
    virtual bool hasFluidInteractionForceFn() const;    // slot 24 Mac 0x439998, Win 0x711180 (name inferred)
    // slot 25 Mac 0x4388de, Win 0x710250: flag bit 2, universe fluid registration (name inferred)
    virtual void setFluidInteractionForceFn(VxFluidInteractionForceFn fn);
    // slot 26 Mac 0x438056, Win 0x7102f0 (name from the warning string)
    virtual bool setDefaultFluidInteractionForceFn();
    // slot 27 Mac 0x438b36, Win 0x710780 (name inferred)
    virtual void setFluidInteractionParameters(const VxVector3& a, const VxVector3& b, VxReal c, VxReal d);
    virtual void setFluidInteractionData(const VxFluidInteractionData& d); // slot 28 Mac 0x438c26, Win 0x7106f0 (name inferred)
    virtual VxFluidInteractionData* getFluidInteractionData() const;      // slot 29 Mac 0x4399aa, Win 0x711190 (name inferred)
    // slot 30 Mac 0x43881c, Win 0x70fc00: owner entity (0 = none: the model gets the identity
    // transform); a part's MdtBody becomes the model's m6c (name inferred)
    virtual void setEntity(VxCollisionSensor* entity);
    virtual void setSweepEnabled(bool b);               // slot 31 Mac 0x437efc, Win 0x710410: model sweep (name inferred)

    // --- non-virtual API ---
    // Mac 0x438df0, Win 0x710a00: relative transform of the geometry on its entity (0: none, or
    // identity inside a composite); the relTM and the composed transform come from
    // MeMemoryAPI (name inferred)
    void setTransformRelative(const VxTransform* tm);
    // Mac 0x439454, Win 0x70fe80: relative transform, identity when none (name inferred)
    void getTransformRelative(VxTransform& tm) const;
    // Mac 0x438446, Win 0x70fde0: the Vinc relative-transform pointer (0 = none) (name inferred)
    const VxTransform* getTransformRelativePtr() const;
    // Mac 0x438ae8 (-> 0x4382a2), Win 0x70ff30: world transform of the geometry (the model's
    // transform: entity transform, times the relative one when set) (name inferred)
    const VxTransform* getTransformPtr() const;
    // Mac 0x4387d2, Win 0x7100e0: world box, swept over the universe time step when the model
    // sweeps (VincModelUpdate) (name inferred)
    void getBoundingBox(VxVector3& minCorner, VxVector3& maxCorner) const;
    // Mac 0x43816c, Win 0x710120: world bounding sphere (VincModelGetBSphere) (name inferred)
    void getBoundingSphere(VxVector3& center, VxReal& radius) const;
    // Mac 0x437fe8, Win 0x7103c0 -> VincModelSetState (glue: 0, 2, flag^1) (name inferred)
    void setCollisionMode(int mode);
    int getCollisionMode() const;                       // Mac 0x437fd2, Win 0x7103e0 (VincModelGetState) (name inferred)
    // Mac 0x437f5c, Win 0x710430: enabled and in a space: Vinc space update of the model over
    // the universe time step, else recurse to the parent composite (name inferred)
    void update();
    // Mac 0x438560, Win 0x70fbd0: a dynamic part with a body recomputes its mass properties
    // (name inferred)
    void updatePartMassProperties();
    // Mac 0x43858c (inlined on Win): the transform changed: slot 21, then update() for a
    // frozen entity, else updatePartMassProperties() (name inferred)
    void transformChanged();
    // Mac 0x438986, Win 0x710810: enabled flag (bit 1). Inside a composite: model state 0 / 2;
    // else the entity's slot 38 re-inserts / removes the model (name inferred)
    void setEnabled(bool b);
    bool isEnabled() const;                             // Mac 0x437eb4, Win 0x710240 (name inferred)
    void setFlags(unsigned int flags);                  // Mac 0x437ea6, Win 0x710230 (name inferred)
    unsigned int getFlags() const { return mFlags; }
    // Mac 0x437da2, Win 0x70ffb0: parent composite (model's parent aggregate model -> its user
    // data) or 0 (name inferred)
    VxCollisionGeometry* getParent() const;
    // Mac 0x43847c, Win 0x70fd70: model m7c; in a universe, a part grows the universe's type
    // table to twice the id when needed, then the intersect manager re-files the geometry.
    // Returns true. (name inferred)
    bool setGeometryTypeID(int id);
    int getGeometryTypeID() const;                      // Mac 0x4384fc, Win 0x70fd60 (name inferred)
    bool setResponseModel(int model);                   // Mac 0x438512, Win 0x70fd40: model m80; true (name inferred)
    int getResponseModel() const;                       // Mac 0x438534 (name inferred)
    // Mac 0x437f26, Win 0x7104b0: model ma4 = value (name unknown)
    void unk_437f26(int value);
    // ma4 is the geometry's intersect filter (VxBridge::canCollide, the Vinc aggregate pair
    // test): the same Mac 0x437f26 with the real type (name inferred)
    // (defined in src/Vx/VxBridge.cpp)
    void setIntersectFilter(VxIntersectFilter* f);
    VxIntersectFilter* getIntersectFilter() const;
    // Mac 0x437f12, Win 0x710480: the global flag (Mac 0x7791c0, Win 0xaf13b4) that makes new
    // collision geometries register with their shape; LDD leaves it false (name inferred)
    static void setRegisterWithGeometry(bool b);

    VxCollisionSensor* getEntity() const { return mEntity; }
    McdModelID getCollisionModel() const { return mModel; }

    static bool sRegisterWithGeometry;      // Mac 0x7791c0, Win 0xaf13b4 (name inferred)
    static VxBase::NameMap* sNameMap;       // Mac 0x7791c4, Win 0xaf13b8 (name inferred)

    VxCollisionSensor* mEntity;             // Mac +0x20, Win +0x38 (part / sensor owning it) (name inferred)
    McdModelID mModel;                      // Mac +0x24, Win +0x3c
    VxMaterial* mMaterial;                  // Mac +0x28, Win +0x40 (slots 11-13)
    VxFluidInteractionForceFn mFluidInteractionForceFn; // Mac +0x2c, Win +0x44 (slots 23-25) (name inferred)
    VxFluidInteractionData* mFluidInteractionData;     // Mac +0x30, Win +0x48 (slots 27-29) (name inferred)
    unsigned int mFlags;                    // Mac +0x34, Win +0x4c (= 3) (name inferred)
    VxGeometry* mGeometry;                  // Mac +0x38, Win +0x50 (addRef'd)
    VxNode mNode;                           // Mac +0x3c, Win +0x54 (released by the dtor; VxEntity::mNode alike) (name inferred)
    void* m40;                              // Mac +0x40, Win +0x58 (0; not used by the item's code)
};

// --------------------------------------------------------------------------------------
// VxCompositeCollisionGeometry (Mac vtable 0x75d6c8, 37 slots; Win 36; class id 0x194)
// Same size as VxCollisionGeometry (new(0x44) Mac 0x43bdc3 / new(0x5c) Win 0x714530).
// The children are the elements of the Vinc aggregate (in insertion order; removal shifts).
class VxCompositeCollisionGeometry : public VxCollisionGeometry {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x43bdc3 / Win 0x714530, delete Mac 0x43bbee / Win 0x714540
    // getStaticClassType Mac 0x43b51a, Win 0x714560 (0x194); types Mac 0x43c09a, Win 0x714ea0;
    // isOfType slot 0 Mac 0x43c422, Win 0x714f20; isOfClassType slot 1 Mac 0x43b524, Win 0x714da0
    VX_DECLARE_CLASS_TYPES();
    // C1 Mac 0x43bb8a -> C2 0x43baf4, Win 0x714570: Vinc aggregate model of maxCount elements,
    // identity relative transform, geometry type ID 5 (glue: 16 or the element count)
    // (argument name inferred)
    explicit VxCompositeCollisionGeometry(int maxCount);
    virtual ~VxCompositeCollisionGeometry();            // slot 2/3 Mac 0x43bdb0 (D2 0x43bce0)/0x43bc06, Win 0x714f60 (D2 0x714dc0)
    virtual const char* getClassName() const;           // slot 4 Mac 0x43c468, Win 0x714f50
    virtual VxCollisionGeometry* clone(bool cloneGeometry); // slot 6 Mac 0x43bdd6, Win 0x714610
    virtual void prepare();                             // slot 7 Mac 0x43baac, Win 0x714710
    virtual void replaceGeometry(VxGeometry* geometry); // slot 8 Mac 0x43b75a, Win 0x714760 (warning)
    virtual VxGeometry* getGeometry();                  // slot 9 Mac 0x43c47a, Win 0x719870: 0
    virtual const VxGeometry* getGeometry() const;      // slot 10 Mac 0x43c484, Win 0x719870: 0
    virtual void setMaterial(VxMaterial* material);     // slot 11 Mac 0x43ba66, Win 0x714780 (every child)
    virtual VxMaterial* getMaterial() const;            // slot 12 Mac 0x43b53a, Win 0x7147d0: 0
    using VxCollisionGeometry::getMaterial;
    virtual bool setDensity(VxReal density);            // slot 14 Mac 0x43ba0e, Win 0x714800 (every child)
    virtual VxReal getDensity() const;                  // slot 15 Mac 0x43b544, Win 0x714860: 1
    using VxCollisionGeometry::getMassProperties;
    // slot 17 Mac 0x43b69c, Win 0x714870: the aggregate geometry's mass properties
    virtual bool getMassProperties(VxReal& mass, VxReal inertia[3][3], VxVector3& com, VxReal* volume);
    virtual bool getScaledMassProperties(VxReal mass, VxVector3& com, VxReal inertia[3][3], VxReal* volume); // slot 18 Mac 0x43b54c, Win 0x7148d0
    virtual void getCenterOfRotation(VxVector3& p) const; // slot 19 Mac 0x43b636, Win 0x714920: (0, 0, 0)
    virtual void updateCollisionModel();                // slot 21 Mac 0x43b728, Win 0x7147e0 (+ element transforms)
    virtual McdGeometryID getMcdGeometry() const;       // slot 22 Mac 0x43b650, Win 0x714940: 0
    virtual bool setDefaultFluidInteractionForceFn();   // slot 26 Mac 0x43c008, Win 0x714950
    virtual void setFluidInteractionParameters(const VxVector3& a, const VxVector3& b, VxReal c, VxReal d); // slot 27 Mac 0x43b996, Win 0x714a40
    virtual void setFluidInteractionData(const VxFluidInteractionData& d); // slot 28 Mac 0x43b950, Win 0x7149d0
    virtual void setEntity(VxCollisionSensor* entity);  // slot 30 Mac 0x43b7f0, Win 0x714d00
    virtual void setSweepEnabled(bool b);               // slot 31 Mac 0x43b79c, Win 0x714d50
    // slot 32 Mac 0x43bec8, Win 0x714ab0 ("VxCompositeCollisionGeometry::addCollisionGeometry")
    virtual void addCollisionGeometry(VxCollisionGeometry* cg);
    virtual void removeCollisionGeometry(int index);    // slot 33 Mac 0x43b8ec, Win 0x714bc0 (name inferred)
    virtual bool removeCollisionGeometry(VxCollisionGeometry* cg); // slot 34 Mac 0x43b890, Win 0x714c10 (name inferred)
    virtual bool hasCollisionGeometry(VxCollisionGeometry* cg) const; // slot 35 Mac 0x43b842, Win 0x714ca0 (name inferred)
    virtual VxCollisionGeometry* getCollisionGeometry(int index) const; // slot 36 Mac 0x43b67c, Win 0x714c80 (name inferred)

    int getCollisionGeometryCount() const;              // Mac 0x43b784 (Win: VincAggregateGetElementCount 0x75a040 directly) (name inferred)
};

} // namespace Vx

#endif
