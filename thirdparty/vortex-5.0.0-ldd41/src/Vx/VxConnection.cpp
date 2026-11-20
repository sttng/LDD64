// Vortex 5.0.0 build 800 "vortexldd41" - VxConnection and the connection functors (restored).
// Mac TU 443 (0x40aa90-0x417c1e, "thirdparty/vortex.cpp"; static initialiser
// 0x417bb9: <iostream>), Win 0x784a20-0x78aa00.
//
// Almost all of the TU's 435 Mac functions are instances of the anonymous templates below (13
// same-type pairs, 11 numeric and 2 text conversions): GenericTemplateConnectionFunctor<A, B,
// Derived> (the VxConnectionFunctor implementation; its constructor subscribes Derived::transfer,
// bound with boostVx::bind, to the input's change delegate), ConnectionFunctor<T, T>,
// NumericConversionFunctor<A, B>, ToTextConversionFunctor<T>; plus the
// VxData::implementation::DelegateCB<Field<A> const&, bind_t<..>> / BaseDelegateCB and
// Delegate::connect instances (e.g. Mac 0x410d92), VxArray growth helpers (Mac 0x4163f6-0x417574)
// and boostVx::numeric exception classes (Mac 0x4163c0-0x417b90). Functor classes and members
// from the RTTI (Mac 0x75bea0-0x75c24c); method names inferred.
#include "Vx/VxConnection.h"
#include "Vx/VxControllerBase.h"
#include "Vx/VxInputOutput.h"
#include "Vx/VxMessage.h"
#include "VxData/Field.h"
// LDD64: Boost 1.92 in place of Vortex's private boostVx copy (Boost 1.42, namespace boostVx).
#include <boost/bind/bind.hpp>
#include <boost/mem_fn.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <iostream>
#include <string>

namespace VxGraphics {
class VxFileGraphicAsset;
}

using VxData::FieldBase;

namespace {

using Vx::VxControllerBase;
using Vx::VxInputOutput;

// Mac 11 slots (vtable e.g. <int,int> 0x75af68), Win 10. Mac 0x14 bytes.
template <class A, class B, class Derived>
class GenericTemplateConnectionFunctor : public Vx::VxConnectionFunctor {
public:
    // e.g. Mac 0x413dfa (<int,int>, register arguments), Win 0x7890b0; inlined in the factory
    // for the pointer types. The fields must be VxInputOutput<A> / <B> (std::bad_cast otherwise).
    GenericTemplateConnectionFunctor(FieldBase* input, FieldBase* output)
        : mConnection(),
          mInput(&dynamic_cast<VxInputOutput<A>&>(*input)),
          mOutput(&dynamic_cast<VxInputOutput<B>&>(*output))
    {
        mConnection = mInput->getDelegate().connect(
            boost::bind(&Derived::transfer, static_cast<Derived*>(this)));
    }
    // e.g. Mac D1 0x40da60 / D0 0x40da08, Win 0x78a420
    virtual ~GenericTemplateConnectionFunctor() { mConnection.disable(); }

    // Mac e.g. 0x40db90 / 0x40db82 (owner at VxInputOutput +0x64), Win 0x786330 / 0x785db0
    virtual VxControllerBase* getInputOwner() const { return mInput->getOwner(); }
    virtual VxControllerBase* getOutputOwner() const { return mOutput->getOwner(); }
    // Mac e.g. 0x40b3c2 / 0x40b3ce, Win 0x7859b0 / 0x785f80 (folded for all)
    virtual FieldBase* getInputField() const { return mInput; }
    virtual FieldBase* getOutputField() const { return mOutput; }
    // Mac e.g. 0x40e60a (the transfer body again), Win 0x787210
    virtual void update() { static_cast<Derived*>(this)->transfer(); }
    // Mac e.g. 0x40c822 / 0x40c5e6, Win 0x7861c0 / 0x7861d0
    virtual void disable() { mConnection.disable(); }
    virtual void enable() { mConnection.enable(); }
    // Mac slots 9 / 10 (e.g. 0x40b934 / 0x40b91e), Win 8 / 9 (0x785c50 / 0x786520)
    virtual const Vx::VxID& getInputName() const { return mInput->getFullName(); }
    virtual const Vx::VxID& getOutputName() const { return mOutput->getFullName(); }

protected:
    // the common tail of every transfer: the output's controller must run again
    void markOutputOwner() { mOutput->getOwner()->setInputsChanged(true); }

