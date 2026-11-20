// Vortex 5.0.0 build 800 "vortexldd41" - VxAssemblyBase, VxMechanism (restored header, V0.2;
// VxAssemblyBase completed by src/Vx/VxAssemblyBase.cpp = Mac TU 455
// 0x425346-0x42e904, Win 0x725960-0x729b80).
//
// An assembly groups parts, constraints, sub-assemblies and update subscribers. The universe's
// root is a VxMechanism ("rootMechanism"); LDD's VortexPhysicsScene creates its own mechanism and
// adds it to the universe (Mac TU 435 0x3aac30). VxAssemblyBase declares a pure virtual
// destructor (Mac slots 2/3 = __cxa_pure_virtual) and embeds a controller (Mac 0x4085d2, the
// VxCompositeController ctor) at Mac +0x54. An assembly owns its parts, constraints and
// sub-assemblies (the destructor deletes them).
//
// Vtables: Mac 0x75cfe8 48 slots + 0x75d0b0 (the VxSubscriberListResponseHandler base, 7);
// Win 46 + the handler's. NOTE: Mac slot 46 (Mac 0x4271c0) is the override of the
// handler's createSubscriberList that the Itanium ABI repeats in the primary vtable (thunk Mac
// 0x4271b5); MSVC does not, so Win slot 45 is Mac slot 47 (Win 0x7114e0: return true). MSVC
// also groups same-name overloads in reverse order: the three disablePairIntersect overloads
// are Mac slots 18 / 19 / 20 = Win 19 / 18 / 17, enablePairIntersect Mac 21 / 22 / 23 = Win
// 22 / 21 / 20. Otherwise Win slot = Mac slot - 1 from slot 4 on.
#ifndef VX_VXASSEMBLYBASE_H
#define VX_VXASSEMBLYBASE_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxSmartPtr.h"
#include "Vx/VxStepperBase.h"
#include "Vx/VxVector3.h"
#include "Vx/VxSubscriberListResponseHandler.h"
#include "Vx/VxCompositeController.h"
#include "Vx/VxSceneGraphInterface.h"

namespace VxData {
class FieldBase;
}

