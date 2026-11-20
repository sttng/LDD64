// LEGO Digital Designer — restored source
// Non-blocking BSD / WinSock sockets of LDD's web services, one select() pass per SystemUpdater
// update: Socket, TCPSocket (buffered stream, line reader), ListenSocket (never instantiated),
// SocketManager ('SOCK' in the ObjectRegistry) and its instantiator.
// TU 051 "SocketManager.cpp" (Mac 0x116160-0x118cab; Win 0x53c710-0x53e60f +
// destructor copies 0x632a1e, 0x63407b-0x634177, 0x634cf7). Class names from RTTI; member and
// method names inferred.

// RTTI / vtables (Mac):
//   Socket     vmi, virtual Object. Vtable 0x70b37c {OnClosed 0x1182b0, OnAccepted 0x1182c0,
//              OnConnected 0x1182d0, OnConnectFailed 0x1182e0, GetTimeout 0x1182f0, D1 0x118300,
//              D0 0x118370, OnReadable/OnWritable/OnException pure}; Object +0x30 (sizeof 0x38).
//              Win: no destructor in the primary vftable, slots 0-7 (OnReadable = Win 5).
//   TCPSocket  si Socket. Vtable 0x70b304 {…, OnAccepted 0x118500, OnConnected 0x118550, …,
//              D1 0x1185a0, D0 0x118600, OnReadable 0x1168f0, OnWritable 0x116a00, OnException
//              0x116b90, OnReceived 0x118670, OnFinalRelease 0x116810}; Object +0x54 (sizeof 0x5c).
//   SocketManager vmi {virtual Object, UpdateObserver +0}. Vtable 0x70b2ac {D1 0x117350,
//              D0 0x1174a0, OnFinalRelease 0x1e330, OnAttached, OnDetached, Update 0x117570};
//              Object +0x1b8 (sizeof 0x1c0). Win sizeof 0x354 (fd_set = count + 64 sockets).
//   SocketManagerInstantiator Mac 0x76436c {D1 0x116c60, D0 0x1189b0, CreateObject 0x1189c0},
//              Win {sdd 0x53d270, CreateObject 0x53d180}.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicList.h"
#include "BasicString.h"
#include "AutoPointer.h"
#include "Updater.h"
#include "ObjectRegistry.h"

namespace LEGO {

class SocketManager;

#if defined(_WIN64)
typedef UInt64 SocketHandle;          // PORT64: SOCKET is UINT_PTR, 64-bit on x64
#elif defined(_WIN32)
typedef UInt32 SocketHandle;          // SOCKET (UINT_PTR, 32-bit)
#else
typedef int    SocketHandle;          // file descriptor
#endif
const SocketHandle kInvalidSocket = (SocketHandle)-1;

// An IPv4 / IPv6 address in network byte order. Mac sizeof 0x14 (copied as 2 × 8 + 4 bytes);
// the family is the platform's AF_INET / AF_INET6 value (Mac 2 / 0x1e, Win 2 / 0x17).
struct SocketAddress {
    SocketAddress() : m_family(0), m_pad(0) { for (int i = 0; i < 16; ++i) m_address[i] = 0; }

    UInt16 m_family;       // +0x00
    UInt16 m_pad;          // +0x02
    UInt8  m_address[16];  // +0x04 IPv4: 4 bytes (sin_addr), IPv6: 16 bytes (sin6_addr)
};

// A select() set with the platform's fd_set layout (the manager keeps three of them; SocketManager.cpp
// casts them to fd_set, which keeps <winsock2.h> out of this header). Mac/Win sizeof 0x80 / 0x104.
struct SocketSet {
#if defined(_WIN64)
    // PORT64: the x64 fd_set (u_int fd_count, then 64 8-byte SOCKETs; sizeof 0x208)
    UInt32 m_count;        // fd_count
    UInt32 m_pad;
    UInt64 m_sockets[64];  // fd_array (FD_SETSIZE 64)
#elif defined(_WIN32)
    UInt32 m_count;        // fd_count
    UInt32 m_sockets[64];  // fd_array (FD_SETSIZE 64)
#else
    UInt32 m_bits[32];     // FD_SETSIZE 1024
#endif
};

// ===========================================================================================
// Socket
// ===========================================================================================
class Socket : public virtual Object {
public:
    // Inline (TCPSocketNotifier construction, Mac 0x136370): no socket, nothing set.
    Socket()
        : m_socket(kInvalidSocket), m_port(0), m_time(0), m_client(false), m_connecting(false),
          m_connected(false)
    {
    }

