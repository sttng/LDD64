// LEGO Digital Designer — restored source
// LEGO::InitializeDesignerCaller — the "InitializeDesigner" web-service call of the start
//   mode: POSTs the application identity (id, locale, platform, name, version) to the URL of the
//   preference "initialiseDesignerURL" and parses the XML reply (latest LDD version, service
//   URLs, collections, countries …) into its data.
//   Home TU 100 "InitializeDesignerCaller.cpp" (Mac 0x1b4600-0x1b53ef, Win 0x687039 / 0x687470);
//   its vtable, destructor and inline constructor are emitted in StartMode.cpp (TU 087).
// LEGO::InitializeDesignerCallerHandler — the interface StartMode implements for it (RTTI
//   class; Mac vtable 3 slots, no destructor).
//
// Mac layout: LDDWebObserver part +0x00..+0x44, data +0x44 (0x6c bytes; ctor Mac 0x1a4fa0, dtor
// 0x1a5070), latest version +0xb0, Object +0xc0 (sizeof 0xc8). Win: data +0x00 of the caller's
// own part (generateBG +0x60), version +0x6c.
// Mac vtable 0x71d3dc (15 slots): LDDWebObserver's with OnResponse 6 0x1b4600 and Call 14
// 0x1b4cd0; Win {…, OnResponse 0x687039, Call 0x687470 (0x687420)}.
// Member names are inferred; the field names follow the reply's element names.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Version.h"
#include "XMLDOMNode.h"
#include "LDDWebObserver.h"

namespace LEGO {

// The reply of InitializeDesigner (Mac 0x6c bytes), handed to LDDWebCommunicator.
struct InitializeDesignerData {
    // Mac 0x1a4fa0. NOTE: the original leaves m_generateBGBeforeUpload uninitialised (the
    // parser always sets it).
    InitializeDesignerData() : m_generateBGBeforeUpload(false) {}

    String                  m_collectionURL;            // +0x00 <collectionURL>
    String                  m_collections;              // +0x10 <collections> "name|version,…"
    String                  m_geometryURL;              // +0x20 <geometryURL>
    String                  m_installerURLPC;           // +0x30 <installerURL_PC>
    String                  m_installerURLMac;          // +0x40 <installerURL_MAC>
    String                  m_shipToCountries;          // +0x50 <shipToCountries> "name|code[|default],…"
    bool                    m_generateBGBeforeUpload;   // +0x60 <generateBGBeforeUpload> (missing: true)
    RefPointer<XMLDOMNode>  m_configURLs;               // +0x64 <configURLs>
    RefPointer<XMLDOMNode>  m_countryCurrency;          // +0x68 <countryCurrency>
};

class InitializeDesignerCallerHandler {
public:
    // (Mac slot / Win slot; names inferred from StartMode's overriders)
    virtual void OnInitializeDesignerProgress(UInt32 received, UInt32 total) = 0;      // 0/0
    // `size`: the reply's size (OnResponse).
    virtual void OnInitializeDesignerSucceeded(UInt32 size,
                                               const InitializeDesignerData& data) = 0; // 1/1
    virtual void OnInitializeDesignerFailed() = 0;                                     // 2/2

protected:
    ~InitializeDesignerCallerHandler() {}
};

class InitializeDesignerCaller : public LDDWebObserver {
public:
    // Inline (Mac copy in StartMode 0x19b800): LDDWebObserver(true, 30000), empty data.
    InitializeDesignerCaller() : LDDWebObserver(true, 30000) {}
    virtual ~InitializeDesignerCaller() {}                                   // Mac 0x1a4970/0x1a4a10 (inline, emitted in TU 087), Win sdd 0x6438aa

    // Parses the reply: <applicationVersionMajor/Minor/Product> into the latest version, then
    // the data; the first missing element fails the call (generateBGBeforeUpload defaults to
    // true, configURLs / countryCurrency are optional). Mac 0x1b4600, Win 0x687039
    virtual void OnResponse(UInt32 size);                                    // 6
    // POSTs "id=<ApplicationID>&locale=<language>&platform=<mac|win>&applicationName=<name>
    // &applicationVersionMajor=…&applicationVersionMinor=…&applicationVersionProduct=…
    // &collections=" to the preference "initialiseDesignerURL". Mac 0x1b4cd0, Win 0x687470
    virtual void Call();                                                     // 14

    const InitializeDesignerData& GetData() const { return m_data; }
    // The newest LDD version known to the server (the patch check). (name inferred)
    const Version& GetLatestVersion() const { return m_latestVersion; }

protected:
    InitializeDesignerData m_data;            // Mac +0x44
    Version                m_latestVersion;   // Mac +0xb0 / Win +0x6c (major, minor, product)
};

} // namespace LEGO
