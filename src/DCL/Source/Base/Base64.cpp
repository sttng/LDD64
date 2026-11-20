// LEGO Digital Designer — restored source
// TU 129 (file name inferred)  Mac 0x1e8700-0x1e88df (4 functions), Win 0x444870-0x444aaf.
// AString / BasicArray<UInt8> front end of the Base64 codec (TU 130). Static init 0x1e88a0.
// Templates emitted here on Win: 0x444ad0 BasicArray<UInt8>::Resize, 0x444b10 its scalar
// deleting dtor, 0x444b70 BasicArray<UInt8>::Insert, 0x444db0 AString::Append.
#include "Base64.h"
#include "LEGO/StdLib.h"
#include <string.h>

#include <iostream>

namespace LEGO {

// Mac 0x1e8700, Win 0x444870
// NOTE: appends (Mac 0x88540 / Win 0x444db0 are AString::Append); the Mac build skips the
// work for size 0, the Win build encodes an empty buffer — same result.
bool Base64Encode(const void* data, UInt32 size, AString& out)
{
    if (size != 0) {
        AString buffer;
        buffer.Reserve(Base64EncodedSize(size), true);
        char* text = buffer.GetWritableBuffer();
        Base64EncodeBuffer(data, size, text);
        out.Append(text, StrLen(text));
    }
    return true;
}

// Mac 0x1e87d0, Win 0x444940
bool Base64Encode(BasicArray<UInt8> data, AString& out)
{
    if (data.Begin() == data.End())
        return true;
    return Base64Encode(data.Begin(), data.GetSize(), out);
}

// Mac 0x1e8800, Win 0x444a00
// NOTE: the buffer is sized by Base64DecodedSize, which subtracts the '=' padding at the end of
// the text, while Base64DecodeBuffer writes three bytes for every complete group of alphabet
// characters (both builds): "AAAA=" sizes 2 bytes and writes 3, text with '=' or other
// characters inside does likewise (heap overflow; GetRedirectURLCaller decodes server replies);
// fixed: the buffer holds the most the decoder can write, length / 4 * 3, and is cut to the
// size afterwards.
bool Base64Decode(const AString& text, BasicArray<UInt8>& out)
{
    UInt32 size = Base64DecodedSize(text.c_str());
    const UInt32 room = (UInt32)(strlen(text.c_str()) / 4) * 3;
    out.Resize(room > size ? room : size, 0);
    if (size == 0) {
        out.Resize(0, 0);
        return true;
    }
    const bool ok = Base64DecodeBuffer(text.c_str(), out.Begin()) == size;
    out.Resize(size, 0);
    return ok;
}

} // namespace LEGO
