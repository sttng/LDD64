// Vortex 5.0.0 build 800 "vortexldd41" - VxHinge (restored header;
// completed in V4.3).
// Mac 0x8c bytes, Win 0xa4 (glue operator new sizes). Class id 0x120 (Mac 0x45a07c).
#ifndef VX_VXHINGE_H
#define VX_VXHINGE_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"

namespace Vx {

class VxHinge : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x45a2a1, Win 0x717cf0, delete Mac 0x45a1c7
    static int getStaticClassType();                    // Mac 0x45a07c: 0x120
    static const VxArray<int>& getStaticClassTypes();   // (function-local static list, see the .cpp)
    VxHinge();                              // C1 Mac 0x45b33c -> C2 0x45b2d0, Win 0x7183a0; MdtHingeCreate Mac 0x48fbab (VxFrame +0x9c world)
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x45a63a, Win 0x717f60
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x45a086, Win 0x717ec0
    virtual ~VxHinge();                     // slot 2/3 Mac 0x45a28e/0x45a1de (D2 0x45a23a), Win 0x7184f0
    virtual const char* getClassName() const;           // slot 4 Mac 0x45b350, Win 0x7184e0
    virtual void resetParts();                          // slot 16 Mac 0x45a15a, Win 0x717d90
    virtual bool getEquationFrictionIndex(int equation, int* friction) const; // slot 17 Mac 0x45a09c, Win 0x717e10

    // Mac 0x45a124, Win 0x717e90: MdtHinge +0xbe0 = the part whose frame gives the axes of the
    // three position rows (-1 = the world; glue: 1 right after construction) (name inferred)
    void setPositionEquationReferencePart(int partIndex);
    void unk_45a124(int value) { setPositionEquationReferencePart(value); } // (V0.2 spelling)
    // VxHinge::kAngularCoordinate: Mac global 0x64407c (= 0), read by the glue through the
    // non-lazy pointer 0x702504 (name inferred)
    static const int kAngularCoordinate;

    // Win only (Mac dead-stripped):
    VxHinge(VxPart* part0, VxPart* part1, const VxVector3& position, const VxVector3& axis); // Win 0x718410
    void setPosition(const VxVector3& p);   // Win 0x717db0: MdtConstraintSetPosition (world) (name inferred)
    void setAxis(const VxVector3& a);       // Win 0x717de0: MdtConstraintSetAxis (world) (name inferred)

private:
    void addAccessors();                    // Mac 0x45a680, Win 0x717f90: the equation / coordinate accessors (name inferred)
};

} // namespace Vx

#endif
