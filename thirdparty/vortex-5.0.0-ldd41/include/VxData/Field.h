// Vortex 5.0.0 build 800 "vortexldd41" - VxData::Field<T> (restored header).
//
// A named value with change notification. Header-only template: the original instances were
// emitted where they were used (Mac: only Field<bool>, TU 446 0x4198de-0x41c3f8 - no other
// Field<T> has a Mac vtable; Win: Field<bool/int/unsigned/long/unsigned long/short/
// unsigned short/double/std::string/VxVector3/VxPart*/VxConstraint*>, 0x7c1fc0-0x7cbe00,
// many bodies folded by ICF).
//
// Layout (Mac, 4-byte T): FieldBase 0x1c, mValue +0x1c, mDelegate +0x20 (0x34 bytes),
// mConnection +0x54, mAlwaysNotify +0x5c; Field<bool> = 0x60 bytes (Mac new(0x60) 0x41a7da).
// Win (4-byte T): mValue +0x34, mDelegate +0x38, callbacks +0x4c; double: +0x38 / +0x40 / +0x54.
//
// Text form: one line per field, value followed by '\n' (std::ostream default formatting;
// VxVector3 with precision 17). readFromString parses the text up to the first '\n' and, on
// success, erases it from the front - NOTE: original quirk, the '\n' itself stays in the string
// (Mac 0x41bd1a erases min(pos, size) characters, Win 0x7cab90 erase(0, pos)), so a second field
// read from the same text sees an empty line and fails (Container::fromString then returns the
// last field's result).
#ifndef VXDATA_FIELD_H
#define VXDATA_FIELD_H

#include "VxData/FieldBase.h"
#include "VxData/Container.h"
#include "VxData/Delegate.h"
#include "VxData/FieldVisitor.h"
#include "Vx/VxVector3.h"
#include "Vx/VxColor.h"
#include "Vx/VxTransform.h"
#include "Vx/VxMessage.h"
#include <string>
#include <sstream>
#include <typeinfo>

namespace Vx {
class VxConstraint;
class VxPart;
class VxEntity;
class VxCable;
class VxMechanism;
}
namespace VxGraphics {
class VxFileGraphicAsset;
}

namespace VxData {

// --------------------------------------------------------------------------------------
// getType() values per T (see FieldType in FieldBase.h) (name inferred)
template <class T> struct FieldTypeOf { enum { value = kFieldTypeUnknown }; };
template <> struct FieldTypeOf<int> { enum { value = kFieldTypeInt }; };
template <> struct FieldTypeOf<bool> { enum { value = kFieldTypeBool }; };
template <> struct FieldTypeOf<double> { enum { value = kFieldTypeDouble }; };
template <> struct FieldTypeOf<Vx::VxVector3> { enum { value = kFieldTypeVector3 }; };
template <> struct FieldTypeOf<Vx::VxColor> { enum { value = kFieldTypeColor }; };
template <> struct FieldTypeOf<std::string> { enum { value = kFieldTypeString }; };
template <> struct FieldTypeOf<Vx::VxTransform> { enum { value = kFieldTypeTransform }; };
template <> struct FieldTypeOf<unsigned int> { enum { value = kFieldTypeUnsignedInt }; };
template <> struct FieldTypeOf<long> { enum { value = kFieldTypeLong }; };
template <> struct FieldTypeOf<unsigned long> { enum { value = kFieldTypeUnsignedLong }; };
template <> struct FieldTypeOf<short> { enum { value = kFieldTypeShort }; };
template <> struct FieldTypeOf<unsigned short> { enum { value = kFieldTypeUnsignedShort }; };
template <> struct FieldTypeOf<Vx::VxConstraint*> { enum { value = kFieldTypeConstraint }; };
template <> struct FieldTypeOf<Vx::VxPart*> { enum { value = kFieldTypePart }; };
template <> struct FieldTypeOf<Vx::VxEntity*> { enum { value = kFieldTypeEntity }; };
template <> struct FieldTypeOf<Vx::VxCable*> { enum { value = kFieldTypeCable }; };
template <> struct FieldTypeOf<Vx::VxMechanism*> { enum { value = kFieldTypeMechanism }; };
template <> struct FieldTypeOf<VxGraphics::VxFileGraphicAsset*> { enum { value = kFieldTypeGraphicAsset }; };

namespace implementation {

// value <-> text (the "Field::writeToString" / "Field::readFromString" helpers; Win bodies:
// bool 0x7caae0 / 0x7cab90, int 0x7ca9b0 (read), double 0x7cb570 / 0x7cb620, std::string
// 0x7cb750 / 0x7c6080, VxVector3 0x7cbbd0 / 0x7cbc80; inlined in Mac Field<bool> 0x41c3f8 /
// 0x41bd1a) (struct name inferred)
template <class T>
struct FieldIO {
    static void write(std::ostream& os, const T& v) { os << v; }
    static void read(std::istream& is, T& v) { is >> v; }

