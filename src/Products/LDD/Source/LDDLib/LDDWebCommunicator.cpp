// LEGO Digital Designer — restored source
// TU 059 "LDDWebCommunicator.cpp" (Mac 0x133720-0x138d3b; Win 0x632141-0x6357d2, scattered):
// LDDWebCommunicator (+ instantiator) and LDDWebObserver.
// See LDDWebCommunicator.h / LDDWebObserver.h.
//
// Also emitted in this object on Mac (restored elsewhere, from headers): the inline copies of
// TCPSocketNotifier (0x137840-0x138710) and the HTTPCommunicator constructor / Connect (0x136160,
// 0x136370), Observer/Notifier instances for HTTPCommunicator and TCPSocketNotifier, the
// destructors of PaletteNames (0x1337f0 / 0x136ff0), Countries (0x133750 / 0x137120) and their
// BasicSet / BasicArray bases, the map helpers of BrandURLMap (0x1388a0, 0x138900).
// Static initialiser Mac 0x138c90: <iostream> and the whitespace set below.
#include "LDDWebCommunicator.h"
#include "LDDWebObserver.h"
#include "InitializeDesignerCaller.h"

#include "LDDApplication.h"
#include "Application.h"
#include "Preferences.h"
#include "StringStream.h"
#include "XMLDOMNode.h"
#include "Updater.h"
#include "Version.h"

#include <iostream>

namespace LEGO {

namespace {

// The characters trimmed from the right of the per-brand URLs (Mac 0x77ae58, initialised in
// 0x138c90). (name inferred)
const String kWhitespace("\t\r\n ");

const UInt32 npos = 0xFFFFFFFF;

// Appends '/' unless `url` already ends with one (inlined twice in Mac 0x133fe0).
void EnsureTrailingSlash(String& url)
{
    if (!url.EndsWith((UniChar16)'/'))
        url.Append((UniChar16)'/');
}

} // namespace

// ===========================================================================================
// LDDWebObserver
// ===========================================================================================

// Mac 0x1355f0, Win 0x6348d2
void LDDWebObserver::OnConnected()
{
    Trigger::Start(m_timeout);
    HTTPCommunicator* communicator = m_communicator.Get();
    if (!m_post)
        communicator->SendGet(m_path.ToAString());
    else
        communicator->SendPost(m_path.ToAString(), m_data, AString());
}

// Mac 0x135770, Win 0x634959
void LDDWebObserver::OnProgress(UInt32 received, UInt32 total)
{
    Trigger::Stop();
    Trigger::Start(m_timeout);
    m_progress.m_total = total;
    m_progress.m_received = received;
}

// Mac 0x1357d0, Win 0x634984
void LDDWebObserver::OnFailed()
{
    if (HasError())
        return;
    Trigger::Stop();
    m_done = true;
    m_failed = true;
}

// Mac 0x135870, Win 0x634e8b
void LDDWebObserver::OnRedirect(const String& url)
{
    MainUpdater::ReleaseLater(m_communicator.Get());
    m_communicator = 0;
    Request(url, m_data);
}

// Mac 0x136ab0 (Trigger-in copy 0x136af0)
void LDDWebObserver::OnTrigger()
{
    HTTPCommunicator* communicator = m_communicator.Get();
    if (communicator->GetSocket()) {
        communicator->GetSocket()->Close();
        communicator->ResetSocket();
    }
}

// Mac 0x1358d0, Win 0x63499a
void LDDWebObserver::Request(const String& url, const AString& data)
{
    String host;
    UInt32 port = 0;
    if (!HTTPCommunicator::ParseURL(url, host, port, m_path)) {
        OnFailed();
        return;
    }
    Application* application = Application::GetInstance();
    String userAgent(application->GetName());
    userAgent << "/";
    Version version;
    application->GetVersion(version);
    ToString(version, userAgent);

    m_communicator = new HTTPCommunicator(userAgent.ToAString());
    HTTPCommunicatorObserver::Attach(m_communicator.Get());
    if (!m_communicator->Connect(host.ToAString(), (UInt16)port))
        OnFailed();
    else
        m_data = data;
}

// Mac 0x136770
void LDDWebObserver::SetSucceeded()
{
    Trigger::Stop();
    m_done = true;
    m_failed = false;
}

// Mac 0x135820
void LDDWebObserver::SetFailed()
{
    Trigger::Stop();
    m_done = true;
    m_failed = true;
}

// Mac 0x1367c0, Win 0x6338e1
bool LDDWebObserver::ParseReply(UInt32 size, XMLDOM* dom)
{
    // NOTE: size + 1 wraps for a reply size of 0xFFFFFFFF (both builds); fixed: a reply over
    // 256 MB is refused (H20; HTTPCommunicator refuses such replies too).
    if (size > 0x10000000u)
        return false;
    char* buffer = new char[size + 1];
    bool result = false;
    if (m_communicator->GetContent(buffer)) {
        buffer[size] = 0;
        AString text(buffer);   // up to the first zero byte (StrLen)
        result = dom->Parse(text);
    }
    delete[] buffer;
    return result;
}

// Mac 0x1368e0, Win 0x632a95
bool LDDWebObserver::GetChildText(const XMLDOMNode* node, const String& name, String& text) const
{
    RefPointer<XMLDOMNode> child = node->FindChild(name);
    if (child.IsNull()) {
        text = String();
        return false;
    }
    text = child->GetText();
    return true;
}

// Mac 0x1369b0, Win 0x632b26
bool LDDWebObserver::GetChildValue(const XMLDOMNode* node, const String& name, UInt32& value) const
{
    String text;
    if (!GetChildText(node, name, text))
        return false;
    return FromString(value, text);
}

// Mac 0x136a30, Win 0x632b80
bool LDDWebObserver::GetChildValue(const XMLDOMNode* node, const String& name, bool& value) const
{
    String text;
    if (!GetChildText(node, name, text))
        return false;
    return FromString(value, text);
}

// Mac 0x136b30 (thunks 0x136c10 / 0x136ca0)
void LDDWebObserver::OnFinalRelease()
{
    Trigger::OnFinalRelease();
    HTTPCommunicatorObserver::OnFinalRelease();
}

// ===========================================================================================
// LDDWebCommunicator
// ===========================================================================================

// Mac 0x133820, Win 0x635457
LDDWebCommunicator::LDDWebCommunicator()
    : m_generateBGBeforeUpload(true), m_defaultCountryIndex(0)
{
    Preferences::GetInstance()->GetValue(String("BrickPricesFileURL"), m_brickPricesFileURL);
}

// Mac 0x133d80/0x133e50 (body 0x133b60), Win sdd 0x635783
LDDWebCommunicator::~LDDWebCommunicator()
{
}

// Mac 0x133f40, Win 0x634f92
LDDWebCommunicator* LDDWebCommunicator::GetInstance()
{
    LDDWebCommunicatorInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryId, instantiator);
    if (object.IsNull())
        return 0;
    return dynamic_cast<LDDWebCommunicator*>(object.Get());
}

