// Vortex 5.0.0 build 800 "vortexldd41" - VxGeometry (restored).
// Mac TU 490 0x4575ea-0x458334 (file guess Vx/VxGeometry.cpp); Win 0x7612e0-0x761820.
// The VxArray / std::for_each template code of the TU (Mac 0x4581f1-0x458263, the
// mem_fun_t for_each instance 0x458263) comes from the headers. Header-inline: slots 6, 8-12.
#include "Vx/VxGeometry.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vinc/VincGeometry.h"

namespace Vx {

// Mac 0x4575ea (0x10a), types Mac 0x4579de / Win 0x761700, isOfType Mac 0x457d70 / Win 0x761780,
// isOfClassType Mac 0x4575fe / Win 0x7613c0
VX_IMPLEMENT_CLASS_TYPES(VxGeometry, VxBaseUserData, 0x10a)

// Mac 0x457852, Win 0x7614c0
VxGeometry::VxGeometry()
    : mGeometry(0)
    , mVolume(-1.0)
{
}

// D2 Mac 0x457e94, Win 0x761520 (D1 Mac 0x458032, D0 Mac 0x458050, Win 0x7617f0)
VxGeometry::~VxGeometry()
{
    VincGeometryDestroy(mGeometry);
}

// Mac 0x457614, Win 0x7612e0
bool VxGeometry::hasClassType(int type) const
{
    bool r = false;
    if (mGeometry != 0)
        r = isOfClassType(type);
    return r;
}

// Mac 0x45763c, Win 0x761610
void VxGeometry::unregisterCollisionGeometry(VxCollisionGeometry* cg)
{
    mCollisionGeometries.remove(cg); // std::find + erase in the original
}

// Win 0x7615e0 (Win only)
bool VxGeometry::isCollisionGeometryRegistered(VxCollisionGeometry* cg) const
{
    VxArray<VxCollisionGeometry*>::const_iterator i = mCollisionGeometries.begin();
    while (i != mCollisionGeometries.end() && *i != cg)
        ++i;
    return i != mCollisionGeometries.end();
}

// Mac 0x457716, Win 0x7613e0
VxReal VxGeometry::getVolume()
{
    if (mVolume == -1.0) {
        MeMatrix4 relTM = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
        MeMatrix3 inertia;
        VincGeometryGetMassProperties(mGeometry, relTM, inertia, &mVolume);
        mCenterOfMass.set(relTM[3][0], relTM[3][1], relTM[3][2]);
    }
    return mVolume;
}

// Mac 0x457816, Win 0x761470
const VxVector3& VxGeometry::getCenterOfMass()
{
    if (mVolume == -1.0)
        getVolume();
    return mCenterOfMass;
}

// Mac 0x4578e2, Win 0x761390
void VxGeometry::getMaximumPoint(const VxTransform& tm, const VxVector3& dir, VxVector3& out) const
{
    VincGeometryMaximumPoint(mGeometry, const_cast<MeReal(*)[4]>(tm.m), dir.data(), out.data());
}

// Win 0x761300 (Win only)
void VxGeometry::getBoundingBox(const VxTransform& tm, VxVector3& minCorner, VxVector3& maxCorner) const
{
    MeReal aabb[6];
    if (mGeometry != 0)
        VincGeometryUpdateAABB(mGeometry, const_cast<MeReal(*)[4]>(tm.m), minCorner.data(), maxCorner.data(), aabb);
}

// Win 0x761330 (Win only)
void VxGeometry::getBoundingSphere(VxVector3& center, VxReal& radius) const
{
    MeReal r = 0;
    VincGeometryGetBSphere(mGeometry, center.data(), &r);
    radius = r;
}

// Win 0x761360 (Win only)
VxReal VxGeometry::getBoundingSphereRadius() const
{
    MeVector3 center;
    MeReal r = 0;
    VincGeometryGetBSphere(mGeometry, center, &r);
    return r;
}

// Win 0x761680 (Win only)
void VxGeometry::setMaterialCount(int count, VxMaterial* material)
{
    mMaterials.clear();
    mMaterials.resize(count, material);
}

// Win 0x761490 (Win only)
bool VxGeometry::setMaterial(int index, VxMaterial* material)
{
    if (index >= 0 && index < mMaterials.size())
        mMaterials.begin()[index] = material;
    return false; // NOTE: the original returns false also after setting the entry
}

// Mac 0x45790e, Win 0x761640: two passes (mem_fun for_each in the original)
void VxGeometry::notifyCollisionGeometries()
{
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != mCollisionGeometries.end(); ++i)
        (*i)->updatePartMassProperties();
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != mCollisionGeometries.end(); ++i)
        (*i)->update();
}

// Mac 0x457db6, Win 0x7617b0
void VxGeometry::registerCollisionGeometry(VxCollisionGeometry* cg)
{
    mCollisionGeometries.push_back(cg);
}

} // namespace Vx