namespace Vx {

class VxPart;
class VxConstraint;
class VxEntity;
class VxTransform;
class VxCollisionGeometry;
class VxCollisionSensor;
class VxStateBase;
class VxDisabledPairs;
class VxVisitor;

// --------------------------------------------------------------------------------------
// VxAssemblyBase (class id 0x109 Mac 0x425346). Mac 0x408 bytes. Ctor Mac 0x4293fc.
class VxAssemblyBase : public VxBaseUserData,
                       public VxSubscriberListResponseHandler,
                       public VxEntityConstraintCollection<VxAssemblyBase> {
public:
    // VxAssemblyBase::UpdateSubscriber (VxRefCount-derived, class id 0x13a; LEGO
    // PointGravitationAction: Mac 7 slots 0/1 isOfType / isOfClassType (0x425386, 0x425370),
    // 2/3 D1/D0, 4 update (0x3af770), 5/6 empty defaults; Win 6). The assembly calls slot 4 in
    // preStep and slot 5 in postStep with the universe's time step (boost::bind of the slot and
    // VxSmartPtr::get, Mac 0x426518 / 0x4267dc), slot 6 in restoreState (method names inferred)
    class UpdateSubscriber : public VxRefCount {
    public:
        static int getStaticClassType();                // Mac 0x425366: 0x13a
        virtual bool isOfType(int type) const;          // slot 0 Mac 0x425386, Win 0x725af0 (= isOfClassType)
        virtual bool isOfClassType(int type) const;     // slot 1 Mac 0x425370, Win 0x725fc0
        virtual ~UpdateSubscriber() {}                  // slot 2/3
        virtual void update(VxReal dt) = 0;             // slot 4
        virtual void postUpdate(VxReal /*dt*/) {}       // slot 5
        virtual void reset() {}                         // slot 6
    };

    // VxAssemblyBase::ExposedFields (Mac vtable 0x75d0d8: VxRefCount's slots 0/1, D1 0x428ad6
    // (body 0x4289da) / D0 0x4288d6, Win 0x728dc0; ctor Mac 0x428ba4 (0x428aea), Win 0x725960):
    // a field of a part / child exposed as a field of the assembly's composite controller. The
    // destructor deletes the exposed copy (name from RTTI; members inferred)
    class ExposedFields : public VxRefCount {
    public:
        struct Entry {
            VxData::FieldBase* field;
            int kind;                                   // VxControllerBase field kinds: 1 input, 2 output, 3 parameter
        };
        ExposedFields(VxData::FieldBase* original, VxData::FieldBase* exposed, int kind);
        virtual ~ExposedFields();
        Entry* mOriginal;                               // +0x08 (new(8))
        Entry* mExposed;                                // +0x0c (new(8))
    };

    static int getStaticClassType();                    // Mac 0x425346: 0x109
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x42769a (name inferred; V1.3)
    // Mac 0x425dd2, Win 0x725b30: the assembly of that name (sNameMap) or 0 (name inferred)
    static VxAssemblyBase* findFromName(const char* name);
    VxAssemblyBase();                                   // Mac 0x4293fc
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x427a22, Win 0x727880
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x425350, Win 0x725fa0
    virtual ~VxAssemblyBase() = 0;                      // slot 2/3 pure (D2 Mac 0x42a7fc, Win 0x7278b0)
    virtual const char* getClassName() const;           // slot 4 Mac 0x42b3b8, Win 0x729b80
    virtual void setName(const char* name);             // slot 5 Mac 0x425e04, Win 0x725b00: + sNameMap, controller name

    // --- stepping (called by the universe / the parent assembly) (names inferred) ---
    // slot 6 Mac 0x426518, Win 0x727ee0: controller preStep, parts applyAnimationDriverIfMode0,
    // sub-assemblies, update subscribers update(universe time step)
    virtual void preStep();
    virtual void step();                                // slot 7 Mac 0x426792, Win 0x726bf0: controller step, sub-assemblies
    // slot 8 Mac 0x4267dc, Win 0x727f90: parts updateBeforeStep, sub-assemblies, update
    // subscribers postUpdate(dt), controller postStep
    virtual void postStep();
    virtual void updateCollisionGeometryTransforms();   // slot 9 Mac 0x4266f6, Win 0x726c30 (parts slot 17, sub-assemblies)
    virtual void applyAnimationDrivers();               // slot 10 Mac 0x426680, Win 0x726c80 (parts applyAnimationDriverIfNotMode2)

    // --- mass / bounds (names inferred) ---
    virtual VxReal getMass() const;                     // slot 11 Mac 0x426376, Win 0x726580: slot 13 + sub-assemblies
    // slot 12 Mac 0x4263c2, Win 0x7264a0: mass-weighted COM of slot 14 and the sub-assemblies
    virtual VxVector3 getCOMPosition(VxReal* mass = 0) const;
    virtual VxReal getPartsMass() const;                // slot 13 Mac 0x426ca8, Win 0x7269a0: dynamic parts only
    virtual VxVector3 getPartsCOMPosition(VxReal* mass = 0) const; // slot 14 Mac 0x426d4c, Win 0x726a30
    virtual void freeze(bool b);                        // slot 15 Mac 0x426090, Win 0x726d30 (entity slot 8)
    // slot 16 Mac 0x425daa, Win 0x727320: every part tm = tm * t (row vectors: then t), the
    // nodes updated (VxMechanism::setTransform)
    virtual void transform(const VxTransform& t);
    virtual void transformLocal(const VxTransform& t);  // slot 17 Mac 0x425c6a, Win 0x727380: tm = t * tm

    // --- pair intersection switches (names inferred; Win order reversed, see the top) ---
    virtual void disablePairIntersect(VxCollisionGeometry* cg);   // slot 18 Mac 0x425fea, Win 0x7260e0
    virtual void disablePairIntersect(VxCollisionSensor* sensor); // slot 19 Mac 0x426a4c, Win 0x726060
    virtual void disablePairIntersect(VxAssemblyBase* other);     // slot 20 Mac 0x42a25e, Win 0x728040
    virtual void enablePairIntersect(VxCollisionGeometry* cg);    // slot 21 Mac 0x426208, Win 0x726160
    virtual void enablePairIntersect(VxCollisionSensor* sensor);  // slot 22 Mac 0x425f4c, Win 0x7261e0
    virtual void enablePairIntersect(VxAssemblyBase* other);      // slot 23 Mac 0x428768, Win 0x7280b0
    virtual void applyDisabledCollisions();                       // slot 24 Mac 0x4261ea, Win 0x725d70 (the disabled-pair list)
    // slot 25 Mac 0x4253a4, Win 0x725b50: collisions between the assembly's own parts on / off
    virtual void setInternalCollisionEnabled(bool b);
    virtual bool isInternalCollisionEnabled() const { return mInternalCollisionEnabled; } // slot 26 Mac 0x4223a2 (emitted in VxMechanism.cpp), Win 0x71f630

    // --- state (names inferred except restoreState) ---
    virtual VxStateBase* createState();                 // slot 27 Mac 0x42582c, Win 0x725e60: new VxState(this)
    virtual void saveState();                           // slot 28 Mac 0x4253f4, Win 0x725e30
    virtual void restoreState();                        // slot 29 Mac 0x42703e, Win 0x7287a0 ("VxAssemblyBase::restoreState")
    virtual bool getBoundingBox(VxVector3& min, VxVector3& max) const;      // slot 30 Mac 0x426ef0, Win 0x7263a0 (+ sub-assemblies)
    virtual bool getPartsBoundingBox(VxVector3& min, VxVector3& max) const; // slot 31 Mac 0x426aea, Win 0x726260
    // slot 32 Mac 0x427296, Win 0x725c80 / slot 33 Mac 0x426938, Win 0x725bb0: the sphere around
    // slot 30's / 31's box; min / max returned when both pointers are set
    virtual bool getBoundingSphere(VxVector3& center, VxReal& radius, VxVector3* min, VxVector3* max) const;
    virtual bool getPartsBoundingSphere(VxVector3& center, VxReal& radius, VxVector3* min, VxVector3* max) const;
    virtual void setGrouping(int grouping);             // slot 34 Mac 0x425ebc, Win 0x726cd0 (parts, sub-assemblies)
    virtual int vf35() const;                           // slot 35 Mac 0x42b3ca, Win 0x719870 (0; VxMechanism: item count) (V1.3)
    virtual void* vf36(int i) const;                    // slot 36 Mac 0x42b3d4, Win 0x7c61f0 (0; VxMechanism: item i) (V1.3)
    // slot 37 Mac 0x4262ae, Win 0x726700: 1 = every part (and sub-assembly) frozen, 0 = none,
    // 2 = mixed
    virtual int getFrozenState() const;
    // slot 38 Mac 0x4286be, Win 0x727780: the merge state of the dynamic parts (recomputed when
    // the merge-changed flag is set): 2 = fewer than 2 parts or not all in one merge tree...,
    // 1 = one merge root, 0 = every part its own root
    virtual int getMergeState();
    // slot 39 Mac 0x427480, Win 0x726880: mergeParts(mode, 0)
    virtual void mergeParts(int mode);
    // slot 40 Mac 0x42575c, Win 0x726930: every dynamic part (and those of the sub-assemblies)
    // merged to root (0: unmerged) unless already in root's tree
    virtual void setMergeRoot(VxPart* root);
    virtual void setParent(VxAssemblyBase* parent);     // slot 41 Mac 0x428fc4, Win 0x728950 (re-exposes the fields)
    virtual void setUniverseRecursive(VxUniverse* u);   // slot 42 Mac 0x425e50, Win 0x727010 (sub-assemblies, controller)
    virtual bool addAssembly(VxAssemblyBase* a); // slot 43 Mac 0x4274cc, Win 0x727cb0 (name from string "VxAssemblyBase::addAssembly"; VxUniverse::addAssembly calls it on the root mechanism)
    virtual bool removeAssembly(VxAssemblyBase* a); // slot 44 Mac 0x427132, Win 0x7270f0 (name inferred)
    // slot 45 Mac 0x425a70, Win 0x727070: the subscriber lists, constraints and parts follow the
    // assembly to universe u (name inferred)
    virtual void setUniverse(VxUniverse* u);
    // VxSubscriberListResponseHandler slot 6 (Mac primary slot 46 0x4271c0 + thunk 0x4271b5;
    // Win: handler vtable only): a list that only passes results of this assembly's sensors
    // (AcceptIntersectPair), registered with the universe
    virtual VxSubscriberListResponse* createSubscriberList(int type1, int type2, int event);
    // slot 47 Mac 0x4223b2 (emitted in VxMechanism.cpp), Win 0x7114e0: exposed fields are
    // forwarded to the parent assembly (name inferred)
    virtual bool forwardsExposedFields() const { return true; }

    // --- non-virtual (Mac VxAssemblyBase.cpp) (names inferred unless noted) ---
    // Mac 0x4273aa, Win 0x7267b0: mode 0 -> setMergeRoot(0); mode 1 -> setMergeRoot(part, or the
    // first dynamic part found by FirstDynamicPartFinder when part == 0)
    void mergeParts(int mode, VxPart* part);
    // Mac 0x42675e, Win 0x725ef0: e belongs to this assembly or to one of its sub-assemblies
    // (walks VxEntity +0x3c and the assemblies' mParent) (V1.3)
    bool contains(VxEntity* e) const;
    // Mac 0x4255de, Win 0x725f80: the ancestor just below the topmost one (the top-level assembly
    // under the universe's root mechanism), this when there is no parent
    VxAssemblyBase* getTopLevelAssembly();
    int getAssemblyCount() const { return m44.size(); }
    VxUniverse* getUniverse() const { return mUniverse; }
    VxAssemblyBase* getParent() const { return mParent; }
    // Mac 0x425434, Win 0x7265c0: visitor slot 4, parts slot 6, constraints slot 7, the items
    // (slots 35 / 36) with slot 8, the sub-assemblies, slot 5; stops at 2
    int accept(VxVisitor* v);
    bool _addPart(VxPart* p);                           // Mac 0x429094, Win 0x727b90 (cf. "VxAssemblyBase::_removeEntity")
    bool _removePart(VxPart* p);                        // Mac 0x42a4d8, Win 0x727220 ("VxAssemblyBase::_removeEntity")
    bool _addConstraint(VxConstraint* c);               // Mac 0x42b0e0 (Win: inlined)
    void _removeConstraint(VxConstraint* c);            // Mac 0x427e02, Win 0x7272b0 ("VxAssemblyBase::_removeConstraint")
    VxPart* findPart(const char* name);                 // Mac 0x429b1c, Win 0x728680 (by name, then the sub-assemblies)
    VxConstraint* findConstraint(const char* name);     // Mac 0x42a08c, Win 0x7286e0
    // Mac 0x42acd8, Win 0x728820: appended (front: inserted first) unless present
    void addUpdateSubscriber(UpdateSubscriber* s, bool front);
    void removeUpdateSubscriber(UpdateSubscriber* s);   // Mac 0x427a68, Win 0x727570
    bool hasUpdateSubscriber(UpdateSubscriber* s) const; // Mac 0x425622
    // Mac 0x4255b4, Win 0x725f50: sets mMergeChanged here and in every parent assembly
    // (VxPart::setMergeParent) (V4.2)
    void setMergeChanged();
    // Mac 0x428bcc, Win 0x728120 ("_exposeField called with a field that is already exposed.")
    void _exposeField(VxData::FieldBase* field, int kind);
    // Mac 0x427b82, Win 0x7273a0 ("_unexposeField called with a field that was not previously exposed.")
    void _unexposeField(VxData::FieldBase* field);
    int getM3d4() const { return m3d4; }                // Mac 0x425426, Win 0x725ee0 (meaning unknown)
    // V1.3 alias of mergeParts (no callers left)
    void vf39Helper(int mode, VxPart* part) { mergeParts(mode, part); }

    static VxBase::NameMap* sNameMap;                   // Mac 0x7791a4 (name inferred)
    static bool sExposeFields;                          // Mac 0x7791a0 (= true) (name inferred)

    // Win layout: not verified past the embedded controller (Win VxMechanism is 0x528 bytes)
    VxAssemblyBase* mParent;                            // Mac +0x40, Win +0x58 (parent assembly; 0 = top level)
    VxArray<VxAssemblyBase*> m44;                       // Mac +0x44 sub-assemblies
    VxCompositeController mController;                  // Mac +0x54, Win +0x70 (0x354 / 0x444 bytes; Win confirmed by
                                                        // the ctor Win 0x727da0: next member at +0x4b4) (V1.3) (name inferred)
    VxArray<VxPart*> mParts;                            // Mac +0x3a8 (getPart / getPartCount)
    VxArray<VxConstraint*> mConstraints;                // Mac +0x3b8 (getConstraint / getConstraintCount)
    VxUniverse* mUniverse;                              // Mac +0x3c8
    VxDisabledPairs* mDisabledPairs;                    // Mac +0x3cc (new(0x30), Mac 0x422bfe)
    bool mInternalCollisionEnabled;                     // Mac +0x3d0 (= true)
    int m3d4;                                           // Mac +0x3d4 (0)
    VxStateBase* mSavedState;                           // Mac +0x3d8 (saveState)
    VxNode mNode;                                       // Mac +0x3dc (released by the dtor)
    VxArray<VxSmartPtr<UpdateSubscriber> > mUpdateSubscribers; // Mac +0x3e0
    bool mMergeChanged;                                 // Mac +0x3f0
    int mMergeState;                                    // Mac +0x3f4 (= 2)
    VxArray<VxSmartPtr<ExposedFields> > mExposedFields; // Mac +0x3f8

private:
    // the merge state computation (Mac 0x428098, Win 0x727610): the dynamic parts of the tree
    // are collected into `parts`
    void computeMergeState(VxArray<VxPart*>& parts);
    void setUniverseOfSubscriberLists(VxUniverse* u);   // Mac 0x4259a0, Win 0x726ed0 (recursive)
    void connectConstraints(VxUniverse* u);             // Mac 0x42591a (recursive; u unused)
    void connectParts(VxUniverse* u);                   // Mac 0x425894, Win 0x726fb0 (recursive)
    void updateNodes();                                 // Mac 0x425b0a, Win 0x726e80 (recursive)
    void transformLocalRecursive(const VxTransform& t); // Mac 0x425b72, Win 0x726de0
    void transformParts(const VxTransform& t);          // Mac 0x425c92
    void transformRecursive(const VxTransform& t);      // Mac 0x425d38
    void mergeRecursive(VxPart* root);                  // Mac 0x4256c6, Win 0x7268a0
};

// --------------------------------------------------------------------------------------
// VxMechanism (Mac vtable 0x75cd68 50 slots + 0x75ce38 7; class id 0x182 Mac 0x420ef4)
// Mac 0x41c bytes, Win 0x528 (new Mac 0x4213bc / Win 0x71eac0).
class VxMechanism : public VxAssemblyBase {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x4213bc / Win 0x71eac0, delete Mac 0x4213a5
    static int getStaticClassType();                    // Mac 0x420ef4: 0x182
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x421798 (name inferred; V1.3)
    VxMechanism();                                      // C1 Mac 0x4214e0 -> C2 0x421478, Win 0x71efd0
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x421b20, Win 0x71f260
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x420efe, Win 0x71ec80
    virtual ~VxMechanism();                             // slot 2/3 Mac 0x421784/0x4215e8, Win 0x71f810
    virtual const char* getClassName() const;           // slot 4 Mac 0x4223bc, Win 0x71f640
    virtual int vf35() const; // slot 35 Mac 0x420f7a, Win 0x71ecd0: mItems.size() (V1.3)
    virtual void* vf36(int i) const; // slot 36 Mac 0x4215ac, Win 0x71efa0: mItems[i] or 0 (V1.3)
    virtual void mergeParts(int mode); // slot 39 Mac 0x420fb2, Win 0x71ec40: mergeParts(mode, reference part) (V1.3)
    using VxAssemblyBase::mergeParts;
    virtual bool addAssembly(VxAssemblyBase* a); // slot 43 Mac 0x42145e, Win 0x71eb00 (name from string "VxAssemblyBase::addAssembly"; VxUniverse::addAssembly calls it on the root mechanism)
    virtual bool removeAssembly(VxAssemblyBase* a); // slot 44 Mac 0x421444, Win 0x71eb10 (name inferred)
    virtual void setReferencePart(VxEntity* p); // slot 48 Mac 0x4214f4, Win 0x71eb60 (name from its warning string) (V1.3)
    virtual VxEntity* getReferencePart(); // slot 49 Mac 0x4222b0, Win 0x71f0c0 (name inferred) (V1.3)

