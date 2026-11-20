// Vortex 5.0.0 build 800 "vortexldd41" - VxConstraint (restored header).
//
// A VxConstraint wraps one MdtConstraint (mConstraint). Parts are attached by index (0, 1);
// "coordinates" are the controllable degrees of freedom (limits / motor / lock: one MdtLimit
// each, MdtConstraintGetLimit); "equations" are the constraint rows (relaxation, force limits).
// src/Vx/VxConstraint.cpp = Mac TU 473 (0x43cdac-0x441013), Win 0x71fe90-0x722e70. The Win
// build keeps many members the Mac linker dead-stripped (marked "Win only"; Mac code calling
// none of them); they forward to Mdt the same way.
// Most setters end with wakeBodies() (Mac 0x43cf12: MdtConstraintEnableBodies when the
// constraint is in a universe and active).
// Frames: "Position / Axes" = world coordinates (MdtConstraintSetBodyPosition / Axes), "...Rel" =
// the part's frame (converted into the merge root's frame when the part is merged into another,
// then MdtConstraintSetBodyPositionCOM / AxesRel), "...COMRel" = the Mdt body (centre of mass)
// frame as stored (Win only).
#ifndef VX_VXCONSTRAINT_H
#define VX_VXCONSTRAINT_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxVector3.h"
#include "Vx/VxAttachment.h"
#include "Mdt/MdtTypes.h"

namespace Vx {

class VxPart;
class VxUniverse;
class VxConstraintFriction;
class ConstraintAccessor;
class ConstraintEquation;
class ConstraintCoordinate;
class ConstraintPartAttachment;

// --------------------------------------------------------------------------------------
// VxConstraint (Mac vtable 0x75d7e8, 18 slots; Win 17; class id 0x10b Mac 0x43cdac)
// Mac 0x8c bytes, Win 0xa4 (every concrete constraint LDD creates has exactly this size:
// operator new(0x8c) Mac / new(0xa4) Win, except VxBallAndSocket 0x90 / 0xa8).
class VxConstraint : public VxBaseUserData {
public:
    // Events of VxConstraint::Subscriber (setPart / the destructor) (names inferred)
    enum Event {
        kEventPartAttached = 0,     // after setPart(i, part) attached a part (part = the new part)
        kEventPartDetached = 1      // a part was detached (part = the old part)
    };
    // No implementation in the binaries: slot 2 is called as (constraint, event, part).
    class Subscriber {
    public:
        virtual ~Subscriber() {}
        virtual void notify(VxConstraint* constraint, Event event, VxPart* part) = 0; // slot 2 (name inferred)
    };

    // Coordinate control modes (setControl / getControl). The Mdt limit stores 1 = motorized,
    // 2 = locked; the Vx values are swapped (Mac 0x43d80a / 0x43d22a) (names inferred)
    enum CoordinateControlEnum {
        kControlFree = 0,
        kControlLocked = 1,
        kControlMotorized = 2
    };
    // Coordinate kinds (MdtLimit::kind, getCoordinateType, Win only) (names inferred)
    enum CoordinateTypeEnum {
        kCoordinateLinear = 0,
        kCoordinateAngular = 1,
        kCoordinateDistance = 2
    };

