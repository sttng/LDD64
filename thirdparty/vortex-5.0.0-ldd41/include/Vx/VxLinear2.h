// Vortex 5.0.0 build 800 "vortexldd41" - VxLinear2 (restored header;
// completed in V4.3).
// Mac 0x8c bytes, Win 0xa4 (glue operator new sizes). Class id 0x122 (Mac 0x45d882).
#ifndef VX_VXLINEAR2_H
#define VX_VXLINEAR2_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"

namespace Vx {

class VxLinear2 : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x45da0b, Win 0x715e90, delete Mac 0x45d931
    static int getStaticClassType();                    // Mac 0x45d882: 0x122
    static const VxArray<int>& getStaticClassTypes();   // (function-local static list, see the .cpp)
    VxLinear2();                            // C1 Mac 0x45e284 -> C2 0x45e218, Win 0x716200
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x45dda6, Win 0x716050
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x45d88c, Win 0x715fb0
    virtual ~VxLinear2();                   // slot 2/3 Mac 0x45d9f8/0x45d948, Win 0x716360
    virtual const char* getClassName() const;           // slot 4 Mac 0x45e298, Win 0x716350
    virtual void resetParts();                          // slot 16 Mac 0x45d8c4, Win 0x715f30

    // Win only (Mac dead-stripped):
    VxLinear2(VxPart* part0, VxPart* part1, const VxVector3& position, const VxVector3& axis); // Win 0x716270
    void setPosition(const VxVector3& p);   // Win 0x715f50 (name inferred)
    void setAxis(const VxVector3& a);       // Win 0x715f80 (name inferred)

private:
    void addAccessors();                    // Mac 0x45ddec, Win 0x716080: the equation / coordinate accessors (name inferred)
};

} // namespace Vx

#endif
