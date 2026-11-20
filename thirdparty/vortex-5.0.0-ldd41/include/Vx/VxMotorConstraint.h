// Vortex 5.0.0 build 800 "vortexldd41" - VxMotorConstraint (restored header).
// src/Vx/VxMotorConstraint.cpp: Mac TU 504 0x463d8a-0x464682, Win
// 0x723e00-0x724120. Mac 0x8c bytes, Win 0xa4 (glue operator new sizes). Class id 0x197.
// A one-coordinate motor between two parts (MdtMotor, Mdt type 0x1a).
#ifndef VX_VXMOTORCONSTRAINT_H
#define VX_VXMOTORCONSTRAINT_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"

namespace Vx {

class VxMotorConstraint : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x463f4b, Win 0x723d60, delete Mac 0x463e71 / Win 0x723e20
    static int getStaticClassType();                    // Mac 0x463d8a: 0x197
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x464200
    VxMotorConstraint();                                // C1 Mac 0x4641ec -> C2 0x464180, Win 0x723e80
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x464588, Win 0x723f00
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x463d94, Win 0x723e60
    virtual ~VxMotorConstraint();                       // slot 2/3 Mac 0x463f38 (D2 0x463ee4)/0x463e88, Win 0x724120
    virtual const char* getClassName() const;           // slot 4 Mac 0x4645ce, Win 0x724110
    virtual void setLinearCoordinateReferencePart(int coordinate, int partIndex); // slot 15 Mac 0x463de0, Win 0x723e40 (limit +0xe8 only)
    virtual void resetParts();                          // slot 16 Mac 0x463e04, Win 0x723e00 (+ MdtMotorReset)

    // Mac 0x463daa: the motor's limit kind (MdtMotor limit +0xe0 = Mdt +0xbc8: 0 linear,
    // 1 angular); the coordinate argument is unused (glue: VortexPhysicsSlideHingeConnection
    // Mac 0x3a4270) (name inferred)
    void setAngular(int coordinate, bool angular);

private:
    void addCoordinateAccessor();                       // Mac 0x463f5e, Win 0x723f30 (name inferred)
};

} // namespace Vx

#endif
