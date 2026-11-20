// LEGO Digital Designer — restored source
// LEGO::UploadModelCaller — the "upload model" web call of UploadModelDialog: POSTs the saved
//   model file (Base64), a PNG picture of the scene (Base64) and the target site (the brand
//   name) to the brand's upload URL ("model=…&modelImage=…&targetSite=…", '+' and '/' sent as
//   %2B / %2F) and reads <resultURL>, <errorDescription> and <error> from the reply.
//
// TU 102 "UploadModelCaller.cpp" (Mac 0x1b6b20-0x1b835b; Win 0x697cf1-0x69819b). The TU also
// emits BasicArray<UInt8> (Mac 0x1b7410 / 0x1b7a30 and its insert helper 0x1b7a60) and the
// class's vtable / destructors. The constructor is inline (copy: UploadModelDialog, Mac
// 0x1e7990; Win 0x694810). Static initialiser Mac 0x1b82e0: <iostream> + a header static.
// RTTI: UploadModelCaller : LDDWebObserver, virtual Object (Mac +0x74, sizeof 0x7c; Win sizeof
//   0x68). Mac vtable: LDDWebObserver's 14 with D1 0x1b7710 / D0 0x1b7770, OnResponse 6
//   0x1b6b20, Call 14 0x1b6ce0. Win vtable 0x9ddb7c {…, OnResponse 3 0x697cf1, …, Call 10
//   0x697fd6}, sdd 0x694a7d.
// Every method and member name is inferred except the class name (RTTI) and the strings.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "BasicArray.h"
#include "LDDWebObserver.h"

namespace LEGO {

class Path;
class Bitmap;

class UploadModelCaller : public LDDWebObserver {
public:
    // A POST with a 3000 s timeout. (inline; Mac 0x1e7990, Win 0x694810)
    UploadModelCaller() : LDDWebObserver(true, 3000000) {}
    virtual ~UploadModelCaller();                                            // Mac 0x1b7710/0x1b7770, Win sdd 0x694a7d

    // <resultURL>, <errorDescription> and <error> of the reply (all three required).
    // Mac 0x1b6b20, Win 0x697cf1
    virtual void OnResponse(UInt32 size);                                    // 6
    // Reads `model` (fails when it cannot be read completely), encodes it and the PNG of
    // `picture` in Base64 and POSTs "model=<model>&modelImage=<picture>&targetSite=<site>"
    // (then every '+' -> "%2B", every '/' -> "%2F") to LDDWebCommunicator's upload URL.
    // Mac 0x1b6ce0, Win 0x697fd6
    virtual void Call(const Path& model, const Bitmap& picture, const AString& targetSite); // 14

    // Closes the connection of a running call (the dialog's cancel). Mac 0x1b7540
    void Cancel();

    const String& GetResultURL() const { return m_resultURL; }
    const String& GetErrorDescription() const { return m_errorDescription; }
    const String& GetError() const { return m_error; }

protected:
    String m_resultURL;          // Mac +0x44 <resultURL> (the page to open next)
    String m_errorDescription;   // Mac +0x54 <errorDescription>
    String m_error;              // Mac +0x64 <error>
                                 // Object +0x74
};

} // namespace LEGO
