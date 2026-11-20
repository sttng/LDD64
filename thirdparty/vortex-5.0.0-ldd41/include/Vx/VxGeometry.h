// Vortex 5.0.0 build 800 "vortexldd41" - VxGeometry and the primitive shapes VxBox, VxSphere,
// VxCapsule, VxCylinder (restored header).
//
// A VxGeometry is a shape description shared (refcounted, VxRefCount base) by collision
// geometries: VxCollisionGeometry's ctor does geometry->addRef(), ~VxCollisionGeometry
// releases it. Each shape owns one Vinc geometry (mGeometry, restored Vinc C layer V3.x:
// Vinc/VincPrimitives.h, Vinc/VincGeometry.h), destroyed with the shape.
// Secondary vtable (VxRefCount-in-VxGeometry): Mac +0x20 (offset_to_top -32, thunks for
// slots 0-3), Win +0x38.
//
// Sources: src/Vx/VxGeometry.cpp (Mac TU 490), VxBox.cpp (461), VxSphere.cpp (516),
// VxCapsule.cpp (466), VxCylinder.cpp (475); VxHeightField in Vx/VxHeightField.h (493).
// The computeSmallestEnclosing overrides of the four primitives (slots 10/11) are compiled in
// the fluid TU (Mac 510: src/Vx/VxOrientedVolume.cpp) and only declared here.
//
// API notes:
//  * run-time types: VX_DECLARE_CLASS_TYPES (Vx/VxBase.h); class ids VxGeometry 0x10a, VxBox
//    0x12b, VxCapsule 0x12c, (Win-only VxConvexMesh 0x12d), VxCylinder 0x12e, VxHeightField 0x12f,
//    VxSphere 0x131, (Win-only VxTerrain 0x132, VxTriangleMesh 0x133).
//  * virtual slot order (Mac): MSVC groups same-name overloads and reverses them, so the Win
//    vtable has computeSmallestEnclosing(pointer) at Win slot 9 and (array) at 10.
//  * collision geometries can register with their shape (VxCollisionGeometry::
//    setRegisterWithGeometry(true), off in LDD); notifyCollisionGeometries() (called by
//    VxHeightField::build) then refreshes their mass properties and space entries.
#ifndef VX_VXGEOMETRY_H
#define VX_VXGEOMETRY_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxVector3.h"
#include "Vx/VxTransform.h"
#include "Vinc/VincTypes.h"

namespace Vx {

class VxCollisionGeometry;
class VxMaterial;

// --------------------------------------------------------------------------------------
// VxGeometry (Mac vtable 0x75dde8 13 slots + 0x75de24 4 slots; Win 0x9fba28 12 + 0x9fba18 3;
// class id 0x10a). Mac 0x6c bytes, Win 0x88 (all four primitive shapes: new(0x6c) / new(0x88)).
// Ctor Mac 0x457852, Win 0x7614c0; dtor Mac 0x457e94 (D2), Win 0x761520.
class VxGeometry : public VxBaseUserData, public VxRefCount {
public:
    // getStaticClassType Mac 0x4575ea (0x10a); getStaticClassTypes Mac 0x4579de, Win 0x761700
    // ({0x10a, 0x198, 0x101}); isOfType slot 0 Mac 0x457d70, Win 0x761780; isOfClassType slot 1
    // Mac 0x4575fe, Win 0x7613c0. (VxRefCount and VxBase both declare slots 0/1: these
    // overriders serve both bases.)
    VX_DECLARE_CLASS_TYPES();

