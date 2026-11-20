// Vortex 5.0.0 build 800 "vortexldd41" - VxHeightField (restored).
// Mac TU 493 0x459446-0x45a07b (file guess Vx/VxHeightField.cpp); Win 0x711cc0-0x714180.
// The TU also holds the out-of-line copies of VxGeometry's inline slots 10-12 (Mac
// 0x459ee8-0x459fec) and the VxArray<double> template code (0x459f00-0x459f8c). The Win-only
// deformable / BMP API of the class is not restored (see Vx/VxHeightField.h).
#include "Vx/VxHeightField.h"
#include "VxCollisionInternal.h"

namespace Vx {

// Mac 0x459446 (0x12f), types Mac 0x459900 / Win 0x713910, isOfType Mac 0x459c92 / Win 0x713990,
// isOfClassType Mac 0x45945a / Win 0x712090
VX_IMPLEMENT_CLASS_TYPES(VxHeightField, VxGeometry, 0x12f)

// operator new Mac 0x4594cf / Win 0x711cc0, delete Mac 0x45952c / Win 0x7125b0
VX_IMPLEMENT_ALLOCATOR(VxHeightField)

// C2 Mac 0x459cd8 (C1 0x459e16), Win 0x7139c0
VxHeightField::VxHeightField(int xCells, int yCells, VxReal xSpacing, VxReal ySpacing, VxReal initialHeight)
    : mOriginX(0)
    , mOriginY(0)
    , mXCount(0)
    , mYCount(0)
    , mCount(0)
    , mXCells(0)
    , mYCells(0)
    , mXSpacing(0)
    , mYSpacing(0)
    , mDeformation(0)
    , mDeformable(false)
{
    init(xCells, yCells, xSpacing, ySpacing, initialHeight);
}

// D2 Mac 0x459568 (D1 0x4595f2), D0 Mac 0x459e62, Win 0x714180
VxHeightField::~VxHeightField()
{
    delete[] mDeformation;
}

// Mac 0x459470, Win 0x711d40
VxGeometry* VxHeightField::clone() const
{
    if (mDeformable)
        VxWarning(0, "VxHeightField::clone(): cloning of a deformable VxHeightField is not implemented, it will be shared.\n");
    return const_cast<VxHeightField*>(this);
}

// Mac 0x459544, Win 0x711e50
void VxHeightField::build()
{
    VincRGHeightFieldUpdate(mGeometry);
    notifyCollisionGeometries();
}

// Mac 0x459606, Win 0x7132a0
bool VxHeightField::init(int xCells, int yCells, VxReal xSpacing, VxReal ySpacing, VxReal initialHeight)
{
    mOriginX = (VxReal)-xCells * xSpacing * 0.5;
    mOriginY = (VxReal)-yCells * ySpacing * 0.5;
    mXCount = xCells + 1;
    mYCount = yCells + 1;
    mCount = (yCells + 1) * mXCount;
    mXCells = xCells;
    mYCells = yCells;
    mXSpacing = xSpacing;
    mYSpacing = ySpacing;
    mHeights.clear();
    mHeights.resize(mCount, initialHeight);
    mGeometry = VincRGHeightFieldCreate(mHeights.begin(), mXCount, mYCount, xSpacing, ySpacing, mOriginX, mOriginY);
    return true;
}

// Mac 0x459eee, Win 0x713ec0
const char* VxHeightField::getClassName() const
{
    return "VxHeightField";
}

} // namespace Vx
