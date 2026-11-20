// Vortex 5.0.0 build 800 "vortexldd41" - VxData::FieldVisitor and InputOutputVisitor (restored
// header). Only in the Win binary (the Mac Field<bool> / Container accept bodies
// still call the visitor by slot). Win FieldVisitor vtable 0x9fe244: dtor 0x784a60 + 15 empty visit
// overloads (0x713eb0). MSVC emits overloaded virtuals in reverse declaration order, so the order
// below (= Itanium) reverses the Win slots. Evidence: Win Container::accept 0x7cbe70 calls +0x38,
// Mac 0x444284 Mac slot 3 (2nd declared); the folded pointer-field accept 0x7c5360 calls +0x3c
// (FieldBase&); Mac Field<bool>::accept (0x41990e) calls Mac slot 11 (10th declared). The overloads
// at Win 2, 3, 13 are called by no surviving accept body; their parameter types are guesses (marked).
#ifndef VXDATA_FIELDVISITOR_H
#define VXDATA_FIELDVISITOR_H

#include "Vx/VxID.h"
#include <string>

namespace Vx {
class VxVector3;
class VxColor;
class VxTransform;
}

namespace VxData {

class FieldBase;
class Container;
template <class T> class Field;

class FieldVisitor {
public:
    virtual ~FieldVisitor() {}                                       // Win 0x784a60
    virtual void visit(FieldBase&) {}                                // Win 15
    virtual void visit(Container&) {}                                // Win 14
    virtual void visit(Field<float>&) {}                             // Win 13 (type inferred)
    virtual void visit(Field<int>&) {}                               // Win 12
    virtual void visit(Field<unsigned int>&) {}                      // Win 11
    virtual void visit(Field<long>&) {}                              // Win 10
    virtual void visit(Field<unsigned long>&) {}                     // Win 9
    virtual void visit(Field<short>&) {}                             // Win 8
    virtual void visit(Field<unsigned short>&) {}                    // Win 7
    virtual void visit(Field<bool>&) {}                              // Win 6, Mac slot 11
    virtual void visit(Field<double>&) {}                            // Win 5
    virtual void visit(Field<Vx::VxVector3>&) {}                     // Win 4
    virtual void visit(Field<Vx::VxColor>&) {}                       // Win 3 (type inferred)
    virtual void visit(Field<Vx::VxTransform>&) {}                   // Win 2 (type inferred)
    virtual void visit(Field<std::string>&) {}                       // Win 1
};

// Win vtable 0x9fe9a4 (dtor 0x788580, visit(Container&) 0x788480): finds the direct child of a
// container whose full name has the wanted checksum, or (when not case sensitive) whose full name
// equals it ignoring case. Used by Win-only name lookups. (member names inferred)
class InputOutputVisitor : public FieldVisitor {
public:
    InputOutputVisitor(const Vx::VxID& name, bool caseSensitive)
        : mName(name), mResult(0), mCaseSensitive(caseSensitive)
    {
    }
    virtual ~InputOutputVisitor() {}
    using FieldVisitor::visit;
    virtual void visit(Container& c);                                // Win 0x788480

    FieldBase* getResult() const { return mResult; }

    Vx::VxID mName;          // Win +0x04
    FieldBase* mResult;      // Win +0x24
    bool mCaseSensitive;     // Win +0x28
};

} // namespace VxData

#endif
