// Vortex 5.0.0 build 800 "vortexldd41" - VxNamePathControllerInterface (restored header):
// the name-path parent / children of controllers, registered for
// VxControllerBase by VxControllerBase.cpp's static initialiser (Mac 0x41d49a).
#ifndef VX_VXNAMEPATHCONTROLLERINTERFACE_H
#define VX_VXNAMEPATHCONTROLLERINTERFACE_H

#include "Vx/VxNamePathBaseInterface.h"

namespace Vx {

// VxNamePathControllerInterface (Mac vtable 0x75c8c8; Win D 0x7fa410)
class VxNamePathControllerInterface : public VxNamePathBaseInterface {
public:
    virtual ~VxNamePathControllerInterface();                                    // Mac D1 0x419df6 / D0 0x419dbe, Win 0x7fa410
    // a controller's parent composite (Mac 0x419d6e, Win 0x7fa3d0)
    virtual VxBase* getParent(VxBase* object) const;
    // a composite's child controllers (Mac 0x41a282, Win 0x7fa8c0)
    virtual void getChildren(VxBase* object, VxArray<VxBase*>& children) const;
};

} // namespace Vx

#endif
