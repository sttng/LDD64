// Vortex 5.0.0 build 800 "vortexldd41" - VxWinch (a constraint on an MdtWinch): the parts in both
// builds (restored header, for VxStateConstraint). Mac keeps the class id and the
// length accessors (Mac 0x486ccc-0x486d8a, static initialiser 0x486d32); the rest is Win-only
// (Win 0x823350-0x823cc0.x). LDD never creates one.
#ifndef VX_VXWINCH_H
#define VX_VXWINCH_H

#include "Vx/VxConstraint.h"

namespace Vx {

class VxWinch : public VxConstraint {
public:
    static int getStaticClassType();                    // Mac 0x486ccc, Win 0x823350: 0x129
    void setLength(VxReal length);                      // Mac 0x486cf8, Win 0x823550: MdtWinchSetLength (name inferred)
    VxReal getLength() const;                           // Mac 0x486d1c, Win 0x823540: MdtWinchGetLength (name inferred)

private:
    VxWinch();
};

} // namespace Vx

#endif
