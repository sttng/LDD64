// LEGO Digital Designer — restored source
// LEGO::NewContentCaller — the "new content" web call: the same POST as InitializeDesigner
//   (preference "initialiseDesignerURL"); the reply's latest LDD version, collections and
//   configURLs are kept. TU 224 "NewContentCaller.cpp" (Mac 0x268420-0x26916f).
// LEGO::NewContentCallerHandler / NewContentCallerDelegator — forwards the call's events to a
//   handler (the checker). TU 226.
// LEGO::NewContentChecker ('NIDC' in the ObjectRegistry) — checks the LEGO servers for new
//   content: a newer LDD (status 1); otherwise the brick version list (BrickVersion.xml) newer
//   than the given brick database version (status 3) or none (0); failures report 4. Users:
//   UploadToFactoryCommand (TU 262, through ObserverN<NewContentChecker, …>).
//   TU 226 "NewContentChecker.cpp" (Mac 0x269c10-0x26bafb, Win 0x693c2d-0x69480f).
//
// RTTI (Mac): NewContentChecker : NewContentCheckerNotifier (= Notifier<NewContentChecker-
// Observer>, +0), NewContentCallerHandler (+0x24), BrickVersionDownloaderHandler (+0x28),
// virtual Object (+0x44, sizeof 0x4c). Mac vtable 0x73797c: D1 0x26abe0, D0 0x26ac40,
// OnFinalRelease 2 0x26acb0, then the handler slots 3-9 (below).
// Every method and member name is inferred except the class names (RTTI) and the strings.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Version.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "ObjectRegistry.h"
#include "LDDWebObserver.h"
#include "BrickVersionDownloaderCaller.h"
#include "XMLDOMNode.h"

namespace LEGO {

class XMLDOMNode;
class NewContentChecker;

// Mac sizeof 0x70 (Object +0x68). Mac vtable 0x77192c {…, OnResponse 6 0x268420, Call 14
// 0x268740}; inline constructor (copy in Mac 0x269cb0), Win sdd 0x693e6b.
class NewContentCaller : public LDDWebObserver {
public:
    NewContentCaller() : LDDWebObserver(true, 30000) {}
    virtual ~NewContentCaller() {}                                           // Mac 0x268e20/0x268e80

    // <applicationVersionMajor/Minor/Product>, <collections> (all required), <configURLs>
    // (optional). Mac 0x268420
    virtual void OnResponse(UInt32 size);
    // The InitializeDesigner request (see InitializeDesignerCaller::Call). Mac 0x268740
    virtual void Call();                                                     // 14

    const String&  GetCollections() const   { return m_collections; }
    const Version& GetLatestVersion() const { return m_latestVersion; }

protected:
    String                 m_collections;     // Mac +0x44 "name|version,…"
    RefPointer<XMLDOMNode> m_configURLs;      // Mac +0x54
    Version                m_latestVersion;   // Mac +0x58 (major, minor, product)
                                              // Object +0x68
};

// (RTTI class; Mac vtable 3 slots, no destructor)
class NewContentCallerHandler {
public:
    virtual void OnNewContentProgress(UInt32 received, UInt32 total) = 0;              // 0
    // `size`: the reply's size; `collections`: the reply's <collections>.
    virtual void OnNewContentSucceeded(UInt32 size, const String& collections) = 0;    // 1
    virtual void OnNewContentFailed() = 0;                                             // 2

protected:
    ~NewContentCallerHandler() {}
};

// Mac sizeof 0x74: NewContentCaller, handler +0x68, Object +0x6c. Mac vtable 0x771834.
class NewContentCallerDelegator : public NewContentCaller {
public:
    explicit NewContentCallerDelegator(NewContentCallerHandler* handler) : m_handler(handler) {}
    virtual ~NewContentCallerDelegator() {}                                  // Mac 0x26b040/0x26b0a0, Win sdd 0x69404f

