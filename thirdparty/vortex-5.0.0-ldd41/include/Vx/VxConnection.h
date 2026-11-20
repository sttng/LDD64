// Vortex 5.0.0 build 800 "vortexldd41" - controller connections (restored header).
// A VxConnection links an output field of one controller to an input (or parameter) field of
// another. connect() asks the factory (Mac 0x414c6a, Win 0x784c90) for a functor matching the two
// field types: ConnectionFunctor<T,T> (copy), NumericConversionFunctor<A,B> (boostVx::numeric_cast)
// or ToTextConversionFunctor<T>, anonymous-namespace templates in src/Vx/VxConnection.cpp (Mac TU
// 443). The functor watches the input field's change delegate, copies each change into the output
// field and sets the output owner's "inputs changed" flag. Class names from RTTI; members inferred.
#ifndef VX_VXCONNECTION_H
#define VX_VXCONNECTION_H

#include "Vx/VxID.h"

namespace VxData {
class FieldBase;
}

namespace Vx {

class VxControllerBase;
class VxCompositeController;

// Mac vtable 0x75c2c8 (9 slots), Win 0x9fe21c (8): dtor + 7 pure.
class VxConnectionFunctor {
public:
    VxConnectionFunctor() {}
    // Mac D1 0x417aea / D0 0x417a36, Win 0x784a20
    virtual ~VxConnectionFunctor() {}
    virtual VxControllerBase* getInputOwner() const = 0;        // Mac 2, Win 1
    virtual VxControllerBase* getOutputOwner() const = 0;       // Mac 3, Win 2
    virtual VxData::FieldBase* getInputField() const = 0;       // Mac 4, Win 3
    virtual VxData::FieldBase* getOutputField() const = 0;      // Mac 5, Win 4
    // copy (convert) the input value to the output                 Mac 6, Win 5
    virtual void update() = 0;
    // stop / resume reacting to input changes                       Mac 7 / 8, Win 6 / 7
    virtual void disable() = 0;
    virtual void enable() = 0;

private:
    VxConnectionFunctor(const VxConnectionFunctor&);
    VxConnectionFunctor& operator=(const VxConnectionFunctor&);
};

// Mac 0x1c bytes, vtable 0x75ae68 (D1 0x40be04 / D0 0x40bd94), Win 0x9fe2cc (0x7868f0).
// Vx::InternalConnection (Mac vtable 0x75d1c0) derives from it (V1.3).
class VxConnection {
public:
    // one end of a connection: the field and its kind (1 input, 2 output, 3 parameter; the
    // kinds are VxControllerBase's) (name inferred)
    struct FieldInfo {
        VxData::FieldBase* field;
        int kind;
    };

    VxConnection();                                    // Mac 0x40aa90
    virtual ~VxConnection();                           // Mac D1 0x40be04 (body 0x40bdd4, Win 0x784bf0) / D0 0x40bd94

    // Mac 0x41632e, Win 0x785460: creates the functor; a fatal error when the types do not match
    void connect(const FieldInfo& from, const FieldInfo& to);
    // Mac 0x40aad0: the composite controller holding the connection; 0 disables the functor
    void setContainer(VxCompositeController* container);
    // Mac 0x40aafe
    void setType(int type);

    VxData::FieldBase* getInputField() const { return mFunctor->getInputField(); }
    VxData::FieldBase* getOutputField() const { return mFunctor->getOutputField(); }
    VxConnectionFunctor* getFunctor() const { return mFunctor; }

    bool mNotDependency;                    // +0x04 (set by the controller graph)
    VxControllerBase* mInputController;     // +0x08 (functor->getInputOwner())
    VxControllerBase* mOutputController;    // +0x0c (functor->getOutputOwner())
    VxCompositeController* mContainer;      // +0x10
    VxConnectionFunctor* mFunctor;          // +0x14 (owned)
    int mType;                              // +0x18 (ctor: 4)

private:
    VxConnection(const VxConnection&);
    VxConnection& operator=(const VxConnection&);
};

// Mac 0x414c6a, Win 0x784c90: the functor for the two field types, 0 if there is none
// (name inferred)
VxConnectionFunctor* createConnectionFunctor(const VxConnection::FieldInfo& from, const VxConnection::FieldInfo& to);

} // namespace Vx

#endif
