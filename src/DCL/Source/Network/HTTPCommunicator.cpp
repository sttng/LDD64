// LEGO Digital Designer — restored source
// TU 058 "HTTPCommunicator.cpp" (Mac 0x12e790-0x13371f, Win 0x53e7e0-0x53fa1f): HTTPCommunicator.
// See HTTPCommunicator.h. Template instances of the TU (Notifier<HTTPCommunicatorObserver>,
// Observer<TCPSocketNotifier, TCPSocketObserver>, the chunk array, AString appends Mac
// 0x1331f0 / RangeException helpers) come from the shared templates.
#include "HTTPCommunicator.h"

#include "LEGO/StdLib.h"
#include "Trace.h"
#include <errno.h>

namespace LEGO {

namespace {

// (not in the original) The largest chunk and the largest response body accepted.
const UInt32 kMaxChunkSize     = 64 * 1024 * 1024;
const UInt32 kMaxContentLength = 256 * 1024 * 1024;

const UInt32 npos = 0xFFFFFFFF;

// Position of `text` in `line` (BasicString::Find with the C string's length; Mac 0xf2730 with
// StrLen, Win 0x43a9f0 with a constant length).
inline UInt32 Find(const AString& line, const char* text)
{
    return line.Find(text, 0, StrLen(text));
}

// First character of `line` at or after `start` that is in `set` (inlined: MemChr over the
// set, Mac 0x12fac0; Win 0x448c10).
UInt32 FindFirstOf(const AString& line, const char* set, UInt32 start)
{
    const UInt32 setLength = StrLen(set);
    const UInt32 length = line.GetLength();
    if (line.IsEmpty() || setLength == 0 || start >= length)
        return npos;
    const char* text = line.GetBuffer();
    for (UInt32 i = start; i < length; ++i)
        if (MemChr(set, text[i], setLength))
            return i;
    return npos;
}

} // namespace

// Mac 0x12e790, Win 0x53e7e0
bool HTTPCommunicator::ParseURL(const String& url, String& host, UInt32& port, String& path)
{
    String hostPort;
    String pathPart;
    if (!url.EqualsNoCase(0, 7, String("http://")))
        return false;
    const UInt32 slash = url.FindFirstOf(String("/"), 7);
    String authority;
    if (slash == npos) {
        authority = url.Substring(7);
    }
    else {
        authority = url.Substring(7, slash - 7);
        pathPart = url.Substring(slash);
    }
    const UInt32 colon = authority.FindFirstOf(String(":"), 0);
    hostPort = authority.Substring(0, colon);
    UInt32 number;
    if (colon == npos) {
        number = 80;
    }
    else {
        UInt32 position = colon + 1;
        if (!authority.Parse(number, position))
            return false;
    }
    if (hostPort.IsEmpty())
        return false;
    host = hostPort;
    port = number;
    path = pathPart;
    return true;
}

// Mac 0x12ead0, Win 0x53ead0
void HTTPCommunicator::SendPost(const AString& path, const AString& data, const AString& contentType)
{
    AString request;
    request.Append("POST ").Append(path).Append(" HTTP/1.1\r\n");
    request.Append("User-Agent: ").Append(m_userAgent).Append("\r\n");
    request.Append("Host: ").Append(m_host).Append("\r\n");
    if (contentType.IsEmpty())
        request.Append("Content-Type: application/x-www-form-urlencoded\r\n");
    else
        request.Append("Content-Type: ").Append(contentType).Append("\r\n");
    char number[128];
    UInt32ToStr(data.GetLength(), number, 10);
    request.Append("Content-Length: ").Append(number).Append("\r\n");
    TraceVerbose(String(request), true);
    request.Append("\r\n");
    request.Append(data);
    m_socket->Send(request.GetBuffer(), request.GetLength());
}

// Mac 0x12eeb0, Win 0x53ecb0
void HTTPCommunicator::SendGet(const AString& path)
{
    AString request("GET ");
    request.Append(path).Append(" HTTP/1.0\r\n");
    request.Append("Host: ").Append(m_host).Append("\r\n");
    request.Append("Accept: */*\r\n");
    request.Append("Accept-Language: en-us, en;q=0.5\r\n");
    request.Append("Accept-Encoding: identity, *;q=0\r\n");
    request.Append("Accept-Charset: utf-8, *;q=0\r\n");
    request.Append("User-Agent: ").Append(m_userAgent).Append("\r\n");
    TraceVerbose(String(request), true);
    request.Append("\r\n");
    m_socket->Send(request.GetBuffer(), request.GetLength());
}

// Mac 0x12f1a0, Win 0x53ee40
UInt32 HTTPCommunicator::GetContentLength() const
{
    return m_finished ? m_contentLength : 0;
}

// Mac 0x12f1c0, Win 0x53ee50
bool HTTPCommunicator::GetContent(void* buffer)
{
    if (!m_finished)
        return false;
    if (!m_chunked)
        return m_socket->Read(buffer, m_contentLength);
    UInt32 offset = 0;
    for (BasicArray<Chunk>::Iterator it = m_chunks.Begin(); it != m_chunks.End(); ++it) {
        MemCopy(static_cast<UInt8*>(buffer) + offset, it->second.Get(), it->first);
        offset += it->first;
    }
    return offset == m_contentLength;
}

// ---- notifications ------------------------------------------------------------------------------

void HTTPCommunicator::NotifyConnected()
{
    Iterator it(*this);
    while (HTTPCommunicatorObserver* observer = it.Next(m_observers))
        observer->OnConnected();
}

void HTTPCommunicator::NotifyResponse(UInt32 size)
{
    Iterator it(*this);
    while (HTTPCommunicatorObserver* observer = it.Next(m_observers))
        observer->OnResponse(size);
}

void HTTPCommunicator::NotifyProgress(UInt32 received, UInt32 total)
{
    Iterator it(*this);
    while (HTTPCommunicatorObserver* observer = it.Next(m_observers))
        observer->OnProgress(received, total);
}

void HTTPCommunicator::NotifyFailed()
{
    Iterator it(*this);
    while (HTTPCommunicatorObserver* observer = it.Next(m_observers))
        observer->OnFailed();
}

void HTTPCommunicator::NotifyRedirect(const String& url)
{
    Iterator it(*this);
    while (HTTPCommunicatorObserver* observer = it.Next(m_observers))
        observer->OnRedirect(url);
}

// ---- TCPSocketObserver ----------------------------------------------------------------------------

// Mac 0x12f240, Win 0x53eed0
void HTTPCommunicator::OnSocketClosed()
{
    if (m_finished)
        return;
    NotifyFailed();
}

// Mac 0x12f4c0, Win 0x53ef80
void HTTPCommunicator::OnSocketConnected()
{
    NotifyConnected();
}

// Mac 0x12f730, Win 0x53f030
void HTTPCommunicator::OnSocketConnectFailed()
{
    NotifyFailed();
}

// Mac 0x12f9a0, Win 0x53f0e0
void HTTPCommunicator::OnSocketReceived()
{
    AString line;
    if (m_readingHeaders) {
        bool informational = false;   // a 1xx status: another header block follows
        bool redirect = false;        // a 3xx status: look for the Location header
        do {
            if (!GetSubject()->ReadLine(line))
                break;
            if (line.IsEmpty()) {
                m_readingHeaders = false;
                break;
            }
            TraceVerbose(String(line), true);

            bool checkLocation = redirect;
            const UInt32 http = Find(line, "HTTP/");
            if (http != npos) {
                const UInt32 space = FindFirstOf(line, " ", http);
                if (space != npos) {
                    const UInt32 status = StrToUInt32(line.GetBuffer() + space, 0, 10);
                    if (errno == ERANGE) {
                        // NOTE (both builds): parsing goes on with this line.
                        GetSubject()->Close();
                        NotifyFailed();
                    }
                    else {
                        switch (status / 100) {
                        case 1:
                            informational = true;
                            break;
                        case 3:
                            redirect = true;
                            checkLocation = true;
                            break;
                        case 4:
                        case 5:
                            GetSubject()->Close();
                            return;
                        default:
                            break;
                        }
                    }
                }
            }

            if (checkLocation) {
                const UInt32 location = Find(line, "Location: ");
                if (location != npos) {
                    m_finished = true;
                    GetSubject()->Close();
                    AString target;
                    if (location + 10 <= line.GetLength())
                        target.Assign(line.GetBuffer() + location + 10, line.GetLength() - location - 10);
                    if (target.IsEmpty())
                        NotifyFailed();
                    else
                        NotifyRedirect(String(target));
                    return;
                }
            }
            else if (!informational) {
                const UInt32 length = Find(line, "Content-Length:");
                if (length == npos) {
                    if (Find(line, "Transfer-Encoding: chunked") != npos) {
                        m_contentLength = 0;
                        m_chunkSize = 0;
                        m_chunked = true;
                        NotifyProgress(0, 1);
                    }
                }
                else if (!m_chunked) {
                    m_contentLength = StrToUInt32(line.GetBuffer() + length + 15, 0, 10);
                    if (errno == ERANGE || m_contentLength > kMaxContentLength) {
                        GetSubject()->Close();
                        NotifyFailed();
                    }
                    else {
                        NotifyProgress(0, m_contentLength);
                    }
                }
            }
        } while (m_readingHeaders);

        if (informational) {
            m_readingHeaders = true;
            return;
        }
        if (m_readingHeaders)
            return;
    }

    if (m_chunked) {
        for (;;) {
            if (m_chunkSize == 0) {
                AString sizeLine;
                bool stop = true;
                if (GetSubject()->ReadLine(sizeLine)) {
                    m_chunkSize = StrToUInt32(sizeLine.GetBuffer(), 0, 16);
                    // NOTE: an out-of-range size fails the request but the loop goes on with
                    // it, and any size is trusted (both builds): FFFFFFFF wraps m_chunkSize + 2
                    // below and the chunk copy overflows the heap; fixed: the request fails and
                    // stops on a size over kMaxChunkSize or a total over kMaxContentLength.
                    if (errno == ERANGE || m_chunkSize > kMaxChunkSize ||
                        (UInt64)m_contentLength + m_chunkSize > kMaxContentLength) {
                        m_chunkSize = 0;
                        GetSubject()->Close();
                        NotifyFailed();
                        return;
                    }
                    stop = false;
                    if (m_chunkSize == 0) {
                        m_finished = true;
                        DetachSocket();
                        NotifyResponse(m_contentLength);
                        stop = true;
                    }
                }
                if (stop)
                    return;
            }
            // NOTE (both builds): until the chunk and its CRLF have arrived this loops without
            // ever returning (nothing receives meanwhile) — a chunk split over two reads hangs
            // the program (Mac 0x130df9 `jb 0x1309e0`, Win 0x53f6a7); fixed: it returns and
            // goes on when more data has arrived (the size is kept), and a failed read stops
            // .
            if (GetSubject()->GetReceivedSize() < (UInt64)m_chunkSize + 2)
                return;
            AutoPointer<UInt8> data(new UInt8[m_chunkSize]);
            if (!m_socket->Read(data.Get(), m_chunkSize)) {
                GetSubject()->Close();
                NotifyFailed();
                return;
            }
            m_chunks.PushBack(Chunk(m_chunkSize, data));
            m_contentLength += m_chunkSize;
            m_chunkSize = 0;
            AString end;
            if (m_socket->ReadLine(end) && !end.IsEmpty()) {
                GetSubject()->Close();
                NotifyFailed();
                return;
            }
        }
    }

    UInt32 received = GetSubject()->GetReceivedSize();
    if (m_contentLength < received)
        received = m_contentLength;
    NotifyProgress(received, m_contentLength);
    if (received != m_contentLength)
        return;
    m_finished = true;
    NotifyResponse(received);
    DetachSocket();
}

// Mac 0x1327d0, Win 0x6348be
void HTTPCommunicator::OnFinalRelease()
{
    NotifierT<HTTPCommunicator, HTTPCommunicatorObserver>::OnFinalRelease();
    TCPSocketObserver::OnFinalRelease();
}

} // namespace LEGO
