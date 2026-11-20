// LEGO Digital Designer — restored source
// LEGO::LDDWebObserver — base of the LDD web-service calls (InitializeDesignerCaller,
//   FileDownloader, NewContentCaller, GetRedirectURLCaller, UploadModelCaller): runs one request
//   over its own HTTPCommunicator and watches it with a timeout Trigger (started when the
//   connection is up, restarted by every progress). A redirect starts the same request on the
//   new URL; a timeout closes the socket (the communicator then reports OnFailed).
//   Home TU 059 "LDDWebCommunicator.cpp" (Mac 0x1354c0-0x136b30, Win 0x632263 / 0x6338e1 /
//   0x6348d2-0x634e8b / 0x63499a).
//
// RTTI (Mac): vmi {HTTPCommunicatorObserver +0, Trigger +8}, virtual Object; sizeof 0x4c
// (Object +0x44). Mac vtable 0x70c5b4 (14 slots): D1 0x135ee0, D0 0x136000, OnFinalRelease
// 0x136b30, OnAttached 0x136db0, OnDetached 0x136dc0, OnConnected 5 0x1355f0, OnResponse 6
// (pure), OnProgress 7 0x135770, OnFailed 8 0x1357d0, OnRedirect 9 0x135870, IsDone 10 0x1354c0,
// HasError 11 0x1354d0, GetProgress 12 0x1354e0, OnTrigger 13 0x136ab0 (Trigger-in copy 0x136af0).
// Trigger-in vtable 0x70c5f8 {thunks 0x135f40, 0x136070, 0x136c10, OnAttached, OnDetached,
// Trigger::Update 0x114180, OnTrigger 0x136af0}.
// Win: OnConnected 0x6348d2, OnProgress 0x634959, OnFailed 0x634984, OnRedirect 0x634e8b, IsDone
// 0x632263, sdd 0x6334f5; Request 0x63499a.
// Every method and member name is inferred except the class names (RTTI) and the strings.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "Trigger.h"
#include "HTTPCommunicator.h"

namespace LEGO {

class XMLDOM;
class XMLDOMNode;

class LDDWebObserver : public HTTPCommunicatorObserver, public Trigger {
public:
    // Bytes received / expected of the reply (GetProgress, Mac +0x30). (name inferred)
    struct Progress {
        UInt32 m_received;   // +0x00
        UInt32 m_total;      // +0x04
    };

    // `post`: the request is a POST of the data given to Request (else a GET); `timeout` in
    // milliseconds (the callers: 30000). Mac 0x1354f0 (C2 with VTT)
    LDDWebObserver(bool post, UInt32 timeout)
        : m_post(post), m_done(false), m_failed(false), m_timeout(timeout)
    {
        m_progress.m_received = 0;
        m_progress.m_total = 0;
    }
    virtual ~LDDWebObserver() {}                                             // Mac 0x135ee0/0x136000 (body 0x135de0), Win sdd 0x6334f5

    // ---- HTTPCommunicatorObserver ----
    // Starts the timeout and sends the GET of the path / the POST of the data.
    // Mac 0x1355f0, Win 0x6348d2
    virtual void OnConnected();
    virtual void OnResponse(UInt32 size) = 0;                                // 6
    // Restarts the timeout and keeps the numbers. Mac 0x135770, Win 0x634959
    virtual void OnProgress(UInt32 received, UInt32 total);
    // Unless already failed: done and failed, the timeout stops. Mac 0x1357d0, Win 0x634984
    virtual void OnFailed();
    // Drops the communicator (released after the update pass) and requests `url` with the
    // same data. Mac 0x135870, Win 0x634e8b
    virtual void OnRedirect(const String& url);

    // ---- state ----
    virtual bool IsDone() const { return m_done; }                           // 10 Mac 0x1354c0, Win 0x632263
    virtual bool HasError() const { return m_failed; }                       // 11 Mac 0x1354d0
    virtual const Progress& GetProgress() const { return m_progress; }       // 12 Mac 0x1354e0

    // ---- Trigger ----
    // Timeout: closes the communicator's socket (which fails the call). Mac 0x136ab0
    // (Trigger-in copy 0x136af0). NOTE: no check for a missing communicator (both builds).
    virtual void OnTrigger();

protected:
    // Splits `url`, creates the communicator (User-Agent "<application name>/<version>"),
    // observes it and connects; the data are kept for the POST. Fails at once (OnFailed)
    // when the URL does not parse or the connection cannot be started.
    // Mac 0x1358d0, Win 0x63499a
    void Request(const String& url, const AString& data);

    // Done without error / with error; both stop the timeout. Mac 0x136770 / 0x135820
    void SetSucceeded();
    void SetFailed();

    // The reply (`size` bytes) parsed into `dom`. Mac 0x1367c0, Win 0x6338e1
    bool ParseReply(UInt32 size, XMLDOM* dom);
    // The text of the child `name` of `node` (empty and false without one).
    // Mac 0x1368e0, Win 0x632a95
    bool GetChildText(const XMLDOMNode* node, const String& name, String& text) const;
    // … parsed as a number / a boolean. Mac 0x1369b0 / 0x136a30, Win 0x632b26 / 0x632b80
    bool GetChildValue(const XMLDOMNode* node, const String& name, UInt32& value) const;
    bool GetChildValue(const XMLDOMNode* node, const String& name, bool& value) const;

    // Unobserved from the end of OnFinalRelease (Trigger part first). Mac 0x136b30
    virtual void OnFinalRelease();

    // Mac / Win offsets (HTTPCommunicatorObserver +0x00, Trigger +0x08)
    String                       m_path;           // +0x18 the request path ("/…" or empty)
    AString                      m_data;           // +0x28 POST data
    bool                         m_post;           // +0x2c
    Progress                     m_progress;       // +0x30
    bool                         m_done;           // +0x38
    bool                         m_failed;         // +0x39
    UInt32                       m_timeout;        // +0x3c ms
    RefPointer<HTTPCommunicator> m_communicator;   // +0x40
                                                   // Object +0x44
};

} // namespace LEGO