// Mac 0x133fe0, Win 0x635667
void LDDWebCommunicator::SetInitializeDesignerData(const InitializeDesignerData& data)
{
    m_collectionURL = data.m_collectionURL;
    EnsureTrailingSlash(m_collectionURL);
    SetCollections(data.m_collections);
    m_geometryURL = data.m_geometryURL;
    EnsureTrailingSlash(m_geometryURL);
    m_installerURLPC = data.m_installerURLPC;
    m_installerURLMac = data.m_installerURLMac;
    SetCountries(data.m_shipToCountries);
    m_generateBGBeforeUpload = data.m_generateBGBeforeUpload;
    if (data.m_configURLs)
        SetConfigURLs(data.m_configURLs);
    if (data.m_countryCurrency)
        SetCountryCurrencies(data.m_countryCurrency);
}

// Mac 0x1340e0
void LDDWebCommunicator::SetCollections(const String& text)
{
    m_collections.Clear();
    BasicArray<String> tokens;
    text.Split((UniChar16)',', tokens);
    for (BasicArray<String>::ConstIterator it = tokens.Begin(); it != tokens.End(); ++it) {
        const UInt32 bar = it->Find((UniChar16)'|', 0);
        Version version;
        if (bar != npos) {
            const String number = it->Substring(bar + 1);
            UInt32 position = 0;
            if (!FromString(version, number, position))
                version = Version();
        }
        m_collections.Insert(PaletteName(it->Substring(0, bar), version));
    }
}

// Mac 0x134340, Win 0x6333b5
void LDDWebCommunicator::SetCountries(const String& text)
{
    m_countries.Clear();
    m_defaultCountryIndex = 0;
    BasicArray<String> tokens;
    text.Split((UniChar16)',', tokens);
    for (BasicArray<String>::ConstIterator it = tokens.Begin(); it != tokens.End(); ++it) {
        if (it->IsEmpty())
            continue;
        BasicArray<String> parts;
        it->Split((UniChar16)'|', parts);
        if (parts.GetSize() < 2) {
            String error("Wrong country token: ");
            error << *it;   // NOTE: built and dropped (both builds)
            continue;
        }
        if (parts.GetSize() != 2)
            m_defaultCountryIndex = m_countries.GetSize();
        Country country;
        country.m_code = parts[1];
        country.m_name = parts[0];
        country.m_extra = String();
        m_countries.PushBack(country);
    }
}