    // Mac 0x26b110, Win 0x693f0c
    virtual void OnResponse(UInt32 size);
    // Mac 0x26b160, Win 0x693f40
    virtual void OnProgress(UInt32 received, UInt32 total);
    // Mac 0x26b1a0, Win 0x693f58
    virtual void OnFailed();

protected:
    NewContentCallerHandler* m_handler;   // Mac +0x68
};

// The observers' interface: slot 5 gets the result. (ObserverN: one observer may watch several
// checkers.) Mac vtable: ObserverN's 0-4, 5 pure.
class NewContentCheckerObserver : public ObserverN<NewContentChecker, NewContentCheckerObserver> {
public:
    // Results of NewContentChecker. (names inferred)
    enum Status {
        kNoNewContent = 0,
        kNewApplication = 1,
        kNewBricks = 3,
        kFailed = 4
    };

    NewContentCheckerObserver() {}
    virtual ~NewContentCheckerObserver() {}

    virtual void OnNewContentChecked(UInt32 status) = 0;                     // 5
};

// RTTI LEGO::NewContentCheckerNotifier (destructor Mac 0x26a520 / 0x26a590, Win sdd 0x694547).
class NewContentCheckerNotifier : public Notifier<NewContentCheckerObserver> {
public:
    NewContentCheckerNotifier() {}
    virtual ~NewContentCheckerNotifier() {}
};

class NewContentChecker : public NewContentCheckerNotifier, public NewContentCallerHandler,
                          public BrickVersionDownloaderHandler {
public:
    enum { kRegistryId = 0x4e494443 };   // 'NIDC'

    NewContentChecker() {}
    virtual ~NewContentChecker() {}                                          // Mac 0x26abe0/0x26ac40, Win sdd 0x694566

    // The registered checker (created on first use). Mac 0x269c10
    static NewContentChecker* GetInstance();

    // Starts a check against the brick database version `brickVersion`: the NewContent call.
    // Mac 0x269cb0
    void Check(const Version& brickVersion);

    // ---- NewContentCallerHandler ----
    virtual void OnNewContentProgress(UInt32, UInt32) {}                     // 3 Mac 0x26acf0
    // A newer LDD: status 1; else the brick version list is downloaded. Mac 0x269e20 (thunk
    // 0x26a310)
    virtual void OnNewContentSucceeded(UInt32 size, const String& collections);  // 4
    // Status 4. Mac 0x26a330 (thunk 0x26a350)
    virtual void OnNewContentFailed();                                       // 5

    // ---- BrickVersionDownloaderHandler ----
    virtual void OnBrickVersionsProgress(UInt32, UInt32) {}                  // 6 Mac 0x26ad00
    virtual void OnBrickVersionsFileProgress(UInt32, UInt32) {}              // 7 Mac 0x26ad10
    // Newer brick versions: status 3, none: 0. NOTE (both builds): nothing is reported when
    // the file cannot be opened or parsed. Mac 0x26a370 (thunk 0x26a4c0), Win 0x694742 (?)
    virtual void OnBrickVersionsDownloaded(UInt32 files, const Path& file);  // 8
    // Status 4. Mac 0x26a4e0
    virtual void OnBrickVersionsFailed();                                    // 9

protected:
    // Mac 0x26acb0
    virtual void OnFinalRelease();

    // Every observer gets `status`. Mac 0x26a090, Win 0x6940ca
    void Notify(UInt32 status);

    RefPointer<NewContentCallerDelegator>    m_caller;               // Mac +0x2c
    RefPointer<BrickVersionDownloaderCaller> m_brickVersionCaller;   // Mac +0x30
    Version                                  m_brickVersion;         // Mac +0x34
                                                                     // Object +0x44
};

class NewContentCheckerInstantiator : public ObjectRegistryInstantiator {
public:
    NewContentCheckerInstantiator() {}
    virtual ~NewContentCheckerInstantiator() {}                              // Mac 0x269ca0/0x26ae80, Win 0x693c52
    virtual RefPointer<Object> CreateObject() const;                         // Mac 0x26ae90, Win 0x693c2d (?)
};

} // namespace LEGO
