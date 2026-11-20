// Vortex 5.0.0 build 800 "vortexldd41" - controller inputs, outputs and parameters
// (restored header).
//
// A controller (Vx/VxControllerBase.h) exposes its data as VxData fields that live in three
// sub-containers of its VxInputOutputContainer ("ControllerFields": "Inputs", "Outputs",
// "Parameters"). Each such field is a VxData::Field<T> plus the VxInputOutputModifier
// interface (Mac +0x60 for 4-byte T, Win +0x78) and a pointer to the owning controller
// (Mac +0x64, Win +0x7c). VxConnection functors (Vx/VxConnection.h) copy values between
// them and mark the destination's owner with setInputsChanged(true).
//
// Only the bool instances are linked in LDD (Mac TU 446, Win 0x7fa960-0x7fb3e0); RTTI of
// VxInputOutput<int/unsigned/double/std::string/VxVector3/VxTransform/VxColor/VxPart*/
// VxEntity*/VxConstraint*/VxMechanism*/...> survives without code.
//
// Mac sizes: VxInputOutput<bool> 0x68 (Field<bool> 0x60 + modifier vptr + owner); Win 0x80.
#ifndef VX_VXINPUTOUTPUT_H
#define VX_VXINPUTOUTPUT_H

#include "Vx/VxID.h"
#include "VxData/Field.h"
#include "VxData/Container.h"
#include <string>

namespace Vx {

class VxControllerBase;

// --------------------------------------------------------------------------------------
// VxInputOutputModifier (Win vtable 0xa031e4: dtor + 3 pure; Mac: secondary vtable of each
// VxInputOutput<T>, thunks to Mac slots 18/19/20) (method names inferred): makes a copy of the
// field as an input / output / parameter of another controller (used to expose a child
// controller's field on a VxCompositeController).
class VxInputOutputModifier {
public:
    virtual ~VxInputOutputModifier() {}                                                   // Win 0x7f97e0
    virtual VxData::FieldBase* createInput(const VxID& name, VxControllerBase* owner) const = 0;     // Mac slot 18
    virtual VxData::FieldBase* createOutput(const VxID& name, VxControllerBase* owner) const = 0;    // Mac slot 19
    virtual VxData::FieldBase* createParameter(const VxID& name, VxControllerBase* owner) const = 0; // Mac slot 20
};

// --------------------------------------------------------------------------------------
// VxInputOutputContainer (Mac vtable 0x75cbe8; Win 0x7f97b0 scalar deleting dtor)
// Mac 0xb4 bytes: Container "ControllerFields" (0x2c) + owner + three sub-containers
// (Mac +0x30 / +0x5c / +0x88, Win +0x48 / +0x8c / +0xd0 relative to the container).
class VxInputOutputContainer : public VxData::Container {
public:
    explicit VxInputOutputContainer(VxControllerBase* owner);      // Mac 0x41c976, Win 0x7fa960
    virtual ~VxInputOutputContainer();                             // Mac D1 0x41b57e / D0 0x419f70, Win 0x7f97b0

    VxControllerBase* getOwner() const { return mOwner; }
    VxData::Container& getInputs() { return mInputs; }
    VxData::Container& getOutputs() { return mOutputs; }
    VxData::Container& getParameters() { return mParameters; }

    // Null-tolerant accessors used by the field constructors: 0 for a 0 owner
    // (Mac 0x417d63 / 0x417d78 / 0x417d8d; Win 0x7f9990 / 0x7f99b0 / 0x7f99d0) (names inferred)
    static VxData::Container* getInputs(VxControllerBase* owner);
    static VxData::Container* getOutputs(VxControllerBase* owner);
    static VxData::Container* getParameters(VxControllerBase* owner);

    VxControllerBase* mOwner;        // Mac +0x2c, Win +0x44 (name inferred)
    VxData::Container mInputs;       // Mac +0x30, Win +0x48 ("Inputs")
    VxData::Container mOutputs;      // Mac +0x5c, Win +0x8c ("Outputs")
    VxData::Container mParameters;   // Mac +0x88, Win +0xd0 ("Parameters")

private:
    VxInputOutputContainer(const VxInputOutputContainer&);
    VxInputOutputContainer& operator=(const VxInputOutputContainer&);
};

// --------------------------------------------------------------------------------------
// VxInputOutput<T> (Mac vtable 0x75ca48 21 slots + 0x75caa4; Win 0xa0326c + 0xa03258)
// Slots: Field<T>'s, slot 12 clone(Container*) overridden, 18-20 = the modifier functions.
template <class T>
class VxInputOutput : public VxData::Field<T>, public VxInputOutputModifier {
public:
    VxInputOutput(const T& value, const VxID& name, VxData::Container* parent)
        : VxData::Field<T>(value, name, parent), mOwner(0)
    {
    }

    // slot 12: a plain VxInputOutput copy owned by the same controller
    // (bool: Mac 0x41bc14, Win 0x7faab0)
    virtual VxData::FieldBase* clone(VxData::Container* parent) const;
    // slots 18-20 (bool: Mac 0x41abee / 0x41b8b0 / 0x41b314; Win 0x7fb140 / 0x7fb220 / 0x7fb300)
    virtual VxData::FieldBase* createInput(const VxID& name, VxControllerBase* owner) const;
    virtual VxData::FieldBase* createOutput(const VxID& name, VxControllerBase* owner) const;
    virtual VxData::FieldBase* createParameter(const VxID& name, VxControllerBase* owner) const;

    VxControllerBase* getOwner() const { return mOwner; }
    void setOwner(VxControllerBase* owner) { mOwner = owner; }

