// LEGO Digital Designer — restored source
// LEGO::GetRedirectURLCaller — asks the LEGO server where to send the user (UploadModelDialog):
//   POSTs the characters 68… of the URL to the URL and reads the target from the reply page
//   (a Base64 "url=" parameter, or a quoted 'http:…' link). TU 225 "GetRedirectURLCaller.cpp"
//   (Mac 0x269170-0x269c0f, Win 0x698219-0x6985cf).
//
// RTTI: GetRedirectURLCaller : LDDWebObserver, virtual Object (Mac +0x74, sizeof 0x7c). Mac
// vtable {…, OnResponse 6 0x269170, Call 14 0x269730}; destructors 0x2698b0 / 0x269910 (body
// 0x269b30). The constructor is inline (copies in UploadModelDialog, Mac 0x1e7990: a POST with
// a 3000 s timeout and three empty Strings).
// Every method and member name is inferred except the class name (RTTI) and the strings.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "LDDWebObserver.h"

namespace LEGO {

class GetRedirectURLCaller : public LDDWebObserver {
public:
    GetRedirectURLCaller() : LDDWebObserver(true, 3000000) {}
    virtual ~GetRedirectURLCaller() {}                                       // Mac 0x2698b0/0x269910

    // Succeeds when ParseRedirect finds the URL. Mac 0x269170, Win 0x698542
    virtual void OnResponse(UInt32 size);
    // POSTs url[68…] (as ASCII) to `url`. NOTE (both builds): the offset 68 is fixed (the
    // length of the service URL prefix the dialog passes). Mac 0x269730, Win 0x698568
    virtual void Call(const String& url);                                    // 14

    // Closes the connection of a running call (the dialog's cancel). Mac 0x269870
    void Cancel();

    const String& GetURL() const { return m_url; }

protected:
    // The reply page (`size` bytes) -> `url`:
    //  * without "?id=": the text after "url=" up to and including the first '=' is Base64
    //    decoded (fails without "url=", without '=' or when the decoding fails);
    //  * with "?id=": the text after the quote of "'http:" (NOTE: the whole page when there is
    //    none) up to the next "';" (fails without one).
    // Mac 0x2691a0, Win 0x698219
    bool ParseRedirect(UInt32 size, String& url);

    String m_url;        // Mac +0x44
    String m_string54;   // Mac +0x54 (not used in this TU)
    String m_string64;   // Mac +0x64 (not used in this TU)
                         // Object +0x74
};

} // namespace LEGO
