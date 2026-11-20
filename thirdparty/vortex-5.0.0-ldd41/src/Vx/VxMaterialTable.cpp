// Vortex 5.0.0 build 800 "vortexldd41" - the material table and the default material combination
// (restored). Mac 0x461246-0x463c0a (inside TU 504), Win 0x73ea10-0x73ec20
// (+ inlined copies). File, class and function names inferred.
#include "Vx/VxContactMaterial.h"
#include "Vx/VxUniqueNameList.h"
#include <string>

namespace Vx {

namespace {
// Mac 0x6440ac: priority of a friction model when two materials disagree (higher wins)
const int kFrictionModelPriority[7] = { 1, 3, 5, 4, 2, 0, 6 };

// Mac 0x4612b6 / 0x4612bf (maxsd / minsd: the second operand when unordered)
inline VxReal Max(VxReal a, VxReal b)
{
    return a > b ? a : b;
}
inline VxReal Min(VxReal a, VxReal b)
{
    return a < b ? a : b;
}
} // namespace

VxCombineMaterialsFn gVxCombineMaterials = vxDefaultCombineMaterials; // Mac 0x779390

// Mac 0x461288
void vxCombineMaterials(const VxMaterial* m0, const VxMaterial* m1, VxContactMaterial* cm)
{
    gVxCombineMaterials(m0, m1, cm);
}

// Mac 0x4613d0
void vxDefaultCombineMaterials(const VxMaterial* m0, const VxMaterial* m1, VxContactMaterial* cm)
{
    const VxMaterial* stiff = m0->mCompliance > m1->mCompliance ? m0 : m1;
    cm->setRestitution(stiff->mRestitution);
    cm->setRestitutionThreshold(stiff->mRestitutionThreshold);
    cm->setCompliance(stiff->mCompliance);
    cm->setDamping(stiff->mDamping);
    cm->setAdhesiveForce(Max(m0->mAdhesiveForce, m1->mAdhesiveForce));
    int isd = m0->mIntegratedSlipDisplacement <= m1->mIntegratedSlipDisplacement
                  ? m1->mIntegratedSlipDisplacement
                  : m0->mIntegratedSlipDisplacement;
    if (isd == VxMaterial::kIntegratedSlipDisplacementActivated) {
        cm->mIntegratedSlipDisplacement = true;
        cm->parametersChanged();
    } else if (isd >= 0 && isd <= VxMaterial::kIntegratedSlipDisplacementNever) {
        cm->mIntegratedSlipDisplacement = false;
        cm->parametersChanged();
    }
    for (int a = 0; a != VxMaterialBase::kFrictionAxisNumber; ++a) {
        const int model0 = m0->mFrictionModel[a];
        const int model1 = m1->mFrictionModel[a];
        if (model0 == model1) {
            cm->setFrictionModel(a, model0);
            cm->setFrictionCoefficient(a, Min(m0->mFrictionCoefficient[a], m1->mFrictionCoefficient[a]));
            cm->setBoxFrictionForce(a, Min(m0->mBoxFrictionForce[a], m1->mBoxFrictionForce[a]));
            cm->setSlip(a, Max(m0->mSlip[a], m1->mSlip[a]));
            cm->setStaticFrictionScale(a, Min(m0->mStaticFrictionScale[a], m1->mStaticFrictionScale[a]));
        } else {
            const VxMaterial* w = kFrictionModelPriority[model0] <= kFrictionModelPriority[model1] ? m1 : m0;
            cm->setFrictionModel(a, w->mFrictionModel[a]);
            cm->setFrictionCoefficient(a, w->mFrictionCoefficient[a]);
            cm->setBoxFrictionForce(a, w->mBoxFrictionForce[a]);
            cm->setSlip(a, w->mSlip[a]);
            cm->setStaticFrictionScale(a, w->mStaticFrictionScale[a]);
        }
        cm->setSlide(a, Max(m0->mSlide[a], m1->mSlide[a]));
    }
}

// C2 Mac 0x463152 (C1 0x463358)
VxMaterialTable::VxMaterialTable()
{
    mContactMaterials.mSize = 1;
    mContactMaterials.mData = (VxContactMaterial**)vxAlloc(sizeof(VxContactMaterial*));
    mContactMaterials.mData[0] = 0;
    reset(true);
}

// D2 Mac 0x462ff6 (D1 0x46313e)
VxMaterialTable::~VxMaterialTable()
{
    reset(false);
    if (mContactMaterials.mData)
        vxFree(mContactMaterials.mData);
}

// Mac 0x461256
VxMaterial* VxMaterialTable::getMaterial(int index)
{
    if (index > -1 && index < getMaterialCount())
        return mMaterials.begin()[index];
    return 0;
}

// Mac 0x4612c8
VxContactMaterial* VxMaterialTable::getContactMaterial(const VxMaterial* m0, const VxMaterial* m1)
{
    // NOTE: like the original, material 0 is read even when the table is empty
    if (m0 == 0)
        m0 = mMaterials.begin()[0];
    if (m1 == 0)
        m1 = mMaterials.begin()[0];
    if (m0 == 0 || m1 == 0)
        return 0;
    const int i = m0->mIndex, j = m1->mIndex, n = getMaterialCount();
    if (i < 0 || i >= n || j < 0 || j >= n)
        return 0;
    return mContactMaterials.getEntry(i, j);
}

// Mac 0x462dc6
void VxMaterialTable::reset(bool createDefault)
{
    for (VxMaterial** m = mDuplicates.begin(); m != mDuplicates.end(); ++m)
        delete *m;
    mDuplicates.clear();
    for (VxMaterial** m = mMaterials.begin(); m != mMaterials.end(); ++m)
        delete *m;
    mMaterials.clear();
    for (int i = 0; i <= (int)mContactMaterials.mSize - 1; ++i)
        for (int j = i; j <= (int)mContactMaterials.mSize - 1; ++j)
            delete mContactMaterials.getEntry(i, j);
    if (mContactMaterials.mData)
        vxFree(mContactMaterials.mData);
    mContactMaterials.mSize = 0;
    mContactMaterials.mData = 0;
    for (VxTireModelRef** t = mTireModels.begin(); t != mTireModels.end(); ++t)
        delete *t;
    mTireModels.clear();
    if (createDefault)
        getMaterial("default");
}

// Mac 0x462af8
VxMaterial* VxMaterialTable::getMaterial(const char* name)
{
    std::string n(name);
    for (VxMaterial** m = mMaterials.begin(); m != mMaterials.end(); ++m)
        if (n.compare((*m)->getName()) == 0)
            return *m;
    VxMaterial* m = new VxMaterial;
    if (!mMaterials.empty())
        *m = *mMaterials.begin()[0];
    m->setName(name);
    return registerMaterial(m);
}

// Mac 0x462546
VxMaterial* VxMaterialTable::registerMaterial(VxMaterial* m)
{
    if (m == 0)
        return 0;
    for (VxMaterial** e = mMaterials.begin(); e != mMaterials.end(); ++e) {
        if (*e == m)
            return m;
        if ((*e)->isEqual(*m)) {
            mDuplicates.pushBackUnique(m);
            m->mIndex = (*e)->mIndex;
            return *e;
        }
    }
    VxUniqueNameList names;
    for (VxMaterial** e = mMaterials.begin(); e != mMaterials.end(); ++e)
        names.add(std::string((*e)->getName()));
    std::string unique(names.addUnique(std::string(m->getName())));
    if (unique.compare(m->getName()) != 0)
        m->setName(unique.c_str());
    m->mIndex = mMaterials.size();
    mMaterials.push_back(m);
    setMaterialCount(mMaterials.size());
    return m;
}

// Mac 0x461c10, Win 0x73ec20
void VxMaterialTable::setMaterialCount(int n)
{
    if (n <= 0)
        return;
    if (mMaterials.size() < n)
        mMaterials.resize(n, 0);
    VxUniqueNameList names;
    for (int i = 0; i != n; ++i) {
        VxMaterial* m = mMaterials.begin()[i];
        if (m == 0) {
            m = new VxMaterial;
            m->setName(names.addUnique(std::string("Material")).c_str());
            m->mIndex = i;
            mMaterials.begin()[i] = m;
        } else {
            names.add(std::string(m->getName()));
        }
    }
    const unsigned int last = (unsigned int)(n - 1);
    if (mContactMaterials.mSize == 0 || mContactMaterials.mSize <= last) {
        if (mContactMaterials.mSize <= last)
            mContactMaterials.resize(n);
        mContactMaterials.mData[mContactMaterials.index(last, last)] = 0;
    }
    for (int i = 0; i != n; ++i) {
        for (int j = i; j < n; ++j) {
            if (mContactMaterials.getEntry(i, j) == 0) {
                VxMaterial* a = mMaterials.begin()[i];
                VxMaterial* b = mMaterials.begin()[j];
                VxContactMaterial* cm = new VxContactMaterial(a, b);
                a->addContactMaterial(cm);
                if (i != j)
                    b->addContactMaterial(cm);
                if ((unsigned int)i < mContactMaterials.mSize && (unsigned int)j < mContactMaterials.mSize)
                    mContactMaterials.mData[mContactMaterials.index(i, j)] = cm;
            }
        }
    }
}

// Mac 0x46190a, Win 0x73ea10
void VxMaterialTable::print(const char* fileName)
{
    VxPrintFile out(fileName);
    FILE* f = out.getFile();
    if (f == 0)
        return;
    fputs("====== Beginning the material table ========= \n\n", f);
    fprintf(f, "Number of registred material %d\n\n", getMaterialCount());
    for (int i = 0; i < getMaterialCount(); ++i) {
        fprintf(f, "Name: %30s\t\t ID = %5d\n\n", mMaterials.begin()[i]->getName(), i);
        getMaterial(i)->print(fileName);
    }
    fputs("\n\n", f);
    fputs("====== Material table Entries ========= \n\n", f);
    fputs("\n\n", f);
    for (int i = 0; i <= (int)mContactMaterials.mSize - 1; ++i) {
        for (int j = i; j <= (int)mContactMaterials.mSize - 1; ++j) {
            VxContactMaterial* cm = mContactMaterials.getEntry(i, j);
            const char* nj = getMaterial(j)->getName();
            const char* ni = getMaterial(i)->getName();
            fprintf(f, "\nentry (%3d, %3d)             %-12s %-12s:\n", i, j, ni, nj);
            cm->print(fileName);
        }
    }
}

} // namespace Vx