    // ---- events, called by SocketManager ----
    // The connection was closed (after it was established).
    virtual void OnClosed() {}                                               // 0 Mac 0x1182b0
    // An incoming connection was accepted into this socket (ListenSocket::CreateSocket).
    virtual void OnAccepted() {}                                             // 1 Mac 0x1182c0
    // The outgoing connection was established.
    virtual void OnConnected() {}                                            // 2 Mac 0x1182d0
    // The outgoing connection failed or timed out.
    virtual void OnConnectFailed() {}                                        // 3 Mac 0x1182e0
    // Milliseconds an outgoing connection may take.
    virtual UInt32 GetTimeout() const { return 5000; }                       // 4 Mac 0x1182f0

    virtual ~Socket();                                                       // 5/6 Mac 0x118300/0x118370, Win sdd 0x632a1e

    virtual void OnReadable() = 0;                                           // 7
    virtual void OnWritable() = 0;                                           // 8
    virtual void OnException() = 0;                                          // 9

    // Closes a connecting or connected socket (the manager calls OnConnectFailed / OnClosed).
    // False when it was neither. Mac 0x116360 (Win inlined: `if (connected || connecting)`)
    bool Close();

    bool IsConnected() const  { return m_connected; }                        // (inlined)
    bool IsConnecting() const { return m_connecting; }                       // (inlined)

protected:
    friend class SocketManager;

    // Mac / Win offsets
    RefPointer<SocketManager> m_manager;   // +0x04 / +0x10 the manager while connecting/connected
    SocketHandle   m_socket;               // +0x08 / +0x14
    SocketAddress  m_address;              // +0x0c / +0x18 the peer
    UInt16         m_port;                 // +0x20 / +0x2c (outgoing: host order; accepted: as received)
    UInt64         m_time;                 // +0x24 / +0x30 GetMilliseconds() of connect / accept (Win timeGetTime)
    bool           m_client;               // +0x2c / +0x38 the connection was made by this side
    bool           m_connecting;           // +0x2d / +0x39
    bool           m_connected;            // +0x2e / +0x3a
                                   // Object +0x30
private:
    Socket(const Socket&);
    Socket& operator=(const Socket&);
};

// ===========================================================================================
// ListenSocket — only the shape SocketManager::Update needs (no RTTI: never instantiated).
// ===========================================================================================
class ListenSocket : public virtual Object {
public:
    ListenSocket() : m_socket(kInvalidSocket) {}

    // A new socket for an incoming connection (null: refuse it). Slot 0 (Mac 0x117570 calls
    // it through the first vtable entry).
    virtual Socket* CreateSocket() = 0;

protected:
    friend class SocketManager;
    RefPointer<SocketManager> m_manager;   // +0x04 (not analysed)
    SocketHandle              m_socket;    // +0x08 (Win +0x0c)
};

// ===========================================================================================
// TCPSocket
// ===========================================================================================
class TCPSocket : public Socket {
public:
    // One block of the send or receive queue (node data of BasicList<TCPSocket::Buf>, 0x10 bytes).
    struct Buf {
        Buf() : m_size(0), m_offset(0) {}
        Buf(UInt8* data, UInt32 size) : m_data(data), m_size(size), m_offset(0) {}

        AutoPointer<UInt8> m_data;     // +0x00 (+0x04 owner flag)
        UInt32             m_size;     // +0x08
        UInt32             m_offset;   // +0x0c bytes already sent / read
    };

    // Inline (Mac 0x136370): 8 KB send blocks until the connection tells its SO_SNDBUF.
    TCPSocket() : m_sendBlockSize(0x2000), m_sendPending(0), m_received(0) {}
    virtual ~TCPSocket() {}                                                  // Mac 0x1185a0/0x118600, Win sdd 0x634cf7

    // Connects to `address`:`port` (flags: bit 0 SO_REUSEADDR, bit 1 SO_KEEPALIVE). False
    // when already connecting/connected or the connection cannot be started; the result
    // arrives as OnConnected / OnConnectFailed. Mac 0x116160, Win 0x53c760
    bool Connect(const SocketAddress& address, UInt16 port, UInt32 flags);
    // `host` is a dotted IPv4 address or a host name (gethostbyname, blocking). Creates the
    // SocketManager first. Mac 0x116250, Win 0x53c7a0
    bool Connect(const AString& host, UInt16 port, UInt32 flags);

    // Queues a copy of `size` bytes (sent at once when the queue was empty). False when not
    // connected. Mac 0x1163a0, Win 0x53c820
    bool Send(const void* data, UInt32 size);
    // Takes `size` received bytes; false (nothing taken) when fewer are buffered.
    // Mac 0x116460 (Win inlined, e.g. 0x53ee50)
    bool Read(void* buffer, UInt32 size);
    // Takes the next received line (without "\n" / "\r\n"); false when no complete line is
    // buffered. Mac 0x116520, Win 0x53c8e0
    bool ReadLine(AString& line);