// Mac 0x1346e0, Win 0x63503f
void LDDWebCommunicator::SetConfigURLs(const XMLDOMNode* node)
{
    m_uploadModelURLs.Clear();
    m_onlineHelpURLs.Clear();
    m_checkPriceURLs.Clear();
    // NOTE (both builds): the brick-prices map is not cleared.
    // NOTE: the first child is taken without checking that there is one (both builds): an empty
    // <configURLs/> from the server crashes the start-up; fixed: then there are no URLs, as
    // offline (H21).
    if (node->GetChildren().IsEmpty())
        return;
    RefPointer<XMLDOMNode> brands = node->GetChildren()[0];
    const XMLDOMNode::NodeArray& children = brands->GetChildren();
    for (XMLDOMNode::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        RefPointer<XMLDOMNode> entry = *it;
        String name;
        if (!entry->GetAttribute(String("name"), name))
            continue;
        // NOTE: the original reads an uninitialised brand when the name does not parse; an
        // out-of-range value stands in for it here.
        Brand brand = (Brand)0xFFFFFFFF;
        UInt32 position = 0;
        if (!FromString(brand, name, position)) {
            String error("Unknown brand from initializeDesigner: ");
            error << name;   // NOTE: built and dropped
        }
        if (RefPointer<XMLDOMNode> value = entry->FindChild(String("uploadModelURL"))) {
            m_uploadModelURLs[brand] = value->GetText();
            m_uploadModelURLs[brand].TrimRight(kWhitespace);
        }
        if (RefPointer<XMLDOMNode> value = entry->FindChild(String("onlineHelpURL"))) {
            m_onlineHelpURLs[brand] = value->GetText();
            m_onlineHelpURLs[brand].TrimRight(kWhitespace);
        }
        if (RefPointer<XMLDOMNode> value = entry->FindChild(String("checkPriceURL"))) {
            m_checkPriceURLs[brand] = value->GetText();
            m_checkPriceURLs[brand].TrimRight(kWhitespace);
        }
        if (RefPointer<XMLDOMNode> value = entry->FindChild(String("brickPricesFileURL"))) {
            m_brickPricesFileURLs[brand] = value->GetText();
            m_brickPricesFileURLs[brand].TrimRight(kWhitespace);
        }
    }
}

// Mac 0x134da0, Win 0x63328b
void LDDWebCommunicator::SetCountryCurrencies(const XMLDOMNode* node)
{
    BasicArray<String> tokens;
    node->GetText().Split((UniChar16)',', tokens);
    for (BasicArray<String>::ConstIterator it = tokens.Begin(); it != tokens.End(); ++it) {
        if (it->IsEmpty())
            continue;
        BasicArray<String> parts;
        it->Split((UniChar16)'|', parts);
        if (parts.GetSize() != 2) {
            String error("Wrong currency token: ");
            error << *it;   // NOTE: built and dropped
            continue;
        }
        for (Countries::Iterator country = m_countries.Begin(); country != m_countries.End(); ++country) {
            if (country->m_code == parts[0]) {
                country->m_extra = parts[1];
                break;
            }
        }
    }
}

// ---- accessors ----------------------------------------------------------------------------------

const String& LDDWebCommunicator::GetCollectionURL() const { return m_collectionURL; }          // Mac 0x1352f0
const WebCollectionMap& LDDWebCommunicator::GetCollections() const { return m_collections; }    // Mac 0x135300

// Mac 0x135310, Win 0x6356d9
const String& LDDWebCommunicator::GetUploadModelURL()
{
    return m_uploadModelURLs[LDDApplication::GetInstance()->GetBrand()];
}

// Mac 0x135350
const String& LDDWebCommunicator::GetUploadModelURL(Brand brand) const
{
    return m_uploadModelURLs[brand];
}

const String& LDDWebCommunicator::GetGeometryURL() const { return m_geometryURL; }             // Mac 0x135380
const String& LDDWebCommunicator::GetInstallerURLPC() const { return m_installerURLPC; }       // Mac 0x135390
const String& LDDWebCommunicator::GetInstallerURLMac() const { return m_installerURLMac; }     // Mac 0x1353a0
const Countries& LDDWebCommunicator::GetCountries() const { return m_countries; }              // Mac 0x1353b0
bool LDDWebCommunicator::GetGenerateBGBeforeUpload() const { return m_generateBGBeforeUpload; } // Mac 0x1353c0
UInt32 LDDWebCommunicator::GetDefaultCountryIndex() const { return m_defaultCountryIndex; }    // Mac 0x1353d0

// Mac 0x1353e0
const String& LDDWebCommunicator::GetCheckPriceURL()
{
    return m_checkPriceURLs[LDDApplication::GetInstance()->GetBrand()];
}

// Mac 0x135420
const String& LDDWebCommunicator::GetCheckPriceURL(Brand brand) const
{
    return m_checkPriceURLs[brand];
}

// Mac 0x135450
AString LDDWebCommunicator::GetBrandName(Brand brand) const
{
    String name;
    ToString(brand, name);
    return name.ToAString();
}

// ===========================================================================================
// LDDWebCommunicatorInstantiator
// ===========================================================================================

// Mac 0x1372f0
RefPointer<Object> LDDWebCommunicatorInstantiator::CreateObject() const
{
    return RefPointer<Object>(new LDDWebCommunicator());
}

} // namespace LEGO
