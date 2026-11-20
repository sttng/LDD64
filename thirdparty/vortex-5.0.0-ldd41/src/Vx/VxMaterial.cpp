// Vortex 5.0.0 build 800 "vortexldd41" - VxMaterial (restored).
// Mac 0x45ff0c-0x4605b0 (start of TU 504), Win 0x7aa150-0x7aa420.
#include "Vx/VxContactMaterial.h"

namespace Vx {

// C2 Mac 0x460194 (C1 0x4601f6)
VxMaterial::VxMaterial()
    : mIndex(-1), mIntegratedSlipDisplacement(kIntegratedSlipDisplacementNeutral)
{
}

// D2 Mac 0x460110 (D1 0x460180, D0 0x46020a / Win 0x7aa420): the contact materials are the
// table's
VxMaterial::~VxMaterial()
{
}

// Mac 0x460382, Win 0x7aa410
const char* VxMaterial::getClassName() const
{
    return "VxMaterial";
}

// Mac 0x4600a8, Win 0x7aa380
void VxMaterial::setName(const char* name)
{
    VxBase::setName(name);
    updateContactMaterials();
}

// Mac 0x460094, Win 0x7aa3a0
void VxMaterial::parametersChanged()
{
    updateContactMaterials();
}

// Mac 0x460044, Win 0x7aa320
void VxMaterial::updateContactMaterials()
{
    for (VxContactMaterial** cm = mContactMaterials.begin(); cm != mContactMaterials.end(); ++cm)
        if (!(*cm)->mLocked)
            (*cm)->update(false);
}

// Mac 0x4600d0, Win 0x7aa350
VxMaterial& VxMaterial::operator=(const VxMaterial& m)
{
    if (this != &m) {
        copyParameters(m);
        mIntegratedSlipDisplacement = m.mIntegratedSlipDisplacement;
        updateContactMaterials();
    }
    return *this;
}

// Mac 0x460280
void VxMaterial::addContactMaterial(VxContactMaterial* cm)
{
    mContactMaterials.push_back(cm);
}

// Mac 0x45ff0c, Win 0x7aa150
void VxMaterial::print(const char* fileName) const
{
    VxPrintFile out(fileName);
    FILE* f = out.getFile();
    VxMaterialBase::print(fileName);
    switch (mIntegratedSlipDisplacement) {
    case kIntegratedSlipDisplacementDeactivated:
        fputs("integrated displacement: \t Deactivated \n", f);
        break;
    case kIntegratedSlipDisplacementNeutral:
        fputs("integrated displacement: \t Neutral \n", f);
        break;
    case kIntegratedSlipDisplacementActivated:
        fputs("integrated displacement: \t Activated \n", f);
        break;
    case kIntegratedSlipDisplacementNever:
        fputs("integrated displacement: \t Never \n", f);
        break;
    }
    fputc('\n', f);
}

} // namespace Vx
