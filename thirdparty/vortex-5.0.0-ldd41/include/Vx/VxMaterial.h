// Vortex 5.0.0 build 800 "vortexldd41" - VxMaterialBase, VxMaterial, the material table
// (restored header).
//
// Sources (Mac TU 504 holds several files without static initialisers, lumped before
// VxMotorConstraint.cpp's initialiser):
//   src/Vx/VxMaterial.cpp       Mac 0x45ff0c-0x4605b0, Win 0x7aa150-0x7aa420
//   src/Vx/VxMaterialBase.cpp   Mac 0x4605b0-0x461246, Win 0x7a3af0-0x7a4820
//   src/Vx/VxMaterialTable.cpp  Mac 0x461246-0x463c0a, Win 0x73ea10-0x73ec20
// (Mac 0x463c0a-0x463d8a is src/Vx/VxMessage.cpp, V1.1.)
//
// A material holds the contact parameters of one surface; the material table (one per VxFrame,
// VxFrame::mMaterialTable Mac +0xd8 = new(0x38)) keeps the materials by index (VxCollisionGeometry's
// material id) and one VxContactMaterial per pair (i, j) in a symmetric table. A contact material
// is recomputed from its two materials (the combine function, default vxDefaultCombineMaterials)
// until it is locked: changing a contact material's parameters locks it (auto lock).
#ifndef VX_VXMATERIAL_H
#define VX_VXMATERIAL_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxMessage.h"
#include <stdio.h>

namespace Vx {

class VxMaterial;
class VxContactMaterial;

// --------------------------------------------------------------------------------------
// The print helper of the material dumps (Mac 0x422e09-0x422f3f: inline functions emitted in TU
// 453 from 0x422e20, 0x422e09 being TU 452's last function; Win 0x750680-0x750750): a
// reference-counted global output file - the first scope opens `fileName` ("w") or takes stderr
// when it is 0, the last one closes it (stderr is never closed). (names inferred)
class VxPrintFile {
public:
    explicit VxPrintFile(const char* fileName)          // Mac 0x422f26 -> 0x422efe -> 0x422eaa
    {
        if (sRefCount == 0)
            sFile = fileName ? fopen(fileName, "w") : stderr;
        ++sRefCount;
    }
    ~VxPrintFile()                                      // Mac 0x422e96 -> 0x422e74 -> 0x422e32
    {
        if (--sRefCount == 0) {
            if (sFile != stderr)
                fclose(sFile);
            sFile = 0;
        }
    }
    FILE* getFile() const { return sFile; }             // Mac 0x422e20

