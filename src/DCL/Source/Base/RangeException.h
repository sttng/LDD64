// LEGO Digital Designer — restored source
// LEGO::Exception / LEGO::RangeException (RTTI ".?AVException@LEGO@@", ".?AVRangeException@LEGO@@";
// thrown as RangeException*). Win: RangeException ctor 0x401080, deleting dtor 0x4010e0; Mac
// vtable 0x764428. File name inferred (not Exception.h: it would hide <exception.h> on a
// case-insensitive file system).
#pragma once

namespace LEGO {

class Exception {
public:
    virtual ~Exception() {}
};

class RangeException : public Exception {
public:
    virtual ~RangeException() {}
};

// Win 0x403eb0 (Mac inlined, e.g. 0x1331f0): throws a pointer to one instance, constructed on
// first use (exit-time dtor Win 0x938190). The string functions call it when a result would not
// fit in 32 bits.
inline void ThrowRangeException()
{
    static RangeException s_instance;
    throw &s_instance;
}

} // namespace LEGO
