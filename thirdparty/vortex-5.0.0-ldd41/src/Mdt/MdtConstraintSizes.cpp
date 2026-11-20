// Vortex 5.0.0 build 800 "vortexldd41" - size of the largest constraint type (restored, V2.1).
// Mac 0x4929de-0x492a79 (static initialiser 0x492a37 / 0x492a62), Win 0x752780.
#include "Mdt/MdtConstraint.h"

// One entry per constraint struct, -1 terminated (Mac table 0x644660, Win: immediates in
// 0x752780). Entries of the types restored in Mdt are sizeof() of the struct (equal to the
// binaries' numbers, checked in MdtTypes.h); the others are the binaries' numbers relative to
// sizeof(MdtConstraint), so the table also works for other pointer sizes (host builds).
#if defined(_WIN32)
#define MDT_SIZE(mac, win) (int)(sizeof(MdtConstraint) + ((win) - 0xb10))
#else
#define MDT_SIZE(mac, win) (int)(sizeof(MdtConstraint) + ((mac) - 0xae8))
#endif

// Mac 0x4929de, Win 0x752780
int MdtGetMaxConstraintSize(void)
{
    const int sizes[27] = {
        (int)sizeof(MdtConstraint),
        (int)sizeof(MdtContact),
        (int)sizeof(MdtBSJoint),
        (int)sizeof(MdtHinge),
        MDT_SIZE(0xc00, 0xc28),
        (int)sizeof(MdtCylindrical),
        (int)sizeof(MdtAngular1Position3),
        MDT_SIZE(0xfc0, 0xfe8),
        MDT_SIZE(0xdd0, 0xdf8),
        MDT_SIZE(0xee4, 0xf10),
        (int)sizeof(MdtType14),
        MDT_SIZE(0xb18, 0xb40),
        MDT_SIZE(0xb88, 0xbb0),       // (= sizeof(MdtRPROJoint))
        MDT_SIZE(0xce4, 0xd10),
        MDT_SIZE(0xb34, 0xb60),
        (int)sizeof(MdtLinear2),
        MDT_SIZE(0xae8, 0xb10),
        MDT_SIZE(0xae8, 0xb10),
        MDT_SIZE(0xaec, 0xb18),
        (int)sizeof(MdtMotor),         // (= sizeof(MdtSpring))
        (int)sizeof(MdtConeLimit),
        MDT_SIZE(0xb44, 0xb70),
        MDT_SIZE(0xdd0, 0xdf8),
        MDT_SIZE(0xb54, 0xb80),
        MDT_SIZE(0x904, 0x928),
        MDT_SIZE(0x1198, 0x11c8),
        -1
    };
    int max = 0;
    for (int i = 0; sizes[i] > 0; ++i) {
        if (max < sizes[i])
            max = sizes[i];
    }
    return max;
}
