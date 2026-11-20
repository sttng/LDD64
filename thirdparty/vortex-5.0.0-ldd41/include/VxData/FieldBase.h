// Vortex 5.0.0 build 800 "vortexldd41" - VxData::FieldBase (restored header).
//
// VxData is Vortex's reflection layer: named, typed values (Field<T>) organised in a tree of
// Containers, with change notification (Delegate), text conversion (toString / fromString) and
// a visitor. In LDD only the controller fields (Vx/VxInputOutput.h) use it.
//
// Code: Mac TU 477 "(anonymous namespace)_InvalidDataField.cpp" (0x4440e2-0x445d7c: Container,
// FieldBase, InvalidDataField, DelegateBase) -> src/VxData/FieldBase.cpp; the out-of-line
// defaults Mac 0x41971e-0x419740 / 0x419b1e were emitted in TU 446 (inline in this header).
// Win: 0x7c1110-0x7cc5a0 (ICF folds many template bodies).
//
// Virtual slots (Win index = Mac index - 1; Mac has D1+D0 at 0/1) - names inferred unless a
// string names them ("Field::copyValue", "Field::copyValueOnly", "Field::readFromString",
// "Field::writeToString", "Container::removeField", "VxData::~Container").
#ifndef VXDATA_FIELDBASE_H
#define VXDATA_FIELDBASE_H

#include "Vx/VxID.h"
#include "Vx/VxArray.h"
#include <string>

namespace VxData {

class Container;
class FieldVisitor;

// type ids returned by getType() (Win slot 10). Values read from the Field<T>::getType bodies
// (Win 0x7c3780 bool 2, 0x752ce0 int 1, 0x7c4690 double 3, 0x7c4d20 VxVector3 4, 0x7c49e0
// std::string 6, 0x7c3a70 unsigned int 8, 0x7c3c70 long 9, 0x7c3e70 unsigned long 10,
// 0x7c4150 short 11, 0x7c4360 unsigned short 12, 0x7c5350 VxConstraint* 13, 0x7c50a0 VxPart* 14)
// and from the connection factory (Mac 0x414c6a: 5 VxColor, 7 VxTransform, 15 VxEntity*,
// 16 VxCable*, 17 VxMechanism*, 18 VxFileGraphicAsset*). InvalidDataField returns 20.
// (enumerator names inferred)
enum FieldType {
    kFieldTypeUnknown = 0,
    kFieldTypeInt = 1,
    kFieldTypeBool = 2,
    kFieldTypeDouble = 3,
    kFieldTypeVector3 = 4,
    kFieldTypeColor = 5,
    kFieldTypeString = 6,
    kFieldTypeTransform = 7,
    kFieldTypeUnsignedInt = 8,
    kFieldTypeLong = 9,
    kFieldTypeUnsignedLong = 10,
    kFieldTypeShort = 11,
    kFieldTypeUnsignedShort = 12,
    kFieldTypeConstraint = 13,
    kFieldTypePart = 14,
    kFieldTypeEntity = 15,
    kFieldTypeCable = 16,
    kFieldTypeMechanism = 17,
    kFieldTypeGraphicAsset = 18,
    kFieldTypeInvalid = 20
};

// Mac 0x1c bytes, Win 0x34 bytes (vtable Mac 0x75db08, Win 0xa01924)
class FieldBase {
public:
    // Mac 0x44579c, Win 0x7c1210: name, parent; registers itself with the parent
    // (Container::addField)
    FieldBase(const Vx::VxID& name, Container* parent);
    // Mac D1 0x445758 (body 0x4455d8) / D0 0x44585c, Win 0x7c6240: leaves the parent, frees the
    // cached full name
    virtual ~FieldBase();

    virtual void accept(FieldVisitor& visitor) = 0;                 // Mac 2,  Win 1
    virtual std::string toString() const = 0;                       // Mac 3,  Win 2
    // parses one line ('\n'-terminated) from the front of text and consumes it    Mac 4, Win 3
    virtual bool fromString(std::string& text) = 0;
    // Mac 5 0x445550, Win 4 0x7c12a0: renames (when the checksum differs) and rebuilds the full name
    virtual void setName(const Vx::VxID& name);
    // value copy with notification; false (and a warning) for another type      Mac 6, Win 5
    virtual bool copyValue(const FieldBase& other) = 0;
    // value copy without notification. Mac 7 0x445d02, Win 6 0x7c2400: false
    virtual bool copyValueOnly(const FieldBase& /*other*/) { return false; }
    // Mac 8 0x41971e / 9 0x41972a, Win 7/8 0x7c23f0 (one folded body)
    virtual Container* getParent() { return mParent; }
    virtual const Container* getParent() const { return mParent; }
    // Mac 10 0x419b1e, Win 9 0x7c2410: a field has no children - a static empty array
    virtual const Vx::VxArray<FieldBase*>& getFields() const;
    // Mac 11 0x445524, Win 10 0x7c1130: 0
    virtual int getType() const { return kFieldTypeUnknown; }
    // a copy that belongs to parent (mOwnedByParent set)                         Mac 12, Win 11
    virtual FieldBase* clone(Container* parent) const = 0;
    // Mac 13 0x419736, Win 12 0x7c2400: false
    virtual bool isEqual(const FieldBase& /*other*/) const { return false; }
    // sets the default value (T()) with notification                            Mac 14, Win 13
    virtual void reset() = 0;
    // Mac 15 0x419740, Win 14 0x719870: 0 (Field<T>: &mValue)
    virtual const void* getValuePointer() const { return 0; }
    // Mac 16 0x445d0c, Win 15 0x7076a0: nothing (Field<T>: calls the change delegate)
    virtual void notify() {}
    // Mac 17 0x445982, Win 16 0x7c13c0: full name = parent's full name + '.' + name
    virtual void updateFullName();

    const Vx::VxID& getName() const { return mName; }
    // Mac 0x445502, Win 0x7c1110 (copy 0x786a30): the cached full name (built on first use)
    const Vx::VxID& getFullName() const;
    bool isOwnedByParent() const { return mOwnedByParent; }
    void setOwnedByParent(bool b) { mOwnedByParent = b; }

    // Mac 0x445b4e, Win 0x7c6150: the "__Invalid__" singleton (anonymous InvalidDataField,
    // a function-local static; atexit 0x44576b) (name inferred)
    static FieldBase& getInvalidField();

    Vx::VxID mName;          // Mac +0x04, Win +0x04
    Container* mParent;      // Mac +0x0c, Win +0x24
    Vx::VxID* mFullName;     // Mac +0x10, Win +0x28 (lazy; new VxID, 8 bytes on Mac)
    bool mOwnedByParent;     // Mac +0x14, Win +0x2c: the parent container deletes the field
    int m18;                 // Mac +0x18, Win +0x30 (ctor 0; copied by the VxInputOutput clones)

private:
    FieldBase(const FieldBase&);
    FieldBase& operator=(const FieldBase&);
};

} // namespace VxData

#endif
