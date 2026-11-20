// Vortex 5.0.0 build 800 "vortexldd41" - VxCylindrical (restored header;
// completed in V4.3).
// Mac 0x8c bytes, Win 0xa4 (glue operator new sizes). Class id 0x11b (Mac 0x442dc4).
#ifndef VX_VXCYLINDRICAL_H
#define VX_VXCYLINDRICAL_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"

namespace Vx {

class VxCylindrical : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x442fb3, Win 0x724150, delete Mac 0x442ed9
    static int getStaticClassType();                    // Mac 0x442dc4: 0x11b
    static const VxArray<int>& getStaticClassTypes();   // (function-local static list, see the .cpp)
    VxCylindrical();                        // C1 Mac 0x44404c -> C2 0x443fe0, Win 0x724790
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x44334e, Win 0x724370
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x442dce, Win 0x7242d0
    virtual ~VxCylindrical();               // slot 2/3 Mac 0x442fa0/0x442ef0, Win 0x7249c0
    virtual const char* getClassName() const;           // slot 4 Mac 0x444060, Win 0x7249b0
    virtual void resetParts();                          // slot 16 Mac 0x442e6c, Win 0x7241f0
    virtual bool getEquationFrictionIndex(int equation, int* friction) const; // slot 17 Mac 0x442de4, Win 0x724260

    // coordinate indices (Mac __const 0x643e30 / 0x643e34 = 0, 1, read by the glue's anchor
    // solver, VortexAnchorSolver::Anchor::SetCylindrical, additive)
    static const int kLinearCoordinate;    // Mac 0x643e30 = 0
    static const int kAngularCoordinate;   // Mac 0x643e34 = 1

    // Win only (Mac dead-stripped): parts, MdtConstraintSetAxis, both attachment points at the
    // parts' origins
    VxCylindrical(VxPart* part0, VxPart* part1, const VxVector3& axis); // Win 0x724800

private:
    void addAccessors();                    // Mac 0x443394, Win 0x7243a0: the equation / coordinate accessors (name inferred)
};

} // namespace Vx

#endif
