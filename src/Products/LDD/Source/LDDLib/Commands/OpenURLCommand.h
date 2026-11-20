// LEGO Digital Designer — restored source
// LEGO::OpenURLCommand — opens a URL in the web browser (BrickModeGlobalObject's
//   "openPrivacyPolicyCommand": "http://lego.com/go/7/privacy").
//
// Home TU 248 "OpenURLCommand.cpp" (Mac 0x2730a0-0x27337f, Win 0x630716 / 0x684c89): Do and the
// destructor in OpenURLCommand.cpp. The constructor is header-inline (Mac
// 0x182bf0 emitted in TU 084, Win 0x630310).
// Mac layout: Command +0x00..+0x28, String url +0x28, Object +0x38 (sizeof 0x40). Win: url
// +0x2c, Object +0x30.
// Vtables: Mac 0x73990c {D1 0x2730c0, D0 0x273120, OnFinalRelease 0x1414d0, Do 0x2730a0,
// SetEnable, GetEnable, UpdateEnable 0x3730c0}; Win 0x9d2a44 {Do 0x684c89, …}.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Command.h"

namespace LEGO {

class OpenURLCommand : public Command {
public:
    // (Mac 0x182bf0, Win 0x630310)
    OpenURLCommand(const String& url, bool enabled) : Command(enabled), m_url(url) {}
    virtual ~OpenURLCommand();                                               // Mac 0x2730c0/0x273120, Win sdd 0x630716

    // Platform::OpenURL(url).
    virtual void Do();                                                       // Mac 0x2730a0, Win 0x684c89

    const String& GetURL() const { return m_url; }

protected:
    String m_url;   // Mac +0x28, Win +0x2c
};

} // namespace LEGO