    static std::string writeToString(const T& v)
    {
        std::ostringstream os;
        write(os, v);
        os << '\n';
        return os.str();
    }
    static bool readFromString(std::string& text, T& v)
    {
        if (text.size() == 0)
            return false;
        std::string::size_type pos = text.find('\n');
        std::istringstream is(std::string(text, 0, pos));
        read(is, v);
        if (is.fail())
            return false;
        // NOTE: original quirk - the '\n' is not consumed (see the file comment)
        text.erase(0, pos < text.size() ? pos : text.size());
        return true;
    }
};

// Win 0x7cb800: precision 17 around the three components, separated by ' '
template <>
inline void FieldIO<Vx::VxVector3>::write(std::ostream& os, const Vx::VxVector3& v)
{
    std::streamsize p = os.precision(17);
    os << v[0] << ' ' << v[1] << ' ' << v[2];
    os.precision(p);
}
// Win 0x7cbc80: three doubles
template <>
inline void FieldIO<Vx::VxVector3>::read(std::istream& is, Vx::VxVector3& v)
{
    is >> v[0] >> v[1] >> v[2];
}

// VxColor / VxTransform: no instance survives in either binary; written like VxVector3
// (format inferred)
template <>
inline void FieldIO<Vx::VxColor>::write(std::ostream& os, const Vx::VxColor& v)
{
    std::streamsize p = os.precision(17);
    os << v[0] << ' ' << v[1] << ' ' << v[2] << ' ' << v[3];
    os.precision(p);
}
template <>
inline void FieldIO<Vx::VxColor>::read(std::istream& is, Vx::VxColor& v)
{
    is >> v[0] >> v[1] >> v[2] >> v[3];
}
template <>
inline void FieldIO<Vx::VxTransform>::write(std::ostream& os, const Vx::VxTransform& v)
{
    std::streamsize p = os.precision(17);
    for (int i = 0; i < 16; ++i) {
        if (i)
            os << ' ';
        os << v.m[i / 4][i % 4];
    }
    os.precision(p);
}
template <>
inline void FieldIO<Vx::VxTransform>::read(std::istream& is, Vx::VxTransform& v)
{
    for (int i = 0; i < 16; ++i)
        is >> v.m[i / 4][i % 4];
}

// Win 0x7c6080: a string field takes the whole first line verbatim (no stream parsing)
template <>
inline bool FieldIO<std::string>::readFromString(std::string& text, std::string& v)
{
    if (text.size() == 0)
        return false;
    std::string::size_type pos = text.find('\n');
    v.assign(text, 0, pos);
    text.erase(0, pos < text.size() ? pos : text.size());
    return true;
}

// pointers: no text form (Win 0x7c5100 / 0x7c50d0 warn and fail).
// NOTE: the original prints typeid(Pointee).name(); the pointee types are incomplete here, so
// typeid(Pointee*).name() is printed instead (warning text only).
template <class P>
struct FieldIO<P*> {
    static std::string writeToString(P* const&)
    {
        Vx::VxWarning(0, "Field::writeToString is unavailable for pointers to %s.\n", typeid(P*).name());
        return std::string();
    }
    static bool readFromString(std::string&, P*&)
    {
        Vx::VxWarning(0, "Field::readFromString is unavailable for pointers to %s.\n", typeid(P*).name());
        return false;
    }
};

} // namespace implementation

// --------------------------------------------------------------------------------------
// Field<T> (Mac Field<bool> vtable 0x75cac8, 18 slots; Win e.g. Field<int> 0xa0196c)
template <class T>
class Field : public FieldBase {
public:
    typedef T ValueType;
    typedef Delegate<const Field<T>&> ChangedDelegate;

