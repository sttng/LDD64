// LEGO Digital Designer — restored source
// TU 225 "GetRedirectURLCaller.cpp" (Mac 0x269170-0x269c0f, Win 0x698219-0x6985cf). See
// GetRedirectURLCaller.h.
#include "GetRedirectURLCaller.h"

#include "Base64.h"
#include "BasicArray.h"
#include "HTTPCommunicator.h"

#include <iostream>

namespace LEGO {

// Mac 0x269170, Win 0x698542
void GetRedirectURLCaller::OnResponse(UInt32 size)
{
    if (ParseRedirect(size, m_url))
        SetSucceeded();
    else
        SetFailed();
}

// Mac 0x2691a0, Win 0x698219
bool GetRedirectURLCaller::ParseRedirect(UInt32 size, String& url)
{
    // NOTE: size + 1 wraps for a reply size of 0xFFFFFFFF (both builds); fixed: a reply over
    // 256 MB is refused (H20; HTTPCommunicator refuses such replies too).
    if (size > 0x10000000u)
        return false;
    char* buffer = new char[size + 1];
    bool result = false;
    if (m_communicator->GetContent(buffer)) {
        buffer[size] = 0;
        const String text(buffer);
        const UInt32 id = text.Find(String("?id="), 0);
        if (id != String::npos) {
            const UInt32 quote = text.Find(String("'http:"), 0);
            const String rest = text.Substring(quote + 1, text.GetLength() - quote);
            const UInt32 end = rest.Find(String("';"), 0);
            if (end != String::npos) {
                url = rest.Substring(0, end);
                result = true;
            }
        } else {
            const UInt32 start = text.Find(String("url="), 0);
            if (start != String::npos) {
                const String rest = text.Substring(start + 4, text.GetLength() - (start + 4));
                const UInt32 equals = rest.Find((UniChar16)'=', 0);
                if (equals != String::npos) {
                    const String encoded = rest.Substring(0, equals + 1);
                    BasicArray<UInt8> decoded;
                    if (Base64Decode(encoded.ToAString(), decoded)) {
                        decoded.PushBack(0);
                        url = reinterpret_cast<const char*>(&decoded[0]);
                        result = true;
                    }
                }
            }
        }
    }
    delete[] buffer;
    return result;
}

// Mac 0x269730, Win 0x698568
void GetRedirectURLCaller::Call(const String& url)
{
    // Mac: BasicString<char>(ascii, 0x44, length - 0x44) (0x118760, start and count clipped).
    const AString ascii = url.ToAString();
    const UInt32 length = ascii.GetLength();
    const UInt32 start = length < 0x44 ? length : 0x44;
    const AString data(ascii.GetBuffer() + start, length - start);
    Request(url, data);
}

// Mac 0x269870
void GetRedirectURLCaller::Cancel()
{
    HTTPCommunicator* communicator = m_communicator.Get();
    if (communicator->GetSocket()) {
        communicator->GetSocket()->Close();
        communicator->ResetSocket();
    }
}

} // namespace LEGO
