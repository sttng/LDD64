// Vortex 5.0.0 build 800 "vortexldd41" - VxAngular1Position3 (restored header;
// completed in V4.3).
// Mac 0x8c bytes, Win 0xa4 (glue operator new sizes). Class id 0x184 (Mac 0x42405c).
#ifndef VX_VXANGULAR1POSITION3_H
#define VX_VXANGULAR1POSITION3_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"

namespace Vx {

class VxAngular1Position3 : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x42421b, Win 0x72b830, delete Mac 0x424141
    static int getStaticClassType();                    // Mac 0x42405c: 0x184
    static const VxArray<int>& getStaticClassTypes();   // (function-local static list, see the .cpp)
    VxAngular1Position3();                  // C1 Mac 0x4252b0 -> C2 0x425244, Win 0x72bdd0
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x4245b6, Win 0x72b9d0
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x424066, Win 0x72b930
    virtual ~VxAngular1Position3();         // slot 2/3 Mac 0x424208/0x424158, Win 0x72bef0
    virtual const char* getClassName() const;           // slot 4 Mac 0x4252c4, Win 0x72bee0
    virtual void resetParts();                          // slot 16 Mac 0x4240d4, Win 0x72b8d0
    virtual bool getEquationFrictionIndex(int equation, int* friction) const; // slot 17 Mac 0x42407c, Win 0x72b8f0

    // coordinate indices (Mac __const 0x643b00-0x643b0c = 1, 2, 3, 0, read by the glue's anchor
    // solver; owner class inferred) (names inferred)
    static const int kLinear0Coordinate;   // Mac 0x643b00 = 1
    static const int kLinear1Coordinate;   // Mac 0x643b04 = 2
    static const int kLinear2Coordinate;   // Mac 0x643b08 = 3
    static const int kAngularCoordinate;   // Mac 0x643b0c = 0

    // Win only (Mac dead-stripped): setPartAndAttachment for both parts with the same frame
    VxAngular1Position3(VxPart* part0, VxPart* part1, const VxVector3& position,
                        const VxVector3& primaryAxis, const VxVector3* secondaryAxis); // Win 0x72be40

private:
    void addAccessors();                    // Mac 0x4245fc, Win 0x72ba00: the equation / coordinate accessors (name inferred)
};

} // namespace Vx

#endif