    VxGeometry();                                       // Mac 0x457852, Win 0x7614c0
    virtual ~VxGeometry();                              // slot 2/3 Mac 0x458032/0x458050 (D2 0x457e94), Win 0x7617f0 (D2 0x761520)
    // slot 4 getClassName: pure (VxBase); slot 5 setName: VxBase's
    // slot 6 Mac 0x442ba8, Win 0x7114d0: false; no geometry class overrides it (name unknown)
    virtual bool vf06() const { return false; }
    // slot 7: a copy of the shape (VxBox/VxSphere/... a new object, VxHeightField itself)
    virtual VxGeometry* clone() const = 0;
    // slot 8 Mac 0x442bb2, Win 0x7114e0: true. Win VxTriangleMesh 0x7aaec0 builds its edge data
    // here when slot 9 says it is missing; VxCollisionGeometry slot 7 forwards to it
    // (name inferred)
    virtual bool prepare() { return true; }
    // slot 9 Mac 0x442bbc, Win 0x7114e0: true (VxTriangleMesh: edge data present) (name inferred)
    virtual bool isPrepared() const { return true; }
    // slot 10 Mac 0x459fec, Win 0x713e70 / slot 11 Mac 0x459fc2, Win 0x713e90 ("VxGeometry::
    // computeSmallestEnclosing: not implemented."): fit the shape around a point set, writing
    // the shape's frame to `transform`; lockAxis -1 = free, else the axis to keep (VxCapsule
    // accepts 2/3, VxSphere only -1). The overrides (box/sphere/capsule/cylinder, Mac TU 510)
    // forward the array to the pointer version. (argument names inferred)
    virtual void computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform,
                                          int lockAxis, bool flag)
    {
        (void)points; (void)transform; (void)lockAxis; (void)flag;
        VxWarning(0, "VxGeometry::computeSmallestEnclosing: not implemented.\n");
    }
    virtual void computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform,
                                          int lockAxis, bool flag)
    {
        (void)points; (void)count; (void)transform; (void)lockAxis; (void)flag;
        VxWarning(0, "VxGeometry::computeSmallestEnclosing: not implemented.\n");
    }
    // slot 12 Mac 0x459ee8, Win 0x713eb0: empty (VxBox: dimensions; VxSphere: radius = d[0];
    // VxCapsule / VxCylinder: radius = d[0], height = d[1]) (name inferred)
    virtual void setDimensions(const VxVector3& d) { (void)d; }

    using VxRefCount::addRef;
    using VxRefCount::release;

    // --- non-virtual API ---
    // Mac 0x457614, Win 0x7612e0: a Vinc geometry exists and isOfClassType(type) (name inferred)
    bool hasClassType(int type) const;
    // Mac 0x457716, Win 0x7613e0: volume (cached in mVolume; -1 = not computed yet) from
    // VincGeometryGetMassProperties with an identity frame, which also gives mCenterOfMass
    // (name inferred)
    VxReal getVolume();
    // Mac 0x457816, Win 0x761470: centre of mass in the shape frame (computes it like
    // getVolume when needed) (name inferred)
    const VxVector3& getCenterOfMass();
    // Mac 0x4578e2, Win 0x761390: support point in direction `dir` under tm (VincGeometryMaximumPoint)
    // (name inferred)
    void getMaximumPoint(const VxTransform& tm, const VxVector3& dir, VxVector3& out) const;
    // Win 0x761300 (Win only): world box under tm (VincGeometryUpdateAABB); nothing without a
    // Vinc geometry (name inferred)
    void getBoundingBox(const VxTransform& tm, VxVector3& minCorner, VxVector3& maxCorner) const;
    // Win 0x761330 (Win only): bounding sphere in the shape frame (name inferred)
    void getBoundingSphere(VxVector3& center, VxReal& radius) const;
    // Win 0x761360 (Win only): bounding sphere radius (name inferred)
    VxReal getBoundingSphereRadius() const;
    // Win 0x761680 (Win only): the per-element materials (VxTriangleMesh: one per triangle):
    // clears the table, then `count` entries of `material` (name inferred)
    void setMaterialCount(int count, VxMaterial* material);
    // Win 0x761490 (Win only): sets entry `index` when valid. NOTE: returns false in every case,
    // as the original does. (name inferred)
    bool setMaterial(int index, VxMaterial* material);
    // Mac 0x457db6, Win 0x7617b0 (push_back inlined in the Win ctor): collision geometries
    // using this shape (only when VxCollisionGeometry::setRegisterWithGeometry(true)) (name inferred)
    void registerCollisionGeometry(VxCollisionGeometry* cg);
    // Mac 0x45763c, Win 0x761610: removes cg from that list (name inferred)
    void unregisterCollisionGeometry(VxCollisionGeometry* cg);
    // Win 0x7615e0 (Win only) (name inferred)
    bool isCollisionGeometryRegistered(VxCollisionGeometry* cg) const;
    // Mac 0x45790e, Win 0x761640: the shape changed: every registered collision geometry
    // updates its part's mass properties, then its space entry (name inferred)
    void notifyCollisionGeometries();

    McdGeometryID getMcdGeometry() const { return mGeometry; }

    McdGeometryID mGeometry;                  // Mac +0x28, Win +0x40 (owned; VincGeometryDestroy)
    VxArray<VxMaterial*> mMaterials;          // Mac +0x2c, Win +0x44 (VxCollisionGeometry slot 13) (name inferred)
    VxReal mVolume;                           // Mac +0x3c, Win +0x58 (-1 = not computed) (name inferred)
    VxVector3 mCenterOfMass;                  // Mac +0x44, Win +0x60 (name inferred)
    VxArray<VxCollisionGeometry*> mCollisionGeometries; // Mac +0x5c, Win +0x78 (name inferred)
};

