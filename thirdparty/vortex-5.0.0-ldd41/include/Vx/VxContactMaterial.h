// Vortex 5.0.0 build 800 "vortexldd41" - VxContactMaterial, VxTable1D (restored header,
// Mac TU 474 0x441014-0x4424a9, Win 0x7a4690-0x7a5460).
// The material of a contact pair (two VxMaterials); the material table owns one per pair. Every
// MdtContact embeds one (Mac +0xb78, Win +0xba0): Mdt calls its default ctor (placement, Mac
// 0x48f4c6 -> 0x441240), dtor and assignment (MdtContactSetContactMaterial Mac 0x48f48c ->
// 0x441476, Win 0x7a4af0).
#ifndef VX_VXCONTACTMATERIAL_H
#define VX_VXCONTACTMATERIAL_H

#include "Vx/VxMaterial.h"

namespace Vx {

// --------------------------------------------------------------------------------------
// VxTable1D (Mac vtable 0x75d9a0: D1, D0, clear, set; Win 0x9fff90). 0x28 bytes on both.
// A table of n (x, y) pairs. (method names inferred)
class VxTable1D {
public:
    VxTable1D() : mCount(0) {}
    virtual ~VxTable1D();                               // Mac 0x441e8c/0x441dde, Win 0x7a5150 (D2 0x7a50e0)
    virtual void clear();                               // slot 2 Mac 0x441d96, Win 0x7a5300
    virtual void set(int n, const VxReal* x, const VxReal* y); // slot 3 Mac 0x441f32, Win 0x7a5360
    // inline in the binaries (VxContactMaterial::operator= Mac 0x441476 / Win 0x7a53f0)
    VxTable1D& operator=(const VxTable1D& t)
    {
        if (this != &t) {
            mX.clear();
            mX.insert(mX.end(), t.mX.begin(), t.mX.end());
            mY.clear();
            mY.insert(mY.end(), t.mY.begin(), t.mY.end());
        }
        mCount = t.mCount;
        return *this;
    }
    bool operator==(const VxTable1D& t) const;         // Win 0x7a51a0 (per array) (Mac dead-stripped)

    VxArray<VxReal> mX;     // +0x04 (name inferred)
    VxArray<VxReal> mY;     // +0x14 (name inferred)
    int mCount;             // +0x24 (name inferred)

private:
    VxTable1D(const VxTable1D&);
};

// --------------------------------------------------------------------------------------
// VxContactMaterial (Mac vtable 0x75d968, 9 slots; Win 8). Mac 0x174 bytes, Win 0x198
// (= MdtContact::materialName - material offset; VxBase::operator new(0x174) Mac 0x461c10).
class VxContactMaterial : public VxMaterialBase {
public:
    // The tail parameter block (Mac +0x130, Win +0x150: 8-aligned): compared as one by Win
    // 0x7a53f0; its setters were dead-stripped. (names inferred)
    struct TableParameters {
        TableParameters() : mType(1), mMin(0), mMax(0) {}
        bool operator==(const TableParameters& p) const // Win 0x7a53f0
        {
            return mType == p.mType && p.mMin == mMin && p.mMax == mMax && mTable == p.mTable;
        }
        int mType;          // +0x00 (1)
        VxReal mMin;        // Mac +0x04, Win +0x08
        VxReal mMax;        // Mac +0x0c, Win +0x10
        VxTable1D mTable;   // Mac +0x14, Win +0x18
    };

    VxContactMaterial();                                // C1 Mac 0x441240 -> C2 0x441168, Win 0x7a4790
    // C1 Mac 0x441b78 -> C2 0x4419d0: named and computed from its materials (the table's ctor)
    VxContactMaterial(VxMaterial* m0, VxMaterial* m1);
    virtual ~VxContactMaterial();                       // slot 2/3 Mac 0x4419bc (D2 0x4417fa)/0x441b9a, Win 0x7a5460
    virtual const char* getClassName() const;           // slot 4 Mac 0x441d62, Win 0x7a5190
    virtual void parametersChanged();                   // slot 6 Mac 0x441452, Win 0x7a4ee0: auto lock
    virtual void print(const char* fileName) const;     // slot 8 Mac 0x441014, Win 0x7a4690
    // Mac 0x441476, Win 0x7a4af0: parameters, flags (locked) and tail; the tire model is shared
    // (not owned) afterwards
    VxContactMaterial& operator=(const VxContactMaterial& m);

    // Mac 0x441254: when both materials are set and the material is unlocked (or force): name
    // "<m0>/<m1>" and recompute through the combine function (auto lock off meanwhile)
    // (name inferred)
    void update(bool force);
    // Mac 0x441414: unlocking recomputes the material (name inferred)
    void setLocked(bool locked);
    bool isLocked() const { return mLocked; }
    VxMaterial* getMaterial(int i) const { return mMaterial[i]; }

    bool mLocked;                       // Mac +0x124, Win +0x140 ("Locked")
    bool mAutoLock;                     // Mac +0x125, Win +0x141 ("Automat Locking", true)
    bool mIntegratedSlipDisplacement;   // Mac +0x126, Win +0x142 (MdtBcl "sticky")
    VxMaterial* mMaterial[2];           // Mac +0x128, Win +0x144 (name inferred)
    TableParameters mTableParameters;   // Mac +0x130, Win +0x150 (name inferred)
    VxTireModelRef mTireModel;          // Mac +0x16c, Win +0x190 ("has a tire model")

private:
    VxContactMaterial(const VxContactMaterial&);
};

} // namespace Vx

#endif
