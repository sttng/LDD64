// Vortex 5.0.0 build 800 "vortexldd41" - ConstraintAccessor and its implementations (restored
// header).
//
// Named handles a constraint keeps for its equations (VxConstraint::mEquations), its coordinates
// (mCoordinates) and its part attachments (mPartAttachmentAccessors). ConstraintAccessor itself is
// a pure interface (vptr only: Mac 0x75d938 = D1 0x474a5e, D0 0x4747ec, two pure slots; Win dtor
// 0x715db0). src/Vx/ConstraintAccessor.cpp = Mac TU 511 (0x474768-0x474c9c); the part attachment
// lives in VxConstraint.cpp (Mac TU 473 0x4403de-0x440c48).
// The constructors are inline: every constraint constructor expands them (e.g. VxHinge Mac
// 0x45a680 / VxMotorConstraint Mac 0x463f5e: operator new(0x10), {vptr, constraint, index, name}).
#ifndef VX_CONSTRAINTACCESSOR_H
#define VX_CONSTRAINTACCESSOR_H

#include "Vx/VxVector3.h"
#include <string>

namespace Vx {

class VxConstraint;
class VxPart;

class ConstraintAccessor {
public:
    virtual ~ConstraintAccessor();                       // slot 0/1 Mac 0x474a5e/0x4747ec, Win 0x715db0
    virtual VxConstraint* getConstraint() const = 0;     // slot 2 (name inferred)
    virtual int getIndex() const = 0;                    // slot 3 (name inferred)
};

// One constraint equation (row): Mac vtable 0x75e5e8, Mac 0x10 bytes, Win 0x28.
class ConstraintEquation : public ConstraintAccessor {
public:
    ConstraintEquation(VxConstraint* constraint, int index, const std::string& name)
        : mConstraint(constraint), mIndex(index), mName(name)
    {
    }
    virtual ~ConstraintEquation();                       // Mac D1 0x474aec / D0 0x474a78, Win 0x7185b0
    virtual VxConstraint* getConstraint() const;         // Mac 0x47478a, Win 0x718590
    virtual int getIndex() const;                        // Mac 0x474796, Win 0x7185a0
    const std::string& getName() const { return mName; } // (inline, name inferred)

    VxConstraint* mConstraint;  // +0x04 (name inferred)
    int mIndex;                 // +0x08 (name inferred)
    std::string mName;          // +0x0c (name inferred)
};

// One coordinate (limits / motor / lock): Mac vtable 0x75e600, Mac 0x10 bytes, Win 0x28.
class ConstraintCoordinate : public ConstraintAccessor {
public:
    ConstraintCoordinate(VxConstraint* constraint, int index, const std::string& name)
        : mConstraint(constraint), mIndex(index), mName(name)
    {
    }
    virtual ~ConstraintCoordinate();                     // Mac D1 0x474b52 / D0 0x474bb8, Win 0x7185b0
    virtual VxConstraint* getConstraint() const;         // Mac 0x4747a2, Win 0x718590
    virtual int getIndex() const;                        // Mac 0x4747ae, Win 0x7185a0
    const std::string& getName() const { return mName; } // (inline, name inferred)

    VxConstraint* mConstraint;  // +0x04 (name inferred)
    int mIndex;                 // +0x08 (name inferred)
    std::string mName;          // +0x0c (name inferred)
};

// Generic "attachment of something to a part" interface (also used by the cable,
// CablePartAttachment). No virtual destructor; Win vtable 0x9f7e74 (11 slots, all pure except
// slot 8). All names inferred.
class AttachmentInterface {
public:
    virtual void setPart(VxPart* part) = 0;                      // slot 0
    virtual void setPosition(const VxVector3& p) = 0;            // slot 1 (part frame)
    virtual void setPrimaryAxis(const VxVector3& a) = 0;         // slot 2 (part frame)
    virtual void setSecondaryAxis(const VxVector3& a) = 0;       // slot 3 (part frame)
    virtual VxPart* getPart() const = 0;                         // slot 4
    virtual VxVector3 getPosition() const = 0;                   // slot 5
    virtual VxVector3 getPrimaryAxis() const = 0;                // slot 6
    virtual VxVector3 getSecondaryAxis() const = 0;              // slot 7
    virtual void update() {}                                     // slot 8 Mac 0x4403de, Win 0x7076a0 (shared no-op)
    virtual VxConstraint* getConstraint() const = 0;             // slot 9
    virtual int getIndex() const = 0;                            // slot 10
};

// The attachment of part `mIndex` of a constraint, as an AttachmentInterface (Mac vtables
// 0x75d888 / 0x75d8c0 (+4), Win 0x9f7edc / 0x9f7eac). Mac 0x10 bytes, Win 0x10. Every setter
// works in the part's frame (VxConstraint::setPartAttachment*Rel); the axis setters keep the
// other axis perpendicular.
class ConstraintPartAttachment : public ConstraintAccessor, public AttachmentInterface {
public:
    ConstraintPartAttachment(VxConstraint* constraint, int index)   // inline (Mac 0x43ebf6)
        : mConstraint(constraint), mIndex(index)
    {
    }
    virtual ~ConstraintPartAttachment();                 // Mac D1 0x440748 / D0 0x440712, Win 0x715db0
    virtual VxConstraint* getConstraint() const;         // Mac 0x440440 (thunk 0x440436), Win 0x7185a0 (ICF)
    virtual int getIndex() const;                        // Mac 0x440456 (thunk 0x44044b), Win 0x7859b0 (ICF)
    virtual void setPart(VxPart* part);                  // Mac 0x4403ee (thunk 0x4403e3), Win 0x722860
    virtual void setPosition(const VxVector3& p);        // Mac 0x4407ca (thunk 0x4407c0), Win 0x7228b0
    virtual void setPrimaryAxis(const VxVector3& a);     // Mac 0x440a86 (thunk 0x440a7b), Win 0x7228d0
    virtual void setSecondaryAxis(const VxVector3& a);   // Mac 0x440902 (thunk 0x4408f7), Win 0x7229c0
    virtual VxPart* getPart() const;                     // Mac 0x440422 (thunk 0x440417), Win 0x7228a0
    virtual VxVector3 getPosition() const;               // Mac 0x44077a (thunk 0x44076f), Win 0x722ab0
    virtual VxVector3 getPrimaryAxis() const;            // Mac 0x44085e (thunk 0x440854), Win 0x722ae0
    virtual VxVector3 getSecondaryAxis() const;          // Mac 0x4407f8 (thunk 0x4407ee), Win 0x722b30

    VxConstraint* mConstraint;  // +0x08 (name inferred)
    int mIndex;                 // +0x0c (name inferred)
};

} // namespace Vx

#endif