// --------------------------------------------------------------------------------------
// VxBox (Mac vtable 0x75d408 + 0x75d444; Win 0x9f61ac + 0x9f619c; class id 0x12b). src/Vx/VxBox.cpp.
class VxBox : public VxGeometry {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x43299f, delete Mac 0x4328e6 / Win 0x707600
    // getStaticClassType Mac 0x43271a (299); types Mac 0x432a08, Win 0x7117c0; isOfType slot 0
    // Mac 0x432d9a, Win 0x711840; isOfClassType slot 1 Mac 0x43272e, Win 0x7116f0
    VX_DECLARE_CLASS_TYPES();
    // C1 Mac 0x432882 -> C2 0x432804, Win 0x711620 (VincBoxCreate: full extents)
    explicit VxBox(const VxVector3& dimensions);
    VxBox(VxReal dx, VxReal dy, VxReal dz);             // Win 0x7115e0 (Win only)
    virtual ~VxBox();                                   // slot 2/3 Mac 0x43298c/0x432908, Win 0x7118c0
    virtual const char* getClassName() const;           // slot 4 Mac 0x432de0, Win 0x711870
    virtual VxGeometry* clone() const;                  // slot 7 Mac 0x4329b2, Win 0x711710
    // slots 10/11 Mac 0x46f4fc / 0x474232, Win 0x76eb50 / 0x76eb90 (fluid TU 510)
    virtual void computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform,
                                          int lockAxis, bool flag);
    virtual void computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform,
                                          int lockAxis, bool flag);
    virtual void setDimensions(const VxVector3& d);     // slot 12 Mac 0x432df2, Win 0x711890
    void getDimensions(VxVector3& d) const;             // Mac 0x432766, Win 0x7116b0 (VincBoxGetDimensions) (name inferred)
    void setDimensions(VxReal dx, VxReal dy, VxReal dz); // Mac 0x4327cc, Win 0x711680 (VincBoxSetDimensions) (name inferred)
};

// --------------------------------------------------------------------------------------
// VxSphere (Mac vtable 0x75e7c8 + 0x75e804; class id 0x131). src/Vx/VxSphere.cpp.
class VxSphere : public VxGeometry {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x478813 / Win 0x714210 (0x88), delete Mac 0x47875a
    // getStaticClassType Mac 0x478604 (0x131); types Mac 0x478880, Win 0x714380; isOfType slot 0
    // Mac 0x478c12, Win 0x714400; isOfClassType slot 1 Mac 0x478618, Win 0x7142d0
    VX_DECLARE_CLASS_TYPES();
    explicit VxSphere(VxReal radius);                   // C1 Mac 0x4786f2 -> C2 0x478688, Win 0x714250
    virtual ~VxSphere();                                // slot 2/3 Mac 0x478800/0x47877c, Win 0x714470
    virtual const char* getClassName() const;           // slot 4 Mac 0x478c58, Win 0x714430
    virtual VxGeometry* clone() const;                  // slot 7 Mac 0x478826, Win 0x7142f0
    // slots 10/11 Mac 0x46f4ac / 0x46d9b0, Win 0x76f130 / 0x76f9a0 (fluid TU 510)
    virtual void computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform,
                                          int lockAxis, bool flag);
    virtual void computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform,
                                          int lockAxis, bool flag);
    virtual void setDimensions(const VxVector3& d);     // slot 12 Mac 0x478c6a, Win 0x714450
    VxReal getRadius() const;                           // Mac 0x478650 (VincSphereGetRadius) (name inferred)
    void setRadius(VxReal r);                           // Mac 0x478666, Win 0x7742c0 (inlined Vinc) (name inferred)

private:
    // (src/Vx/VxOrientedVolume.cpp) the fit behind slot 11: the minimal sphere, or the radius
    // about the kept centre (Mac 0x46d406, Win 0x76f790; array overload Win 0x76f9e0) (name inferred)
    void _computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, bool keepCenter);
    void _computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, bool keepCenter);
};