    VX_DECLARE_ALLOCATOR();                             // new (inline), delete Mac 0x43d8e2
    static int getStaticClassType();                    // Mac 0x43cdac: 0x10b
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x43e828, Win 0x7222f0 ({0x10b, 0x198, 0x101})
    VxConstraint(MdtConstraintID c, int partCount);     // Mac 0x43ebf6, Win 0x7225e0
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x43ebb0, Win 0x722370
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x43cdb6, Win 0x720f30
    virtual ~VxConstraint();                            // slot 2/3 Mac 0x43fe10/0x43f184 (D2 0x43f7d0), Win 0x722e70 (D2 0x7223a0)
    // slot 4 getClassName: pure here
    virtual void setName(const char* name);             // slot 5 Mac 0x43dec6, Win 0x71fec0 (sNameMap; MdtConstraint name)
    virtual void setPart(int index, VxPart* part);      // slot 6 Mac 0x43fe24, Win 0x7221a0
    // slots 7 / 8 (MSVC orders the two overloads the other way round: Win 6 = x,y,z, Win 7 = vector)
    virtual void setPartAttachmentPosition(int index, const VxVector3& p);           // slot 7 Mac 0x43dcc4, Win 0x71ff80
    virtual void setPartAttachmentPosition(int index, VxReal x, VxReal y, VxReal z);  // slot 8 Mac 0x43dc86, Win 0x71ffb0
    virtual void getPartAttachmentPosition(int index, VxVector3& p) const;           // slot 9 Mac 0x43dc62, Win 0x71ffe0
    // slot 10 Mac 0x43e08e, Win 0x720100: secondary axis = a perpendicular of primary, then slot 11
    virtual void setPartAttachmentAxis(int index, const VxVector3& primary);
    // slot 11 Mac 0x43d926, Win 0x720080: secondary == 0 -> MdtConstraintSetBodyAxis
    virtual void setPartAttachmentAxes(int index, const VxVector3& primary, const VxVector3* secondary);
    virtual void getPartAttachmentAxes(int index, VxVector3& primary, VxVector3& secondary) const; // slot 12 Mac 0x43d8fa, Win 0x720150
    virtual void setPartAttachmentAxesRel(int index, const VxVector3& primary, const VxVector3& secondary); // slot 13 Mac 0x43e398, Win 0x721a70
    virtual void setPartAttachmentAxisRel(int index, const VxVector3& primary);      // slot 14 Mac 0x43e58c, Win 0x721ba0
    // slot 15 Mac 0x43d27e, Win 0x7217d0: linear coordinates only: the part whose frame gives the
    // direction (MdtLimit +0xe8) and the friction's axis (name inferred)
    virtual void setLinearCoordinateReferencePart(int coordinate, int partIndex);
    virtual void resetParts();                          // slot 16 Mac 0x43e1d6, Win 0x7218e0: setPart(i, 0) for all (name inferred)
    // slot 17 Mac 0x4403d4, Win 0x716340 (returns false): the friction (index into the Mdt
    // frictions, 0-2 linear, 3-5 angular) of an equation (name inferred)
    virtual bool getEquationFrictionIndex(int equation, int* friction) const;

    // ---- type / parts ----------------------------------------------------------------------
    bool isClassType(int type) const;                    // Mac 0x43cdcc, Win 0x71feb0: slot 1 (name inferred)
    int getPartCount() const;                            // Mac 0x43ce40, Win 0x721d30: number of non-null parts
    VxPart* getPart(int index) const { return mParts.begin()[index]; } // (inline, name inferred)
    // Mac 0x43cdea, Win 0x7201d0: slots 6, 7, 11 (world)
    void setPartAndAttachment(int index, VxPart* part, const VxVector3& position,
                              const VxVector3& primaryAxis, const VxVector3* secondaryAxis);
    // Mac 0x43db02, Win 0x721cf0: slot 6, setPartAttachmentPositionRel, slot 13 (part frame)
    void setPartAndAttachmentRel(int index, VxPart* part, const VxVector3& position,
                                 const VxVector3& primaryAxis, const VxVector3& secondaryAxis);
    void setPartAttachmentPositionRel(int index, const VxVector3& p);                // Mac 0x43d9c4, Win 0x721980
    void setPartAttachmentPositionRel(int index, VxReal x, VxReal y, VxReal z);      // Win 0x721a40 (Win only)
    void getPartAttachmentPositionRel(int index, VxVector3& p) const;                // Mac 0x43db58, Win 0x721920
    void getPartAttachmentAxesRel(int index, VxVector3& primary, VxVector3& secondary) const; // Mac 0x43e226, Win 0x721c60
    void getPartAttachmentPositionCOMRel(int index, VxVector3& p) const;             // Win 0x720000 (Win only)
    void setPartAttachmentPositionCOMRel(int index, const VxVector3& p);             // Win 0x720020 (Win only)
    void setPartAttachmentPositionCOMRel(int index, VxReal x, VxReal y, VxReal z);   // Win 0x720050 (Win only)
    // Win 0x720170 (Win only): the other part's attachment (1 - index) is set to part index's
    // world frame (name inferred)
    void alignOtherPartAttachment(int index);
    // VxAttachment form of the attachments (Win only)
    void setPartAttachment(int index, const VxAttachment& a);  // Win 0x721d70 (name inferred)
    void getPartAttachment(int index, VxAttachment& a);        // Win 0x722170 (name inferred)
    void updatePartAttachment(int index);                      // Win 0x721e30: refresh the cached frame if null (name inferred)
    void refreshPartAttachment(int index);                     // Win 0x721f40: refresh the cached frame (name inferred)
    // constraint forces of the last step on part index (Mdt force / torque)
    void getPartTorque(int index, VxVector3& t) const;   // Mac 0x43dd04, Win 0x71ff60 (MdtConstraintGetTorque)
    void getPartForce(int index, VxVector3& f) const;    // Mac 0x43dd28, Win 0x71ff40 (MdtConstraintGetForce)
    void resetForces();                                  // Mac 0x43df12, Win 0x71fe90 (MdtConstraintResetForces) (name inferred)

