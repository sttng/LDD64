// Vortex 5.0.0 build 800 "vortexldd41" - VxConeConstraint (restored header).
// Mac 0x8c bytes, Win 0xa4 (glue operator new sizes). Class id 0x119 (Mac 0x43c4f4).
#ifndef VX_VXCONECONSTRAINT_H
#define VX_VXCONECONSTRAINT_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"

namespace Vx {

class VxConeConstraint : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x43c5e8, Win 0x7249f0, delete Mac 0x43c5d1
    static int getStaticClassType();                    // Mac 0x43c4f4: 0x119
    static const VxArray<int>& getStaticClassTypes();   // (function-local static list, see the .cpp)
    VxConeConstraint();                     // C1 Mac 0x43c948 -> C2 0x43c8dc, Win 0x724e00; MdtConeLimitCreate Mac 0x48cee8 (type 0xf)
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x43cce4, Win 0x724cf0
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x43c4fe, Win 0x724af0
    virtual ~VxConeConstraint();            // slot 2/3 Mac 0x43c6ac/0x43c5fc, Win 0x724f20
    virtual const char* getClassName() const;           // slot 4 Mac 0x43cd2a, Win 0x724f10
    virtual void resetParts();                          // slot 16 Mac 0x43c564, Win 0x724a90

    // Mac 0x43c536, Win 0x724ab0: MdtConeLimitSetConeHalfAngle, wakeBodies (name inferred)
    void setConeAngle(VxReal angle);

    // Win only (Mac dead-stripped):
    VxConeConstraint(VxPart* part0, VxPart* part1, const VxVector3& axis0, const VxVector3& axis1); // Win 0x724e70
    void setPart0Axis(const VxVector3& a);  // Win 0x724b90: world axes of part 0 (a, a perpendicular) (name inferred)
    void setPart1Axis(const VxVector3& a);  // Win 0x724c00: the same for part 1 (name inferred)

private:
    void addAccessors();                    // Mac 0x43c6c0, Win 0x724d20: the equation / coordinate accessors (name inferred)
};

} // namespace Vx

#endif