    static int sRefCount;                               // Mac 0x77cdc8
    static FILE* sFile;                                 // Mac 0x77f71c

private:
    VxPrintFile(const VxPrintFile&);
    VxPrintFile& operator=(const VxPrintFile&);
};

// --------------------------------------------------------------------------------------
// Win builds: the material data starts at +0x30 as in the MSVC 2008 binary (std::string 28 bytes)
// whatever the std::string of the build, so that a VxContactMaterial inside MdtContact keeps the
// original layout (MdtContact offsets, raw constraint images of the differential tests). Not
// original code: an empty base when VxBase already has the original size. (V4.2)
template <int N> struct VxLayoutPad { char mLayoutPad[N]; };
template <> struct VxLayoutPad<0> {};
#if defined(_WIN32)
enum { kVxMaterialBasePad = (int)sizeof(VxBase) < 0x30 ? 0x30 - (int)sizeof(VxBase) : 0 };
#else
enum { kVxMaterialBasePad = 0 };
#endif

// --------------------------------------------------------------------------------------
// VxMaterialBase (Mac vtable 0x75e1a8, 9 slots; Win 8). Mac 0x124 bytes, Win 0x140. Abstract
// (slots 4 and 6 pure). The data block from +0x18 / Win +0x30 is what MdtBcl reads from a contact's
// material (Mdt/MdtBclRows.h MdtBclContactMaterialData).
class VxMaterialBase : public VxBase, private VxLayoutPad<kVxMaterialBasePad> {
public:
    // friction axes (Mac strings "Axis kFrictionAxisLinearPrimary" ...); kFrictionAxisLinear = 6
    // addresses both linear axes (setters) / axis 0 (getFrictionModel)
    enum eFrictionAxis {
        kFrictionAxisLinearPrimary = 0,
        kFrictionAxisLinearSecondary = 1,
        kFrictionAxisAngularNormal = 2,
        kFrictionAxisAngularPrimary = 3,
        kFrictionAxisAngularSecondary = 4,
        kFrictionAxisNumber = 5,
        kFrictionAxisLinear = 6
    };
    // friction models (print jump table Mac 0x460944: "Friction Model Box", "Scaled Box", "Box
    // proportional Low", "Box proportional Hi", "Scaled Box Fast"); 6 = none (no friction row,
    // MdtBcl). 5 has no print entry. (names 5 / 6 inferred)
    enum eFrictionModel {
        kFrictionModelBox = 0,
        kFrictionModelScaledBox = 1,
        kFrictionModelBoxProportionalLow = 2,
        kFrictionModelBoxProportionalHigh = 3,
        kFrictionModelScaledBoxFast = 4,
        kFrictionModelNeutral = 5,
        kFrictionModelNone = 6
    };
    enum { kOptionCompliance = 4 }; // options bit set while compliance or damping != 0 (name inferred)

    VxMaterialBase();                                   // Mac 0x460c9a, Win 0x7a4080 (name "material", reset())
    virtual ~VxMaterialBase();                          // slot 2/3 Mac 0x460c86 (D2 0x460c5c)/0x460c24, Win 0x7a4560 (D2 0x7a4820)
    // slot 4 getClassName: pure
    // slot 5 setName: VxBase's
    // slot 6: called after every parameter change (VxMaterial: recompute its contact materials,
    // VxContactMaterial: auto lock) (name inferred)
    virtual void parametersChanged() = 0;
    virtual void reset();                               // slot 7 Mac 0x4605b0, Win 0x7a3af0: defaults (name inferred)
    virtual void print(const char* fileName) const;     // slot 8 Mac 0x4607d6, Win 0x7a3e40 (name inferred)

    // Mac 0x4604f4 (inline copy, also in VxMaterial / VxContactMaterial operator=): the parameter
    // block (not the name) (name inferred)
    void copyParameters(const VxMaterialBase& m);
    // Mac 0x460d81, Win 0x7a4160: same name and same parameters (name inferred)
    bool isEqual(const VxMaterialBase& m) const;

    // --- setters (each calls parametersChanged); axis 6 = both linear axes ---
    void setFrictionModel(int axis, int model);         // Mac 0x46068a (name inferred)
    void setBoxFrictionForce(int axis, VxReal f);       // Mac 0x4607a4 (name inferred)
    void setSlip(int axis, VxReal slip);                // Mac 0x4606b4 (name inferred)
    void setSlide(int axis, VxReal slide);              // Mac 0x4606f0 (name inferred)
    void setFrictionCoefficient(int axis, VxReal mu);   // Mac 0x460768 (name inferred)
    void setStaticFrictionScale(int axis, VxReal s);    // Mac 0x46072c (name inferred)
    void setRestitution(VxReal r);                      // Mac 0x46064e (name inferred)
    void setRestitutionThreshold(VxReal v);             // Mac 0x46066c (name inferred)
    void setAdhesiveForce(VxReal f);                    // Mac 0x460630 (name inferred)
    void setCompliance(VxReal c);                       // Mac 0x460cfe (also the options bit) (name inferred)
    void setDamping(VxReal d);                          // Mac 0x460d40 (also the options bit) (name inferred)

