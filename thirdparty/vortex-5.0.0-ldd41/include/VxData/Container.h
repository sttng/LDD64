// Vortex 5.0.0 build 800 "vortexldd41" - VxData::Container (restored header).
//
// A field holding other fields (by pointer, in insertion order). Every structural operation
// forwards to the children; copies match children by name checksum.
// Mac 0x2c bytes (vtable 0x75da88), Win 0x44 bytes (vtable 0xa01fcc). Code: Mac TU 477.
#ifndef VXDATA_CONTAINER_H
#define VXDATA_CONTAINER_H

#include "VxData/FieldBase.h"

namespace VxData {

class Container : public FieldBase {
public:
    typedef Vx::VxArray<FieldBase*> Fields;

    // Mac C2 0x44416e / C1 0x4441c2 (inlined on Win: FieldBase ctor 0x7c1210 + vptr, e.g. in
    // clone 0x7cc1a0)
    explicit Container(const Vx::VxID& name, Container* parent = 0);
    // Mac D1 0x44495c (body 0x4448c2) / D0 0x444826, Win 0x7cc570 (body 0x7cc330): removes every
    // child (0x44462e) and deletes the ones it owns
    virtual ~Container();

    virtual void accept(FieldVisitor& visitor);                 // Mac 0x444284, Win 0x7cbe70
    virtual std::string toString() const;                       // Mac 0x4446de, Win 0x7cc4b0
    virtual bool fromString(std::string& text);                 // Mac 0x44447e, Win 0x7cbee0
    virtual bool copyValue(const FieldBase& other);             // Mac 0x4441e4, Win 0x7cbf80
    virtual bool copyValueOnly(const FieldBase& other);         // Mac 0x4444bc, Win 0x7cc000
    // Mac 0x41974a, Win 0x7cc560 (folded with Field<T>::getValuePointer on Win)
    virtual const Fields& getFields() const { return mFields; }
    virtual FieldBase* clone(Container* parent) const;          // Mac 0x44432a, Win 0x7cc1a0
    virtual void reset();                                       // Mac 0x44441a, Win 0x7cc300
    virtual void notify();                                      // Mac 0x4442c6, Win 0x7cc480
    virtual void updateFullName();                              // Mac 0x44439a, Win 0x7cc3a0

    // Mac 0x444970, Win 0x7cc450: appends and sets the field's parent (name inferred)
    void addField(FieldBase* field);
    // Mac 0x44459c, Win 0x7cc130 (name from the warning string)
    void removeField(FieldBase* field);
    // Mac 0x4440e2 (inlined on Win, e.g. 0x7cbf10): first child whose name checksum equals name's, or end
    // (name inferred)
    Fields::iterator findField(const Vx::VxID& name);
    // Mac 0x44411a, Win 0x7cbf10: copyValue of every child of o into the same-named child
    // (name inferred: operator=)
    Container& operator=(const Container& o);

    Fields mFields; // Mac +0x1c, Win +0x34

private:
    Container(const Container&);
    void removeAllFields(); // Mac 0x44462e, Win 0x7cc240 (the "VxData::~Container()" loop)
};

} // namespace VxData

#endif
