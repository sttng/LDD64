// Vortex 5.0.0 build 800 "vortexldd41" - VxRPRO (restored header).
// Mac 0x8c bytes, Win 0xa4 (glue operator new sizes). Class id 0x12a (Mac 0x474fc8).
#ifndef VX_VXRPRO_H
#define VX_VXRPRO_H

#include "Vx/VxConstraint.h"
#include "Vx/VxTransform.h"
#include "Vx/VxQuaternion.h"

namespace Vx {

class VxRPRO : public VxConstraint {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x475489, Win 0x7150b0, delete Mac 0x4753af
    static int getStaticClassType();                    // Mac 0x474fc8: 0x12a
    static const VxArray<int>& getStaticClassTypes();   // (function-local static list, see the .cpp)
    VxRPRO();                               // C1 Mac 0x47699c -> C2 0x476930, Win 0x715c20
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x475a8a, Win 0x7157b0
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x474fd2, Win 0x7154f0
    virtual ~VxRPRO();                      // slot 2/3 Mac 0x475476/0x4753c6, Win 0x715df0
    virtual const char* getClassName() const;           // slot 4 Mac 0x4769b0, Win 0x715de0
    virtual void resetParts();                          // slot 16 Mac 0x475342, Win 0x715150
    virtual bool getEquationFrictionIndex(int equation, int* friction) const; // slot 17 Mac 0x474fe8, Win 0x715460

    // C1 Mac 0x476908 -> C2 0x4766ec, Win 0x715c90: both parts attached with identity frames
    // at `position` (world), then the relative orientation is taken (glue: VxRPRO(part, 0,
    // part translation))
    VxRPRO(VxPart* part0, VxPart* part1, const VxVector3& position);
    // slot 11 Mac 0x47549c, Win 0x715640: world axes -> the part's frame, then slot 13
    virtual void setPartAttachmentAxes(int index, const VxVector3& primary, const VxVector3* secondary);
    // slot 13 Mac 0x47561a, Win 0x7155a0: the frame's orientation as a quaternion
    virtual void setPartAttachmentAxesRel(int index, const VxVector3& primary, const VxVector3& secondary);

    // Mac 0x4750aa, Win 0x715350: wakeBodies, MdtRPROJointSetAngularStrength (name inferred)
    void setAngularStrength(const VxVector3& v);
    // Mac 0x4750f8, Win 0x715310: wakeBodies, MdtRPROJointSetLinearStrength (name inferred)
    void setLinearStrength(const VxVector3& v);
    // Mac 0x475146: wakeBodies, MdtRPROJointUpdateRelativeQuaternion (name inferred)
    void updateRelativeOrientation();
    // Mac 0x47516a, Win 0x7151b0 (+ helper 0x714ff0): MdtRPROJointSetQuaternion(q, index), then the base class's
    // setPartAttachmentAxesRel with the quaternion's first two axes (name inferred)
    void setPartAttachmentOrientationRel(int index, const VxQuaternion& q);
    // Mac 0x47530a, Win 0x715510: the rotation of tm as a quaternion (MeQuaternionFromTM)
    // (name inferred)
    void setPartAttachmentOrientationRel(int index, const VxTransform& tm);
    void unk_4750aa(const VxVector3& v) { setAngularStrength(v); } // (earlier spelling)
    void unk_4750f8(const VxVector3& v) { setLinearStrength(v); }  // (earlier spelling)

    // Not restored (Win only, Mac dead-stripped; they forward to Win-only MdtRPROJoint accessors
    // that are not restored): Win 0x715170 (setPosition), 0x715210, 0x715230,
    // 0x715250, 0x715270, 0x715290, 0x7152b0, 0x7152d0, 0x7152f0, 0x715390, 0x7153b0, 0x7153d0,
    // 0x715400, 0x715430, 0x715540.

private:
    void addAccessors();                    // Mac 0x475ad0, Win 0x7157e0: the equation / coordinate accessors (name inferred)
};

} // namespace Vx

#endif