    // Out of line in the binaries at Mac 0x4a0e56 / Win 0x73e300 (inside MdtBclRowsUtil's TU; V2.2
    // restored it as MdtBclContactMaterialGetFrictionModel on the data block) (name inferred)
    int getFrictionModel(int axis) const { return axis == kFrictionAxisLinear ? mFrictionModel[0] : mFrictionModel[axis]; }
    VxReal getBoxFrictionForce(int axis) const { return mBoxFrictionForce[axis]; }
    VxReal getSlip(int axis) const { return mSlip[axis]; }
    VxReal getSlide(int axis) const { return mSlide[axis]; }
    VxReal getFrictionCoefficient(int axis) const { return mFrictionCoefficient[axis]; }
    VxReal getStaticFrictionScale(int axis) const { return mStaticFrictionScale[axis]; }
    VxReal getRestitution() const { return mRestitution; }
    VxReal getRestitutionThreshold() const { return mRestitutionThreshold; }
    VxReal getCompliance() const { return mCompliance; }
    VxReal getDamping() const { return mDamping; }
    VxReal getAdhesiveForce() const { return mAdhesiveForce; }
    unsigned int getOptions() const { return mOptions; }

    int m18;                            // Mac +0x18, Win +0x30 (0; copied, never read here) (name unknown)
    unsigned int mOptions;              // Mac +0x1c, Win +0x34 ("options")
    VxReal mRestitution;                // Mac +0x20, Win +0x38
    VxReal mRestitutionThreshold;       // Mac +0x28, Win +0x40 ("velThreshold", 0.001)
    VxReal mCompliance;                 // Mac +0x30, Win +0x48
    VxReal mDamping;                    // Mac +0x38, Win +0x50
    VxReal mAdhesiveForce;              // Mac +0x40, Win +0x58 ("max_adhesive_force")
    int mFrictionModel[5];              // Mac +0x48, Win +0x60 (6)
    VxReal mBoxFrictionForce[5];        // Mac +0x5c, Win +0x78 ("max force" / "max accel")
    VxReal mSlip[5];                    // Mac +0x84, Win +0xa0
    VxReal mSlide[5];                   // Mac +0xac, Win +0xc8
    VxReal mFrictionCoefficient[5];     // Mac +0xd4, Win +0xf0 ("coefficient")
    VxReal mStaticFrictionScale[5];     // Mac +0xfc, Win +0x118 (1.0)
};

// --------------------------------------------------------------------------------------
// VxMaterial (Mac vtable 0x75e168, 9 slots; Win 8). Mac 0x13c bytes (VxBase::operator new(0x13c)
// Mac 0x461c10), Win 0x158.
class VxMaterial : public VxMaterialBase {
public:
    // "integrated displacement" (print Mac 0x45ff0c) (names inferred)
    enum IntegratedSlipDisplacement {
        kIntegratedSlipDisplacementNeutral = 0,
        kIntegratedSlipDisplacementDeactivated = 1,
        kIntegratedSlipDisplacementActivated = 2,
        kIntegratedSlipDisplacementNever = 3
    };

    VxMaterial();                                       // C1 Mac 0x4601f6 -> C2 0x460194
    virtual ~VxMaterial();                              // slot 2/3 Mac 0x460180 (D2 0x460110)/0x46020a, Win 0x7aa420
    virtual const char* getClassName() const;           // slot 4 Mac 0x460382, Win 0x7aa410 "VxMaterial"
    virtual void setName(const char* name);             // slot 5 Mac 0x4600a8, Win 0x7aa380 (+ contact material names)
    virtual void parametersChanged();                   // slot 6 Mac 0x460094, Win 0x7aa3a0
    virtual void print(const char* fileName) const;     // slot 8 Mac 0x45ff0c, Win 0x7aa150

    VxMaterial& operator=(const VxMaterial& m);         // Mac 0x4600d0, Win 0x7aa350
    // Mac 0x460044, Win 0x7aa320: every unlocked contact material using this material is recomputed
    // (name inferred)
    void updateContactMaterials();
    void addContactMaterial(VxContactMaterial* cm);     // Mac 0x460280 (name inferred)
    int getIndex() const { return mIndex; }

