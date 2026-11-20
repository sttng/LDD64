// Vortex 5.0.0 build 800 "vortexldd41" - VxData::Container, FieldBase, the invalid field,
// DelegateBase / DelegateConnection (restored). Mac TU 477
// "(anonymous namespace)_InvalidDataField.cpp", 0x4440e2-0x445d7c (static initialiser <iostream>
// 0x445d17). Also defined here: FieldBase::getFields' empty-children array (Mac 0x419b1e, emitted
// in TU 446) and the Win-only InputOutputVisitor (Win 0x788480).
#include "VxData/FieldBase.h"
#include "VxData/Container.h"
#include "VxData/Delegate.h"
#include "VxData/FieldVisitor.h"
#include "Vx/VxMessage.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>
#endif

using Vx::VxID;
using Vx::VxWarning;

namespace VxData {

namespace {

// Mac vtable 0x75db88 (typeinfo 0x75db58), Win 0xa01dc4. The name of the field returned when a
// lookup fails (FieldBase::getInvalidField).
class InvalidDataField : public FieldBase {
public:
    explicit InvalidDataField(const VxID& name) : FieldBase(name, 0) {}
    // Mac D1 0x44572e / D0 0x4456f6, Win 0x7c6240 (folded with ~FieldBase)
    virtual ~InvalidDataField() {}
    virtual void accept(FieldVisitor&) {}                          // Mac 0x4454e2, Win 0x713eb0
    virtual std::string toString() const { return ""; }            // Mac 0x445594, Win 0x7c6210
    virtual bool fromString(std::string&) { return false; }        // Mac 0x4454ce, Win 0x7c2400
    virtual bool copyValue(const FieldBase&) { return false; }     // Mac 0x4454d8, Win 0x7c2400
    virtual int getType() const { return kFieldTypeInvalid; }      // Mac 0x4454f2, Win 0x7c6200
    virtual FieldBase* clone(Container*) const { return 0; }       // Mac 0x4454e8, Win 0x7c61f0
    virtual void reset() {}                                        // Mac 0x4454fc, Win 0x7076a0
};

} // namespace

// ======================================================================================
// FieldBase

// Mac 0x44579c, Win 0x7c1210
FieldBase::FieldBase(const VxID& name, Container* parent)
    : mName(name), mParent(parent), mFullName(0), mOwnedByParent(false), m18(0)
{
    if (parent != 0)
        parent->addField(this);
}

// Mac D1 0x445758 -> 0x4455d8, D0 0x44585c, Win 0x7c6240
FieldBase::~FieldBase()
{
    if (mParent != 0)
        mParent->removeField(this);
    delete mFullName;
}

// Mac 0x445550, Win 0x7c12a0
void FieldBase::setName(const VxID& name)
{
    if (mName.mChecksum != name.mChecksum) {
        mName = name;
        updateFullName();
    }
}

// Mac 0x419b1e, Win 0x7c2410: a guarded function-local static (atexit Mac 0x4183bf)
const Vx::VxArray<FieldBase*>& FieldBase::getFields() const
{
    static const Vx::VxArray<FieldBase*> sNoFields;
    return sNoFields;
}

// Mac 0x445502, Win 0x7c1110
const VxID& FieldBase::getFullName() const
{
    if (mFullName == 0)
        const_cast<FieldBase*>(this)->updateFullName();
    return *mFullName;
}

// Mac 0x445982, Win 0x7c13c0
void FieldBase::updateFullName()
{
    std::string s;
    if (mParent != 0) {
        s = mParent->getFullName().c_str();
        s += '.';
    }
    s.append(mName.c_str());
    VxID* old = mFullName;
    mFullName = new VxID(s);
    delete old;
}

// Mac 0x445b4e, Win 0x7c6150 (guard 0x77fd38, object 0x77fd40, atexit 0x44576b)
FieldBase& FieldBase::getInvalidField()
{
    static InvalidDataField sInvalid(VxID("__Invalid__"));
    return sInvalid;
}

// ======================================================================================
// Container

// Mac C2 0x44416e / C1 0x4441c2 (inlined on Win)
Container::Container(const VxID& name, Container* parent)
    : FieldBase(name, parent), mFields()
{
}

// Mac D1 0x44495c -> 0x4448c2, D0 0x444826, Win 0x7cc570 -> 0x7cc330
Container::~Container()
{
    removeAllFields();
}

// Mac 0x44462e, Win 0x7cc240
void Container::removeAllFields()
{
    while (mFields.begin() != mFields.end()) {
        FieldBase* f = mFields.end()[-1];
        if (f->getParent() != this) {
            VxWarning(0,
                      "VxData::~Container(): Removing the field \"%s\" having parent \"%s\" which is not me \"%s\". This should not happen and may cause memory leak or undefined behavior.\n",
                      f->getFullName().c_str(), f->getParent()->getFullName().c_str(), getFullName().c_str());
        }
        removeField(f);
        if (f->mOwnedByParent && f != 0)
            delete f;
    }
}

// Mac 0x444970, Win 0x7cc450
void Container::addField(FieldBase* field)
{
    mFields.push_back(field);
    field->mParent = this;
}

// Mac 0x44459c, Win 0x7cc130
void Container::removeField(FieldBase* field)
{
    Fields::iterator i = std::find(mFields.begin(), mFields.end(), field);
    if (i == mFields.end()) {
        VxWarning(0, "Container::removeField: Field %s doesn't exist in container %s \r\n",
                  field->mName.c_str(), mName.c_str());
        return;
    }
    for (Fields::iterator j = i + 1; j != mFields.end(); ++j)
        *(j - 1) = *j;
    mFields.pop_back();
    field->mParent = 0;
}

// Mac 0x4440e2
Container::Fields::iterator Container::findField(const VxID& name)
{
    Fields::iterator i = mFields.begin();
    for (; i != mFields.end(); ++i)
        if ((*i)->mName.mChecksum == name.mChecksum)
            break;
    return i;
}

// Mac 0x44411a, Win 0x7cbf10
Container& Container::operator=(const Container& o)
{
    if (this != &o) {
        for (Fields::const_iterator i = o.mFields.begin(); i != o.mFields.end(); ++i) {
            Fields::iterator j = findField((*i)->mName);
            if (j != mFields.end())
                (*j)->copyValue(**i);
        }
    }
    return *this;
}

// Mac 0x444284, Win 0x7cbe70
void Container::accept(FieldVisitor& visitor)
{
    visitor.visit(*this);
    for (Fields::iterator i = mFields.begin(); i != mFields.end(); ++i)
        (*i)->accept(visitor);
}

// Mac 0x4446de, Win 0x7cc4b0: the children's text, concatenated
std::string Container::toString() const
{
    std::string s;
    for (Fields::const_iterator i = mFields.begin(); i != mFields.end(); ++i)
        s.append((*i)->toString());
    return s;
}

// Mac 0x44447e, Win 0x7cbee0: NOTE: returns the result of the last child only
bool Container::fromString(std::string& text)
{
    bool ok = true;
    for (Fields::iterator i = mFields.begin(); i != mFields.end(); ++i)
        ok = (*i)->fromString(text);
    return ok;
}

// Mac 0x4441e4, Win 0x7cbf80
bool Container::copyValue(const FieldBase& other)
{
    const Container* c = dynamic_cast<const Container*>(&other);
    if (c != 0) {
        *this = *c;
        return true;
    }
    VxWarning(0, "Container: unable to copy value from %s to %s.\n", other.getFullName().c_str(),
              getFullName().c_str());
    return false;
}

// Mac 0x4444bc, Win 0x7cc000: through the virtual getFields of both sides
bool Container::copyValueOnly(const FieldBase& other)
{
    const Container* c = dynamic_cast<const Container*>(&other);
    if (c != 0) {
        for (Fields::const_iterator i = c->getFields().begin(); i != c->getFields().end(); ++i) {
            Fields::iterator j = findField((*i)->mName);
            if (j != getFields().end())
                (*j)->copyValueOnly(**i);
        }
        return true;
    }
    VxWarning(0, "Container: unable to copy value from %s to %s.\n", other.getFullName().c_str(),
              getFullName().c_str());
    return false;
}

// Mac 0x44432a (new 0x2c), Win 0x7cc1a0
FieldBase* Container::clone(Container* parent) const
{
    Container* c = new Container(mName, parent);
    for (Fields::const_iterator i = mFields.begin(); i != mFields.end(); ++i)
        (*i)->clone(c);
    c->mOwnedByParent = true;
    return c;
}

// Mac 0x44441a, Win 0x7cc300
void Container::reset()
{
    std::for_each(mFields.begin(), mFields.end(), std::mem_fun(&FieldBase::reset));
}

// Mac 0x4442c6, Win 0x7cc480
void Container::notify()
{
    std::for_each(mFields.begin(), mFields.end(), std::mem_fun(&FieldBase::notify));
}

// Mac 0x44439a, Win 0x7cc3a0
void Container::updateFullName()
{
    FieldBase::updateFullName();
    std::for_each(mFields.begin(), mFields.end(), std::mem_fun(&FieldBase::updateFullName));
}

// ======================================================================================
// DelegateBase / DelegateConnection

// Mac 0x444d30
DelegateBase::DelegateBase()
    : mConnections()
{
}

// Mac D1 0x444f40 -> 0x444eae, D0 0x444e14, Win 0x81b210
DelegateBase::~DelegateBase()
{
    disconnectAll();
}

// Mac 0x444f54
void DelegateBase::addConnection(DelegateConnection* c)
{
    mConnections.push_back(c);
}

// Mac 0x445032
void DelegateBase::addConnectionUnique(DelegateConnection* c)
{
    if (std::find(mConnections.begin(), mConnections.end(), c) == mConnections.end())
        mConnections.push_back(c);
}

// Mac 0x444dbc
void DelegateBase::removeConnection(DelegateConnection* c)
{
    Vx::VxArray<DelegateConnection*>::iterator i = std::find(mConnections.begin(), mConnections.end(), c);
    if (i == mConnections.end())
        return;
    for (Vx::VxArray<DelegateConnection*>::iterator j = i + 1; j != mConnections.end(); ++j)
        *(j - 1) = *j;
    mConnections.pop_back();
}

// Mac 0x444d5e, Win 0x7f7440
void DelegateBase::disconnectAll()
{
    std::for_each(mConnections.begin(), mConnections.end(), std::mem_fun(&DelegateConnection::invalidate));
    mConnections.clear();
}

// Mac C2 0x445386 / C1 0x44539c, Win 0x7f7380
DelegateConnection::DelegateConnection()
    : mCallback(0), mDelegate(0)
{
}

// Mac C2 0x445486 / C1 0x4454ac
DelegateConnection::DelegateConnection(implementation::NoTypeDelegateCB* callback, DelegateBase* delegate)
    : mCallback(callback), mDelegate(delegate)
{
    if (delegate != 0)
        delegate->addConnection(this);
}

// not in the binaries (see Delegate.h)
DelegateConnection::DelegateConnection(const DelegateConnection& o)
    : mCallback(o.mCallback), mDelegate(o.mDelegate)
{
    if (mDelegate != 0)
        mDelegate->addConnection(this);
}

// Mac C2 0x445406 / C1 0x445424
DelegateConnection::~DelegateConnection()
{
    if (mDelegate != 0)
        mDelegate->removeConnection(this);
}

// Mac 0x445438
DelegateConnection& DelegateConnection::operator=(const DelegateConnection& o)
{
    if (this != &o) {
        if (mDelegate != 0)
            mDelegate->removeConnection(this);
        mCallback = o.mCallback;
        mDelegate = o.mDelegate;
        if (mDelegate != 0)
            mDelegate->addConnectionUnique(this);
    }
    return *this;
}

// Mac 0x4453b0
void DelegateConnection::disable()
{
    if (mDelegate != 0)
        mDelegate->disable(mCallback);
}

// Mac 0x4453d0
void DelegateConnection::enable()
{
    if (mDelegate != 0)
        mDelegate->enable(mCallback);
}

// Mac 0x4453f0
void DelegateConnection::invalidate()
{
    mDelegate = 0;
    mCallback = 0;
}

// ======================================================================================
// InputOutputVisitor (Win only)

// Win 0x788480: iterates a copy of the children; stops at the first match
void InputOutputVisitor::visit(Container& c)
{
    Vx::VxArray<FieldBase*> fields(c.getFields());
    for (Vx::VxArray<FieldBase*>::iterator i = fields.begin(); mResult == 0 && i != fields.end(); ++i) {
        if ((*i)->getFullName().mChecksum == mName.mChecksum) {
            mResult = *i;
        } else if (!mCaseSensitive) {
#if defined(_WIN32)
            if (_stricmp((*i)->getFullName().c_str(), mName.c_str()) == 0)
#else
            if (strcasecmp((*i)->getFullName().c_str(), mName.c_str()) == 0)
#endif
                mResult = *i;
        }
    }
}

} // namespace VxData
