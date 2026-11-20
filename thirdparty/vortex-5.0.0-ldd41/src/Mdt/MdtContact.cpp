// Vortex 5.0.0 build 800 "vortexldd41" - MdtContact (restored).
// Mac 0x48f407-0x48f771 (static initialiser 0x48f72f / 0x48f75a), Win 0x7845c0-0x7848f0
// (/LTCG; Win-only accessors not restored).
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

#include <new>

// Mac 0x48f407, Win 0x784740 (VxDynamicsContact::getPenetration)
MeReal MEAPI MdtContactGetPenetration(const MdtConstraint* c)
{
    return ((const MdtContact*)c)->penetration;
}

// Mac 0x48f415, Win 0x784790
Vx::VxContactMaterial* MEAPI MdtContactGetContactMaterial(MdtConstraint* c)
{
    return &((MdtContact*)c)->material;
}

// Mac 0x48f422, Win 0x7847a0
MdtConstraint* MEAPI MdtContactGetNext(const MdtConstraint* c)
{
    return &((const MdtContact*)c)->next->head;
}

// Mac 0x48f430, Win 0x784810
void MEAPI MdtContactSetPenetration(MdtConstraint* c, MeReal p)
{
    ((MdtContact*)c)->penetration = p;
}

// Mac 0x48f445, Win 0x784830
void MEAPI MdtContactSetM_b58(MdtConstraint* c, MeReal x)
{
    ((MdtContact*)c)->m_b58 = x;
}

// Mac 0x48f45a
void MEAPI MdtContactSetM_d04(MdtConstraint* c, int x)
{
    ((MdtContact*)c)->m_d04 = x;
}

// Mac 0x48f46b
void MEAPI MdtContactSetWakeBodies(MdtConstraint* c, int wake)
{
    MdtConstraintSetFlags(c, wake != 0);
}

// Mac 0x48f48c, Win 0x784850
void MEAPI MdtContactSetContactMaterial(MdtConstraint* c, const Vx::VxContactMaterial* m)
{
    MdtContact* k = (MdtContact*)c;
    k->material = *m;
    k->materialName = m->getName();
}

// Mac 0x48f4c6, Win 0x7845f0
void MEAPI MdtContactReset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtContact* k = (MdtContact*)c;
    k->penetration = 0;
    k->m_b58 = 1.0;
    for (int i = 0; i != 3; ++i) {
        k->position[i] = 0;
        k->m_b00[i] = 0;
        k->m_b18[i] = 0;
        k->normal[i] = 0;
        k->direction[i] = 0;
    }
    k->direction[0] = 1.0;
    k->m_cf8 = 0;
    k->m_cf0 = 0;
    k->m_d00 = 0;
    k->m_d04 = -1;
    k->m_b48 = 0;
    k->m_b4c = 0;
    k->materialName = 0;
    k->normal[1] = 1.0;
    k->m_d08 = 0;
    k->m_d0c = 0;
    k->next = 0;
    ::new (&k->material) Vx::VxContactMaterial();
    MdtConstraintReset(c);
}

// Mac 0x48f5f9, Win 0x7845c0
void MEAPI MdtContactDestroy(MdtConstraint* c)
{
    ((MdtContact*)c)->material.~VxContactMaterial();
    MdtConstraintDestroy(c);
}

// Mac 0x48f61f
MdtConstraint* MEAPI MdtContactCreate(MdtWorld* w)
{
    MdtConstraint* c = MdtConstraintCreate(w);
    if (c != 0) {
        MdtContact* k = (MdtContact*)c;
        c->bcl.type = kMdtTypeContact;
        c->rowBuilder = MdtBclAddContact;
        c->bcl.maxRows = 6;
        k->next = 0;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtContactSetNormal;
        c->numLimits = 0;
        k->m_d04 = -1;
        k->m_b48 = 0;
        k->m_b4c = 0;
        k->m_d08 = 0;
        k->m_d0c = 0;
        MdtContactReset(c);
    }
    return c;
}

// Mac 0x48f6d1, Win 0x7847e0 (the contact's MdtConstraint::setAxis)
void MEAPI MdtContactSetNormal(MdtConstraint* c, MeReal x, MeReal y, MeReal z)
{
    MdtContact* k = (MdtContact*)c;
    k->normal[0] = x;
    k->normal[1] = y;
    k->normal[2] = z;
}

// Mac 0x48f700, Win 0x7847b0
void MEAPI MdtContactSetPosition(MdtConstraint* c, MeReal x, MeReal y, MeReal z)
{
    MdtContact* k = (MdtContact*)c;
    k->position[0] = x;
    k->position[1] = y;
    k->position[2] = z;
}
