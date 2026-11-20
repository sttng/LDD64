// LEGO Digital Designer — restored source
// TU 051 "SocketManager.cpp" (Mac 0x116160-0x118cab) + the Windows twin (Win 0x53c710-0x53e60f):
// Socket, TCPSocket, SocketManager, SocketManagerInstantiator. See Socket.h.

// Mac BSD sockets vs Win WinSock 2 (imported by ordinal):
//   * non-blocking: fcntl(O_NONBLOCK) / ioctlsocket(FIONBIO); connect in progress: EINPROGRESS /
//     WSAEWOULDBLOCK; close / closesocket;
//   * time stamps: GetMilliseconds() 64-bit / timeGetTime() 32-bit (a deadline beyond 2^32 ms
//     never expires);
//   * fd_set: bit sets (nfds = highest + 1) / arrays of 64 (FD_SET ignores a 65th socket); Win's
//     Update returns at once when all three sets are empty (WinSock's select fails on them);
//   * Win: WSAStartup(2.0) in the instantiator, WSACleanup in the destructor.
// The template instances of the TU (BasicArray<Socket*> / <ListenSocket*> destructors Mac
// 0x117180 / 0x1171b0 / 0x118bd0 / 0x118c00, Win 0x53e5b0 / 0x53e5e0; BasicList<TCPSocket::Buf>
// Mac 0x118a20-0x118b60, Win 0x63407b / 0x634177; AString::Assign Mac 0x118760 / Erase 0x1188a0)
// come from the shared templates.
#if defined(_WIN32)
#include <winsock2.h>   // before <windows.h> (Object.h)
#include <ws2tcpip.h>   // sockaddr_in6
#include <mmsystem.h>   // timeGetTime
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "Socket.h"

#include "LEGO/StdLib.h"
#include "SystemTime.h"
#include "SystemUpdater.h"
#include <string.h>

