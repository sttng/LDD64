// LEGO Digital Designer — restored source
// TCPSocketNotifier: a TCPSocket forwarding its events to observers (header-only; Mac copies in
// TU 059 0x137840-0x138710, Win 0x63460c-0x634d19). HTTPCommunicator: one HTTP/1.0 GET or 1.1
// POST over it (Location redirects, Content-Length or chunked bodies) for LDDWebObserver.
// TU 058 "HTTPCommunicator.cpp" (Mac 0x12e790-0x13371f, Win 0x53e7e0-0x53fa1f + destructor copies
// 0x632700-0x6348be). Ctor and Connect inline (Mac copies in TU 059: 0x136160, 0x136370).
// Class names from RTTI; member and method names inferred.

// Vtables (Mac):
//   TCPSocketObserver   0x70c3fc {D1 0x131f00, D0 0x131f30, OnFinalRelease 0x131f80, OnAttached,
//                       OnDetached, 5-9 pure}; Win {OnAttached, OnDetached, 5 × _purecall}.
//   TCPSocketNotifier   0x765cfc {OnClosed 0x137840, OnAccepted 0x137ab0, OnConnected 0x137d30,
//                       OnConnectFailed 0x137fb0, GetTimeout, D1 0x138220, D0 0x1382c0, OnReadable,
//                       OnWritable, OnException, OnReceived 0x138300, OnFinalRelease 0x138570};
//                       Notifier<TCPSocketObserver> +0x54, Object +0x78 (sizeof 0x80).
//   HTTPCommunicatorObserver 0x70c6ac {D1 0x136d30, D0 0x136d60, OnFinalRelease 0x136bc0,
//                       OnAttached, OnDetached, 5-9 pure}.
//   HTTPCommunicator    0x70c34c {D1 0x132700, D0 0x132760, OnFinalRelease 0x1327d0, OnAttached,
//                       OnDetached, OnSocketClosed 0x12f240, OnSocketAccepted 0x132870,
//                       OnSocketConnected 0x12f4c0, OnSocketConnectFailed 0x12f730,
//                       OnSocketReceived 0x12f9a0}; Notifier<HTTPCommunicatorObserver> +0x08,
//                       Object +0x58 (sizeof 0x60). Win: OnSocketReceived 0x53f0e0, sdd 0x633ffa.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "AutoPointer.h"
#include "Notifier.h"
#include "Socket.h"

namespace LEGO {

class TCPSocketNotifier;
class HTTPCommunicator;

// ===========================================================================================
// TCPSocketObserver / TCPSocketNotifier
// ===========================================================================================
class TCPSocketObserver : public Observer<TCPSocketNotifier, TCPSocketObserver> {
public:
    TCPSocketObserver() {}
    virtual ~TCPSocketObserver() {}                                          // Mac 0x131f00/0x131f30, Win sdd 0x632a3d

    virtual void OnSocketClosed() = 0;                                       // 5
    virtual void OnSocketAccepted() = 0;                                     // 6
    virtual void OnSocketConnected() = 0;                                    // 7
    virtual void OnSocketConnectFailed() = 0;                                // 8
    virtual void OnSocketReceived() = 0;                                     // 9
};

class TCPSocketNotifier : public TCPSocket, public Notifier<TCPSocketObserver> {
public:
    // Inline (Mac 0x136370: operator new 0x80, every field zero, send block 8 KB).
    TCPSocketNotifier() {}
    virtual ~TCPSocketNotifier() {}                                          // Mac 0x138220/0x1382c0, Win sdd 0x634d19

    // Each event is forwarded to the observers (OnAccepted / OnConnected update the send
    // block size first). Mac 0x137840 / 0x137ab0 / 0x137d30 / 0x137fb0 / 0x138300,
    // Win 0x63460c / 0x634661 / 0x6346bb / 0x634715 / …
    virtual void OnClosed()
    {
        Iterator it(*this);
        while (TCPSocketObserver* observer = it.Next(m_observers))
            observer->OnSocketClosed();
    }
    virtual void OnAccepted()
    {
        TCPSocket::OnAccepted();
        Iterator it(*this);
        while (TCPSocketObserver* observer = it.Next(m_observers))
            observer->OnSocketAccepted();
    }
    virtual void OnConnected()
    {
        TCPSocket::OnConnected();
        Iterator it(*this);
        while (TCPSocketObserver* observer = it.Next(m_observers))
            observer->OnSocketConnected();
    }
    virtual void OnConnectFailed()
    {
        Iterator it(*this);
        while (TCPSocketObserver* observer = it.Next(m_observers))
            observer->OnSocketConnectFailed();
    }
    virtual void OnReceived()
    {
        Iterator it(*this);
        while (TCPSocketObserver* observer = it.Next(m_observers))
            observer->OnSocketReceived();
    }

protected:
    // The observers leave, then the socket closes. Mac 0x138570 (thunks 0x138640, 0x138710)
    virtual void OnFinalRelease()
    {
        NotifierT<TCPSocketNotifier, TCPSocketObserver>::OnFinalRelease();
        TCPSocket::OnFinalRelease();
    }
};

// ===========================================================================================
// HTTPCommunicatorObserver
// ===========================================================================================
class HTTPCommunicatorObserver : public Observer<HTTPCommunicator, HTTPCommunicatorObserver> {
public:
    HTTPCommunicatorObserver() {}
    virtual ~HTTPCommunicatorObserver() {}                                   // Mac 0x136d30/0x136d60, Win sdd 0x632a5c