    // ---- universe / enabling --------------------------------------------------------------------
    // Mac 0x43de50, Win 0x7220e0: suspend, leave the old universe's world, join the new one,
    // re-enable if enabled; returns true (name inferred)
    bool setUniverse(VxUniverse* universe);
    VxUniverse* getUniverse() const { return mUniverse; }
    // Mac 0x43ddd0, Win 0x722030: mEnabled = mActive = enable; mActive stays false without a
    // universe, without parts or with a part outside a universe; then updateEnabled
    void setEnabled(bool enable);
    bool isEnabled() const { return mEnabled; }
    void suspend();                                      // Mac 0x43ddb6, Win 0x720f90: mActive = false, updateEnabled
    void updateEnabled();                                // Mac 0x43dd62, Win 0x71fef0: Mdt enable state = mActive (name inferred)
    void wakeBodies();                                   // Mac 0x43cf12, Win 0x720bd0 (name inferred)
    void setLocked(bool locked);                         // Win 0x720f50 (Win only): MdtConstraintLock / Unlock (name inferred)
    void setMdtFlag2(int unused);                        // Mac 0x43cef4, Win 0x720c40: MdtConstraint flags = 2 (name unknown)

    // ---- coordinates -----------------------------------------------------------------------
    int getCoordinateCount() const;                      // Mac 0x43dd4c, Win 0x71ff30 (MdtConstraintGetNumLimits)
    MdtLimit* getCoordinateLimit(int coordinate);        // Mac 0x43d260, Win 0x720210 (name inferred)
    const MdtLimit* getCoordinateLimit(int coordinate) const; // Mac 0x43cf92, Win 0x720230 (name inferred)
    MdtSingleLimit* getLimit(int coordinate, int bound);      // Mac 0x43d63a, Win 0x720290 (bound 0 lower, 1 upper) (name inferred)
    const MdtSingleLimit* getLimit(int coordinate, int bound) const; // Mac 0x43d186, Win 0x720250 (name inferred)
    int getCoordinateType(int coordinate) const;         // Win 0x720990 (Win only) (name inferred)
    bool isCoordinateAngular(int coordinate) const;      // Win 0x7209d0 (Win only) (name inferred)
    int getLinearCoordinateReferencePart(int coordinate) const; // Win 0x720950 (Win only) (name inferred)
    // friction of a coordinate: limit kind 0 -> linear friction [index], 1 -> angular [index]
    bool getCoordinateFrictionIndex(int coordinate, int* friction) const; // Mac 0x43cfb0, Win 0x720c80 (name inferred)
    VxConstraintFriction* getCoordinateFriction(int coordinate);  // Mac 0x43d006, Win 0x720d80 (name inferred)
    VxReal getCoordinateFrictionForce(int coordinate) const;      // Win 0x720de0 (Win only) (name inferred)
    VxConstraintFriction* getEquationFriction(int equation);      // Win 0x720e70 (Win only, slot 17) (name inferred)
    VxReal getEquationFrictionForce(int equation) const;          // Win 0x720eb0 (Win only) (name inferred)
    // NOTE: the assertion text says "setFriction" (shared with getFrictionForce)
    VxConstraintFriction* getFriction(int axis, bool linear);     // Mac 0x43ce98, Win 0x720cd0 (name inferred)
    VxReal getFrictionForce(int axis, bool linear) const;         // Win 0x720d10 (Win only) (name inferred)