namespace LEGO {

namespace {

#if defined(_WIN32)
typedef int SocketLength;
#else
typedef socklen_t SocketLength;
#endif

inline fd_set* Native(SocketSet& set) { return reinterpret_cast<fd_set*>(&set); }
// SocketSet mirrors the platform's fd_set (PORT64: also on x64, compile-time only)
typedef char SocketSetSizeCheck[sizeof(SocketSet) == sizeof(fd_set) ? 1 : -1];

inline bool IsSet(SocketHandle socket, SocketSet& set)
{
    return FD_ISSET(socket, Native(set)) != 0;
}
inline void Add(SocketHandle socket, SocketSet& set)
{
    if (!IsSet(socket, set))
        FD_SET(socket, Native(set));
}
inline void Remove(SocketHandle socket, SocketSet& set)
{
    FD_CLR(socket, Native(set));
}

inline void CloseHandle(SocketHandle socket)
{
#if defined(_WIN32)
    closesocket(socket);
#else
    close(socket);
#endif
}

// Makes `socket` non-blocking; false on failure.
inline bool SetNonBlocking(SocketHandle socket)
{
#if defined(_WIN32)
    u_long on = 1;
    return ioctlsocket(socket, FIONBIO, &on) != -1;
#else
    return fcntl(socket, F_SETFL, O_NONBLOCK) != -1;
#endif
}

// The time stamp of a connect / accept.
inline UInt64 Now()
{
#if defined(_WIN32)
    return (UInt64)timeGetTime();
#else
    return (UInt64)GetMilliseconds();
#endif
}

// `time` + `timeout` lies in the past.
inline bool IsExpired(UInt64 time, UInt32 timeout)
{
    const UInt64 deadline = time + timeout;
#if defined(_WIN32)
    // Win 0x53dc70: compared with a 32-bit timeGetTime() only when the sum has no high word.
    return (deadline >> 32) == 0 && (UInt32)deadline < (UInt32)timeGetTime();
#else
    return deadline < (UInt64)GetMilliseconds();
#endif
}

// std::find + "replace by the last, drop the last" (Mac 0x1180b0, Win 0x53e6c0): the order of
// the remaining sockets changes.
template <class T>
void RemoveSwap(BasicArray<T*>& array, T* item)
{
    typename BasicArray<T*>::Iterator it = array.Begin();
    while (it != array.End() && *it != item)
        ++it;
    if (it == array.End())
        return;
    typename BasicArray<T*>::Iterator last = array.End() - 1;
    if (last != it)
        *it = *last;
    array.PopBack();
}

// SocketAddress -> sockaddr (Win 0x53d040, Mac inlined in Connect). False for other families.
bool ToNative(const SocketAddress& address, UInt16 port, sockaddr_storage& out, SocketLength& length)
{
    memset(&out, 0, sizeof(out));
    if (address.m_family == AF_INET6) {
        sockaddr_in6& in6 = reinterpret_cast<sockaddr_in6&>(out);
        in6.sin6_family = AF_INET6;
        in6.sin6_port = htons(port);
        memcpy(&in6.sin6_addr, address.m_address, 16);
        length = 0x1c;
        return true;
    }
    if (address.m_family == AF_INET) {
        sockaddr_in& in4 = reinterpret_cast<sockaddr_in&>(out);
        in4.sin_family = AF_INET;
        in4.sin_port = htons(port);
        memcpy(&in4.sin_addr, address.m_address, 4);
        length = 0x10;
        return true;
    }
    return false;
}

// sockaddr of an accepted connection -> address + port (Win 0x53d0d0, Mac inlined). The port is
// stored as received (network order). Other families leave both unchanged.
void FromNative(const sockaddr_storage& in, SocketAddress& address, UInt16& port)
{
    if (in.ss_family == AF_INET6) {
        const sockaddr_in6& in6 = reinterpret_cast<const sockaddr_in6&>(in);
        address.m_family = AF_INET6;
        memcpy(address.m_address, &in6.sin6_addr, 16);
        port = in6.sin6_port;
    }
    else if (in.ss_family == AF_INET) {
        const sockaddr_in& in4 = reinterpret_cast<const sockaddr_in&>(in);
        address.m_family = AF_INET;
        memcpy(address.m_address, &in4.sin_addr, 4);
        port = in4.sin_port;
    }
}

// Resolves `host` (dotted address, else gethostbyname) into an IPv4 address.
// Win 0x53c710, Mac inlined in TCPSocket::Connect 0x116250.
bool ResolveAddress(const AString& host, SocketAddress& address)
{
    address.m_family = AF_INET;
    UInt32 ip = (UInt32)inet_addr(host.GetBuffer());
    if (ip == 0xFFFFFFFF) {
        hostent* entry = gethostbyname(host.GetBuffer());
        if (!entry)
            return false;
        ip = *reinterpret_cast<const UInt32*>(entry->h_addr_list[0]);
    }
    memcpy(address.m_address, &ip, 4);
    return true;
}

} // namespace

// ===========================================================================================
// Socket
// ===========================================================================================

// Mac 0x118300 / 0x118370 (releases the manager), Win sdd 0x632a1e
Socket::~Socket()
{
}

// Mac 0x116360. NOTE: the original tests `connecting` twice in its first branch; the effect is
// "close when connected or connecting".
bool Socket::Close()
{
    if (!m_connected && !m_connecting)
        return false;
    m_manager->Close(this);
    return true;
}

// ===========================================================================================
// TCPSocket
// ===========================================================================================

// Mac 0x116160, Win 0x53c760
bool TCPSocket::Connect(const SocketAddress& address, UInt16 port, UInt32 flags)
{
    if (m_connected || m_connecting)
        return false;
    SocketManager* manager = SocketManager::GetInstance();
    if (m_connected || m_connecting)
        return false;
    return manager->Connect(this, address, port, flags, SocketManager::kTCP);
}

// Mac 0x116250, Win 0x53c7a0
bool TCPSocket::Connect(const AString& host, UInt16 port, UInt32 flags)
{
    if (m_connected || m_connecting)
        return false;
    // The manager first (Win: WSAStartup before the name lookup).
    SocketManager::GetInstance();
    SocketAddress address;
    if (!ResolveAddress(host, address))
        return false;
    return Connect(address, port, flags);
}

// Mac 0x1163a0, Win 0x53c820
bool TCPSocket::Send(const void* data, UInt32 size)
{
    if (!m_connected)
        return false;
    if (size == 0)
        return true;
    UInt8* copy = new UInt8[size];
    memcpy(copy, data, size);
    m_sendQueue.PushBack(Buf(copy, size));
    m_sendPending += size;
    if (m_sendQueue.GetSize() == 1)
        OnWritable();
    return true;
}

// Win 0x53cae0 (EAX = this, EDX = size, ECX = buffer); Mac inlined.
void TCPSocket::TakeReceived(void* buffer, UInt32 size)
{
    m_received -= size;
    UInt8* out = static_cast<UInt8*>(buffer);
    while (size != 0) {
        Buf& block = m_receiveQueue.Front();
        UInt32 count = block.m_size - block.m_offset;
        if (size < count)
            count = size;
        memcpy(out, block.m_data.Get() + block.m_offset, count);
        size -= count;
        block.m_offset += count;
        if (block.m_offset == block.m_size)
            m_receiveQueue.PopFront();
        out += count;
    }
}

// Mac 0x116460
bool TCPSocket::Read(void* buffer, UInt32 size)
{
    if (m_received < size)
        return false;
    TakeReceived(buffer, size);
    return true;
}

// Mac 0x116520, Win 0x53c8e0
bool TCPSocket::ReadLine(AString& line)
{
    UInt32 before = 0;    // bytes in the blocks before the current one
    bool   cr = false;    // the previous byte was '\r'
    for (BasicList<Buf>::Iterator it = m_receiveQueue.Begin(); it != m_receiveQueue.End(); ++it) {
        const Buf& block = *it;
        const UInt8* data = block.m_data.Get();
        for (UInt32 i = block.m_offset; i < block.m_size; ++i) {
            const char c = (char)data[i];
            if (c == '\n') {
                const UInt32 length = before + (i - block.m_offset) - (cr ? 1 : 0);
                if (length == 0) {
                    line = AString();
                }
                else {
                    char* text = new char[length];
                    TakeReceived(text, length);
                    line.Assign(text, length);
                    delete[] text;
                }
                char terminator[2];
                TakeReceived(terminator, cr ? 2 : 1);
                return true;
            }
            cr = c == '\r';
        }
        before += block.m_size - block.m_offset;
    }
    return false;
}

// Mac 0x1168a0, Win 0x53cb80. NOTE: the original reads the value uninitialised when getsockopt
// fails; zero here (no change of the block size either way, except for garbage in range).
void TCPSocket::UpdateSendBlockSize()
{
    int size = 0;
    SocketLength length = 4;
    getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&size), &length);
    if ((UInt32)(size - 0x1001) < 0xefff)
        m_sendBlockSize = (UInt32)size;
}

