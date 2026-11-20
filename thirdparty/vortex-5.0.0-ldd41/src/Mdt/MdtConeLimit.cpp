// Vortex 5.0.0 build 800 "vortexldd41" - cone limit (restored).
// Mac 0x48ce36-0x48cfa2 (static initialiser 0x48cf5f / 0x48cf8a), Win 0x783dc0-0x783ee0.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

#include <float.h>
#include <math.h>
#include "Me/MeCrtMath.h"

// Mac 0x48ce36, Win 0x783e10. NOTE: cos() is the platform's (Mac libm; Win the MSVC 2008 CRT,
// reproduced by Me/MeCrtMath.h); the two can differ in the last bit.
void MEAPI MdtConeLimitSetConeHalfAngle(MdtConstraint* c, MeReal a)
{
    MdtConeLimit* l = (MdtConeLimit*)c;
    l->halfAngle = a;
    l->cosHalfAngle = cos(a);
}

// Mac 0x48ce63, Win 0x783dc0
void MEAPI MdtConeLimitReset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtConeLimit* l = (MdtConeLimit*)c;
    l->halfAngle = 3.141592653589793;
    l->cosHalfAngle = -1.0;
    l->invStiffness = 0;
    l->damping = DBL_MAX;
    MdtConstraintReset(c);
}

// Mac 0x48cee8, Win 0x783ee0 (VxConeConstraint)
MdtConstraint* MEAPI MdtConeLimitCreate(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        c->bcl.type = kMdtTypeConeLimit;
        c->rowBuilder = MdtBclAddConeLimit;
        c->bcl.maxRows = 7;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        c->numLimits = 0;
        MdtConeLimitReset(c);
    }
    return c;
}