    Field(const T& value, const Vx::VxID& name, Container* parent = 0)
        : FieldBase(name, parent), mValue(value), mDelegate(), mConnection(), mAlwaysNotify(false)
    {
    }
    // bool: Mac D1 0x41acf8 / D0 0x41a74a, Win 0x7c55d0 (int 0x7c55a0 ...)
    virtual ~Field() { mConnection.disable(); }

    // bool: Mac 0x41990e (-> 0x4198f3), Win 0x7c3790
    virtual void accept(FieldVisitor& visitor) { visitor.visit(*this); }
    // bool: Mac 0x41c3f8, Win 0x7cb8c0
    virtual std::string toString() const { return implementation::FieldIO<T>::writeToString(mValue); }
    // bool: Mac 0x41bd1a, Win 0x7cb8f0
    virtual bool fromString(std::string& text)
    {
        T v = T();
        bool ok = implementation::FieldIO<T>::readFromString(text, v);
        if (ok)
            setValue(v);
        return ok;
    }
    // bool: Mac 0x41a0ce, Win 0x7c1fc0
    virtual bool copyValue(const FieldBase& other)
    {
        int a = getType();
        int b = other.getType();
        if (a == b) {
            setValue(static_cast<const Field<T>&>(other).mValue);
            return true;
        }
        Vx::VxWarning(0, "Field::copyValue: unable to copy value from %s to %s.\n",
                      other.getFullName().c_str(), getFullName().c_str());
        return false;
    }
    // bool: Mac 0x41a002, Win 0x7c37f0
    virtual bool copyValueOnly(const FieldBase& other)
    {
        int a = getType();
        int b = other.getType();
        if (a != b) {
            Vx::VxWarning(0, "Field::copyValueOnly: unable to copy value from %s to %s.\n",
                          other.getFullName().c_str(), getFullName().c_str());
        } else {
            mValue = static_cast<const Field<T>&>(other).mValue;
        }
        return a == b;
    }
    // bool: Mac 0x4198de, Win 0x7c3780
    virtual int getType() const { return FieldTypeOf<T>::value; }
    // bool: Mac 0x41a7da (new 0x60), Win 0x7c62e0. mAlwaysNotify / m18 are not copied.
    virtual FieldBase* clone(Container* parent) const
    {
        Field<T>* p = new Field<T>(mValue, mName, parent);
        p->mOwnedByParent = true;
        return p;
    }
    // bool: Mac 0x419928, Win 0x7c37b0
    virtual bool isEqual(const FieldBase& other) const
    {
        int a = other.getType();
        int b = getType();
        if (a == b)
            return mValue == static_cast<const Field<T>&>(other).mValue;
        return false;
    }
    // bool: Mac 0x41a1b0, Win 0x7c3880
    virtual void reset() { setValue(T()); }
    // bool: Mac 0x4198e8, Win 0x7cc560 (folded)
    virtual const void* getValuePointer() const { return &mValue; }
    // bool: Mac 0x41a180, Win 0x7c5380
    virtual void notify() { mDelegate(*this); }

    const T& getValue() const { return mValue; }
    // implicit read access: the delegates pass the field itself to callbacks that take the
    // value (Mac 0x419a16: bind(&VxControllerBase::stateChanged, c, e, _1) receives
    // field.mValue)
    operator const T&() const { return mValue; }
    // bool: Mac 0x4197b0, Win 0x78cd40 (inlined everywhere else, e.g. the connection functors)
    void setValue(const T& v)
    {
        if (mAlwaysNotify || v != mValue) {
            mValue = v;
            mDelegate(*this);
        }
    }
    // copy without notification (name inferred)
    void setValueNoNotify(const T& v) { mValue = v; }
    ChangedDelegate& getDelegate() { return mDelegate; }
    void setAlwaysNotify(bool b) { mAlwaysNotify = b; } // (name inferred)

    T mValue;                        // Mac +0x1c, Win +0x34 (+0x38 for double / VxVector3)
    ChangedDelegate mDelegate;       // Mac +0x1c + sizeof(T) (4-aligned)
    DelegateConnection mConnection;  // Mac +0x54 (4-byte T): a handle disabled by the dtor (name inferred)
    bool mAlwaysNotify;              // Mac +0x5c (4-byte T): notify even when the value is unchanged
                                     // (VxOutput / VxParameter set it) (name inferred)
};

} // namespace VxData

#endif