// Mac 0x1168f0, Win 0x53cbd0
void TCPSocket::OnReadable()
{
    char buffer[0x8000];
    const int count = (int)recv(m_socket, buffer, 0x8000, 0);
    if (count == 0 || count == -1) {
        if (m_connected || m_connecting)
            m_manager->Close(this);
        return;
    }
    m_received += (UInt32)count;
    UInt8* copy = new UInt8[count];
    memcpy(copy, buffer, count);
    m_receiveQueue.PushBack(Buf(copy, (UInt32)count));
    OnReceived();
}

// Mac 0x116a00, Win 0x53cce0
void TCPSocket::OnWritable()
{
    if (m_sendQueue.IsEmpty()) {
        m_manager->WatchWrite(this, false);
        return;
    }
    m_manager->WatchWrite(this, true);

    // Up to one block of the queue in one send().
    UInt8* block = new UInt8[m_sendBlockSize];
    UInt32 left = m_sendBlockSize;
    UInt8* out = block;
    for (BasicList<Buf>::Iterator it = m_sendQueue.Begin(); left != 0 && it != m_sendQueue.End(); ++it) {
        UInt32 count = it->m_size - it->m_offset;
        if (left < count)
            count = left;
        memcpy(out, it->m_data.Get() + it->m_offset, count);
        left -= count;
        out += count;
    }
    int sent = (int)send(m_socket, reinterpret_cast<const char*>(block), (int)(m_sendBlockSize - left), 0);
    if (sent == -1) {
        if (m_connected || m_connecting)
            m_manager->Close(this);
    }
    else {
        m_sendPending -= (UInt32)sent;
        for (UInt32 done = (UInt32)sent; done != 0;) {
            Buf& front = m_sendQueue.Front();
            UInt32 count = front.m_size - front.m_offset;
            if (done < count)
                count = done;
            front.m_offset += count;
            done -= count;
            if (front.m_offset == front.m_size)
                m_sendQueue.PopFront();
        }
    }
    // NOTE: scalar operator delete in the original (Mac); fixed: delete[]
    delete[] block;
}

// Mac 0x116810, Win 0x53cab0
void TCPSocket::OnFinalRelease()
{
    if (m_connected || m_connecting)
        m_manager->Close(this);
    Object::OnFinalRelease();
}

// ===========================================================================================
// SocketManager
// ===========================================================================================

// Mac 0x1171e0, Win 0x53d3a0
SocketManager::SocketManager()
{
    memset(&m_readSet, 0, sizeof(m_readSet));
    memset(&m_writeSet, 0, sizeof(m_writeSet));
    memset(&m_exceptSet, 0, sizeof(m_exceptSet));
    RefPointer<Updater> updater = SystemUpdater::GetInstance();
    Attach(updater.Get());
}