// --------------------------------------------------------------------------------------
// VxCapsule (Mac vtable 0x75d508 + 0x75d544; class id 0x12c). src/Vx/VxCapsule.cpp.
// Axis z; `height` is the length of the core segment (Vinc half height * 2).
class VxCapsule : public VxGeometry {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x437527 / Win 0x711910, delete Mac 0x43746f
    // getStaticClassType Mac 0x4372ac (300); types Mac 0x4375aa, Win 0x711ad0; isOfType slot 0
    // Mac 0x43793c, Win 0x711b50; isOfClassType slot 1 Mac 0x4372c0, Win 0x711a10
    VX_DECLARE_CLASS_TYPES();
    VxCapsule(VxReal radius, VxReal height);            // C1 Mac 0x4373fc -> C2 0x43737c, Win 0x711950
    virtual ~VxCapsule();                               // slot 2/3 Mac 0x437514/0x437490, Win 0x711be0
    virtual const char* getClassName() const;           // slot 4 Mac 0x437982, Win 0x711b80
    virtual VxGeometry* clone() const;                  // slot 7 Mac 0x43753a, Win 0x711a30
    // slots 10/11 Mac 0x46f54c / 0x4720e0, Win 0x76eb10 / 0x76f910 (fluid TU 510)
    virtual void computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform,
                                          int lockAxis, bool flag);
    virtual void computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform,
                                          int lockAxis, bool flag);
    virtual void setDimensions(const VxVector3& d);     // slot 12 Mac 0x437994, Win 0x711ba0
    VxReal getRadius() const;                           // Mac 0x437344, Win 0x7119d0 (VincCapsuleGetRadius) (name inferred)
    void setRadius(VxReal r);                           // Mac 0x43735a (name inferred)
    VxReal getHeight() const;                           // Mac 0x4372f8, Win 0x711a00: 2 * Vinc half height (name inferred)
    void setHeight(VxReal h);                           // Mac 0x437322, Win 0x7119e0 (VincCapsuleSetHeight) (name inferred)

private:
    // (src/Vx/VxOrientedVolume.cpp) the fit behind slot 11 (keepAxes: lock axis 2 / 3)
    // (Mac 0x471cf0, Win 0x76f570; array overload Win 0x76f960) (name inferred)
    void _computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, bool keepAxes,
                                   bool keepCenter);
    void _computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, bool keepAxes,
                                   bool keepCenter);
};

// --------------------------------------------------------------------------------------
// VxCylinder (Mac vtable 0x75d9c8 + 0x75da04; class id 0x12e). src/Vx/VxCylinder.cpp.
class VxCylinder : public VxGeometry {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x44274d / Win 0x711210, delete Mac 0x442695
    // getStaticClassType Mac 0x4424aa (0x12e); types Mac 0x4427d0, Win 0x711420; isOfType slot 0
    // Mac 0x442b62, Win 0x7114a0; isOfClassType slot 1 Mac 0x4424be, Win 0x711330
    VX_DECLARE_CLASS_TYPES();
    // C1 Mac 0x442622 -> C2 0x442588, Win 0x711350: VincCylinderCreate, then cos tolerance 0.98f
    VxCylinder(VxReal radius, VxReal height);
    virtual ~VxCylinder();                              // slot 2/3 Mac 0x44273a/0x4426b6, Win 0x711550
    virtual const char* getClassName() const;           // slot 4 Mac 0x442bc6, Win 0x7114f0
    virtual VxGeometry* clone() const;                  // slot 7 Mac 0x442760, Win 0x7113a0
    // slots 10/11 Mac 0x46f59c / 0x472574, Win 0x76ead0 / 0x76f880 (fluid TU 510)
    virtual void computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform,
                                          int lockAxis, bool flag);
    virtual void computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform,
                                          int lockAxis, bool flag);
    virtual void setDimensions(const VxVector3& d);     // slot 12 Mac 0x442bfa, Win 0x711510
    VxReal getRadius() const;                           // Mac 0x442550, Win 0x711290 (name inferred)
    void setRadius(VxReal r);                           // Mac 0x442566 (VincCylinderSetRadius) (name inferred)
    VxReal getHeight() const;                           // Mac 0x442518, Win 0x7112c0 (name inferred)
    void setHeight(VxReal h);                           // Mac 0x44252e, Win 0x7112a0 (VincCylinderSetHeight) (name inferred)
    // Mac 0x4424f6, Win 0x7112d0 (VincCylinderSetCosTolerance): parallel-axis tolerance of the triangle-list
    // test (name inferred)
    void setCosTolerance(VxReal c);

private:
    // (src/Vx/VxOrientedVolume.cpp) the fit behind slot 11 (keepAxes: lock axis 2 / 3)
    // (Mac 0x472184, Win 0x76f3a0; array overload Win 0x76f8d0) (name inferred)
    void _computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, bool keepAxes,
                                   bool keepCenter);
    void _computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, bool keepAxes,
                                   bool keepCenter);
};

} // namespace Vx

#endif