    // --- API used by the glue (Mac VxMechanism.cpp, forwarding to VxAssemblyBase) ---
    bool addPart(VxPart* p);                            // Mac 0x421426 -> _addPart (name inferred)
    bool removePart(VxPart* p);                         // Mac 0x421408 -> _removePart (name inferred)
    bool addConstraint(VxConstraint* c);                // Mac 0x4213ea -> _addConstraint (name inferred)
    void removeConstraint(VxConstraint* c);             // Mac 0x4213d0 -> _removeConstraint (name inferred)
    // Mac VxMechanism.cpp (names inferred):
    int getAssemblyCount() const;                       // Mac 0x42156a, Win 0x71eca0 (m44)
    VxAssemblyBase* getAssembly(int i) const;           // Mac 0x420f14, Win 0x71ee90 (0 when out of range)
    // the reference part's transform, or a static identity (Mac 0x42101c, Win 0x71ebb0)
    const VxTransform& getReferenceTransform();
    // moves the mechanism so that its reference part ends at t (Mac 0x4210aa)
    void setTransform(const VxTransform& t);
    int getPartCount() const;                           // Mac 0x42157c (mParts) (name inferred)
    VxPart* getPart(int i) const;                       // Mac 0x420f3e (0 when i < 0) (name inferred)
    int getConstraintCount() const;                     // Mac 0x421594 (mConstraints) (name inferred)
    VxConstraint* getConstraint(int i) const;           // Mac 0x420f5c (name inferred)