    VxControllerBase* mOwner; // Mac +0x64, Win +0x7c (bool) (name inferred)

protected:
    // the common tail of every clone / create: the copy belongs to its parent container and
    // inherits m18 (Mac: "*(p + 0x14) = 1; *(p + 0x18) = *(this + 0x18)")
    VxData::FieldBase* finishCopy(VxData::FieldBase* p) const
    {
        p->m18 = this->m18;
        p->mOwnedByParent = true;
        return p;
    }
};

// --------------------------------------------------------------------------------------
// VxInput<T> / VxOutput<T> / VxParameter<T> (bool: Mac vtables 0x75cb48 / 0x75c908 / 0x75c9a8)
// The constructor puts the field into the owner's Inputs / Outputs / Parameters container.
// Outputs and parameters always notify (Field::mAlwaysNotify = true).
template <class T>
class VxInput : public VxInputOutput<T> {
public:
    // bool: Mac C2 0x41aa08 / C1 0x41abbc, Win 0x7fab50
    VxInput(const T& value, const std::string& name, VxControllerBase* owner)
        : VxInputOutput<T>(value, VxID(name), VxInputOutputContainer::getInputs(owner))
    {
        this->mOwner = owner;
    }
    // slot 12 (bool: Mac 0x41b9ba, Win 0x7fac00)
    virtual VxData::FieldBase* clone(VxData::Container* parent) const;
};

template <class T>
class VxOutput : public VxInputOutput<T> {
public:
    // bool: Mac C2 0x41b6c6 / C1 0x41b87e, Win 0x7fad20
    VxOutput(const T& value, const std::string& name, VxControllerBase* owner)
        : VxInputOutput<T>(value, VxID(name), VxInputOutputContainer::getOutputs(owner))
    {
        this->mOwner = owner;
        this->mAlwaysNotify = true;
    }
    // slot 12 (bool: Mac 0x41d046, Win 0x7fae10)
    virtual VxData::FieldBase* clone(VxData::Container* parent) const;
};

template <class T>
class VxParameter : public VxInputOutput<T> {
public:
    // bool: Mac C2 0x41aed0 / C1 0x41b088, Win 0x7faf60
    VxParameter(const T& value, const std::string& name, VxControllerBase* owner)
        : VxInputOutput<T>(value, VxID(name), VxInputOutputContainer::getParameters(owner))
    {
        this->mOwner = owner;
        this->mAlwaysNotify = true;
    }
    // slot 12 (bool: Mac 0x41b0b0, Win 0x7fb020)
    virtual VxData::FieldBase* clone(VxData::Container* parent) const;
};

// --------------------------------------------------------------------------------------
// template bodies

template <class T>
VxData::FieldBase* VxInputOutput<T>::clone(VxData::Container* parent) const
{
    VxInputOutput<T>* p = new VxInputOutput<T>(this->mValue, this->getName(), parent);
    p->mOwner = mOwner;
    return finishCopy(p);
}

template <class T>
VxData::FieldBase* VxInputOutput<T>::createInput(const VxID& name, VxControllerBase* owner) const
{
    VxInput<T>* p = new VxInput<T>(this->mValue, std::string(name.c_str()), owner);
    p->m18 = this->m18;
    p->mOwnedByParent = true;
    return p;
}

template <class T>
VxData::FieldBase* VxInputOutput<T>::createOutput(const VxID& name, VxControllerBase* owner) const
{
    VxOutput<T>* p = new VxOutput<T>(this->mValue, std::string(name.c_str()), owner);
    p->m18 = this->m18;
    p->mOwnedByParent = true;
    return p;
}

template <class T>
VxData::FieldBase* VxInputOutput<T>::createParameter(const VxID& name, VxControllerBase* owner) const
{
    VxParameter<T>* p = new VxParameter<T>(this->mValue, std::string(name.c_str()), owner);
    p->m18 = this->m18;
    p->mOwnedByParent = true;
    return p;
}

// The derived clones keep their kind when the new parent is another controller's field
// container (the copy then belongs to that controller); any other parent gets a plain
// VxInputOutput that keeps this field's owner.
template <class T>
VxData::FieldBase* VxInput<T>::clone(VxData::Container* parent) const
{
    VxInputOutputContainer* c = parent ? dynamic_cast<VxInputOutputContainer*>(parent) : 0;
    if (c != 0) {
        VxInput<T>* p = new VxInput<T>(this->mValue, std::string(this->getName().c_str()), c->getOwner());
        return this->finishCopy(p);
    }
    return VxInputOutput<T>::clone(parent);
}

template <class T>
VxData::FieldBase* VxOutput<T>::clone(VxData::Container* parent) const
{
    VxInputOutputContainer* c = parent ? dynamic_cast<VxInputOutputContainer*>(parent) : 0;
    if (c != 0) {
        VxOutput<T>* p = new VxOutput<T>(this->mValue, std::string(this->getName().c_str()), c->getOwner());
        return this->finishCopy(p);
    }
    return VxInputOutput<T>::clone(parent);
}

template <class T>
VxData::FieldBase* VxParameter<T>::clone(VxData::Container* parent) const
{
    VxInputOutputContainer* c = parent ? dynamic_cast<VxInputOutputContainer*>(parent) : 0;
    if (c != 0) {
        VxParameter<T>* p = new VxParameter<T>(this->mValue, std::string(this->getName().c_str()), c->getOwner());
        return this->finishCopy(p);
    }
    return VxInputOutput<T>::clone(parent);
}

} // namespace Vx

#endif