    VxData::DelegateConnection mConnection; // +0x04
    VxInputOutput<A>* mInput;               // +0x0c
    VxInputOutput<B>* mOutput;              // +0x10
};

// Same-type copy (13 instances; Mac 11 slots, e.g. <int,int> vtable 0x75af28,
// D1 0x411b32 / D0 0x411ada; transfer e.g. 0x40ab0c).
template <class A, class B>
class ConnectionFunctor : public GenericTemplateConnectionFunctor<A, B, ConnectionFunctor<A, B> > {
public:
    ConnectionFunctor(FieldBase* input, FieldBase* output)
        : GenericTemplateConnectionFunctor<A, B, ConnectionFunctor<A, B> >(input, output)
    {
    }
    void transfer()
    {
        this->mOutput->setValue(this->mInput->getValue());
        this->markOutputOwner();
    }
};

// Checked numeric conversion (11 instances): boostVx::numeric_cast (Trunc rounding). An
// out-of-range value is caught (catch (const bad_numeric_cast&): Win FuncInfo 0xa7c044, Mac
// landing pad 0x41119d) and reported; the output and its owner stay unchanged. E.g.
// <double,unsigned int> transfer Mac 0x41280e, update 0x411036 / Win 0x788fb0 -> 0x7886d0.
template <class A, class B>
class NumericConversionFunctor : public GenericTemplateConnectionFunctor<A, B, NumericConversionFunctor<A, B> > {
public:
    NumericConversionFunctor(FieldBase* input, FieldBase* output)
        : GenericTemplateConnectionFunctor<A, B, NumericConversionFunctor<A, B> >(input, output)
    {
    }
    void transfer()
    {
        try {
            this->mOutput->setValue(boost::numeric_cast<B>(this->mInput->getValue()));
            this->markOutputOwner();
        } catch (const boost::numeric::bad_numeric_cast&) {
            Vx::VxWarning(0, "VxConnection: cannot convert value of %s -> %s \n",
                          this->mInput->getName().c_str(), this->mOutput->getName().c_str());
        }
    }
};

// Number to text (int, double): the input's toString() (value + '\n'), e.g. <int> transfer
// Mac 0x40f0d2, update 0x40f3aa.
template <class A>
class ToTextConversionFunctor : public GenericTemplateConnectionFunctor<A, std::string, ToTextConversionFunctor<A> > {
public:
    ToTextConversionFunctor(FieldBase* input, FieldBase* output)
        : GenericTemplateConnectionFunctor<A, std::string, ToTextConversionFunctor<A> >(input, output)
    {
    }
    void transfer()
    {
        this->mOutput->setValue(this->mInput->toString());
        this->markOutputOwner();
    }
};

inline bool isPair(FieldBase* a, FieldBase* b, int ta, int tb)
{
    return a->getType() == ta && b->getType() == tb;
}

} // namespace

