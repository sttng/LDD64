// Vortex 5.0.0 build 800 "vortexldd41" - VxCollisionPairRequest (restored header;
// code in src/Vx/VxResponse.cpp = Mac TU 514 head 0x476aa2-0x476c2c, Win
// 0x722ea0-0x723070). A 4-byte handle that OWNS a VincRequest (Vinc/VincIntersect.h; new(0x34) Mac,
// (0x38) Win). VxBridge keeps one per geometry type pair and one per response model pair; VxFrame
// a default one (Mac 0x476c2c). Class and two setter names from warning strings; the rest inferred.
#ifndef VX_VXCOLLISIONPAIRREQUEST_H
#define VX_VXCOLLISIONPAIRREQUEST_H

#include "Vx/VxPlatform.h"

struct VincRequest;

namespace Vx {

class VxCollisionPairRequest {
public:
    VxCollisionPairRequest();                           // C1 Mac 0x476c2c -> C2 0x476c0a, Win 0x722ea0: VincRequestInit defaults
    // Win 0x722f00 (dead-stripped on the Mac): defaults, then contact max count and face normals
    VxCollisionPairRequest(int contactMaxCount, bool faceNormalsFirst);
    VxCollisionPairRequest(const VxCollisionPairRequest& o); // C1 Mac 0x476bf0 -> C2 0x476bb0, Win 0x722ec0
    ~VxCollisionPairRequest();                          // D1 Mac 0x476b72 -> D2 0x476b5c, Win 0x722f70
    VxCollisionPairRequest& operator=(const VxCollisionPairRequest& o); // Mac 0x476b86, Win 0x722f50

    int getContactMaxCount() const;                     // Mac 0x476aa2, Win 0x722f80
    // Mac 0x476b14, Win 0x722f90 (<= 0x400, else a warning and 0x400)
    void setContactMaxCount(int n);
    int getGroupMaxCount() const;                       // Mac 0x476ac0, Win 0x723060
    // Mac 0x476ace, Win 0x723070 (<= 6 = VINC_MAX_CONTACT_GROUPS, else a warning and 6)
    void setGroupMaxCount(int n);
    void setFaceNormalsFirst(bool b);                   // Mac 0x476aae, Win 0x722fe0 (record +0x04)
    void setM08(VxReal x);                              // Win 0x723010 (record +0x08) (name unknown)
    void setM10(VxReal x);                              // Win 0x723030 (record +0x10, only 0 < x < 1) (name unknown)
    VincRequest* getRecord() const { return mRecord; }

    VincRequest* mRecord; // +0x00
};

} // namespace Vx

#endif