    int mIndex;                         // Mac +0x124, Win +0x140 (-1; index in the material table) (name inferred)
    int mIntegratedSlipDisplacement;    // Mac +0x128, Win +0x144 (name inferred)
    VxArray<VxContactMaterial*> mContactMaterials; // Mac +0x12c, Win +0x148 (name inferred)

private:
    VxMaterial(const VxMaterial&);
};

// --------------------------------------------------------------------------------------
// Upper triangle of a symmetric n x n table, entry (i, j) = data[min*n + max - min*(min+1)/2]
// (inlined everywhere, "VxSymmetricTable::getEntry: invalid index (%d,%d).\n" Mac 0x46190a).
// 8 bytes. (members inferred)
template <class T>
class VxSymmetricTable {
public:
    VxSymmetricTable() : mSize(0), mData(0) {}
    unsigned int index(unsigned int i, unsigned int j) const
    {
        unsigned int a = i < j ? i : j, b = i < j ? j : i;
        return a * mSize + b - (a + 1) * a / 2;
    }
    T& getEntry(unsigned int i, unsigned int j)
    {
        if (i >= mSize || j >= mSize)
            VxFatalError(0, "VxSymmetricTable::getEntry: invalid index (%d,%d).\n", i, j);
        return mData[index(i, j)];
    }
    // keeps the entries of the old table, new ones are 0 (Mac 0x461c10)
    void resize(unsigned int n)
    {
        unsigned int count = (n + 1) * n / 2;
        T* data = (T*)vxAlloc(count * sizeof(T));
        for (unsigned int k = 0; k < count; ++k)
            data[k] = 0;
        for (unsigned int i = 0; i < n; ++i)
            for (unsigned int j = i; j < n; ++j)
                if (i < mSize && j < mSize)
                    data[(i * n + j) - (i + 1) * i / 2] = mData[index(i, j)];
        if (mData)
            vxFree(mData);
        mData = data;
        mSize = n;
    }

    // --- tables of class types (VxBridge's response entries and pair requests;
    // Win outlines them: ctor 0x7425a0 / 0x742730, grow + set 0x742640 / 0x7427d0, dtor
    // 0x742600 / 0x742790; Mac inlines them) ---
    // n x n entries, all = fill (Mac 0x4361a8: vxAlloc, default-construct, then assign)
    void init(unsigned int n, const T& fill)
    {
        mSize = n;
        mData = 0;
        unsigned int count = (n + 1) * n / 2;
        if (count == 0)
            return;
        mData = (T*)vxAlloc(count * sizeof(T));
        for (unsigned int k = 0; k < count; ++k)
            new (mData + k) T();
        for (unsigned int i = 0; i < mSize; ++i)
            for (unsigned int j = i; j < mSize; ++j)
                mData[index(i, j)] = fill;
    }
    // n x n entries: the old ones where both indices are old, the others = fill (Mac 0x435148)
    void grow(unsigned int n, const T& fill)
    {
        unsigned int count = (n + 1) * n / 2;
        T* data = (T*)vxAlloc(count * sizeof(T));
        for (unsigned int k = 0; k < count; ++k)
            new (data + k) T();
        for (unsigned int i = 0; i < n; ++i)
            for (unsigned int j = i; j < n; ++j) {
                T& d = data[(i * n + j) - (i + 1) * i / 2];
                if (i < mSize && j < mSize)
                    d = mData[index(i, j)];
                else
                    d = fill;
            }
        destroy();
        mData = data;
        mSize = n;
    }
    // entry (i, j) = v, growing to max(i, j) + 1 first (new entries = v) (Mac 0x435148)
    void setEntry(unsigned int i, unsigned int j, const T& v)
    {
        if (i >= mSize || j >= mSize)
            grow((i < j ? j : i) + 1, v);
        mData[index(i, j)] = v;
    }
    // destructs the entries and frees the storage (size kept)
    void destroy()
    {
        if (mData) {
            unsigned int count = (mSize + 1) * mSize / 2;
            for (unsigned int k = 0; k < count; ++k)
                mData[k].~T();
            vxFree(mData);
        }
        mData = 0;
    }

