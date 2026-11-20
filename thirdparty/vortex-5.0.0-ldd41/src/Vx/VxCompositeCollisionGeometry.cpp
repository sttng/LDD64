// Vortex 5.0.0 build 800 "vortexldd41" - VxCompositeCollisionGeometry (restored).
// Mac TU 471 0x43b51a-0x43c4f3 (file guess Vx/VxCompositeCollisionGeometry.cpp);
// Win 0x714530-0x714f60 (+ getStaticClassType Win 0x714560).
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxEntity.h"
#include "Vx/VxUniverse.h"
#include "VxCollisionInternal.h"

namespace Vx {

namespace {
// Mac 0x779260 (Win 0xabb810 is shared with VxCollisionGeometry.cpp): the aggregate's transform
// and the children's default relative transform (name inferred)
MeReal sIdentityTM[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
} // namespace

// Mac 0x43b51a / Win 0x714560 (0x194), types Mac 0x43c09a / Win 0x714ea0, isOfType Mac 0x43c422
// / Win 0x714f20, isOfClassType Mac 0x43b524 / Win 0x714da0
VX_IMPLEMENT_CLASS_TYPES(VxCompositeCollisionGeometry, VxCollisionGeometry, 0x194)

// operator new Mac 0x43bdc3 / Win 0x714530, delete Mac 0x43bbee / Win 0x714540
VX_IMPLEMENT_ALLOCATOR(VxCompositeCollisionGeometry)

// C2 Mac 0x43baf4 (C1 0x43bb8a), Win 0x714570
VxCompositeCollisionGeometry::VxCompositeCollisionGeometry(int maxCount)
{
    mModel = VincAggregateModelCreate(maxCount);
    VincModelSetTransformPtr(mModel, sIdentityTM);
    setTransformRelative((const VxTransform*)sIdentityTM);
    VincModelSetUserData(mModel, this);
    setGeometryTypeID(5);
}

// D2 Mac 0x43bce0 (D1 0x43bdb0, D0 0x43bc06), Win 0x714dc0 (scalar deleting 0x714f60): the
// children are removed (from the last) and deleted
VxCompositeCollisionGeometry::~VxCompositeCollisionGeometry()
{
    if (mEntity != 0)
        VxAssert(false, "VxCompositeCollisionGeometry::~VxCompositeCollisionGeometry: cannot delete a collision geometry still in a VxEntity.\n");
    for (int i = getCollisionGeometryCount() - 1; i >= 0; --i) {
        VxCollisionGeometry* cg = VxCompositeCollisionGeometry::getCollisionGeometry(i);
        VincAggregateRemoveElement(mModel, i);
        cg->setEntity(0);
        delete cg;
    }
    if (mModel->relTransform != 0)
        setTransformRelative(0);
    VincAggregateModelDestroy(mModel);
    mModel = 0;
}

// Mac 0x43c468, Win 0x714f50
const char* VxCompositeCollisionGeometry::getClassName() const
{
    return "VxCompositeCollisionGeometry";
}

// Mac 0x43bdd6, Win 0x714610
VxCollisionGeometry* VxCompositeCollisionGeometry::clone(bool cloneGeometry)
{
    int maxCount = VincAggregateGetMaxElementCount(mModel->geometry);
    VxCompositeCollisionGeometry* c = new VxCompositeCollisionGeometry(maxCount);
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        c->addCollisionGeometry(getCollisionGeometry(i)->clone(cloneGeometry));
    if (getTransformRelativePtr() != 0)
        c->setTransformRelative(getTransformRelativePtr());
    c->setFlags(mFlags);
    c->setCollisionMode(getCollisionMode());
    return c;
}

// Mac 0x43baac, Win 0x714710
void VxCompositeCollisionGeometry::prepare()
{
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        getCollisionGeometry(i)->prepare();
    VincAggregateModelUpdate(mModel);
}

// Mac 0x43b75a, Win 0x714760
void VxCompositeCollisionGeometry::replaceGeometry(VxGeometry* /*geometry*/)
{
    VxAssert(false, "VxCompositeCollisionGeometry::replaceGeometry: composite doesn't accept geometry.\n");
}

// Mac 0x43c47a, Win 0x719870
VxGeometry* VxCompositeCollisionGeometry::getGeometry()
{
    return 0;
}

// Mac 0x43c484, Win 0x719870
const VxGeometry* VxCompositeCollisionGeometry::getGeometry() const
{
    return 0;
}

// Mac 0x43ba66, Win 0x714780
void VxCompositeCollisionGeometry::setMaterial(VxMaterial* material)
{
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        getCollisionGeometry(i)->setMaterial(material);
}

// Mac 0x43b53a, Win 0x7147d0
VxMaterial* VxCompositeCollisionGeometry::getMaterial() const
{
    return 0;
}

// Mac 0x43ba0e, Win 0x714800
bool VxCompositeCollisionGeometry::setDensity(VxReal density)
{
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        getCollisionGeometry(i)->setDensity(density);
    return true;
}

// Mac 0x43b544, Win 0x714860
VxReal VxCompositeCollisionGeometry::getDensity() const
{
    return 1.0;
}

// Mac 0x43b69c, Win 0x714870
bool VxCompositeCollisionGeometry::getMassProperties(VxReal& mass, VxReal inertia[3][3], VxVector3& com, VxReal* volume)
{
    MeMatrix4 relTM;
    MeReal v, m;
    MeI16 status = VincAggregateGetMassProperties(mModel->geometry, relTM, inertia, &v, &m);
    if (status != 0) {
        mass = m;
        if (volume != 0)
            *volume = v;
        com.set(relTM[3][0], relTM[3][1], relTM[3][2]);
    }
    return status != 0;
}

// Mac 0x43b54c, Win 0x7148d0 (the same code as the base's slot 18)
bool VxCompositeCollisionGeometry::getScaledMassProperties(VxReal mass, VxVector3& com, VxReal inertia[3][3], VxReal* volume)
{
    VxReal m, v;
    bool ok = getMassProperties(m, inertia, com, &v);
    if (ok) {
        VxReal s = mass / m;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                inertia[i][j] = inertia[i][j] * s;
        if (volume != 0)
            *volume = v;
    }
    return ok;
}

// Mac 0x43b636, Win 0x714920
void VxCompositeCollisionGeometry::getCenterOfRotation(VxVector3& p) const
{
    p[2] = 0;
    p[1] = 0;
    p[0] = 0;
}

// Mac 0x43b728, Win 0x7147e0
void VxCompositeCollisionGeometry::updateCollisionModel()
{
    if (mModel != 0) {
        VxCollisionGeometry::updateCollisionModel();
        VincAggregateUpdateElementTransforms(mModel->geometry, mModel->transform);
    }
}

// Mac 0x43b650, Win 0x714940
McdGeometryID VxCompositeCollisionGeometry::getMcdGeometry() const
{
    return 0;
}

// Mac 0x43c008, Win 0x714950
bool VxCompositeCollisionGeometry::setDefaultFluidInteractionForceFn()
{
    setFluidInteractionForceFn(vxCompositeFluidInteractionForce);
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        getCollisionGeometry(i)->setDefaultFluidInteractionForceFn();
    if (mEntity != 0 && mEntity->mUniverse != 0 && getParent() == 0)
        mEntity->mUniverse->registerFluidInteraction(this, true);
    return true;
}

// Mac 0x43b996, Win 0x714a40
void VxCompositeCollisionGeometry::setFluidInteractionParameters(const VxVector3& a, const VxVector3& b, VxReal c, VxReal d)
{
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        getCollisionGeometry(i)->setFluidInteractionParameters(a, b, c, d);
}

// Mac 0x43b950, Win 0x7149d0
void VxCompositeCollisionGeometry::setFluidInteractionData(const VxFluidInteractionData& d)
{
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        getCollisionGeometry(i)->setFluidInteractionData(d);
}

// Mac 0x43b7f0, Win 0x714d00
void VxCompositeCollisionGeometry::setEntity(VxCollisionSensor* entity)
{
    VxCollisionGeometry::setEntity(entity);
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        getCollisionGeometry(i)->setEntity(entity);
}

// Mac 0x43b79c, Win 0x714d50
void VxCompositeCollisionGeometry::setSweepEnabled(bool b)
{
    VxCollisionGeometry::setSweepEnabled(b);
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        getCollisionGeometry(i)->setSweepEnabled(b);
}

// Mac 0x43bec8, Win 0x714ab0
void VxCompositeCollisionGeometry::addCollisionGeometry(VxCollisionGeometry* cg)
{
    if (this != cg) {
        McdModelID root = mModel;
        while (root->parent != 0)
            root = root->parent;
        if (root != cg->mModel) {
            if (cg->mEntity != 0 || cg->mModel->parent != 0) {
                VxWarning(0, "VxCompositeCollisionGeometry::addCollisionGeometry: cannot add a collision geometry alrady added to a part of a composite geometry.\n");
                return;
            }
            if (hasCollisionGeometry(cg))
                return;
            if (mEntity != 0 && mEntity->isPart())
                cg->setEntity(mEntity);
            if (cg->mModel->relTransform == 0)
                cg->setTransformRelative((const VxTransform*)sIdentityTM);
            VincAggregateAddElement(mModel, cg->mModel, cg->mModel->relTransform);
            cg->setSweepEnabled(mModel->sweep != 0);
            if (!cg->isEnabled())
                cg->setEnabled(false);
            updatePartMassProperties();
            return;
        }
    }
    VxWarning(0, "VxCompositeCollisionGeometry::addCollisionGeometry: cannot add a composite to itself.\n");
}

// Mac 0x43b8ec, Win 0x714bc0
void VxCompositeCollisionGeometry::removeCollisionGeometry(int index)
{
    if (index < getCollisionGeometryCount()) {
        VxCollisionGeometry* cg = getCollisionGeometry(index);
        VincAggregateRemoveElement(mModel, index);
        cg->setEntity(0);
        updatePartMassProperties();
    }
}

// Mac 0x43b890, Win 0x714c10
bool VxCompositeCollisionGeometry::removeCollisionGeometry(VxCollisionGeometry* cg)
{
    for (int i = 0; i < getCollisionGeometryCount(); ++i) {
        if (getCollisionGeometry(i) == cg) {
            removeCollisionGeometry(i);
            return true;
        }
    }
    return false;
}

// Mac 0x43b842, Win 0x714ca0
bool VxCompositeCollisionGeometry::hasCollisionGeometry(VxCollisionGeometry* cg) const
{
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        if (getCollisionGeometry(i) == cg)
            return true;
    return false;
}

// Mac 0x43b67c, Win 0x714c80
VxCollisionGeometry* VxCompositeCollisionGeometry::getCollisionGeometry(int index) const
{
    return (VxCollisionGeometry*)VincAggregateGetElementModel(mModel, index)->userData;
}

// Mac 0x43b784 (Win calls VincAggregateGetElementCount 0x75a040 directly)
int VxCompositeCollisionGeometry::getCollisionGeometryCount() const
{
    return VincAggregateGetElementCount(mModel->geometry);
}

} // namespace Vx
