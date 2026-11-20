// Vortex 5.0.0 build 800 "vortexldd41" - VxUniverse (restored).
// Mac TU 528 (0x4806ee-0x486cb1; static initialiser 0x486c41: <iostream>), Win 0x707650-0x70d7a0.
// The TU also holds VxStepperBase::setTimeStep (Mac 0x485352), the VxVisitor defaults
// (0x485364-0x4853a0), VxUniverse::UpdateSubscriber's slot 3 (0x4853aa), the gathering visitors
// (GatherAllConstraints.h) and the inline destructors of the response handler interfaces
// (Vx/VxResponse.h). The std::set / std::find / std::for_each instances (Mac 0x4853c7-0x485a62,
// 0x486664) are library code.
#include "Vx/VxUniverse.h"
#include "Vx/VxFrame.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxMechanism.h"
#include "Vx/VxBridge.h"
#include "Vx/VxResponse.h"
#include "Vx/VxInternalIDIntersectSubscriber.h"
#include "Vx/VxFluidInteractionUniverseUpdate.h"
#include "Vx/VxVisitor.h"
#include "Vx/VxPart.h"
#include "Vx/VxConstraint.h"
#include "Vx/VxDifferential.h"
#include "Vx/VxGearRatio.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxCompositeCollisionGeometry.h"
#include "Vx/VxGeometry.h"
#include "Vx/VxMaterial.h"
#include "Vx/VxTimer.h"
#include "Vx/VxMessage.h"
#include "Mdt/MdtWorld.h"
#include "Mdt/MdtBody.h"
#include "Mdt/MdtConstraint.h"
#include "Vinc/VincSpace.h"
#include "Vinc/VincModel.h"
#include "GatherAllConstraints.h"
#include "UniverseConnector.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <set>
#include <iostream>