// Mac 0x117350/0x1174a0, Win 0x53d510
SocketManager::~SocketManager()
{
#if defined(_WIN32)
    WSACleanup();
#endif
}

// Win 0x53d2e0; Mac inlined (Get('SOCK') + dynamic_cast, the registry keeps the reference).
SocketManager* SocketManager::GetInstance()
{
    SocketManagerInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryId, instantiator);
    if (object.IsNull())
        return 0;
    return dynamic_cast<SocketManager*>(object.Get());
}

// Write-set membership. (Mac 0x116a00 inline, Win 0x53e170 / FD macros)
void SocketManager::WatchWrite(Socket* socket, bool watch)
{
    if (watch)
        Add(socket->m_socket, m_writeSet);
    else
        Remove(socket->m_socket, m_writeSet);
}

// Mac 0x116ba0, Win 0x53cfa0
// NOTE (both builds): an unknown protocol name and a failing socket() return 0 (not
// kInvalidSocket), which the caller then uses as a valid socket.
SocketHandle SocketManager::CreateHandle(UInt32 family, int type, const char* protocol, UInt32 flags)
{
    int number = 0;
    if (protocol && *protocol) {
        protoent* entry = getprotobyname(protocol);
        if (!entry)
            return 0;
        number = entry->p_proto;
    }
    SocketHandle handle = (SocketHandle)socket((int)family, type, number);
    if (handle == kInvalidSocket)
        return 0;
    int value = (int)(flags & 1);
    if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&value), 4) != -1) {
        value = (int)((flags >> 1) & 1);
        if (setsockopt(handle, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&value), 4) != -1)
            return handle;
    }
    CloseHandle(handle);
    return kInvalidSocket;
}

// Mac 0x116c70, Win 0x53dd70
bool SocketManager::Connect(Socket* socket, const SocketAddress& address, UInt16 port, UInt32 flags,
                            UInt32 protocol)
{
    const char* protocolName;
    int type;
    if (protocol == kTCP) {
        protocolName = "tcp";
        type = SOCK_STREAM;
    }
    else if (protocol == kUDP) {
        protocolName = "udp";
        type = SOCK_DGRAM;
    }
    else {
        return false;
    }
    sockaddr_storage native;
    SocketLength length = 0;
    if (!ToNative(address, port, native, length))
        return false;

    socket->m_socket = CreateHandle(address.m_family, type, protocolName, flags);
    if (socket->m_socket == kInvalidSocket)
        return false;
    if (SetNonBlocking(socket->m_socket)) {
        socket->m_time = Now();
        const int result = connect(socket->m_socket, reinterpret_cast<sockaddr*>(&native), length);
#if defined(_WIN32)
        const bool inProgress = result == -1 && WSAGetLastError() == WSAEWOULDBLOCK;
#else
        const bool inProgress = result == -1 && errno == EINPROGRESS;
#endif
        // NOTE (both builds): a connect that succeeds at once counts as a failure.
        if (inProgress) {
            socket->m_manager = this;
            socket->m_address = address;
            socket->m_port = port;
            socket->m_connecting = true;
            m_connecting.PushBack(socket);
            Remove(socket->m_socket, m_readSet);
            Add(socket->m_socket, m_writeSet);
            Add(socket->m_socket, m_exceptSet);
            return true;
        }
    }
    CloseHandle(socket->m_socket);
    socket->m_socket = kInvalidSocket;
    return false;
}

// Mac 0x117060, Win 0x53ded0
void SocketManager::Close(Socket* socket)
{
    RefPointer<SocketManager> self(this);
    Remove(socket->m_socket, m_readSet);
    Remove(socket->m_socket, m_writeSet);
    Remove(socket->m_socket, m_exceptSet);
    socket->m_manager = 0;
    CloseHandle(socket->m_socket);
    socket->m_socket = kInvalidSocket;
    if (!socket->m_connected) {
        RemoveSwap(m_connecting, socket);
        socket->m_connecting = false;
        socket->OnConnectFailed();
    }
    else {
        RemoveSwap(m_sockets, socket);
        socket->m_connected = false;
        socket->OnClosed();
    }
}

