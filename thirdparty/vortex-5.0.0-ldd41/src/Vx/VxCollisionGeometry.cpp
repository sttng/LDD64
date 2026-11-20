// Vortex 5.0.0 build 800 "vortexldd41" - VxCollisionGeometry (restored).
// Mac TU 469 0x437d52-0x439a4b (file guess Vx/VxCollisionGeometry.cpp); Win 0x70f990-0x7111a0.
// Not restored (Win only, dead-stripped on the Mac, not reached by LDD): setMaterial(const char*)
// 0x70fcd0 (VxFrame material lookup), the transform setters 0x710b40 / 0x710be0 / 0x710cb0 /
// 0x710ce0 / 0x710dd0, getTransform 0x70ff40, a bool getTransformRelative 0x70fe10.
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxGeometry.h"
#include "Vx/VxEntity.h"
#include "Vx/VxPart.h"
#include "Vx/VxUniverse.h"
#include "Me/MeMath.h"
#include "Me/MeMemory.h"
#include "VxCollisionInternal.h"

#include <new>
#include <string.h>

#include "Me/VinPool.h"
#include <stdlib.h>

namespace Vx {

namespace {

// LDD64: setTransformRelative's two 128-byte heap blocks per model (2.5 million for 70,000 bricks):
// * the composed (world) transform stays one per model, from a pool growing by blocks of 4096;
// * the relative transform is shared by all models with the same 16 values (a brick's geometries
//   sit at the same place in every copy of its design), refcounted in a hash table by their bits.
//   setTransformRelative swaps the pointer instead of overwriting, so readers see the same values.
// Used under the same conditions as Vortex's model pool (one thread at a time).
VinPool sComposedPool;
bool sComposedPoolReady = false;

MeReal* composedTransformCreate()
{
    if (!sComposedPoolReady) {
        sComposedPool.name = "VxCollisionGeometry transforms";
        VinPoolInit(&sComposedPool, 4096, 16 * (int)sizeof(MeReal), 4096, 0.0f, 16);
        sComposedPoolReady = true;
    }
    return (MeReal*)VinPoolGetStruct(&sComposedPool);
}

void composedTransformDestroy(MeReal* tm)
{
    VinPoolPutStruct(&sComposedPool, tm);
}

struct SharedTransform {
    MeReal m[16];               // first: a model's relTransform points here
    SharedTransform* next;      // hash chain
    unsigned refs;
    unsigned hash;
};

VinPool sSharedPool;
SharedTransform** sSharedBuckets = 0;
unsigned sSharedBucketCount = 0;    // a power of two
unsigned sSharedCount = 0;

unsigned sharedTransformHash(const MeReal* m)
{
    // FNV-1a over the bits of the 16 values
    const unsigned char* p = (const unsigned char*)m;
    unsigned long long h = 14695981039346656037ULL;
    for (size_t i = 0; i < 16 * sizeof(MeReal); ++i)
        h = (h ^ p[i]) * 1099511628211ULL;
    return (unsigned)(h ^ (h >> 32));
}

void sharedTransformRehash(unsigned bucketCount)
{
    SharedTransform** buckets = (SharedTransform**)calloc(bucketCount, sizeof(SharedTransform*));
    for (unsigned i = 0; i < sSharedBucketCount; ++i) {
        SharedTransform* e = sSharedBuckets[i];
        while (e != 0) {
            SharedTransform* next = e->next;
            SharedTransform*& head = buckets[e->hash & (bucketCount - 1)];
            e->next = head;
            head = e;
            e = next;
        }
    }
    free(sSharedBuckets);
    sSharedBuckets = buckets;
    sSharedBucketCount = bucketCount;
}

// the shared transform with the values of src (counted once more)
MeReal* sharedTransformAcquire(const MeReal* src)
{
    if (sSharedBuckets == 0) {
        sSharedPool.name = "VxCollisionGeometry relative transforms";
        VinPoolInit(&sSharedPool, 1024, (int)sizeof(SharedTransform), 1024, 0.0f, 16);
        sharedTransformRehash(1024);
    }
    unsigned hash = sharedTransformHash(src);
    for (SharedTransform* e = sSharedBuckets[hash & (sSharedBucketCount - 1)]; e != 0; e = e->next) {
        if (e->hash == hash && memcmp(e->m, src, sizeof(e->m)) == 0) {
            ++e->refs;
            return e->m;
        }
    }
    SharedTransform* e = (SharedTransform*)VinPoolGetStruct(&sSharedPool);
    memcpy(e->m, src, sizeof(e->m));
    e->refs = 1;
    e->hash = hash;
    SharedTransform*& head = sSharedBuckets[hash & (sSharedBucketCount - 1)];
    e->next = head;
    head = e;
    if (++sSharedCount > sSharedBucketCount)
        sharedTransformRehash(sSharedBucketCount * 2);
    return e->m;
}

void sharedTransformRelease(MeReal* m)
{
    SharedTransform* e = (SharedTransform*)m;
    if (--e->refs != 0)
        return;
    SharedTransform** link = &sSharedBuckets[e->hash & (sSharedBucketCount - 1)];
    while (*link != e)
        link = &(*link)->next;
    *link = e->next;
    --sSharedCount;
    VinPoolPutStruct(&sSharedPool, e);
}

// Mac 0x7791e0, Win 0xabb810: the transform of models without an entity (name inferred)
MeReal sIdentityTM[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

// Mac 0x438005 (static, geometry in EAX; inlined on Win): the default geometry type ID: 1 for a
// triangle mesh (0x133), terrain (0x132) or height field (0x12f), else 0 (name inferred)
int defaultGeometryTypeID(const VxGeometry* g)
{
    if (g->hasClassType(0x133) || g->hasClassType(0x132))
        return 1;
    return g->hasClassType(0x12f);
}

// Mac 0x4382a2 / 0x438338 (two out-of-line copies; Win 0x74faa0 = VincModelGetTransformPtr)
inline const MeReal* modelTransform(McdModelID m)
{
    return VincModelGetTransformPtr(m);
}

// the inlined fluid parameter block allocation (global operator new, Mac 0x438b36 / 0x438c26)
VxFluidInteractionData* fluidData(VxFluidInteractionData*& d)
{
    if (d == 0)
        d = new VxFluidInteractionData;
    return d;
}

} // namespace

bool VxCollisionGeometry::sRegisterWithGeometry = false;   // Mac 0x7791c0, Win 0xaf13b4
VxBase::NameMap* VxCollisionGeometry::sNameMap = 0;        // Mac 0x7791c4, Win 0xaf13b8

// Mac 0x437d52 (0x161), types Mac 0x4395a0 / Win 0x711060, isOfType Mac 0x439928 / Win 0x7110e0,
// isOfClassType Mac 0x437d5c / Win 0x7104d0
VX_IMPLEMENT_CLASS_TYPES(VxCollisionGeometry, VxBaseUserData, 0x161)

// operator new Mac 0x4387bf / Win 0x70fb50, delete Mac 0x4387a8 / Win 0x70fb60
VX_IMPLEMENT_ALLOCATOR(VxCollisionGeometry)

// Mac 0x4385dc, Win 0x710600
VxCollisionGeometry::VxCollisionGeometry()
    : mEntity(0)
    , mModel(0)
    , mMaterial(0)
    , mFluidInteractionForceFn(0)
    , mFluidInteractionData(0)
    , mFlags(3)
    , mGeometry(0)
    , m40(0)
{
}

// C2 Mac 0x43864c (C1 0x438744), Win 0x7104f0
VxCollisionGeometry::VxCollisionGeometry(VxGeometry* geometry)
    : mEntity(0)
    , mModel(0)
    , mMaterial(0)
    , mFluidInteractionForceFn(0)
    , mFluidInteractionData(0)
    , mFlags(3)
    , mGeometry(geometry)
    , m40(0)
{
    geometry->addRef();
    if (mGeometry->mGeometry != 0) {
        mModel = VincModelCreate(mGeometry->mGeometry);
        VincModelSetTransformPtr(mModel, sIdentityTM);
        VincModelSetUserData(mModel, this);
        setGeometryTypeID(defaultGeometryTypeID(mGeometry));
        if (sRegisterWithGeometry)
            mGeometry->registerCollisionGeometry(this);
    }
}

// D2 Mac 0x439318 (D1 0x439440, D0 0x4391e8), Win 0x710e00 (scalar deleting 0x7111a0)
VxCollisionGeometry::~VxCollisionGeometry()
{
    if (mModel != 0 && mModel->parent != 0)
        VxAssert(false, "VxCollisionGeometry::~VxCollisionGeometry: cannot delete a collision geometry still in a VxCompositeGeometry.\n");
    if (mEntity != 0)
        VxAssert(false, "VxCollisionGeometry::~VxCollisionGeometry: cannot delete a collision geometry still in a VxEntity.\n");
    if (mGeometry != 0) {
        if (sRegisterWithGeometry)
            mGeometry->unregisterCollisionGeometry(this);
        if (mModel != 0) {
            if (mModel->relTransform != 0)
                setTransformRelative(0);
            VincModelDestroy(mModel);
        }
        mGeometry->release();
    }
    mEntity = 0;
    operator delete(mFluidInteractionData);
    VxBase::setName(0, &sNameMap);
    mNode.release(); // the member's destructor in the original (Mac 0x4673c6, Win 0x760b30)
}

// Mac 0x43996e, Win 0x711160
const char* VxCollisionGeometry::getClassName() const
{
    return "VxCollisionGeometry";
}

// Mac 0x4399b6, Win 0x70f990
void VxCollisionGeometry::setName(const char* name)
{
    VxBase::setName(name, &sNameMap);
}

// Mac 0x439036, Win 0x710f00
VxCollisionGeometry* VxCollisionGeometry::clone(bool cloneGeometry)
{
    VxGeometry* g = mGeometry;
    if (cloneGeometry)
        g = g->clone();
    VxCollisionGeometry* cg = new VxCollisionGeometry(g);
    cg->setDensity(getDensity());
    cg->setMaterial(getMaterial());
    cg->setResponseModel(getResponseModel());
    if (getTransformRelativePtr() != 0)
        cg->setTransformRelative(getTransformRelativePtr());
    cg->setFlags(mFlags);
    cg->setCollisionMode(getCollisionMode());
    if (mFluidInteractionData != 0) {
        cg->mFluidInteractionData = new VxFluidInteractionData;
        *cg->mFluidInteractionData = *mFluidInteractionData;
    }
    cg->mFluidInteractionForceFn = mFluidInteractionForceFn;
    return cg;
}

// Mac 0x437d72, Win 0x70fbf0
void VxCollisionGeometry::prepare()
{
    mGeometry->prepare();
}

// Mac 0x438d06, Win 0x710890
void VxCollisionGeometry::replaceGeometry(VxGeometry* geometry)
{
    if (mGeometry == geometry)
        return;
    if (sRegisterWithGeometry)
        mGeometry->unregisterCollisionGeometry(this);
    mGeometry->release();
    mGeometry = geometry;
    geometry->addRef();
    if (sRegisterWithGeometry)
        mGeometry->registerCollisionGeometry(this);
    VincModelSetGeometry(mModel, geometry->mGeometry);
    if (mEntity != 0 && mEntity->mUniverse != 0)
        prepare();
    transformChanged();
    if (mFluidInteractionForceFn != 0)
        setFluidInteractionForceFn(0);
    int type = getGeometryTypeID();
    if (type == 0 || type == 1)
        setGeometryTypeID(defaultGeometryTypeID(mGeometry));
}

// Mac 0x439980, Win 0x711170
VxGeometry* VxCollisionGeometry::getGeometry()
{
    return mGeometry;
}

// Mac 0x43998c, Win 0x711170
const VxGeometry* VxCollisionGeometry::getGeometry() const
{
    return mGeometry;
}

// Mac 0x437d88, Win 0x70fcb0
void VxCollisionGeometry::setMaterial(VxMaterial* material)
{
    mMaterial = material;
}

// Mac 0x437d96, Win 0x70fcc0
VxMaterial* VxCollisionGeometry::getMaterial() const
{
    return mMaterial;
}

// Mac 0x438b0a, Win 0x7109d0
VxMaterial* VxCollisionGeometry::getMaterial(int index) const
{
    if (index < 0 || index >= mGeometry->mMaterials.size())
        return mMaterial;
    return mGeometry->mMaterials.begin()[index];
}

// Mac 0x438a6c, Win 0x710640
bool VxCollisionGeometry::setDensity(VxReal density)
{
    if (density != 0.0)
        setFlags(mFlags | 1);
    else
        setFlags(mFlags & ~1u);
    VincModelSetDensity(mModel, density);
    updatePartMassProperties();
    return true;
}

// Mac 0x43854a, Win 0x70fca0
VxReal VxCollisionGeometry::getDensity() const
{
    return VincModelGetDensity(mModel);
}

// Mac 0x438190, Win 0x710030
bool VxCollisionGeometry::getMassProperties(VxMassProperties& mp, VxReal* volume)
{
    VxReal mass;
    VxReal inertia[3][3];
    VxVector3 com;
    bool ok = getMassProperties(mass, inertia, com, volume);
    if (ok) {
        mp.setInertiaTensor(inertia);
        mp.setMass(mass);
        mp.setCOMPosition(com);
    }
    return ok;
}

// Mac 0x438218, Win 0x70ffd0
bool VxCollisionGeometry::getMassProperties(VxReal& mass, VxReal inertia[3][3], VxVector3& com, VxReal* volume)
{
    MeMatrix4 relTM;
    MeReal v, m;
    MeI16 status = VincModelGetMassProperties(mModel, relTM, inertia, &v, &m);
    if (status != 0) {
        mass = m;
        if (volume != 0)
            *volume = v;
        com.set(relTM[3][0], relTM[3][1], relTM[3][2]);
    }
    return status != 0;
}

// Mac 0x437dbc, Win 0x710090
bool VxCollisionGeometry::getScaledMassProperties(VxReal mass, VxVector3& com, VxReal inertia[3][3], VxReal* volume)
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

// Mac 0x43834e, Win 0x710190
void VxCollisionGeometry::getCenterOfRotation(VxVector3& p) const
{
    McdModelID m = mModel;
    const MeReal* t = modelTransform(mModel);
    p[0] = t[0] * m->negCOM[0] + t[4] * m->negCOM[1] + t[8] * m->negCOM[2];
    p[1] = t[1] * m->negCOM[0] + t[5] * m->negCOM[1] + t[9] * m->negCOM[2];
    p[2] = t[2] * m->negCOM[0] + t[6] * m->negCOM[1] + t[10] * m->negCOM[2];
    t = modelTransform(mModel);
    p[0] = t[12] - p[0];
    p[1] = t[13] - p[1];
    p[2] = t[14] - p[2];
}

// Mac 0x4382b8, Win 0x710140
void VxCollisionGeometry::predictTransform(VxTransform& out, VxReal dt) const
{
    McdModelID m = mModel;
    MeMatrix4TMUpdateFromVelocitiesCOM(out.m, 1e-4, dt, m->linearVelocity, m->angularVelocity,
                                       (MeReal(*)[4])modelTransform(mModel), 0, m->negCOM);
}

// Mac 0x437fb8, Win 0x7103f0
void VxCollisionGeometry::updateCollisionModel()
{
    if (mModel != 0)
        VincModelUpdateTransform(mModel);
}

// Mac 0x438afc, Win 0x710400
McdGeometryID VxCollisionGeometry::getMcdGeometry() const
{
    return mGeometry->mGeometry;
}

// Mac 0x437ec4, Win 0x7102c0
bool VxCollisionGeometry::applyFluidInteractionForce(void* fluidState)
{
    if (mFluidInteractionForceFn == 0 || mEntity == 0 || mEntity->mInternalControl != VxEntity::kControlDynamic)
        return false;
    return mFluidInteractionForceFn(this, fluidState);
}

// Mac 0x439998, Win 0x711180
bool VxCollisionGeometry::hasFluidInteractionForceFn() const
{
    return mFluidInteractionForceFn != 0;
}

// Mac 0x4388de, Win 0x710250
void VxCollisionGeometry::setFluidInteractionForceFn(VxFluidInteractionForceFn fn)
{
    mFluidInteractionForceFn = fn;
    if (fn == 0) {
        setFlags(mFlags & ~4u);
        VxCollisionSensor* e = mEntity;
        if (e != 0 && e->mUniverse != 0 && getParent() == 0)
            e->mUniverse->registerFluidInteraction(this, false);
    } else {
        setFlags(mFlags | 4);
        VxCollisionSensor* e = mEntity;
        if (e != 0 && e->mUniverse != 0 && getParent() == 0)
            e->mUniverse->registerFluidInteraction(this, true);
    }
}

// Mac 0x438056, Win 0x7102f0
bool VxCollisionGeometry::setDefaultFluidInteractionForceFn()
{
    VxGeometry* g = getGeometry();
    VxFluidInteractionForceFn fn;
    if (g->hasClassType(0x131))
        fn = vxSphereFluidInteractionForce;
    else if (g->hasClassType(0x12e))
        fn = vxCylinderFluidInteractionForce;
    else if (g->hasClassType(300))
        fn = vxCapsuleFluidInteractionForce;
    else if (g->hasClassType(299))
        fn = vxBoxFluidInteractionForce;
    else if (g->hasClassType(0x133))
        fn = vxTriangleMeshFluidInteractionForce;
    else if (g->hasClassType(0x12d))
        fn = vxConvexMeshFluidInteractionForce;
    else {
        VxInfo(0, "VxCollisionGeometry::setDefaultFluidInteractionForceFn: geom %s not supported\n", g->getClassName());
        fn = 0;
    }
    setFluidInteractionForceFn(fn);
    return fn != 0;
}

// Mac 0x438b36, Win 0x710780
void VxCollisionGeometry::setFluidInteractionParameters(const VxVector3& a, const VxVector3& b, VxReal c, VxReal d)
{
    setFlags(mFlags | 4);
    VxFluidInteractionData* p = fluidData(mFluidInteractionData);
    p->m00 = a;
    p->m18 = b;
    p->m30 = d;
    p->m38 = c;
}

// Mac 0x438c26, Win 0x7106f0
void VxCollisionGeometry::setFluidInteractionData(const VxFluidInteractionData& d)
{
    setFlags(mFlags | 4);
    *fluidData(mFluidInteractionData) = d;
}

// Mac 0x4399aa, Win 0x711190
VxFluidInteractionData* VxCollisionGeometry::getFluidInteractionData() const
{
    return mFluidInteractionData;
}

// Mac 0x43881c, Win 0x70fc00
void VxCollisionGeometry::setEntity(VxCollisionSensor* entity)
{
    if (mFluidInteractionForceFn != 0 && mEntity != 0 && mEntity->mUniverse != 0)
        mEntity->mUniverse->registerFluidInteraction(this, false);
    mEntity = entity;
    if (mFluidInteractionForceFn != 0 && entity != 0 && entity->mUniverse != 0)
        entity->mUniverse->registerFluidInteraction(this, true);
    if (mEntity != 0) {
        if (mEntity->isPart())
            mModel->m6c = (ptrdiff_t)static_cast<VxPart*>(mEntity)->mBody; // ptrdiff_t: no truncation on 64-bit hosts
        return;
    }
    if (mModel->parent == 0) {
        if (mModel->relTransform == 0)
            mModel->transform = sIdentityTM;
        else
            mModel->bodyTransform = sIdentityTM;
    }
}

// Mac 0x437efc, Win 0x710410
void VxCollisionGeometry::setSweepEnabled(bool b)
{
    mModel->sweep = b;
}

// Mac 0x438df0, Win 0x710a00
void VxCollisionGeometry::setTransformRelative(const VxTransform* tm)
{
    MeReal* relTM = 0;
    MeReal* bodyTM = 0;
    VincModelGetRelativeTransformPtrs(mModel, &relTM, &bodyTM);
    MeReal* composed = VincModelGetTransformPtr(mModel);
    const MeReal* src;
    if (tm == 0) {
        if (mModel->parent == 0) {
            if (relTM != 0) {
                sharedTransformRelease(relTM);
                composedTransformDestroy(composed);
                VincModelSetRelativeTransformPtrs(mModel, 0, 0, 0);
                if (bodyTM != 0)
                    VincModelSetTransformPtrRaw(mModel, bodyTM);
            }
            transformChanged();
            return;
        }
        src = sIdentityTM;
    } else {
        src = &tm->m[0][0];
    }
    // acquired before the old one is released: src may be the model's own relative transform
    MeReal* shared = sharedTransformAcquire(src);
    if (relTM == 0) {
        MeReal* t = composedTransformCreate();
        for (int i = 0; i < 16; ++i)
            t[i] = (i % 5 == 0) ? 1.0 : 0.0;
        bodyTM = VincModelGetTransformPtr(mModel);
        VincModelSetRelativeTransformPtrs(mModel, shared, bodyTM, t);
    } else {
        sharedTransformRelease(relTM);
        mModel->relTransform = shared;
    }
    transformChanged();
}

// Mac 0x439454, Win 0x70fe80
void VxCollisionGeometry::getTransformRelative(VxTransform& tm) const
{
    const VxTransform* rel = getTransformRelativePtr();
    if (rel == 0)
        tm = VxTransform();
    else
        tm = *rel;
}

// Mac 0x438446, Win 0x70fde0
const VxTransform* VxCollisionGeometry::getTransformRelativePtr() const
{
    MeReal* relTM = 0;
    MeReal* bodyTM = 0;
    VincModelGetRelativeTransformPtrs(mModel, &relTM, &bodyTM);
    return (const VxTransform*)relTM;
}

// Mac 0x438ae8, Win 0x70ff30
const VxTransform* VxCollisionGeometry::getTransformPtr() const
{
    return (const VxTransform*)modelTransform(mModel);
}

// Mac 0x4387d2, Win 0x7100e0
void VxCollisionGeometry::getBoundingBox(VxVector3& minCorner, VxVector3& maxCorner) const
{
    VxReal dt;
    if (mEntity == 0 || mEntity->mUniverse == 0)
        dt = 0;
    else
        dt = mEntity->mUniverse->getTimeStep();
    VincModelUpdate(mModel, minCorner.data(), maxCorner.data(), dt);
}

// Mac 0x43816c, Win 0x710120
void VxCollisionGeometry::getBoundingSphere(VxVector3& center, VxReal& radius) const
{
    VincModelGetBSphere(mModel, center.data(), &radius);
}

// Mac 0x437fe8, Win 0x7103c0
void VxCollisionGeometry::setCollisionMode(int mode)
{
    VincModelSetState(mModel, mode);
}

// Mac 0x437fd2, Win 0x7103e0
int VxCollisionGeometry::getCollisionMode() const
{
    return VincModelGetState(mModel);
}

// Mac 0x437f5c, Win 0x710430
void VxCollisionGeometry::update()
{
    if (!isEnabled())
        return;
    if (mModel->space == 0) {
        VxCollisionGeometry* parent = getParent();
        if (parent != 0)
            parent->update();
    } else {
        VincSpaceUpdateModelWithTime(mModel, mEntity->mUniverse->getTimeStep(), 1);
    }
}

// Mac 0x438560, Win 0x70fbd0
void VxCollisionGeometry::updatePartMassProperties()
{
    VxCollisionSensor* e = mEntity;
    if (e != 0 && e->mInternalControl == VxEntity::kControlDynamic && static_cast<VxPart*>(e)->mBody != 0)
        static_cast<VxPart*>(e)->computeMassProperties();
}

// Mac 0x43858c (inlined on Win)
void VxCollisionGeometry::transformChanged()
{
    if (mEntity == 0)
        return;
    updateCollisionModel();
    if (mEntity->mInternalControl == VxEntity::kControlFrozen)
        update();
    else
        updatePartMassProperties();
}

// Mac 0x438986, Win 0x710810
void VxCollisionGeometry::setEnabled(bool b)
{
    if (mModel->parent == 0) {
        if (isEnabled()) {
            if (b)
                return;
            setFlags(mFlags & ~2u);
        } else {
            if (!b)
                return;
            setFlags(mFlags | 2);
        }
        if (mEntity != 0 && mEntity->mUniverse != 0)
            mEntity->collisionGeometryEnableChanged(this);
    } else if (!b) {
        setCollisionMode(2);
        setFlags(mFlags & ~2u);
    } else {
        setCollisionMode(0);
        setFlags(mFlags | 2);
    }
}

// Mac 0x437eb4, Win 0x710240
bool VxCollisionGeometry::isEnabled() const
{
    return (mFlags >> 1) & 1;
}

// Mac 0x437ea6, Win 0x710230
void VxCollisionGeometry::setFlags(unsigned int flags)
{
    mFlags = flags;
}

// Mac 0x437da2, Win 0x70ffb0
VxCollisionGeometry* VxCollisionGeometry::getParent() const
{
    McdModelID parent = mModel->parent;
    if (parent == 0)
        return 0;
    return (VxCollisionGeometry*)parent->userData;
}

// Mac 0x43847c, Win 0x70fd70
bool VxCollisionGeometry::setGeometryTypeID(int id)
{
    VincModelSetM7C(mModel, (void*)(size_t)id);
    VxCollisionSensor* e = mEntity;
    if (e != 0 && e->mUniverse != 0) {
        if (e->isPart()) {
            if (e->mUniverse->getMaxCollisionGeometryType() <= id)
                e->mUniverse->setCollisionGeometryTypeCount(id * 2);
        }
        e->mUniverse->updateCollisionGeometryType(this);
    }
    return true;
}

// Mac 0x4384fc, Win 0x70fd60
int VxCollisionGeometry::getGeometryTypeID() const
{
    return (int)(size_t)VincModelGetM7C(mModel);
}

// Mac 0x438512, Win 0x70fd40
bool VxCollisionGeometry::setResponseModel(int model)
{
    VincModelSetM80(mModel, (void*)(size_t)model);
    return true;
}

// Mac 0x438534
int VxCollisionGeometry::getResponseModel() const
{
    return (int)(size_t)VincModelGetM80(mModel);
}

// Mac 0x437f26, Win 0x7104b0
void VxCollisionGeometry::unk_437f26(int value)
{
    mModel->ma4 = value;
}

// Mac 0x437f12, Win 0x710480
void VxCollisionGeometry::setRegisterWithGeometry(bool b)
{
    sRegisterWithGeometry = b;
}

} // namespace Vx