    unsigned int mSize; // +0x00
    T* mData;           // +0x04
};

// The tire model reference of a contact material and of the material table (Mac 0x44236a /
// 0x44238a (D2 / D1), Win inlined): deletes the object when owned. (name inferred)
struct VxTireModelRef {
    VxTireModelRef() : mObject(0), mOwned(false) {}
    ~VxTireModelRef()
    {
        if (mOwned && mObject)
            delete mObject;
    }
    VxBase* mObject;    // +0x00 (VxTireModelParams on Win; dead-stripped on Mac)
    bool mOwned;        // +0x04
};

// The combine function: computes a contact material from its two materials (Mac 0x461288 calls
// the pointer Mac 0x779390 = vxDefaultCombineMaterials) (names inferred)
typedef void (*VxCombineMaterialsFn)(const VxMaterial* m0, const VxMaterial* m1, VxContactMaterial* cm);
extern VxCombineMaterialsFn gVxCombineMaterials;         // Mac 0x779390
// Mac 0x4613d0: the stiffer (less compliant) material's restitution / threshold / compliance /
// damping, the larger adhesive force and integrated displacement mode; per axis: the common
// friction model with the smaller coefficient, box force and static scale and the larger slip and
// slide, or the parameters of the material whose model has the higher priority (Mac 0x6440ac)
void vxDefaultCombineMaterials(const VxMaterial* m0, const VxMaterial* m1, VxContactMaterial* cm);
void vxCombineMaterials(const VxMaterial* m0, const VxMaterial* m1, VxContactMaterial* cm); // Mac 0x461288

// --------------------------------------------------------------------------------------
// VxMaterialTable: no vtable, 0x38 bytes on both (VxFrame Mac 0x45548a: new(0x38) + ctor; dtor
// Mac 0x46313e from VxFrame's destructor). (class and member names inferred)
class VxMaterialTable {
public:
    VxMaterialTable();                                  // C1 Mac 0x463358 -> C2 0x463152: reset(true)
    ~VxMaterialTable();                                 // D1 Mac 0x46313e -> D2 0x462ff6

    int getMaterialCount() const { return mMaterials.size(); }  // Mac 0x461246 (out of line)
    VxMaterial* getMaterial(int index);                 // Mac 0x461256: 0 when out of range
    // Mac 0x462af8: the material of that name, created (a copy of material 0) when missing
    VxMaterial* getMaterial(const char* name);
    // Mac 0x462546: registers m; an existing material with the same name and parameters is
    // returned instead (m is kept in mDuplicates); a taken name is made unique
    VxMaterial* registerMaterial(VxMaterial* m);
    // Mac 0x461c10, Win 0x73ec20: n materials (missing ones created as "Material..."), and every
    // missing contact material of the pairs
    void setMaterialCount(int n);
    // Mac 0x4612c8: contact material of the pair (0 = material 0), 0 for an unknown material
    VxContactMaterial* getContactMaterial(const VxMaterial* m0, const VxMaterial* m1);
    // Mac 0x462dc6: deletes everything; with createDefault a material "default" is created
    void reset(bool createDefault);
    void print(const char* fileName);                   // Mac 0x46190a, Win 0x73ea10

    VxArray<VxMaterial*> mMaterials;                    // +0x00
    VxSymmetricTable<VxContactMaterial*> mContactMaterials; // +0x10
    VxArray<VxTireModelRef*> mTireModels;               // +0x18 (deleted with operator delete)
    VxArray<VxMaterial*> mDuplicates;                   // +0x28 materials registered as duplicates

private:
    VxMaterialTable(const VxMaterialTable&);
    VxMaterialTable& operator=(const VxMaterialTable&);
};

} // namespace Vx

#endif