    bool setControl(int coordinate, CoordinateControlEnum control); // Mac 0x43d80a, Win 0x720fe0: true if changed
    CoordinateControlEnum getControl(int coordinate) const;       // Mac 0x43d22a, Win 0x721080 (name inferred)
    bool isCoordinateLocked(int coordinate) const;               // Mac 0x43e1ac, Win 0x7208b0 (name inferred)
    bool isCoordinateMotorized(int coordinate) const;            // Mac 0x43d06e, Win 0x720890 (name inferred)
    VxReal getCoordinateForce(int coordinate) const;             // Win 0x720970 (Win only) (name inferred)
    // position: recomputed from the bodies / the solver's one / the stored one
    VxReal getCoordinateCurrentPosition(int coordinate) const;   // Mac 0x43e6c4, Win 0x721220 (0 out of range)
    VxReal getCoordinatePosition(int coordinate) const;          // Mac 0x43e708, Win 0x7211e0 (name inferred)
    VxReal getCoordinateStoredPosition(int coordinate) const;    // Mac 0x43e796, Win 0x721140 (name inferred)
    void setCoordinatePosition(int coordinate, VxReal position); // Mac 0x43e7da, Win 0x721100 (name inferred)
    // Win 0x7210c0 (Win only): offset so that the current position reads `position` (name inferred)
    void setCoordinateCurrentPosition(int coordinate, VxReal position);
    VxReal getCoordinatePositionOffset(int coordinate) const;           // Mac 0x43d20c, Win 0x721180 (name inferred)
    void setCoordinatePositionOffset(int coordinate, VxReal offset);    // Mac 0x43e74c, Win 0x7211a0 (name inferred)
    // MdtLimit fields without a known meaning (Win only) (names unknown)
    void setCoordinateFlag4(int coordinate, bool b);     // Win 0x7208d0 (+0xf4)
    bool getCoordinateFlag4(int coordinate) const;       // Win 0x7208f0
    void setCoordinateFlag0(int coordinate, bool b);     // Win 0x7209f0 (+0xf0, MdtConstraintGetPosition)
    VxReal getCoordinateValue50(int coordinate) const;   // Win 0x720930 (+0x50)
    bool isCoordinateLimitState2(int coordinate) const;  // Win 0x720650 (limitsActive == 2)

