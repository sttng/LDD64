// Vortex 5.0.0 build 800 "vortexldd41" - VxMechanism (restored).
// Mac TU 450 (0x420ef4-0x42247b), Win 0x71eb00-0x71f810. The assembly behaviour itself is
// VxAssemblyBase's; VxMechanism adds the reference part and forwards the part /
// constraint API.
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxCable.h"
#include "Vx/VxEntity.h"
#include "Vx/VxPart.h"
#include "Vx/VxTransform.h"
#include "Vx/VxMessage.h"

namespace Vx {

// Mac 0x4213bc / 0x4213a5, Win 0x71eac0
void* VxMechanism::operator new(size_t size)
{
    return vxAlloc(size);
}

void VxMechanism::operator delete(void* p)
{
    if (p != 0)
        vxFree(p);
}

// Mac 0x420ef4 (0x182), 0x420efe (isOfClassType), 0x421798 (types), 0x421b20 (isOfType);
// Win 0x71ec80, 0x71f1e0, 0x71f260
VX_IMPLEMENT_CLASS_TYPES(VxMechanism, VxAssemblyBase, 0x182)

// Mac C2 0x421478 / C1 0x4214e0, Win 0x71efd0
VxMechanism::VxMechanism() : VxAssemblyBase(), mReferencePart(0), mItems()
{
}

// Mac D1 0x421784 (body 0x4216b4) / D0 0x4215e8, Win 0x71f810 (body 0x71f130): release the items
// (Mac 0x4223ce: VxSmartPtr-style release of the VxRefCount at +sizeof(VxBase), then 0)
VxMechanism::~VxMechanism()
{
    for (void** i = mItems.begin(); i != mItems.end(); ++i) {
        if (*i != 0) {
            VxRefCount* r = reinterpret_cast<VxRefCount*>(static_cast<char*>(*i) + sizeof(VxBase));
            r->release();
        }
        *i = 0;
    }
}

// Mac 0x4223bc, Win 0x71f640
const char* VxMechanism::getClassName() const
{
    return "VxMechanism";
}

// Mac 0x420f7a, Win 0x71ecd0
int VxMechanism::vf35() const
{
    return mItems.size();
}

// Mac 0x4215ac, Win 0x71efa0
void* VxMechanism::vf36(int i) const
{
    if (i < 0 || i >= mItems.size())
        return 0;
    return mItems.begin()[i];
}

// Mac 0x420fb2, Win 0x71ec40
void VxMechanism::mergeParts(int mode)
{
    VxEntity* e = getReferencePart();
    VxPart* part = e != 0 ? dynamic_cast<VxPart*>(e) : 0;
    VxAssemblyBase::mergeParts(mode, part);
}

// Mac 0x42145e / 0x421444, Win 0x71eb00 / 0x71eb10
bool VxMechanism::addAssembly(VxAssemblyBase* a)
{
    return VxAssemblyBase::addAssembly(a);
}

bool VxMechanism::removeAssembly(VxAssemblyBase* a)
{
    return VxAssemblyBase::removeAssembly(a);
}

// Mac 0x4214f4, Win 0x71eb60
void VxMechanism::setReferencePart(VxEntity* p)
{
    if (p != 0 && !contains(p)) {
        VxBase* owner = static_cast<VxBase*>(static_cast<VxAssemblyBase*>(p->mAssembly));
        VxWarning(0, "VxMechanism::setReferencePart cannot use part '%s' since it belongs to '%s'.\n",
                  p->getName(), owner->getName());
        return;
    }
    mReferencePart = p;
}

namespace {

// The first part that is not a cable part, or the cable's first part (the find_if predicate
// of Mac 0x421b67) (name inferred)
VxPart** findFirstDynamicPart(VxPart** b, VxPart** e)
{
    for (; b != e; ++b) {
        VxPart* p = *b;
        VxAssemblyBase* a = static_cast<VxAssemblyBase*>(static_cast<VxEntity*>(p)->mAssembly);
        if (!a->isOfType(VxCable::getStaticClassType()))
            return b;
        VxCable* cable = a != 0 ? static_cast<VxCable*>(a) : 0;
        if (p == cable->getPart(0, 0))
            return b;
    }
    return e;
}

// depth-first through the sub-assemblies (Mac 0x421f5c; GCC unrolled the recursion 9 levels)
VxPart* findFirstDynamicPart(VxAssemblyBase* a)
{
    VxPart** i = findFirstDynamicPart(a->mParts.begin(), a->mParts.end());
    if (i != a->mParts.end())
        return *i;
    for (VxAssemblyBase** s = a->m44.begin(); s != a->m44.end(); ++s) {
        VxPart* p = findFirstDynamicPart(*s);
        if (p != 0)
            return p;
    }
    return 0;
}

} // namespace

// Mac 0x4222b0, Win 0x71f0c0: the reference part, else the first dynamic part; a cable part
// stands for its cable's first part.
VxEntity* VxMechanism::getReferencePart()
{
    VxEntity* r = mReferencePart;
    if (r != 0)
        return r;
    VxPart* p = 0;
    VxPart** i = findFirstDynamicPart(mParts.begin(), mParts.end());
    if (i != mParts.end()) {
        p = *i;
    } else {
        for (VxAssemblyBase** s = m44.begin(); s != m44.end(); ++s) {
            p = findFirstDynamicPart(*s);
            if (p != 0)
                break;
        }
    }
    if (p == 0)
        return 0;
    VxAssemblyBase* a = static_cast<VxAssemblyBase*>(static_cast<VxEntity*>(p)->mAssembly);
    if (a->isOfType(VxCable::getStaticClassType())) {
        VxCable* cable = a != 0 ? static_cast<VxCable*>(a) : 0;
        return cable->getPart(0, 0);
    }
    return p;
}

// Mac 0x42101c, Win 0x71ebb0
const VxTransform& VxMechanism::getReferenceTransform()
{
    VxEntity* e = getReferencePart();
    if (e == 0) {
        static VxTransform sIdentity;
        return sIdentity;
    }
    return *e->mTransformPtr;
}

// Mac 0x4210aa: D = inverse(reference) * t (row vectors); moves the mechanism by D unless D is
// the identity within 1e-7.
void VxMechanism::setTransform(const VxTransform& t)
{
    const VxTransform& r = getReferenceTransform();
    VxTransform d;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            d.m[i][j] = r.m[0][i] * t.m[0][j] + r.m[1][i] * t.m[1][j] + r.m[2][i] * t.m[2][j];
    for (int j = 0; j < 3; ++j)
        d.m[3][j] = ((t.m[3][j] - r.m[3][0] * d.m[0][j]) - r.m[3][1] * d.m[1][j]) - r.m[3][2] * d.m[2][j];
    d.m[0][3] = 0;
    d.m[1][3] = 0;
    d.m[2][3] = 0;
    d.m[3][3] = 1;
    d.orthonormalize(0);
    if (!d.isIdentity(1e-7))
        transform(d);
}

// Mac 0x421426 / 0x421408 / 0x4213ea / 0x4213d0, Win 0x71eb20 / 0x71eb30 / 0x71eb40 / 0x71eb50
bool VxMechanism::addPart(VxPart* p)
{
    return _addPart(p);
}

bool VxMechanism::removePart(VxPart* p)
{
    return _removePart(p);
}

bool VxMechanism::addConstraint(VxConstraint* c)
{
    return _addConstraint(c);
}

void VxMechanism::removeConstraint(VxConstraint* c)
{
    _removeConstraint(c);
}

// Mac 0x42156a, Win 0x71eca0
int VxMechanism::getAssemblyCount() const
{
    return m44.size();
}

// Mac 0x420f14, Win 0x71ee90
VxAssemblyBase* VxMechanism::getAssembly(int i) const
{
    if (i < 0 || i >= m44.size())
        return 0;
    return static_cast<VxAssemblyBase*>(m44.begin()[i]);
}

// Mac 0x42157c / 0x421594, Win 0x71ecb0 / 0x71ecc0
int VxMechanism::getPartCount() const
{
    return mParts.size();
}

int VxMechanism::getConstraintCount() const
{
    return mConstraints.size();
}

// Mac 0x420f3e, Win 0x71ef10. NOTE: no upper bound check.
VxPart* VxMechanism::getPart(int i) const
{
    if (i < 0)
        return 0;
    return mParts.begin()[i];
}

// Mac 0x420f5c, Win 0x71ef80. NOTE: no upper bound check.
VxConstraint* VxMechanism::getConstraint(int i) const
{
    if (i < 0)
        return 0;
    return mConstraints.begin()[i];
}

} // namespace Vx
