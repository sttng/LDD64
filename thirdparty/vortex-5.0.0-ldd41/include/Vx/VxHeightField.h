// Vortex 5.0.0 build 800 "vortexldd41" - VxHeightField (restored header,
// src/Vx/VxHeightField.cpp = Mac TU 493). Mac vtable 0x75dee8 (13 + 4 slots); class id 0x12f.
// Mac 0xb8 bytes (new(0xb8), LEGO VortexPhysicsGeometrySampledHeightField Mac 0x3a7080), Win 0xd8.
// A regular grid of (xCells + 1) x (yCells + 1) heights centred on the origin (Vinc "RGHeightField",
// type 8, V3.7). The glue fills mHeights directly (row-major, mXCount per row), then calls build().
// Win only (dead-stripped on the Mac, not reached by LDD, not restored): the deformable API
// (setHeights 0x7124e0, setUseExtendedBoundingBox / setSurfaceThickness 0x711d60 / 0x711dc0,
// normals 0x712d30 ..), the BMP constructor 0x713aa0 + CreateHeightMatrixFromBmp 0x713570,
// setDefaultPrimitiveBufferSize 0x711d00.
#ifndef VX_VXHEIGHTFIELD_H
#define VX_VXHEIGHTFIELD_H

#include "Vx/VxGeometry.h"

namespace Vx {

class VxHeightField : public VxGeometry {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x4594cf / Win 0x711cc0, delete Mac 0x45952c / Win 0x7125b0
    // getStaticClassType Mac 0x459446 (0x12f); types Mac 0x459900, Win 0x713910; isOfType slot 0
    // Mac 0x459c92, Win 0x713990; isOfClassType slot 1 Mac 0x45945a, Win 0x712090
    VX_DECLARE_CLASS_TYPES();
    // C1 Mac 0x459e16 -> C2 0x459cd8, Win 0x7139c0: zeroed members, then init(). The glue passes
    // (columns - 1, rows - 1, spacing x, spacing y, 0). (argument names inferred)
    VxHeightField(int xCells, int yCells, VxReal xSpacing, VxReal ySpacing, VxReal initialHeight);
    virtual ~VxHeightField();                           // slot 2/3 Mac 0x4595f2 (D2 0x459568)/0x459e62, Win 0x714180
    virtual const char* getClassName() const;           // slot 4 Mac 0x459eee, Win 0x713ec0
    // slot 7 Mac 0x459470, Win 0x711d40: returns this (shared); a deformable one warns
    // "VxHeightField::clone(): cloning of a deformable VxHeightField is not implemented, it
    // will be shared." NOTE: no addRef, as in the original.
    virtual VxGeometry* clone() const;

    // Mac 0x459606, Win 0x7132a0: origin (centred grid), counts, spacing; the height table is
    // cleared and refilled with (xCells + 1) * (yCells + 1) copies of initialHeight; a new Vinc
    // height field over it (VincRGHeightFieldCreate). Returns true. (name inferred)
    bool init(int xCells, int yCells, VxReal xSpacing, VxReal ySpacing, VxReal initialHeight);
    // Mac 0x459544, Win 0x711e50 (the name from the Win-only overload build(heights, ...) 0x712c60,
    // not restored): Vinc rebuild from mHeights, then the collision geometries using it are refreshed
    void build();

    VxArray<VxReal> mHeights;   // Mac +0x6c, Win +0x88 (the glue fills it, row stride mXCount)
    VxReal mOriginX;            // Mac +0x7c, Win +0x98: -xCells * xSpacing / 2 (name inferred)
    VxReal mOriginY;            // Mac +0x84, Win +0xa0: -yCells * ySpacing / 2 (name inferred)
    int mXCount;                // Mac +0x8c, Win +0xa8: xCells + 1 (row stride used by the glue) (name inferred)
    int mYCount;                // Mac +0x90, Win +0xac: yCells + 1 (name inferred)
    int mCount;                 // Mac +0x94, Win +0xb0: mXCount * mYCount (name inferred)
    int mXCells;                // Mac +0x98, Win +0xb4 (glue vf04: (mXCells, mYCells) = cell counts) (name inferred)
    int mYCells;                // Mac +0x9c, Win +0xb8 (name inferred)
    VxReal mXSpacing;           // Mac +0xa0, Win +0xc0 (name inferred)
    VxReal mYSpacing;           // Mac +0xa8, Win +0xc8 (name inferred)
    VxReal* mDeformation;       // Mac +0xb0, Win +0xd0: delete[]'d by the dtor; only the Win-only
                                // deformable API sets it (name inferred)
    bool mDeformable;           // Mac +0xb4, Win +0xd4 (clone() warning) (name inferred)
};

} // namespace Vx

#endif