    // motor
    void setMotorMaximumForce(int coordinate, VxReal maxForce);          // Mac 0x43d340, Win 0x720730: [-f, f]
    VxReal getMotorMaximumForce(int coordinate) const;                   // Mac 0x43d098, Win 0x720870 (name inferred)
    void setMotorDesiredVelocity(int coordinate, VxReal velocity);       // Mac 0x43d3c8, Win 0x721710
    VxReal getMotorDesiredVelocity(int coordinate) const;                // Mac 0x43d0dc, Win 0x720830 (name inferred)
    void setMotorStiffnessAndDamping(int coordinate, VxReal k, VxReal d); // Mac 0x43d2f4, Win 0x7207c0 (name inferred)
    void setMotorStiffness(int coordinate, VxReal k);                    // Win 0x721750 (Win only) (name inferred)
    void setMotorDamping(int coordinate, VxReal d);                      // Win 0x721790 (Win only) (name inferred)
    VxReal getMotorStiffness(int coordinate) const;                      // Mac 0x43d120, Win 0x7207f0 (name inferred)
    VxReal getMotorDamping(int coordinate) const;                        // Mac 0x43d0fe, Win 0x720810 (name inferred)
    void setMotorLoss(int coordinate, VxReal loss);                      // Mac 0x43d390, Win 0x720700 (MdtLimit +0x18) (name inferred)
    VxReal getMotorLoss(int coordinate) const;                           // Mac 0x43d0ba, Win 0x720850 (name inferred)
    // Mac 0x43d40c, Win 0x721680: velocity, max force [-f, f], stiffness, damping, loss (name inferred)
    void setMotorParameters(int coordinate, VxReal velocity, VxReal maxForce,
                            VxReal stiffness, VxReal damping, VxReal loss);
    // lock
    void setLockPosition(int coordinate, VxReal position);               // Mac 0x43d4ee, Win 0x721520 (name inferred)
    VxReal getLockPosition(int coordinate) const;                        // Mac 0x43d164, Win 0x720680 (name inferred)
    void setLockMaximumForce(int coordinate, VxReal maxForce);           // Win 0x721580 (Win only) (name inferred)
    void setLockForceLimits(int coordinate, VxReal lo, VxReal hi);       // Win 0x7215e0 (Win only) (name inferred)
    VxReal getLockMaximumForce(int coordinate) const;                    // Mac 0x43d142, Win 0x7206c0 (name inferred)
    VxReal getLockMinimumForce(int coordinate) const;                    // Win 0x7206e0 (Win only) (name inferred)
    void setLockStiffness(int coordinate, VxReal k);                     // Win 0x721640 (Win only) (name inferred)
    VxReal getLockStiffness(int coordinate) const;                       // Win 0x7206a0 (Win only) (name inferred)
    // Mac 0x43d55e, Win 0x7214c0: lock position, max force [-f, f], stiffness (name inferred)
    void setLockParameters(int coordinate, VxReal position, VxReal maxForce, VxReal stiffness);
    // limits (bound 0 lower, 1 upper)
    void setLimitsActive(int coordinate, bool active);                   // Mac 0x43d5f8, Win 0x7213e0 (name inferred)
    bool getLimitsActive(int coordinate) const;                          // Mac 0x43d044, Win 0x720910 (name inferred)
    void setLimitPosition(int coordinate, int bound, VxReal position);   // Mac 0x43d6ba, Win 0x7212c0
    VxReal getLimitPosition(int coordinate, int bound) const;            // Mac 0x43d1e8, Win 0x720370 (name inferred)
    void setLimitPositions(int coordinate, VxReal lower, VxReal upper);  // Mac 0x43d6fc, Win 0x721260
    void setLimitVelocity(int coordinate, int bound, VxReal v);          // Mac 0x43d678, Win 0x721320 (MdtSingleLimit +0x20) (name inferred)
    VxReal getLimitVelocity(int coordinate, int bound) const;            // Mac 0x43d1c4, Win 0x7203b0 (name inferred)
    void setLimitRestitution(int coordinate, int bound, VxReal r);       // Win 0x7203f0 (Win only, [0, 1]) (name inferred)
    VxReal getLimitRestitution(int coordinate, int bound) const;         // Win 0x720450 (Win only) (name inferred)
    void setLimitStiffness(int coordinate, int bound, VxReal k);         // Win 0x721380 (Win only) (name inferred)
    VxReal getLimitStiffness(int coordinate, int bound) const;           // Win 0x720550 (Win only) (name inferred)
    void setLimitDamping(int coordinate, int bound, VxReal d);           // Win 0x7205a0 (Win only) (name inferred)
    VxReal getLimitDamping(int coordinate, int bound) const;             // Win 0x720600 (Win only) (name inferred)
    void setLimitMaximumForce(int coordinate, int bound, VxReal f);      // Win 0x7204a0 (Win only, string)
    VxReal getLimitMaximumForce(int coordinate, int bound) const;        // Win 0x720510 (Win only) (name inferred)
    // Mac 0x43d76c (upper), Win 0x720320 / 0x7202d0 (lower, Win only): position, velocity (+0x20),
    // restitution (+0x18), stiffness, damping; no wakeBodies (names inferred)
    void setUpperLimit(int coordinate, VxReal position, VxReal velocity, VxReal restitution,
                       VxReal stiffness, VxReal damping);
    void setLowerLimit(int coordinate, VxReal position, VxReal velocity, VxReal restitution,
                       VxReal stiffness, VxReal damping);
    // Win 0x721420 (Win only): limits active and |position - stop| < tolerance (name inferred)
    bool isCoordinateAtLimit(int coordinate, int bound, VxReal tolerance) const;

