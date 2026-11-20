// LEGO Digital Designer — restored source
// TU 224 "NewContentCaller.cpp" (Mac 0x268420-0x26916f; Win: the destructor 0x693e6b only
// located). See NewContentChecker.h. Also emitted here (Mac): the destructors 0x268e20 /
// 0x268e80 and their thunks.
#include "NewContentChecker.h"

#include "Application.h"
#include "Platform.h"
#include "Preferences.h"
#include "TextLocale.h"
#include "LEGO/StdLib.h"
#include "XMLDOMNode.h"

#include <iostream>

namespace LEGO {

// Mac 0x268420
void NewContentCaller::OnResponse(UInt32 size)
{
    RefPointer<XMLDOM> dom = new XMLDOM();
    if (!ParseReply(size, dom.Get())) {
        SetFailed();
        return;
    }
    if (!GetChildValue(dom.Get(), String("applicationVersionMajor"), m_latestVersion.m_major)
        || !GetChildValue(dom.Get(), String("applicationVersionMinor"), m_latestVersion.m_minor)
        || !GetChildValue(dom.Get(), String("applicationVersionProduct"), m_latestVersion.m_build)) {
        SetFailed();
        return;
    }
    String text;
    if (!GetChildText(dom.Get(), String("collections"), text)) {
        SetFailed();
        return;
    }
    m_collections = text;
    RefPointer<XMLDOMNode> configURLs = dom->FindChild(String("configURLs"));
    if (configURLs)
        m_configURLs = configURLs;
    SetSucceeded();
}

// Mac 0x268740 (the body of InitializeDesignerCaller::Call, Mac 0x1b4cd0)
void NewContentCaller::Call()
{
    AString data;
    String id;
    Preferences::GetInstance()->GetValue(String("ApplicationID"), id);
    data.Append("id=").Append(id.ToAString());

    data.Append("&locale=");
    {
        RefPointer<TextLocale> locale = TextLocale::GetInstance();
        data.Append(locale->GetLanguage().ToAString());
    }

    data.Append("&platform=").Append(Platform::GetInstance()->GetWebPlatformName().ToAString());

    data.Append("&applicationName=").Append(Application::GetInstance()->GetName().ToAString());

    Version version;
    Application::GetInstance()->GetVersion(version);
    char number[128];
    data.Append("&applicationVersionMajor=");
    UInt32ToStr(version.m_major, number, 10);
    data.Append(number);
    data.Append("&applicationVersionMinor=");
    UInt32ToStr(version.m_minor, number, 10);
    data.Append(number);
    data.Append("&applicationVersionProduct=");
    UInt32ToStr(version.m_build, number, 10);
    data.Append(number);
    // NOTE: the collections are never filled in (as InitializeDesignerCaller::Call).
    data.Append("&collections=");

    String url;
    Preferences::GetInstance()->GetValue(String("initialiseDesignerURL"), url);
    Request(url, data);
}

} // namespace LEGO
