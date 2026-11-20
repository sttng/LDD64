// Vortex 5.0.0 build 800 "vortexldd41" - VxSpring (restored header).
// Mac 0x8c bytes, Win 0xa4 (glue operator new sizes). Class id 0x126 (Mac 0x478cf0).
#ifndef VX_VXSPRING_H
#define VX_VXSPRING_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"

namespace Vx {

class VxSpring : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x478eb1, Win 0x71f840, delete Mac 0x478dd7
    static int getStaticClassType();                    // Mac 0x478cf0: 0x126
    static const VxArray<int>& getStaticClassTypes();   // (function-local static list, see the .cpp)
    VxSpring();                             // C1 Mac 0x479520 -> C2 0x4794b4, Win 0x71fbd0
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x47924c, Win 0x71fad0
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x478cfa, Win 0x71fa30
    virtual ~VxSpring();                    // slot 2/3 Mac 0x478e9e/0x478dee, Win 0x71fd50
    virtual const char* getClassName() const;           // slot 4 Mac 0x479534, Win 0x71fd40
    virtual void resetParts();                          // slot 16 Mac 0x478d6a, Win 0x71f8e0

    // Mac 0x478d48, Win 0x71f900: MdtSpringSetNaturalLength; the glue sets 0 (name inferred)
    void setNaturalLength(VxReal length);
    VxReal getNaturalLength() const;                    // Mac 0x478d32, Win 0x71fa00 (name inferred)

    // Win only (Mac dead-stripped):
    VxSpring(VxPart* part0, VxPart* part1, VxReal length, VxReal stiffness, VxReal damping); // Win 0x71fc40
    void setNaturalLengthFromAttachments(); // Win 0x71f920: the attachment points' distance (name inferred)
    void setStiffness(VxReal k);            // Win 0x71f9b0 (MdtSpringSetStiffness) (name inferred)
    void setDamping(VxReal d);              // Win 0x71f9d0 (MdtSpringSetDamping) (name inferred)

private:
    void addAccessors();                    // Mac 0x479292, Win 0x71fb00: the equation / coordinate accessors (name inferred)
};

} // namespace Vx

#endif