    // The connection is up: send the request now.
    virtual void OnConnected() = 0;                                          // 5
    // The whole body (`size` bytes) is there (HTTPCommunicator::GetContent).
    virtual void OnResponse(UInt32 size) = 0;                                // 6
    // `received` of `total` body bytes (chunked: (0, 1) at the header).
    virtual void OnProgress(UInt32 received, UInt32 total) = 0;              // 7
    // No connection, an error status, a broken reply or the connection closed early.
    virtual void OnFailed() = 0;                                             // 8
    // A 3xx reply with a Location header.
    virtual void OnRedirect(const String& url) = 0;                          // 9
};

// ===========================================================================================
// HTTPCommunicator
// ===========================================================================================
class HTTPCommunicator : public TCPSocketObserver, public Notifier<HTTPCommunicatorObserver> {
public:
    typedef Pair<UInt32, AutoPointer<UInt8> > Chunk;   // (size, data)

    // Inline (Mac copy 0x136160, emitted in TU 059; operator new 0x60).
    explicit HTTPCommunicator(const AString& userAgent)
        : m_userAgent(userAgent), m_contentLength(0), m_chunkSize(0), m_chunked(false),
          m_finished(false), m_readingHeaders(true)
    {
    }
    virtual ~HTTPCommunicator() {}                                           // Mac 0x132700/0x132760, Win sdd 0x633ffa

    // Splits "http://host[:port][/path]" (scheme case-insensitive): port 80 by default, `path`
    // empty without one. False for another scheme, a bad port or an empty host.
    // Mac 0x12e790, Win 0x53e7e0
    static bool ParseURL(const String& url, String& host, UInt32& port, String& path);

    // Creates the socket, observes it and starts connecting (SO_REUSEADDR | SO_KEEPALIVE).
    // Inline (Mac copy 0x136370 in TU 059).
    bool Connect(const AString& host, UInt16 port)
    {
        m_host = host;
        m_socket = new TCPSocketNotifier();
        Attach(m_socket.Get());
        return m_socket->Connect(host, port, 3);
    }

    // "POST <path> HTTP/1.1" with User-Agent, Host, Content-Type (`contentType`, default
    // application/x-www-form-urlencoded) and Content-Length, then `data`. Mac 0x12ead0, Win 0x53ead0
    void SendPost(const AString& path, const AString& data, const AString& contentType);
    // "GET <path> HTTP/1.0" with Host, Accept*, User-Agent. Mac 0x12eeb0, Win 0x53ecb0
    void SendGet(const AString& path);

    // The body size once the reply is complete, else 0. Mac 0x12f1a0, Win 0x53ee40
    UInt32 GetContentLength() const;
    // Copies the complete body (GetContentLength() bytes) to `buffer`. Mac 0x12f1c0, Win 0x53ee50
    bool GetContent(void* buffer);

    TCPSocketNotifier* GetSocket() const { return m_socket.Get(); }          // (inlined: +0x34)
    // Drops the socket reference (LDDWebObserver::OnTrigger after closing it). (inlined)
    void ResetSocket() { m_socket = 0; }

    // ---- TCPSocketObserver ----
    // Closed before the reply was complete: OnFailed. Mac 0x12f240, Win 0x53eed0
    virtual void OnSocketClosed();
    virtual void OnSocketAccepted() {}                                       // Mac 0x132870
    // Mac 0x12f4c0, Win 0x53ef80
    virtual void OnSocketConnected();
    // Mac 0x12f730, Win 0x53f030
    virtual void OnSocketConnectFailed();
    // Parses what arrived: header lines, then the body. Mac 0x12f9a0, Win 0x53f0e0
    virtual void OnSocketReceived();

protected:
    // The observers leave, then this leaves the socket. Mac 0x1327d0, Win 0x6348be
    virtual void OnFinalRelease();

private:
    // Observer::Detach (inlined: RemoveObserver, subject cleared, OnDetached). Win 0x53fa70
    void DetachSocket() { Detach(); }

    void NotifyConnected();
    void NotifyResponse(UInt32 size);
    void NotifyProgress(UInt32 received, UInt32 total);
    void NotifyFailed();
    void NotifyRedirect(const String& url);

    // Mac / Win offsets
    AString                        m_host;            // +0x2c / +0x30
    AString                        m_userAgent;       // +0x30 / +0x34
    RefPointer<TCPSocketNotifier>  m_socket;          // +0x34 / +0x38
    UInt32                         m_contentLength;   // +0x38 / +0x3c body size (chunked: so far)
    UInt32                         m_chunkSize;       // +0x3c / +0x40 size of the chunk being read
    bool                           m_chunked;         // +0x40 / +0x44
    BasicArray<Chunk>              m_chunks;          // +0x44 / +0x48
    bool                           m_finished;        // +0x54 / +0x58
    bool                           m_readingHeaders;  // +0x55 / +0x59
                                                      // Object +0x58
};

} // namespace LEGO
