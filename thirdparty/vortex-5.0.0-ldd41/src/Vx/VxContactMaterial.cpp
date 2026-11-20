// Vortex 5.0.0 build 800 "vortexldd41" - VxContactMaterial, VxTable1D (restored).
// Mac TU 474 0x441014-0x4424a9 (static initialiser 0x442444: <iostream>), Win
// 0x7a4690-0x7a5460. The TU also holds out-of-line copies of four VxConvexMesh-style inline
// forwarders to Vinc convex functions (Mac 0x4423c0-0x442420; their class is dead-stripped on Mac).
#include "Vx/VxContactMaterial.h"
#include <string>
#include <iostream>

namespace Vx {

// --------------------------------------------------------------------------------------
// D1 Mac 0x441e8c / D0 0x441dde, Win 0x7a5150 (D2 0x7a50e0)
VxTable1D::~VxTable1D()
{
}

// Mac 0x441d96, Win 0x7a5300
void VxTable1D::clear()
{
    mCount = 0;
    mX.clear();
    mY.clear();
}

// Mac 0x441f32, Win 0x7a5360
void VxTable1D::set(int n, const VxReal* x, const VxReal* y)
{
    mCount = n;
    mX.clear();
    mX.insert(mX.end(), x, x + n);
    mY.clear();
    mY.insert(mY.end(), y, y + n);
}

// Win 0x7a51a0 (per array, inlined twice)
bool VxTable1D::operator==(const VxTable1D& t) const
{
    if (mX.size() != t.mX.size() || mY.size() != t.mY.size())
        return false;
    for (int i = 0; i < mX.size(); ++i)
        if (!(t.mX.begin()[i] == mX.begin()[i]))
            return false;
    for (int i = 0; i < mY.size(); ++i)
        if (!(t.mY.begin()[i] == mY.begin()[i]))
            return false;
    return true;
}

// --------------------------------------------------------------------------------------
// C2 Mac 0x441168 (C1 0x441240), Win 0x7a4790
VxContactMaterial::VxContactMaterial()
    : mLocked(false), mAutoLock(true), mIntegratedSlipDisplacement(false)
{
    mMaterial[0] = 0;
    mMaterial[1] = 0;
}

// C2 Mac 0x4419d0 (C1 0x441b78)
VxContactMaterial::VxContactMaterial(VxMaterial* m0, VxMaterial* m1)
    : mLocked(false), mAutoLock(true), mIntegratedSlipDisplacement(false)
{
    mMaterial[0] = m0;
    mMaterial[1] = m1;
    update(false);
}

// D2 Mac 0x4417fa (D1 0x4419bc, D0 0x441b9a / Win 0x7a5460): the tire model reference deletes an
// owned model
VxContactMaterial::~VxContactMaterial()
{
}

// Mac 0x441d62, Win 0x7a5190
const char* VxContactMaterial::getClassName() const
{
    return "VxContactMaterial";
}

// Mac 0x441452, Win 0x7a4ee0
void VxContactMaterial::parametersChanged()
{
    if (mAutoLock)
        setLocked(true);
}

// Mac 0x441414
void VxContactMaterial::setLocked(bool locked)
{
    if (mLocked && !locked)
        update(true);
    mLocked = locked;
}

// Mac 0x441254
void VxContactMaterial::update(bool force)
{
    if (mMaterial[0] == 0 || mMaterial[1] == 0 || (mLocked && !force))
        return;
    bool autoLock = mAutoLock;
    mAutoLock = false;
    std::string name(mMaterial[0]->getName());
    name += '/';
    name.append(mMaterial[1]->getName());
    setName(name.c_str());
    vxCombineMaterials(mMaterial[0], mMaterial[1], this);
    mAutoLock = autoLock;
}

// Mac 0x441476, Win 0x7a4af0
VxContactMaterial& VxContactMaterial::operator=(const VxContactMaterial& m)
{
    if (this != &m) {
        copyParameters(m);
        mLocked = true;
        mIntegratedSlipDisplacement = m.mIntegratedSlipDisplacement;
        mTableParameters.mType = m.mTableParameters.mType;
        mTableParameters.mMin = m.mTableParameters.mMin;
        mTableParameters.mMax = m.mTableParameters.mMax;
        mTableParameters.mTable = m.mTableParameters.mTable;
        if (mTireModel.mOwned && mTireModel.mObject)
            delete mTireModel.mObject;
        mTireModel.mObject = m.mTireModel.mObject;
        mTireModel.mOwned = false;
    }
    return *this;
}

// Mac 0x441014, Win 0x7a4690
void VxContactMaterial::print(const char* fileName) const
{
    VxPrintFile out(fileName);
    VxMaterialBase::print(fileName);
    FILE* f = out.getFile();
    fprintf(f, "This material is : %s\n", mLocked ? "Locked" : "Not Locked");
    fprintf(f, "This material is : %s\n", mAutoLock ? "Automat Locking" : "Not Automat Locking");
    fprintf(f, "Integrated Slip Displacement : %s\n", mIntegratedSlipDisplacement ? "Activated" : "Not Activated");
    if (mTireModel.mObject == 0)
        fputs("This material doesn't have a tire model\n", f);
    else
        fputs("This material has a tire model\n", f);
    fputc('\n', f);
}

} // namespace Vx