namespace Vx {

namespace {

int sDisablePairWarnings = 0; // Mac 0x780140 (the "disablePairIntersect" warning count)

// The euler order of the print (Mac 0x779320, a VxEntity static int that is 0) (name inferred)
const int kPrintEulerOrder = 0;

// the space's axis mask (Mac 0x779d38 = 7: the three axes)
const int kSpaceAxisMask = 7;

} // namespace

// =============================================================================================
// VxStepperBase / VxVisitor / UpdateSubscriber pieces of this TU
// =============================================================================================

// Mac 0x485352, Win 0x707650
void VxStepperBase::setTimeStep(VxReal dt)
{
    mTimeStep = dt;
}

// Mac 0x486cb2 (D2 of the pure destructor)
VxVisitor::~VxVisitor()
{
}

// Mac 0x485364 .. 0x4853a0, Win 0x7c61f0 (one folded "return 0")
int VxVisitor::visitUniverse(VxUniverse*) { return 0; }
int VxVisitor::endVisitUniverse(VxUniverse*) { return 0; }
int VxVisitor::visitAssembly(VxAssemblyBase*) { return 0; }
int VxVisitor::endVisitAssembly(VxAssemblyBase*) { return 0; }
int VxVisitor::visitEntity(VxEntity*) { return 0; }
int VxVisitor::visitConstraint(VxConstraint*) { return 0; }
int VxVisitor::visitOther(void*) { return 0; }

// Mac 0x4853aa, Win 0x7076a0 (shared empty function)
void VxUniverse::UpdateSubscriber::intersectionsUpdated()
{
}

// Mac 0x485e4c
int GatherAllConstraints::visitConstraint(VxConstraint* c)
{
    mConstraints.push_back(c);
    return 0;
}

// Mac 0x4869ba
int GatherAllCollisionSensor::visitEntity(VxEntity* e)
{
    if (e != 0) {
        VxCollisionSensor* s = dynamic_cast<VxCollisionSensor*>(e);
        if (s != 0)
            mSensors.push_back(s);
    }
    return 0;
}

// =============================================================================================
// VxUniverse
// =============================================================================================

// Mac 0x4806ee (0x106), 0x4806f8, 0x482276, 0x4825fe; Win 0x708bf0, 0x70ae10
VX_IMPLEMENT_CLASS_TYPES(VxUniverse, VxStepperBase, 0x106)

// Mac 0x480bec / 0x480bd5, Win 0x7077c0 / 0x7077d0
VX_IMPLEMENT_ALLOCATOR(VxUniverse)

// Mac 0x48070e, Win 0x707aa0
void VxUniverse::setMaxPartitionsPerUnit(int n)
{
    if (0 < n)
        mWorld->maxPartitionsPerUnit = n;
}

// Mac 0x480726, Win 0x707e20
bool VxUniverse::addAssembly(VxAssemblyBase* a)
{
    bool free = a->mParent == 0;
    if (free)
        mRootMechanism->addAssembly(a);
    return free;
}

// Mac 0x480756, Win 0x707e50
bool VxUniverse::removeAssembly(VxAssemblyBase* a)
{
    mRootMechanism->removeAssembly(a);
    return true;
}

// Mac 0x48077c, Win 0x707e80
void VxUniverse::beginStep()
{
    mRootMechanism->preStep();
    mStepping = true;
}

// Mac 0x48079c, Win 0x707f60
void VxUniverse::addIntersectSubscriber(int type1, int type2, int event, IntersectSubscriber* s, unsigned int index)
{
    mIntersectManager->addSubscriber(type1, type2, event, s, index);
}

// Mac 0x4807d6
bool VxUniverse::removeIntersectSubscriber(int type1, int type2, int event, IntersectSubscriber* s)
{
    return mIntersectManager->removeSubscriber(type1, type2, event, s);
}

// Mac 0x480824, Win 0x708430
VxReal VxUniverse::getTime1() const
{
    return mTime1;
}

// Mac 0x480830, Win 0x708440
VxReal VxUniverse::getTime2() const
{
    return mTime2;
}

// --- DynamicsContactIterator -----------------------------------------------------------------

// Mac 0x480864
VxUniverse::DynamicsContactIterator VxUniverse::dynamicsContactEnd()
{
    return DynamicsContactIterator(0);
}

// Mac 0x480888
VxDynamicsContact* VxUniverse::DynamicsContactIterator::operator*() const
{
    return (VxDynamicsContact*)mContact;
}

// Mac 0x480892
bool VxUniverse::DynamicsContactIterator::operator!=(const DynamicsContactIterator& o) const
{
    return mContact != o.mContact;
}

// Mac 0x480aea
VxUniverse::DynamicsContactIterator& VxUniverse::DynamicsContactIterator::operator++()
{
    MdtConstraintID c = MdtConstraintGetNextEnabled(mContact);
    while (c != 0 && c->bcl.type != kMdtTypeContact)
        c = MdtConstraintGetNextEnabled(c);
    mContact = c;
    return *this;
}

// Mac 0x480b32
VxUniverse::DynamicsContactIterator VxUniverse::dynamicsContactBegin()
{
    MdtConstraintID c = mWorld->enabledConstraints; // (MdtConstraintGetNext applied to the world: its +4)
    while (c != 0 && c->bcl.type != kMdtTypeContact)
        c = MdtConstraintGetNextEnabled(c);
    return DynamicsContactIterator(c);
}

// --- DynamicPartIterator ---------------------------------------------------------------------

// Mac 0x480910 (C1 0x48094e)
VxUniverse::DynamicPartIterator::DynamicPartIterator(VxPart* part)
{
    if (part == 0 || part->mUniverse == 0 || part->mInternalControl != VxEntity::kControlDynamic) {
        mNext = 0;
        mBody = 0;
    } else {
        mBody = part->mBody;
        mNext = mBody->nextEnabled;
    }
}

// Mac 0x4809a0, Win 0x708a70
VxUniverse::DynamicPartIterator& VxUniverse::DynamicPartIterator::operator++()
{
    if (mNext == 0) {
        mBody = 0;
    } else if (MdtBodyIsEnabled(mBody)) {
        mBody = mBody->nextEnabled;
        mNext = mBody->nextEnabled;
    } else if (MdtBodyIsEnabled(mNext)) {
        mBody = mNext;
        mNext = mNext->nextEnabled;
    } else {
        mNext = 0;
        mBody = 0;
        VxWarning(0, "VxUniverse::DynamicPartIterator: part should not be disabled or removed from the universe while iterating them. Visit is aborted\n");
    }
    return *this;
}

// Mac 0x480a32, Win 0x708af0
VxUniverse::DynamicPartIterator VxUniverse::DynamicPartIterator::operator++(int)
{
    DynamicPartIterator old;
    if (mBody == 0) {
        old.mBody = 0;
        old.mNext = mNext;
    } else {
        old.mBody = mBody;
        old.mNext = mNext;
        ++*this;
    }
    return old;
}

// Mac 0x480a7a
VxPart* VxUniverse::DynamicPartIterator::operator*() const
{
    if (mBody == 0)
        return 0;
    return (VxPart*)MdtBodyGetUserData(mBody);
}

// Mac 0x480a9c
VxUniverse::DynamicPartIterator VxUniverse::dynamicPartBegin()
{
    MdtBodyID b = mWorld->enabledBodies;
    if (b == 0)
        return DynamicPartIterator();
    return DynamicPartIterator((VxPart*)MdtBodyGetUserData(b));
}

// Mac 0x4808f4
VxUniverse::DynamicPartIterator VxUniverse::dynamicPartEnd()
{
    return DynamicPartIterator();
}

// --- bridge wrappers ---------------------------------------------------------------------------

// Mac 0x4808a8, Win 0x708f80
int VxUniverse::getMaxCollisionGeometryType() const
{
    return mIntersectManager->mPairRequests.mSize - 1;
}

// Mac 0x4808b8, Win 0x7086d0
void VxUniverse::setSleepingPairsIntersect(bool b)
{
    mIntersectManager->setSleepingPairsIntersect(b);
}

// Mac 0x480c38
void VxUniverse::registerResponseModel(int model)
{
    mIntersectManager->registerResponseModel(model);
}

// Mac 0x480c56, Win 0x7086a0
bool VxUniverse::canChangeResponseModel(int oldModel, int newModel) const
{
    return mIntersectManager->canChangeResponseModel(oldModel, newModel);
}

// Mac 0x480c7e, Win 0x708630
void VxUniverse::insertBody(MdtBodyID body)
{
    if (mStepping)
        VxWarning(0, "VxUniverse insert part: cannot be called while stepping (check intersect subscribers).\n");
    MdtWorldInsertBody(mWorld, body);
}

// Mac 0x480cca, Win 0x708600
void VxUniverse::setCollisionGeometryTypeCount(int count)
{
    mIntersectManager->setCollisionGeometryTypeCount(count);
}

// Mac 0x480ce8, Win 0x708f90
VxCollisionPairRequest* VxUniverse::getCollisionPairRequest(int type1, int type2)
{
    int n = getMaxCollisionGeometryType();
    if (n <= type1 || n <= type2)
        setCollisionGeometryTypeCount(type1 <= type2 ? type2 : type1);
    return &mIntersectManager->getPairRequest(type1, type2);
}

// VxBridge::updateCollisionGeometryType (Mac 0x4346c8, Win 0x741030)
void VxUniverse::updateCollisionGeometryType(VxCollisionGeometry* cg)
{
    mIntersectManager->updateCollisionGeometryType(cg);
}

// Mac 0x480d42, Win 0x70ce80
int VxUniverse::accept(VxVisitor* v)
{
    int r = v->visitUniverse(this);
    if (r == 0) {
        r = mRootMechanism->accept(v);
        if (r != 2)
            r = v->endVisitUniverse(this);
    }
    return r;
}

// --- pair switches -----------------------------------------------------------------------------

// PORT64: the model's space is read through McdModel::space, not at the byte offset
// Vinc::kMcdModelSpace (Mac 0x54 / Win 0x58, where the field sits on 32-bit: checked here).
typedef char VxUniverseMcdModelSpaceCheck[
    (sizeof(void*) != 4 || offsetof(struct McdModel, space) == (size_t)Vinc::kMcdModelSpace) ? 1 : -1];

// Mac 0x480d8c, Win 0x708330
bool VxUniverse::enablePairIntersect(VxCollisionGeometry* cg0, VxCollisionGeometry* cg1)
{
    if (cg0->mEntity != cg1->mEntity && cg0->isEnabled() && cg1->isEnabled()) {
        void* space = cg0->mModel->space;
        if (space != 0 && space == cg1->mModel->space)
            return VincSpaceEnablePair(cg0->mModel, cg1->mModel) != 0;
    }
    return false;
}

// Mac 0x480df4, Win 0x708d40
void VxUniverse::enablePairIntersect(VxCollisionGeometry* cg, VxCollisionSensor* sensor)
{
    if (sensor == 0 || cg == 0)
        return;
    int n = sensor->getCollisionGeometryCount();
    for (int i = 0; i != n; ++i)
        enablePairIntersect(cg, sensor->getCollisionGeometry(i));
}

// Mac 0x480e4c, Win 0x709640
void VxUniverse::enablePairIntersect(VxCollisionSensor* s0, VxCollisionSensor* s1)
{
    if (s0 == 0 || s1 == 0 || s0 == s1)
        return;
    int n = s0->getCollisionGeometryCount();
    for (int i = 0; i != n; ++i)
        enablePairIntersect(s0->getCollisionGeometry(i), s1);
}

// Mac 0x480eaa, Win 0x7082a0
bool VxUniverse::disablePairIntersect(VxCollisionGeometry* cg0, VxCollisionGeometry* cg1)
{
    if (cg0->isEnabled() && cg1->isEnabled()) {
        void* space = cg0->mModel->space;
        if (space != 0 && space == cg1->mModel->space)
            return VincSpaceDisablePair(cg0->mModel, cg1->mModel) != 0;
    } else if (sDisablePairWarnings < 0x1e) {
        VxInfo(0, "VxUniverse::disablePairIntersect: both collision gemoetry must be added directly to a part and the part added to the universe.\n");
        ++sDisablePairWarnings;
        if (sDisablePairWarnings == 0x1e) {
            VxInfo(0, "Further messages will be suppressed.\n");
            return false;
        }
    }
    return false;
}

// Mac 0x480f68, Win 0x708c80: only for a sensor of this universe and a geometry whose entity is in it
void VxUniverse::disablePairIntersect(VxCollisionGeometry* cg, VxCollisionSensor* sensor)
{
    if (sensor == 0 || cg == 0)
        return;
    VxUniverse* u = sensor->mUniverse;
    if (u != this || u != cg->mEntity->mUniverse)
        return;
    int n = sensor->getCollisionGeometryCount();
    for (int i = 0; i != n; ++i)
        u->disablePairIntersect(cg, sensor->getCollisionGeometry(i));
}

// Mac 0x480fe0, Win 0x7095d0
void VxUniverse::disablePairIntersect(VxCollisionSensor* s0, VxCollisionSensor* s1)
{
    if (s0 == 0 || s1 == 0 || s0 == s1)
        return;
    VxUniverse* u = s0->mUniverse;
    if (u != this || u != s1->mUniverse)
        return;
    int n = s0->getCollisionGeometryCount();
    for (int i = 0; i != n; ++i)
        u->disablePairIntersect(s0->getCollisionGeometry(i), s1);
}

// Mac 0x48105a, Win 0x707fd0
VxInternalIDIntersectSubscriber* VxUniverse::getInternalIDIntersectSubscriber()
{
    if (mInternalIDSubscriber == 0) {
        mInternalIDSubscriber = new VxInternalIDIntersectSubscriber();
        addIntersectSubscriber(0, 0, 2, mInternalIDSubscriber, 0);
        addIntersectSubscriber(0, 0, 0, mInternalIDSubscriber, 0);
        addIntersectSubscriber(0, 0, 3, mInternalIDSubscriber, 0);
        addIntersectSubscriber(0, 1, 2, mInternalIDSubscriber, 0);
        addIntersectSubscriber(0, 1, 0, mInternalIDSubscriber, 0);
        addIntersectSubscriber(0, 1, 3, mInternalIDSubscriber, 0);
    }
    return mInternalIDSubscriber;
}

// Mac 0x4811da, Win 0x707f40
void VxUniverse::endStep()
{
    mStepping = false;
    mRootMechanism->postStep();
    mIntersectManager->disableUserContacts();
}

// Mac 0x481204, Win 0x707da0 (0x707dd0: a second copy)
void VxUniverse::setGravity(VxReal x, VxReal y, VxReal z)
{
    MdtWorldSetGravity(mWorld, x, y, z);
}

// Mac 0x48123c, Win 0x707ce0
int VxUniverse::getCollisionGeometryCount() const
{
    return VincSpaceGetModelCount((VincSpaceID)mSpace);
}

// Mac 0x481252, Win 0x707c90
void VxUniverse::setAutoDisable(bool b)
{
    MdtWorldSetAutoDisable(mWorld, b);
}

// Mac 0x481270, Win 0x707c00: groupings created by the call copy grouping 0's parameters
MdtGrouping* VxUniverse::getGrouping(int grouping)
{
    int n = mWorld->numGroupings;
    MdtGrouping* g = MdtWorldGetGrouping(mWorld, grouping);
    for (; n < mWorld->numGroupings; ++n) {
        MdtGrouping* d = MdtWorldGetGrouping(mWorld, n);
        MdtGrouping* s = MdtWorldGetGrouping(mWorld, 0);
        memcpy(&d->bclParams, &s->bclParams, sizeof(MdtBclSolverParameters));
        memcpy(&d->keaParams, &s->keaParams, sizeof(MdtKeaParameters));
    }
    return g;
}

// Mac 0x481320
int VxUniverse::getKeaM2c() const
{
    return MdtWorldGetKeaM2c(mWorld);
}

// Mac 0x481336
int VxUniverse::getMaxConstraintsPerLevel() const
{
    return MdtWorldGetMaxConstraintsPerLevel(mWorld);
}

// Mac 0x48134c, Win 0x707a10
int VxUniverse::getPartitionMaxDepth() const
{
    if (MdtPartitionParamsGetOptimize(&mWorld->partitionParams) == 0)
        return -1;
    return mWorld->partitionParams.defaultMaxDepth;
}

// Mac 0x481380, Win 0x707700
void MEAPI VxUniverse::bodyDisabledCallback(MdtBodyID body)
{
    VxPart* part = (VxPart*)body->userData;
    if (part == 0)
        return;
    for (int i = 0; i < part->getCollisionGeometryCount(); ++i) {
        VxCollisionGeometry* cg = part->getCollisionGeometry(i);
        if (cg->isEnabled())
            VincSpaceFreezeModel(cg->mModel);
    }
}

// Mac 0x4813da, Win 0x707760
void MEAPI VxUniverse::bodyEnabledCallback(MdtBodyID body)
{
    VxPart* part = (VxPart*)body->userData;
    if (part == 0)
        return;
    for (int i = 0; i < part->getCollisionGeometryCount(); ++i) {
        VxCollisionGeometry* cg = part->getCollisionGeometry(i);
        if (cg->isEnabled())
            VincSpaceUnfreezeModel(cg->mModel);
    }
}

// --- stepping ----------------------------------------------------------------------------------

// Mac 0x481434, Win 0x707ea0
void VxUniverse::stepDynamics()
{
    VxTimer timer;
    timer.start();
    mWorld->useBodyMaxTimeStep = 0;
    MdtWorldStep(mWorld, mTimeStep);
    mTime2 = timer.stop() * 1000.0;
}

// Mac 0x4814c8, Win 0x7096d0
void VxUniverse::stepPhase1()
{
    VxTimer timer;
    timer.start();
    VincSpacePathUpdate((VincSpaceID)mSpace, mTimeStep);
    mIntersectManager->update(mTimeStep);
    mTime1 = timer.stop() * 1000.0;
    for (UpdateSubscriber** i = m44.begin(); i != m44.end(); ++i)
        (*i)->update(this);
    mRootMechanism->step();
}

// Mac 0x4853c2, Win 0x7076a0
void VxUniverse::stepPhase3()
{
}

// Mac 0x4853b0, Win 0x70d130
const char* VxUniverse::getClassName() const
{
    return "VxUniverse";
}

// --- print -------------------------------------------------------------------------------------

namespace {

// Mac 0x4815ac, Win 0x70b220 (register arguments on the Mac: cg in eax, file in edx)
void printCollisionGeometry(FILE* f, VxCollisionGeometry* cg, int depth)
{
    if (cg->isOfClassType(VxCompositeCollisionGeometry::getStaticClassType())) {
        VxCompositeCollisionGeometry* c = static_cast<VxCompositeCollisionGeometry*>(cg);
        fprintf(f, "  startComposite depth %d, %d subcg\n", depth, c->getCollisionGeometryCount());
        for (int i = 0; i < c->getCollisionGeometryCount(); ++i)
            printCollisionGeometry(f, c->getCollisionGeometry(i), depth + 1);
        for (int i = 0; i < depth; ++i)
            fputc(' ', f);
        fprintf(f, "  stopComposite depth %d\n", depth);
        return;
    }
    fprintf(f, "   cg depth %d  ", depth);
    char name[30];
    VxMaterial* m = cg->getMaterial();
    if (m == 0) {
        memset(name, 0, sizeof(name));
        strcpy(name, "material not registered");
    } else {
        strncpy(name, m->getName(), 0x1e);
    }
    // NOTE: the original prints the material pointer as the "Id"
    fprintf(f, "%s (Id=%d)", name, (int)(size_t)m);
    VxVector3 mn(0, 0, 0), mx(0, 0, 0), rpos(0, 0, 0);
    cg->getBoundingBox(mn, mx);
    VxVector3 aabb(mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]);
    if (cg->getTransformRelativePtr() != 0) {
        const VxTransform* r = cg->getTransformRelativePtr();
        rpos = VxVector3(r->m[3][0], r->m[3][1], r->m[3][2]);
    }
    const VxTransform* t = cg->getTransformPtr();
    fprintf(f, ", type=%s, rpos = %.3f %.3f %.3f, pos = %.3f %.3f %.3f, aabb = %.3f %.3f %.3f\n",
            cg->getGeometry()->getClassName(), rpos[0], rpos[1], rpos[2], t->m[3][0], t->m[3][1], t->m[3][2],
            aabb[0], aabb[1], aabb[2]);
    VxGeometry* g = cg->getGeometry();
    if (g->mMaterials.size() != 0) {
        std::set<VxMaterial*> materials(g->mMaterials.begin(), g->mMaterials.end());
        fwrite("\n    Begin list of material ids found in the triangle index materials map:\n", 1, 0x4b, f);
        int n = 0;
        for (std::set<VxMaterial*>::iterator i = materials.begin(); i != materials.end(); ++i) {
            fprintf(f, "%3d ", (*i)->mIndex);
            ++n;
            if (n == (n / 10) * 10)
                fputc('\n', f);
        }
        fwrite("\n    End triangle index map\n", 1, 0x1c, f);
    }
}

VxReal length3(const VxVector3& v)
{
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void printGrouping(FILE* f, int id, MdtGrouping* g, int crossDepth)
{
    static const char* const kSolvers[3] = {"Std", "Iterative", "Keller"};
    int s = 0;
    if (g->keaParams.constraintSolver != 0)
        s = (g->keaParams.constraintSolver != 1) + 1;
    fprintf(f, "Parameters for group id: %d (%s), partition part max count = %3d, cross grp depth = %3d, max coupling iterations = %3d, secondary solver group = %3d\n",
            id, kSolvers[s], g->maxConstraints, crossDepth, g->keaParams.maxIterations, g->id);
    fprintf(f, "\tLC = %12.4g   LD = %12.4g   LK = %12.4g\n", g->bclParams.epsilon, g->bclParams.gamma, g->bclParams.m28);
    fprintf(f, "\tAC = %12.4g   AD = %12.4g   AK = %12.4g\n", g->bclParams.epsilon2, g->bclParams.gamma2, g->bclParams.m30);
    fprintf(f, "\tLCP Parameters: max iteration = %3d, scale box max iteration = %3d, scale box tolerance = %12.4g\n",
            g->keaParams.lcp.maxIterations, g->keaParams.refineIterations, g->keaParams.refineTolerance);
    fprintf(f, "\tIterative Constraint Solver Parameters: max iteration = %3d, solver tolerance = %12.4g\n",
            g->keaParams.itrMaxIterations, g->keaParams.itrTolerance);
    fputc('\n', f);
}

} // namespace

// Mac 0x482b0a, Win 0x70b570: the content of the universe, then the material table, to the file
void VxUniverse::print(const char* fileName)
{
    VxPrintFile out(fileName);
    FILE* f = out.getFile();
    fwrite("------> Top Level Assemblies <------\n\n", 1, 0x26, f);
    for (int i = 0; i < mRootMechanism->getAssemblyCount(); ++i)
        fprintf(f, "Assembly %d name=%s\n", i, mRootMechanism->getAssembly(i)->getName());
    fputc('\n', f);

    std::set<int> groupings;
    GatherAllConstraintsAndSensors gather;
    accept(&gather);
    int dynamicCount = 0, frozenCount = 0, asleepCount = 0;
    const int frozen = VxEntity::kControlFrozen;
    for (VxCollisionSensor** i = gather.mSensors.begin(); i != gather.mSensors.end(); ++i) {
        VxCollisionSensor* e = *i;
        const char* name = e->getName();
        VxVector3 pos, euler;
        e->getPosition(pos);
        e->getEulerAngles(euler, kPrintEulerOrder);
        int control = e->mControl;
        if (control >= 0) {
            if (control < 2) {
                if (!e->isPart()) {
                    fprintf(f, "%s of sensor type\n", name);
                    const char* kind = control == 1 ? "node driven entity" : "animated entity";
                    e->getPosition(pos);
                    e->getEulerAngles(euler, kPrintEulerOrder);
                    VxReal v[3], w[3];
                    e->getLinearVelocity(v[0], v[1], v[2]);
                    e->getAngularVelocity(w[0], w[1], w[2]);
                    fprintf(f, "%s: \n position = %.2f, %.2f, %.2f, euler = %.2f, %.2f, %.2f\n", kind, pos[0], pos[1],
                            pos[2], euler[0], euler[1], euler[2]);
                    fprintf(f, "                     v  , w    = %.2f, %.2f, %.2f and %.2f, %.2f, %.2f\n", v[0], v[1],
                            v[2], w[0], w[1], w[2]);
                } else {
                    e->getPosition(pos);
                    e->getEulerAngles(euler, kPrintEulerOrder);
                    fprintf(f, "%s of entity type: \n position = %.2f, %.2f, %.2f, euler = %.2f, %.2f, %.2f\n", name,
                            pos[0], pos[1], pos[2], euler[0], euler[1], euler[2]);
                }
            } else if (control == 2) {
                VxPart* p = e != 0 ? dynamic_cast<VxPart*>(e) : 0;
                int grouping = p->getGrouping();
                VxVector3 v, w, linAcc, angAcc;
                p->getLinearVelocity(v);
                p->getAngularVelocity(w);
                p->getFrameLinearAcceleration(linAcc);
                p->getAngularAcceleration(angAcc);
                VxReal vThr = p->getAutoDisableVelocityThreshold();
                VxReal aThr = p->getAutoDisableAccelerationThreshold();
                VxReal wThr = p->getAutoDisableAngularVelocityThreshold();
                VxReal waThr = p->getAutoDisableAngularAccelerationThreshold();
                int window = p->getAutoDisableAliveWindow();
                int alive = p->getEnabledTime();
                if (p->getMergeParent() == 0) {
                    fprintf(f, "dynamic part %s: mass = %g in group %d \n position = %.2f, %.2f, %.2f, euler = %.2f, %.2f, %.2f\n",
                            name, static_cast<VxPart*>(e)->getMass(), grouping, pos[0], pos[1], pos[2], euler[0], euler[1],
                            euler[2]);
                    fprintf(f, "   v  , w    = %.2f, %.2f, %.2f and %.2f, %.2f, %.2f\n", v[0], v[1], v[2], w[0], w[1], w[2]);
                    fprintf(f, "   lin_speed  = %12.6f lin_acc  = %12.6f ang_speed = %12.6f ang_ acc = %12.6f          alive = %ld\n",
                            length3(v), length3(linAcc), length3(w), length3(angAcc), (long)alive);
                    fprintf(f, "   thresholds = %12.6f            %12.6f             %12.6f            %12.6f          alive = %ld\n",
                            vThr, aThr, wThr, waThr, (long)window);
                } else if (p->getMergeParent() == p->getMergeRoot()) {
                    fprintf(f, "dynamic part %s is merged to root part %s\n", name, p->getMergeParent()->getName());
                } else {
                    fprintf(f, "dynamic part %s is merged to part %s, with root %s\n", name, p->getMergeParent()->getName(),
                            p->getMergeRoot()->getName());
                }
                dynamicCount += e->mInternalControl != frozen;
                asleepCount += !p->isEnabled();
                groupings.insert(grouping);
            }
        }
        if (frozen == e->mInternalControl) {
            ++frozenCount;
            fwrite(" is frozen\n", 1, 0xb, f);
        }
        int n = e->getCollisionGeometryCount();
        if (0 < n) {
            fprintf(f, " %d Collision geometries:\n", n);
            for (int k = 0; k != n; ++k)
                printCollisionGeometry(f, e->getCollisionGeometry(k), 0);
            fputc('\n', f);
        }
        fputc('\n', f);
    }

    int enabledCount = 0;
    for (VxConstraint** i = gather.mConstraints.begin(); i != gather.mConstraints.end(); ++i) {
        VxConstraint* c = *i;
        const char* name = c->getName();
        if (!c->mEnabled) {
            fprintf(f, "constraint %s: is disabled\n", name);
            continue;
        }
        ++enabledCount;
        fprintf(f, "constraint %s: attached to:\n", name);
        for (int k = 0; k < c->mParts.size(); ++k) {
            VxPart* p = c->mParts.begin()[k];
            fprintf(f, " %s\n", p != 0 ? p->getName() : "world");
            VxVector3 primary, secondary, rel;
            c->getPartAttachmentAxesRel(k, primary, secondary);
            c->getPartAttachmentPositionRel(k, rel);
            fprintf(f, "  relpos = %.2f, %.2f, %.2f\n", rel[0], rel[1], rel[2]);
            fprintf(f, "  relax0 = %.2f, %.2f, %.2f\n", primary[0], primary[1], primary[2]);
            fprintf(f, "  relax1 = %.2f, %.2f, %.2f\n", secondary[0], secondary[1], secondary[2]);
        }
        for (int k = 0; k < c->mParts.size(); ++k) {
            VxVector3 p;
            c->getPartAttachmentPosition(k, p);
            fprintf(f, "  pos %d = %.2f, %.2f, %.2f\n", k, p[0], p[1], p[2]);
        }
        if (c->isOfType(VxDifferential::getStaticClassType())) {
            VxDifferential* d = static_cast<VxDifferential*>(c);
            VxReal r2 = d->getValue(2), r1 = d->getValue(1), r0 = d->getValue(0);
            fprintf(f, "  ratios = %.2f, %.2f, %.2f\n", r0, r1, r2);
        } else if (c->isOfType(VxGearRatio::getStaticClassType())) {
            fprintf(f, "  ratios = %.2f \n", static_cast<VxGearRatio*>(c)->getRatio());
        }
        if (!c->mEnabled) {
            fwrite(" is disabled\n", 1, 0xd, f);
            continue;
        }
        // NOTE: the texts follow the original, which reports isCoordinateMotorized as "locked"
        // (with the motor values) and isCoordinateLocked as "motorized" (with the lock values)
        for (int k = 0; k < c->getCoordinateCount(); ++k) {
            if (!c->isCoordinateMotorized(k)) {
                if (!c->isCoordinateLocked(k))
                    fprintf(f, "  Coord %d is free\n", k);
                else
                    fprintf(f, "  Coord %d is motorized at velocity %g with max force %g\n", k, c->getLockPosition(k),
                            c->getLockMaximumForce(k));
            } else {
                VxReal damping = c->getMotorDamping(k), stiffness = c->getMotorStiffness(k);
                VxReal force = c->getMotorMaximumForce(k), velocity = c->getMotorDesiredVelocity(k);
                fprintf(f, "  Coord %d is locked at position %g with max force %g lock stiffness %g and lock damping %g\n", k,
                        velocity, force, stiffness, damping);
            }
            if (!c->getLimitsActive(k)) {
                fprintf(f, "  Coord %d is not limited, curr pos = %g\n", k, c->getCoordinatePosition(k));
            } else {
                VxReal cur = c->getCoordinatePosition(k), hi = c->getLimitPosition(k, 1), lo = c->getLimitPosition(k, 0);
                fprintf(f, "  Coord %d is limited at %g %g, curr pos = %g\n", k, lo, hi, cur);
            }
        }
        fputc('\n', f);
    }

    printf("\ndynamic part count = %d\n", dynamicCount);
    printf("frozen part count = %d\n", frozenCount);
    printf("asleep part count = %d\n", asleepCount);
    printf("enabled constraint count = %d\n\n", enabledCount);
    fprintf(f, "\ndynamic part count = %d\n", dynamicCount);
    fprintf(f, "frozen part count = %d\n", frozenCount);
    fprintf(f, "asleep part count = %d\n", asleepCount);
    fprintf(f, "enabled constraint count = %d\n\n", enabledCount);
    fwrite("Global solver parameters:\n", 1, 0x1a, f);
    int keaM2c = getKeaM2c();
    int depth = getPartitionMaxDepth();
    int perLevel = getMaxConstraintsPerLevel();
    fprintf(f, "cross grp depth = %3d, max coupling iterations = %3d, secondary partition max size = %3d\n", depth, keaM2c,
            perLevel);
    fputc('\n', f);
    fwrite("Solver group in use:\n", 1, 0x15, f);
    fwrite("\tLC: LinearCompliance \t LD: LinearDamping \t\t LK: LinearKineticLoss\n", 1, 0x43, f);
    fwrite("\tAC: AngularCompliance \t AD: AngularDamping \t AK: AngularKineticLoss\n", 1, 0x45, f);
    std::set<int> secondary;
    for (std::set<int>::iterator i = groupings.begin(); i != groupings.end(); ++i) {
        MdtGrouping* g = getGrouping(*i);
        secondary.insert(g->id);
        printGrouping(f, *i, g, depth);
    }
    fputc('\n', f);
    fwrite("Secondary solver group in use:\n", 1, 0x1f, f);
    fwrite("\tLC: LinearCompliance \t LD: LinearDamping \t\t LK: LinearKineticLoss\n", 1, 0x43, f);
    fwrite("\tAC: AngularCompliance \t AD: AngularDamping \t AK: AngularKineticLoss\n", 1, 0x45, f);
    for (std::set<int>::iterator i = secondary.begin(); i != secondary.end(); ++i)
        printGrouping(f, *i, getGrouping(*i), depth);
    fputc('\n', f);
    fwrite("Default Auto-Disable Parameters:\n", 1, 0x21, f);
    VxReal v, a, w, wa;
    int steps;
    VxPart::getDefaultAutoDisableParameters(v, a, w, wa, steps);
    fprintf(f, "Lin Speed = %.4g Lin Acc = %.4g   Ang Speed = %.4g  Ang Acc = %.4g  steps = %d\n", v, a, w, wa, steps);
    fwrite("\n\n", 1, 2, f);
    mIntersectManager->mMaterialTable->print(fileName);
}

// --- disabled collisions / construction ------------------------------------------------------

namespace {

// Vx::VxUniverse::applyDisabledCollisions(bool)::ApplyDisabledCollisions (a local class in the
// original: Mac vtable 0x75ed68, visitAssembly 0x48080c, dtor 0x481b48 / 0x480c00)
class ApplyDisabledCollisions : public VxVisitor {
public:
    virtual ~ApplyDisabledCollisions() {}
    virtual int visitAssembly(VxAssemblyBase* a)
    {
        a->applyDisabledCollisions();
        return 0;
    }
};

} // namespace

// Mac 0x481b72, Win 0x708dc0
void VxUniverse::applyDisabledCollisions(bool visit)
{
    m88->apply(this);
    if (visit) {
        ApplyDisabledCollisions v;
        accept(&v);
    }
}

// Mac 0x481c06, Win 0x709180
void VxUniverse::initialize(int maxPartCount, int maxPairCount)
{
    mStepperParam0 = 5;
    mStepperParam1 = 5;
    mStepperParam2 = 5;
    mStepperParam3 = 5;
    mStepperParam4 = 5;
    VxFrame* frame = VxFrame::instance();
    mTimeStep = 0.02;

    // the world (its pools are the frame's; MdtBodyCreateFromPool / the constraint ctors)
    mWorld = MdtWorldCreate(1, 1);
    mWorld->bodyDisabledCallback = &VxUniverse::bodyDisabledCallback;
    mWorld->bodyEnabledCallback = &VxUniverse::bodyEnabledCallback;
    MdtWorldSetPartitionOptimize(mWorld, 1);
    MdtGrouping* g = MdtWorldGetGrouping(mWorld, 0);
    memcpy(&g->bclParams, &mWorld->bclParams, sizeof(MdtBclSolverParameters));
    memcpy(&g->keaParams, &mWorld->keaParams, sizeof(MdtKeaParameters));
    setMaxPartitionsPerUnit(8);

    // the space
    m58 = (maxPartCount + 1) / 2 * 2;
    mSpace = (McdSpaceID)VincSpaceAxisSortCreate(kSpaceAxisMask, m58, maxPairCount, mFlag);
    setAutoDisable(true);
    mStepping = false;

    // the bridge with the response table of bodies / sensors / particle systems
    mIntersectManager = new VxBridge(frame->mMaterialTable, (CxSpace*)mSpace, mWorld, 0x401);
    mIntersectManager->setPairContainerSize(maxPairCount);
    VxResponseEntry e;
    e.mHello = new VxResponseStartBody_Body;
    e.mActive = new VxResponseBody_Body;
    e.mGoodbye = new VxResponseEndBody_Body;
    mIntersectManager->setResponseEntry(0, 0, e);
    e.mHello = new VxResponseStart_Sensor_Body;
    e.mActive = 0;
    e.mGoodbye = 0;
    mIntersectManager->setResponseEntry(1, 0, e);
    e.mHello = new VxResponseStart_Sensor_Sensor;
    mIntersectManager->setResponseEntry(1, 1, e);
    e.mHello = new VxResponseStart_Sensor_PS;
    mIntersectManager->setResponseEntry(2, 1, e);
    mIntersectManager->getResponseEntry(0, 1).mRequest.setContactMaxCount(0);
    mIntersectManager->getResponseEntry(1, 1).mRequest.setContactMaxCount(0);
}

// Mac 0x482644, Win 0x787720
void VxUniverse::addUpdateSubscriber(UpdateSubscriber* s)
{
    m44.push_back(s);
}

// Mac 0x482722, Win 0x70af80
void VxUniverse::registerFluidInteraction(VxCollisionGeometry* cg, bool add)
{
    if (m78 == 0) {
        if (!cg->hasFluidInteractionForceFn())
            return;
        m78 = new VxFluidInteractionUniverseUpdate();
        addUpdateSubscriber(m78);
    }
    VxFluidInteractionUniverseUpdate* fluid = static_cast<VxFluidInteractionUniverseUpdate*>(m78);
    if (!add)
        fluid->removeCollisionGeometry(cg);
    else if (cg->hasFluidInteractionForceFn())
        fluid->addCollisionGeometry(cg);
}

// Mac 0x4827ee, Win 0x70ac10
void VxUniverse::updateIntersections(bool notify)
{
    VxArray<VxPart*> woken;
    woken.reserve(100);
    DynamicPartIterator i = dynamicPartBegin();
    DynamicPartIterator end = dynamicPartEnd();
    while (i != end) {
        if (!(*i)->isEnabled()) {
            woken.push_back(*i);
            (*i)->enableDynamics(true);
        }
        i++;
    }
    VincSpacePathUpdate((VincSpaceID)mSpace, 0);
    if (!notify)
        mIntersectManager->setNotifyEnabled(false);
    mIntersectManager->update(0);
    mIntersectManager->updatePairs();
    if (!notify)
        mIntersectManager->setNotifyEnabled(true);
    for (VxPart** p = woken.begin(); p != woken.end(); ++p)
        (*p)->enableDynamics(false);
    for (UpdateSubscriber** s = m44.begin(); s != m44.end(); ++s)
        (*s)->intersectionsUpdated();
}

// D2 Mac 0x484620 (D1 0x4848e6, D0 0x484b9a), Win 0x70ae40 (sdd 0x70d640)
VxUniverse::~VxUniverse()
{
    UniverseConnector::disconnectAssembly(mRootMechanism);
    if (mRootMechanism != 0)
        delete mRootMechanism;
    delete m88;
    mIntersectManager->removeAllSubscribers();
    if (mInternalIDSubscriber != 0)
        delete mInternalIDSubscriber;
    if (m84 != 0)
        delete m84;
    if (mIntersectManager != 0)
        delete mIntersectManager;
    MdtWorldDestroy(mWorld);
    VincSpaceDestroy((VincSpaceID)mSpace);
    if (m78 != 0)
        delete m78;
}

// C2 Mac 0x4848fa (C1 0x484b72), Win 0x70b0d0
VxUniverse::VxUniverse(int maxPartCount, int maxPairCount, bool flag)
    : VxStepperBase(), VxEntityConstraintCollection<VxUniverse>(), mRootMechanism(new VxMechanism()), m44(),
      mStepping(false), m58(0), mWorld(0), mSpace(0), mIntersectManager(0), mTime1(0), mTime2(0), m78(0), mFlag(flag),
      mInternalIDSubscriber(0), m84(0), m88(new VxDisabledPairs()), m8c(false)
{
    mRootMechanism->setName("rootMechanism");
    UniverseConnector::connectAssembly(this, mRootMechanism);
    initialize(maxPartCount, maxPairCount);
}

// Mac 0x484e6c, Win 0x70a650
void VxUniverse::insertCollisionModel(McdModelID model)
{
    if (mStepping)
        VxWarning(0, "VxUniverse insert collision geometry: cannot be called while stepping (check intersect subscribers).\n");
    if (VincSpaceInsertModel((VincSpaceID)mSpace, model) == 0)
        return;
    VxCollisionGeometry* cg = (VxCollisionGeometry*)VincModelGetUserData(model);
    if (cg != 0)
        notifyCollection(&VxEntityConstraintCollection<VxUniverse>::Subscriber::notifyCollisionGeometry, this, kCollisionGeometryAdded, cg);
}

// Mac 0x4850e2, Win 0x70a6b0
void VxUniverse::removeCollisionModel(McdModelID model)
{
    if (mStepping)
        VxWarning(0, "VxUniverse remove collision geometry: cannot be called while stepping (check intersect subscribers).\n");
    if (VincSpaceRemoveModel(model) == 0)
        return;
    mIntersectManager->updatePairs();
    if (VincModelGetUserData(model) != 0) {
        VxCollisionGeometry* cg = (VxCollisionGeometry*)VincModelGetUserData(model);
        notifyCollection(&VxEntityConstraintCollection<VxUniverse>::Subscriber::notifyCollisionGeometry, this, kCollisionGeometryRemoved, cg);
    }
}

// --- Win-only API ------------------------------------------------------------------------------

// Win 0x708b80
void VxUniverse::setContactBufferSize(int contactCount, int bufferCount)
{
    if (!mStepping && contactCount != 0 && bufferCount != 0) {
        mIntersectManager->setContactBufferSize(contactCount, bufferCount);
        return;
    }
    VxInfo(0, "VxUniverse::setContactBufferSize: this should not be called during an update step with positive values.\n");
}

// Win 0x708bc0
void VxUniverse::getContactBufferSize(int& contactCount, int& bufferCount) const
{
    contactCount = mIntersectManager->getContactBufferSize();
    bufferCount = mIntersectManager->getContactBufferCount();
}

// Win 0x708760
VxDynamicsContact* VxUniverse::createUserDynamicsContact(VxPart* p0, VxPart* p1, VxContactMaterial* material,
                                                         const VxVector3& position, const VxVector3& normal, VxReal penetration)
{
    VxAssert(p0 != 0 || p1 != 0, "VxUniverse::createUserDynamicsContact: at least one input parts must be added\n");
    int dynamic = 0;
    if (p0 != 0 && p0->mInternalControl == VxEntity::kControlDynamic && p0->mUniverse == this)
        dynamic = 1;
    if (p1 != 0 && p1->mInternalControl == VxEntity::kControlDynamic && p1->mUniverse == this)
        ++dynamic;
    if (dynamic == 0) {
        VxWarning(0, "VxUniverse::createUserDynamicsContact: need at least one dynamic, non-frozen part for the contact to be valid.\n");
        return 0;
    }
    if (material == 0)
        material = VxFrame::instance()->mMaterialTable->getContactMaterial(0, 0);
    MdtContactID c = mIntersectManager->createUserContact(p0, p1, material, position, normal, penetration);
    VxAssert(c != 0, "VxUniverse::createUserDynamicsContact : contact could not be created\n");
    return (VxDynamicsContact*)c;
}

// Win 0x708830
void VxUniverse::deleteUserDynamicsContact(VxDynamicsContact* c)
{
    VxAssert(c != 0, "VxUniverse::deleteUserDynamicsContact: need a valid contact.\n");
    mIntersectManager->destroyUserContact((MdtContactID)c);
}

} // namespace Vx