// Mac 0x117570, Win 0x53d610
void SocketManager::Update()
{
#if defined(_WIN32)
    if (m_readSet.m_count == 0 && m_writeSet.m_count == 0 && m_exceptSet.m_count == 0)
        return;
#endif
    RefPointer<SocketManager> self(this);
    SocketSet readSet = m_readSet;
    SocketSet writeSet = m_writeSet;
    SocketSet exceptSet = m_exceptSet;
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    int highest = 0;
    for (BasicArray<Socket*>::Iterator it = m_sockets.Begin(); it != m_sockets.End(); ++it)
        if (highest < (int)(*it)->m_socket)
            highest = (int)(*it)->m_socket;
    for (BasicArray<Socket*>::Iterator it = m_connecting.Begin(); it != m_connecting.End(); ++it)
        if (highest < (int)(*it)->m_socket)
            highest = (int)(*it)->m_socket;
    for (BasicArray<ListenSocket*>::Iterator it = m_listening.Begin(); it != m_listening.End(); ++it)
        if (highest < (int)(*it)->m_socket)
            highest = (int)(*it)->m_socket;

    const int ready = select(highest + 1, Native(readSet), Native(writeSet), Native(exceptSet), &timeout);
    if (ready > 0) {
        // Connected sockets (one removed while handled lets the next one move to its index).
        for (UInt32 i = 0; i < m_sockets.GetSize();) {
            Socket* socket = m_sockets[i];
            if (socket->m_socket != kInvalidSocket) {
                if (IsSet(socket->m_socket, readSet))
                    socket->OnReadable();
                if (socket->m_socket != kInvalidSocket && IsSet(socket->m_socket, writeSet))
                    socket->OnWritable();
                if (socket->m_socket != kInvalidSocket && IsSet(socket->m_socket, exceptSet))
                    socket->OnException();
            }
            if (i < m_sockets.GetSize() && socket == m_sockets[i])
                ++i;
        }
        // Listening sockets: accept one connection each.
        for (UInt32 i = 0; i < m_listening.GetSize();) {
            ListenSocket* listener = m_listening[i];
            if (IsSet(listener->m_socket, readSet)) {
                sockaddr_storage peer;
                SocketLength length = 0x1c;
                SocketHandle accepted = (SocketHandle)accept(listener->m_socket, reinterpret_cast<sockaddr*>(&peer), &length);
                if (accepted != kInvalidSocket) {
                    if (!SetNonBlocking(accepted)) {
                        CloseHandle(accepted);
                    }
                    else {
                        RefPointer<Socket> socket = listener->CreateSocket();
                        if (socket.IsNull()) {
                            CloseHandle(accepted);
                        }
                        else {
                            socket->m_manager = this;
                            socket->m_socket = accepted;
                            FromNative(peer, socket->m_address, socket->m_port);
                            socket->m_client = false;
                            socket->m_connected = true;
                            socket->m_time = Now();
                            m_sockets.PushBack(socket.Get());
                            Add(accepted, m_readSet);
                            Remove(accepted, m_writeSet);
                            Remove(accepted, m_exceptSet);
                            socket->OnAccepted();
                        }
                    }
                }
            }
            if (i < m_listening.GetSize() && listener == m_listening[i])
                ++i;
        }
    }

    // Connecting sockets: writable = connected (SO_ERROR 0) or failed; exception = failed;
    // otherwise the timeout.
    for (UInt32 i = 0; i < m_connecting.GetSize();) {
        Socket* socket = m_connecting[i];
        const SocketHandle handle = socket->m_socket;
        if (IsSet(handle, writeSet)) {
            int error = 0;
            SocketLength length = 4;
            getsockopt(handle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &length);
            if (error == 0) {
                socket->m_client = true;
                socket->m_connecting = false;
                socket->m_connected = true;
                socket->m_time = Now();
                m_sockets.PushBack(socket);
                RemoveSwap(m_connecting, socket);
                Add(handle, m_readSet);
                Remove(handle, m_writeSet);
                Remove(handle, m_exceptSet);
                socket->OnConnected();
            }
            else {
                Close(socket);
            }
        }
        else if (IsSet(handle, exceptSet)) {
            Close(socket);
        }
        else if (IsExpired(socket->m_time, socket->GetTimeout())) {
            Close(socket);
        }
        if (i < m_connecting.GetSize() && socket == m_connecting[i])
            ++i;
    }
}

// ===========================================================================================
// SocketManagerInstantiator
// ===========================================================================================

// Mac 0x1189c0, Win 0x53d180
RefPointer<Object> SocketManagerInstantiator::CreateObject() const
{
#if defined(_WIN32)
    WSADATA data;
    if (WSAStartup(2, &data) != 0)
        return RefPointer<Object>();
#endif
    SocketManager* manager = new SocketManager();
    return RefPointer<Object>(manager);
}

} // namespace LEGO
