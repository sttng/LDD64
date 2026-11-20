// LEGO Digital Designer — restored source
// TU 100 "InitializeDesignerCaller.cpp" (Mac 0x1b4600-0x1b53ef; Win 0x687039, 0x687470): the
// reply parser and the request of InitializeDesignerCaller (the class's vtable, destructor and
// inline constructor are emitted in StartMode.cpp). Static initialiser Mac 0x1b53b0: <iostream>.
#include "InitializeDesignerCaller.h"

#include "Application.h"
#include "Platform.h"
#include "Preferences.h"
#include "TextLocale.h"
#include "LEGO/StdLib.h"
#include "XMLDOMNode.h"

#include <iostream>

namespace LEGO {

// Mac 0x1b4600, Win 0x687039
void InitializeDesignerCaller::OnResponse(UInt32 size)
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
    if (!GetChildText(dom.Get(), String("collectionURL"), text)) {
        SetFailed();
        return;
    }
    m_data.m_collectionURL = text;
    if (!GetChildText(dom.Get(), String("collections"), text)) {
        SetFailed();
        return;
    }
    m_data.m_collections = text;
    if (!GetChildText(dom.Get(), String("geometryURL"), text)) {
        SetFailed();
        return;
    }
    m_data.m_geometryURL = text;
    if (!GetChildText(dom.Get(), String("installerURL_PC"), text)) {
        SetFailed();
        return;
    }
    m_data.m_installerURLPC = text;
    if (!GetChildText(dom.Get(), String("installerURL_MAC"), text)) {
        SetFailed();
        return;
    }
    m_data.m_installerURLMac = text;
    if (!GetChildText(dom.Get(), String("shipToCountries"), text)) {
        SetFailed();
        return;
    }
    m_data.m_shipToCountries = text;
    if (!GetChildValue(dom.Get(), String("generateBGBeforeUpload"), m_data.m_generateBGBeforeUpload))
        m_data.m_generateBGBeforeUpload = true;

    RefPointer<XMLDOMNode> configURLs = dom->FindChild(String("configURLs"));
    if (configURLs)
        m_data.m_configURLs = configURLs;
    RefPointer<XMLDOMNode> countryCurrency = dom->FindChild(String("countryCurrency"));
    if (countryCurrency)
        m_data.m_countryCurrency = countryCurrency;
    SetSucceeded();
}

// Mac 0x1b4cd0, Win 0x687470
void InitializeDesignerCaller::Call()
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
    // NOTE (both builds): the collections are never filled in.
    data.Append("&collections=");

    String url;
    Preferences::GetInstance()->GetValue(String("initialiseDesignerURL"), url);
    Request(url, data);
}

} // namespace LEGO