namespace Vx {

// Mac 0x414c6a, Win 0x784c90. The checks run in this order (both getType() calls each time).
VxConnectionFunctor* createConnectionFunctor(const VxConnection::FieldInfo& from, const VxConnection::FieldInfo& to)
{
    using namespace VxData;
    FieldBase* a = from.field;
    FieldBase* b = to.field;
    if (isPair(a, b, kFieldTypeBool, kFieldTypeBool))
        return new ConnectionFunctor<bool, bool>(a, b);
    if (isPair(a, b, kFieldTypeInt, kFieldTypeInt))
        return new ConnectionFunctor<int, int>(a, b);
    if (isPair(a, b, kFieldTypeString, kFieldTypeString))
        return new ConnectionFunctor<std::string, std::string>(a, b);
    if (isPair(a, b, kFieldTypeDouble, kFieldTypeDouble))
        return new ConnectionFunctor<double, double>(a, b);
    if (isPair(a, b, kFieldTypeVector3, kFieldTypeVector3))
        return new ConnectionFunctor<VxVector3, VxVector3>(a, b);
    if (isPair(a, b, kFieldTypeColor, kFieldTypeColor))
        return new ConnectionFunctor<VxColor, VxColor>(a, b);
    if (isPair(a, b, kFieldTypeTransform, kFieldTypeTransform))
        return new ConnectionFunctor<VxTransform, VxTransform>(a, b);
    if (isPair(a, b, kFieldTypeConstraint, kFieldTypeConstraint))
        return new ConnectionFunctor<VxConstraint*, VxConstraint*>(a, b);
    if (isPair(a, b, kFieldTypePart, kFieldTypePart))
        return new ConnectionFunctor<VxPart*, VxPart*>(a, b);
    if (isPair(a, b, kFieldTypeEntity, kFieldTypeEntity))
        return new ConnectionFunctor<VxEntity*, VxEntity*>(a, b);
    if (isPair(a, b, kFieldTypeCable, kFieldTypeCable))
        return new ConnectionFunctor<VxCable*, VxCable*>(a, b);
    if (isPair(a, b, kFieldTypeMechanism, kFieldTypeMechanism))
        return new ConnectionFunctor<VxMechanism*, VxMechanism*>(a, b);
    if (isPair(a, b, kFieldTypeGraphicAsset, kFieldTypeGraphicAsset))
        return new ConnectionFunctor<VxGraphics::VxFileGraphicAsset*, VxGraphics::VxFileGraphicAsset*>(a, b);
    if (isPair(a, b, kFieldTypeInt, kFieldTypeDouble))
        return new NumericConversionFunctor<int, double>(a, b);
    if (isPair(a, b, kFieldTypeUnsignedInt, kFieldTypeDouble))
        return new NumericConversionFunctor<unsigned int, double>(a, b);
    if (isPair(a, b, kFieldTypeDouble, kFieldTypeInt))
        return new NumericConversionFunctor<double, int>(a, b);
    if (isPair(a, b, kFieldTypeDouble, kFieldTypeUnsignedInt))
        return new NumericConversionFunctor<double, unsigned int>(a, b);
    if (isPair(a, b, kFieldTypeBool, kFieldTypeInt))
        return new NumericConversionFunctor<bool, int>(a, b);
    if (isPair(a, b, kFieldTypeBool, kFieldTypeUnsignedInt))
        return new NumericConversionFunctor<bool, unsigned int>(a, b);
    if (isPair(a, b, kFieldTypeInt, kFieldTypeUnsignedInt))
        return new NumericConversionFunctor<int, unsigned int>(a, b);
    if (isPair(a, b, kFieldTypeInt, kFieldTypeBool))
        return new NumericConversionFunctor<int, bool>(a, b);
    if (isPair(a, b, kFieldTypeUnsignedInt, kFieldTypeBool))
        return new NumericConversionFunctor<unsigned int, bool>(a, b);
    if (isPair(a, b, kFieldTypeBool, kFieldTypeDouble))
        return new NumericConversionFunctor<bool, double>(a, b);
    if (isPair(a, b, kFieldTypeDouble, kFieldTypeBool))
        return new NumericConversionFunctor<double, bool>(a, b);
    if (isPair(a, b, kFieldTypeDouble, kFieldTypeString))
        return new ToTextConversionFunctor<double>(a, b);
    if (isPair(a, b, kFieldTypeInt, kFieldTypeString))
        return new ToTextConversionFunctor<int>(a, b);
    return 0;
}

// Mac 0x40aa90
VxConnection::VxConnection()
    : mNotDependency(false), mInputController(0), mOutputController(0), mContainer(0), mFunctor(0), mType(4)
{
}

// Mac D1 0x40be04 -> 0x40bdd4, D0 0x40bd94, Win 0x7868f0 -> 0x784bf0
VxConnection::~VxConnection()
{
    delete mFunctor;
}

// Mac 0x41632e, Win 0x785460. NOTE: a previous functor is not deleted (original behaviour).
void VxConnection::connect(const FieldInfo& from, const FieldInfo& to)
{
    mFunctor = createConnectionFunctor(from, to);
    if (mFunctor == 0) {
        VxFatalError(0, "VxConnection: could not create connection from %s -> %s. Check that types match. \n",
                     from.field->getFullName().c_str(), to.field->getFullName().c_str());
    } else {
        mInputController = mFunctor->getInputOwner();
        mOutputController = mFunctor->getOutputOwner();
    }
}

// Mac 0x40aad0
void VxConnection::setContainer(VxCompositeController* container)
{
    mContainer = container;
    if (mFunctor != 0) {
        if (container == 0)
            mFunctor->disable();
        else
            mFunctor->enable();
    }
}

// Mac 0x40aafe
void VxConnection::setType(int type)
{
    mType = type;
}

} // namespace Vx