    UInt32 GetReceivedSize() const { return m_received; }                   // (inlined: +0x38)

    // ---- Socket ----
    // Both read the send block size (SO_SNDBUF, used when 0x1001..0xffff). Mac 0x118500 / 0x118550
    virtual void OnAccepted() { UpdateSendBlockSize(); }
    virtual void OnConnected() { UpdateSendBlockSize(); }
    // Receives up to 32 KB into the queue (end of stream / error: Close), then OnReceived.
    // Mac 0x1168f0, Win 0x53cbd0
    virtual void OnReadable();
    // Sends the next block of the queue; an empty queue leaves the manager's write set.
    // Mac 0x116a00, Win 0x53cce0
    virtual void OnWritable();
    virtual void OnException() {}                                            // 9 Mac 0x116b90

    // New data are in the receive queue. Slot 10, Mac 0x118670 (empty)
    virtual void OnReceived() {}

protected:
    // Closes the socket before the last reference goes away. Slot 11, Mac 0x116810, Win 0x53cab0
    virtual void OnFinalRelease();

    // SO_SNDBUF -> m_sendBlockSize when 0x1001 <= value <= 0xffff. Mac 0x1168a0, Win 0x53cb80
    void UpdateSendBlockSize();
    // Copies (and consumes) `size` bytes from the front of the receive queue. (helper; inlined
    // in Read / ReadLine, Win 0x53cae0)
    void TakeReceived(void* buffer, UInt32 size);

    UInt32           m_sendBlockSize;   // +0x30 / +0x40
    UInt32           m_sendPending;     // +0x34 / +0x44 bytes in the send queue
    UInt32           m_received;        // +0x38 / +0x48 bytes in the receive queue
    BasicList<Buf>   m_sendQueue;       // +0x3c (list +0x40)
    BasicList<Buf>   m_receiveQueue;    // +0x48 (list +0x4c)
                                        // Object +0x54
};

// ===========================================================================================
// SocketManager
// ===========================================================================================
class SocketManager : public virtual Object, public UpdateObserver {
public:
    enum { kRegistryId = 0x534f434b };   // 'SOCK'
    // Protocols of Connect. (names inferred)
    enum Protocol { kUDP = 1, kTCP = 2 };

    // Attaches to the SystemUpdater. Mac 0x1171e0, Win 0x53d3a0
    SocketManager();
    // Win: WSACleanup. Mac 0x117350/0x1174a0, Win 0x53d510 / sdd 0x53d4d0
    virtual ~SocketManager();

    // The registered manager (created on first use; the registry keeps it). Mac inlined in
    // TCPSocket::Connect, Win 0x53d2e0
    static SocketManager* GetInstance();

    // One non-blocking select() over every socket: read / write / exception events of the
    // connected sockets, accepts of the listening ones, then the outcome or timeout of the
    // connecting ones. Mac 0x117570, Win 0x53d610
    virtual void Update();

    // Starts a non-blocking connect of `socket`. Mac 0x116c70, Win 0x53dd70 (TCP only)
    bool Connect(Socket* socket, const SocketAddress& address, UInt16 port, UInt32 flags,
                 UInt32 protocol);
    // Closes `socket` and calls its OnConnectFailed (connecting) or OnClosed (connected).
    // Mac 0x117060, Win 0x53ded0
    void Close(Socket* socket);

    // Adds `socket` to / removes it from the write set (TCPSocket::OnWritable). (inlined)
    void WatchWrite(Socket* socket, bool watch);

private:
    // socket() + SO_REUSEADDR / SO_KEEPALIVE. Mac 0x116ba0, Win 0x53cfa0
    static SocketHandle CreateHandle(UInt32 family, int type, const char* protocol, UInt32 flags);

    BasicArray<Socket*>       m_sockets;      // +0x08 / +0x0c connected
    BasicArray<Socket*>       m_connecting;   // +0x18 / +0x1c
    BasicArray<ListenSocket*> m_listening;    // +0x28 / +0x2c
    SocketSet                 m_readSet;      // +0x38 / +0x3c
    SocketSet                 m_writeSet;     // +0xb8 / +0x140
    SocketSet                 m_exceptSet;    // +0x138 / +0x244
                                              // Object +0x1b8 / +0x34c
};

class SocketManagerInstantiator : public ObjectRegistryInstantiator {
public:
    SocketManagerInstantiator() {}
    virtual ~SocketManagerInstantiator() {}                                  // Mac 0x116c60/0x1189b0, Win sdd 0x53d270
    // Win: null when WSAStartup(2.0) fails. Mac 0x1189c0, Win 0x53d180
    virtual RefPointer<Object> CreateObject() const;
};

} // namespace LEGO