    // ---- equations (relaxation) --------------------------------------------------------------
    int getEquationCount() const;                                        // Win 0x721840 (Win only): mEquations size (name inferred)
    void setRelaxationActive(int equation, bool active);                 // Mac 0x43cf38, Win 0x720ba0 (name inferred)
    bool isRelaxationActive(int equation) const;                         // Mac 0x43cf6c, Win 0x720b80 (name inferred)
    // Mac 0x43e026, Win 0x721850 (MdtConstraintRelaxConstraint + setRelaxationActive)
    void setRelaxationParameters(int equation, VxReal stiffness, VxReal damping, VxReal loss,
                                 bool active);
    // Win 0x7218a0 (Win only) (name inferred)
    void getRelaxationParameters(int equation, VxReal* stiffness, VxReal* damping, VxReal* loss,
                                 bool* active) const;
    VxReal getRelaxationStiffness(int equation) const;                   // Mac 0x43df28, Win 0x720b40 (name inferred)
    void setRelaxationStiffness(int equation, VxReal k);                 // Mac 0x43df46, Win 0x720ae0 (name inferred)
    void setRelaxationStiffnessAndDamping(int equation, VxReal k, VxReal d); // Mac 0x43df6e, Win 0x720a80 (name inferred)
    void setRelaxationDamping(int equation, VxReal d);                   // Win 0x720b00 (Win only) (name inferred)
    VxReal getRelaxationDamping(int equation) const;                     // Win 0x720b60 (Win only) (name inferred)
    void setRelaxationLoss(int equation, VxReal loss);                   // Win 0x720ac0 (Win only) (name inferred)
    VxReal getRelaxationLoss(int equation) const;                        // Win 0x720b20 (Win only) (name inferred)
    void getRelaxationForceLimits(int equation, VxReal* lo, VxReal* hi) const; // Mac 0x43dfc6, Win 0x720a40 (name inferred)
    void setRelaxationForceLimits(int equation, VxReal lo, VxReal hi);   // Mac 0x43dff2, Win 0x720a10 (name inferred)
    VxReal getEquationForce(int equation) const;                         // Win 0x720a60 (Win only, Kea lambda) (name inferred)
    bool isEquationAtLowerLimit(int equation) const;                     // Win 0x720bf0 (Win only, row status 2 / 4) (name inferred)
    bool isEquationAtUpperLimit(int equation) const;                     // Win 0x720c10 (Win only, row status 0x18 / 0x28) (name inferred)

    MdtConstraintID getMdtConstraint() const { return mConstraint; }

    // ---- earlier spellings (deprecated; non-colliding ones only) ----
    void unk_43d2f4(int c, VxReal a, VxReal b) { setMotorStiffnessAndDamping(c, a, b); }
    VxReal unk_43d120(int c) const { return getMotorStiffness(c); }
    void unk_43cef4(int value) { setMdtFlag2(value); }
    VxReal getCoordinateVelocity(int c) const { return getCoordinatePosition(c); }
    VxConstraintFriction* getCoordinateFrictionPtr(int c) { return getCoordinateFriction(c); }

    // Equation names (Mac globals 0x7792e4-0x7792f8; strings Mac 0x618e52..) (names inferred)
    static const char* kPrimaryAxisOrientationName;     // "Primary axis orientation"
    static const char* kSecondaryAxisOrientationName;   // "Secondary axis orientation"
    static const char* kThirdAxisOrientationName;       // "Third axis orientation"
    static const char* kPrimaryAxisPositionName;        // "Primary axis position"
    static const char* kSecondaryAxisPositionName;      // "Secondary axis position"
    static const char* kThirdAxisPositionName;          // "Third axis position"
    static VxBase::NameMap* sNameMap;                   // Mac 0x7792e0, Win 0xaf1624 (name inferred)

    VxSubscriberCollection<Event, Subscriber, 2> mSubscribers; // Mac +0x20, Win +0x38
    MdtConstraintID mConstraint;                // Mac +0x2c, Win +0x44
    VxUniverse* mUniverse;                      // Mac +0x30, Win +0x48 (name inferred)
    void* m34;                                  // Mac +0x34, Win +0x4c: the owning VxAssemblyBase (UniverseConnector; read by the name paths / VxStateConstraint)
    VxArray<VxPart*> mParts;                    // Mac +0x38, Win +0x50 (sized partCount)
    VxArray<VxAttachment> mPartAttachments;     // Mac +0x48, Win +0x60 (cache, Win only users) (name inferred)
    VxArray<ConstraintPartAttachment*> mPartAttachmentAccessors; // Mac +0x58, Win +0x70 (name inferred)
    VxArray<ConstraintEquation*> mEquations;    // Mac +0x68, Win +0x80 (name inferred)
    VxArray<ConstraintCoordinate*> mCoordinates; // Mac +0x78, Win +0x90 (name inferred)
    bool mEnabled;                              // Mac +0x88, Win +0xa0 (= true)
    bool mActive;                               // Mac +0x89, Win +0xa1 (Mdt enable state wanted) (name inferred)

protected:
    // the accessor pushes of the subclass constructors (inlined push_back in both builds)
    void addEquation(int index, const char* name);   // (helper, no binary counterpart)
    void addCoordinate(int index, const char* name); // (helper, no binary counterpart)
};

} // namespace Vx

#endif
