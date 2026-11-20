// Vortex 5.0.0 build 800 "vortexldd41" - VxMaterialBase (restored).
// Mac 0x4605b0-0x461246 (inside TU 504, no static initialiser of its own), Win 0x7a3af0-0x7a4820.
#include "Vx/VxMaterial.h"
#include <string>

namespace Vx {

// The print helper's globals (Mac 0x77cdc8 / 0x77f71c; its inline functions are emitted in
// Mac TU 453); placed here by choice
int VxPrintFile::sRefCount = 0;
FILE* VxPrintFile::sFile = 0;

// Mac 0x460c9a, Win 0x7a4080
VxMaterialBase::VxMaterialBase()
    : m18(0)
{
    VxBase::setName("material");
    VxMaterialBase::reset();
}

// D2 Mac 0x460c5c, Win 0x7a4820 (D1 Mac 0x460c86, D0 Mac 0x460c24 / Win 0x7a4560)
VxMaterialBase::~VxMaterialBase()
{
}

// Mac 0x4605b0, Win 0x7a3af0
void VxMaterialBase::reset()
{
    for (int i = 0; i != kFrictionAxisNumber; ++i) {
        mFrictionModel[i] = kFrictionModelNone;
        mBoxFrictionForce[i] = 0;
        mSlip[i] = 0;
        mSlide[i] = 0;
        mFrictionCoefficient[i] = 0;
        mStaticFrictionScale[i] = 1.0;
    }
    mOptions = 0;
    mRestitution = 0;
    mRestitutionThreshold = 0.001;
    mCompliance = 0;
    mDamping = 0;
    mAdhesiveForce = 0;
}

// Mac 0x4604f4
void VxMaterialBase::copyParameters(const VxMaterialBase& m)
{
    if (this == &m)
        return;
    for (int i = 0; i != kFrictionAxisNumber; ++i) {
        mFrictionModel[i] = m.mFrictionModel[i];
        mBoxFrictionForce[i] = m.mBoxFrictionForce[i];
        mSlip[i] = m.mSlip[i];
        mSlide[i] = m.mSlide[i];
        mFrictionCoefficient[i] = m.mFrictionCoefficient[i];
        mStaticFrictionScale[i] = m.mStaticFrictionScale[i];
    }
    mOptions = m.mOptions;
    mRestitution = m.mRestitution;
    mRestitutionThreshold = m.mRestitutionThreshold;
    mCompliance = m.mCompliance;
    mDamping = m.mDamping;
    mAdhesiveForce = m.mAdhesiveForce;
    m18 = m.m18;
}

// Mac 0x460d81, Win 0x7a4160
bool VxMaterialBase::isEqual(const VxMaterialBase& m) const
{
    std::string other(m.getName());
    std::string mine(getName());
    if (mine != other)
        return false;
    if (!(mRestitution == m.mRestitution && mRestitutionThreshold == m.mRestitutionThreshold
          && mCompliance == m.mCompliance && mDamping == m.mDamping
          && mAdhesiveForce == m.mAdhesiveForce && mOptions == m.mOptions
          && mFrictionModel[0] == m.mFrictionModel[0]))
        return false;
    for (int i = 0; i != kFrictionAxisNumber; ++i) {
        if (!(mBoxFrictionForce[i] == m.mBoxFrictionForce[i] && mSlip[i] == m.mSlip[i]
              && mSlide[i] == m.mSlide[i] && mFrictionCoefficient[i] == m.mFrictionCoefficient[i]
              && mStaticFrictionScale[i] == m.mStaticFrictionScale[i]))
            return false;
        if (i + 1 < kFrictionAxisNumber && mFrictionModel[i + 1] != m.mFrictionModel[i + 1])
            return false;
    }
    return true;
}

// Mac 0x46068a
void VxMaterialBase::setFrictionModel(int axis, int model)
{
    if (axis == kFrictionAxisLinear) {
        mFrictionModel[0] = model;
        mFrictionModel[1] = model;
    } else {
        mFrictionModel[axis] = model;
    }
    parametersChanged();
}

#define VX_AXIS_SETTER(fn, member)              \
    void VxMaterialBase::fn(int axis, VxReal v) \
    {                                           \
        if (axis == kFrictionAxisLinear) {      \
            member[0] = v;                      \
            member[1] = v;                      \
        } else {                                \
            member[axis] = v;                   \
        }                                       \
        parametersChanged();                    \
    }

VX_AXIS_SETTER(setBoxFrictionForce, mBoxFrictionForce)        // Mac 0x4607a4
VX_AXIS_SETTER(setSlip, mSlip)                                // Mac 0x4606b4
VX_AXIS_SETTER(setSlide, mSlide)                              // Mac 0x4606f0
VX_AXIS_SETTER(setFrictionCoefficient, mFrictionCoefficient)  // Mac 0x460768
VX_AXIS_SETTER(setStaticFrictionScale, mStaticFrictionScale)  // Mac 0x46072c
#undef VX_AXIS_SETTER

// Mac 0x46064e
void VxMaterialBase::setRestitution(VxReal r)
{
    mRestitution = r;
    parametersChanged();
}

// Mac 0x46066c
void VxMaterialBase::setRestitutionThreshold(VxReal v)
{
    mRestitutionThreshold = v;
    parametersChanged();
}

// Mac 0x460630
void VxMaterialBase::setAdhesiveForce(VxReal f)
{
    mAdhesiveForce = f;
    parametersChanged();
}

// Mac 0x460cfe
void VxMaterialBase::setCompliance(VxReal c)
{
    mCompliance = c;
    if (c != 0.0 || mDamping != 0.0)
        mOptions |= kOptionCompliance;
    else
        mOptions &= ~(unsigned int)kOptionCompliance;
    parametersChanged();
}

// Mac 0x460d40
void VxMaterialBase::setDamping(VxReal d)
{
    mDamping = d;
    if (mCompliance != 0.0 || d != 0.0)
        mOptions |= kOptionCompliance;
    else
        mOptions &= ~(unsigned int)kOptionCompliance;
    parametersChanged();
}

// Mac 0x4607d6, Win 0x7a3e40
void VxMaterialBase::print(const char* fileName) const
{
    static const char* const kAxisNames[kFrictionAxisNumber] = {
        "Axis kFrictionAxisLinearPrimary: ", "Axis kFrictionAxisLinearSecondary: ",
        "Axis kFrictionAxisAngularNormal: ", "Axis kFrictionAxisAngularPrimary: ",
        "Axis kFrictionAxisAngularSecondary: "
    };
    VxPrintFile out(fileName);
    FILE* f = out.getFile();
    for (int i = 0; i != kFrictionAxisNumber; ++i) {
        const unsigned int model = (unsigned int)mFrictionModel[i];
        if (model > kFrictionModelScaledBoxFast)
            continue;
        fputs(kAxisNames[i], f);
        switch (model) {
        case kFrictionModelBox:
            fputs("Friction Model Box\n", f);
            fprintf(f, "   max force            %16.6g\n", mBoxFrictionForce[i]);
            break;
        case kFrictionModelBoxProportionalHigh:
            fputs("Friction Model Box proportional Hi\n", f);
            fprintf(f, "   max accel            %16.6g\n", mBoxFrictionForce[i]);
            break;
        case kFrictionModelBoxProportionalLow:
            fputs("Friction Model Box proportional Low\n", f);
            fprintf(f, "   max accel            %16.6g\n", mBoxFrictionForce[i]);
            break;
        case kFrictionModelScaledBox:
            fputs("Friction Model Scaled Box\n", f);
            fprintf(f, "   coefficient           %16.6g\n", mFrictionCoefficient[i]);
            break;
        case kFrictionModelScaledBoxFast:
            fputs("Friction Model Scaled Box Fast\n", f);
            fprintf(f, "   coefficient           %16.6g\n", mFrictionCoefficient[i]);
            break;
        }
        fprintf(f, "   slip                   %16.6g\n", mSlip[i]);
        fprintf(f, "   slide                  %16.6g\n", mSlide[i]);
        fprintf(f, "   static friction scale  %16.6g\n", mStaticFrictionScale[i]);
    }
    fprintf(f, "restitution               %16.6g\n", mRestitution);
    fprintf(f, "velThreshold              %16.6g\n", mRestitutionThreshold);
    fprintf(f, "compliance                %16.6g\n", mCompliance);
    fprintf(f, "damping                   %16.6g\n", mDamping);
    fprintf(f, "max_adhesive_force        %16.6g\n", mAdhesiveForce);
    fprintf(f, "options                   %16d\n", mOptions);
}

} // namespace Vx
