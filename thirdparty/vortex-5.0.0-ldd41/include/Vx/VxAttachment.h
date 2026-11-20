// Vortex 5.0.0 build 800 "vortexldd41" - VxAttachment (restored header):
// Mac TU 456 (0x42e904-0x42f0db: constructors, assignment, the type-name strings) and the Win-only
// members used by VxConstraint (Win 0x7824b0-0x782f20).
//
// An attachment frame (primary / secondary axis, position) relative to a part, a part's centre of
// mass, the world, a scene-graph node or "slave" (names from the five static strings of Mac TU 456,
// "Relative To Part", ...). VxConstraint keeps one per part (mPartAttachments, a cache filled by the
// Win-only getPartAttachment). Mac 0x58 bytes, Win 0x60 (MSVC pads the vfptr to 8: explicit pad
// below so GCC-style builds match).
#ifndef VX_VXATTACHMENT_H
#define VX_VXATTACHMENT_H

#include "Vx/VxVector3.h"
#include "Vx/VxTransform.h"
#include "Vx/VxSceneGraphInterface.h"
#include <string>

namespace Vx {

class VxPart;

// Mac vtable 0x75d848 (one slot), Win vtable "Vx::VxAttachment::vftable"
class VxAttachment {
public:
    // mType values (the order of the type-name strings) (names inferred)
    enum Type {
        kRelativeToPart = 0,
        kRelativeToPartCOM = 1,
        kAbsolute = 2,
        kRelativeToNode = 3,
        kSlave = 4
    };

    explicit VxAttachment(int type = kRelativeToPart);  // C1 Mac 0x42e962 -> C2 0x42e904, Win 0x7824b0
    VxAttachment(const VxAttachment& o);                // C1 Mac 0x42eaec -> C2 0x42ea66, Win 0x782b20
    VxAttachment& operator=(const VxAttachment& o);     // Mac 0x42e99e, Win 0x7825b0
    // inline in both builds (the element dtor of VxConstraint's array: vptr, then the node release)
    ~VxAttachment() { mNode.release(); }

    // slot 0: Mac 0x4403c2 (emitted in VxConstraint's TU), Win 0x7815e0 (name inferred)
    virtual const char* getClassName() const { return "VxAttachment"; }

    // the node assignment inlined in operator= (Mac 0x42e99e): addRef the new handle, release
    // the old one, only when they differ (name inferred)
    void setNode(const VxNode& node);
    // Win 0x7824f0: everything zero, no node (name inferred)
    void reset();
    // Win 0x782540: the primary axis is (0, 0, 0), i.e. no frame was set yet (name inferred)
    bool isNull() const;
    // Win 0x782640: rows 0 / 1 = primary / secondary axis, row 2 = primary x secondary,
    // row 3 = position (name inferred)
    void getLocalTransform(VxTransform& tm) const;
    // Win 0x7826f0: rows 0, 1 and 3 back into the axes / position (name inferred)
    void setLocalTransform(const VxTransform& tm);
    // Win 0x782750: the frame in world coordinates (local transform times the reference's
    // transform); identity when the frame is null (name inferred)
    void getTransform(VxTransform& tm) const;
    // Win 0x7828f0: the frame from a transform in world coordinates (the transform
    // relative to the reference's: rotation world * ref^T, translation (world.t - ref.t) * ref^T,
    // Win 0x719940) (name inferred)
    void setTransform(const VxTransform& world);
    // Win 0x782a10: the two axes orthonormalised as rows 0 / 1 of a transform (VxTransform::
    // orthonormalize(0)) (name inferred)
    void orthonormalize();
    // Win 0x782bb0: the type's name ("" for an invalid type); Mac keeps the five static strings
    // (TU 456 initialiser 0x42ef01) (name inferred)
    static std::string getTypeName(int type);

#if defined(_WIN32)
    int mVfptrPad;              // Win +0x04 (never written)
#endif
    VxVector3 mPrimaryAxis;     // Mac +0x04, Win +0x08 (name inferred)
    VxVector3 mSecondaryAxis;   // Mac +0x1c, Win +0x20 (name inferred)
    VxVector3 mPosition;        // Mac +0x34, Win +0x38 (name inferred)
    VxPart* mPart;              // Mac +0x4c, Win +0x50 (name inferred)
    VxNode mNode;               // Mac +0x50, Win +0x54 (name inferred)
    int mType;                  // Mac +0x54, Win +0x58 (Type) (name inferred)
};

} // namespace Vx

#endif