    VxEntity* mReferencePart;                           // Mac +0x408, Win +0x514 (V1.3)
    // reference-counted objects (a VxRefCount at Mac +0x18 / Win +0x30 = sizeof(VxBase), released by
    // the dtor; type unknown, never filled in LDD) (V1.3)
    VxArray<void*> mItems;                              // Mac +0x40c, Win +0x518
};

// --------------------------------------------------------------------------------------
// The disabled-pair lists of a universe (VxUniverse +0x88) and of an assembly (+0x3cc): Mac
// TU 452 (ctor 0x422bfe (0x422bb8), apply 0x422c32, dtor 0x422d98 (0x422cb4)), Win 0x72d690 /
// 0x72d620 / 0x72d6b0. 0x30 bytes. Nothing fills them in either binary (the adders were
// dead-stripped). (class and member names inferred; restored by V4.1)
class VxDisabledPairs {
public:
    struct GeometryPair { VxCollisionGeometry* cg0; VxCollisionGeometry* cg1; };
    struct GeometryAssembly { VxCollisionGeometry* cg; VxAssemblyBase* assembly; };
    struct AssemblyPair { VxAssemblyBase* a0; VxAssemblyBase* a1; };
    VxDisabledPairs() {}
    // the assembly pairs, the geometry / assembly pairs, the geometry pairs; nothing without a
    // universe
    void apply(VxUniverse* u);

    VxArray<GeometryPair> mGeometryPairs;               // +0x00
    VxArray<GeometryAssembly> mGeometryAssemblyPairs;   // +0x10
    VxArray<AssemblyPair> mAssemblyPairs;               // +0x20
};

} // namespace Vx

#endif
