// LEGO Digital Designer — restored source
// LEGO::LDDWebCommunicator — the service data of the LEGO servers ('WEBC' in the
//   ObjectRegistry): the URLs and lists the InitializeDesigner reply delivered (collections,
//   geometry and installer URLs, per-brand upload / online-help / check-price / brick-prices
//   URLs, ship-to countries with currencies, "generate building guide before upload").
// LEGO::LDDWebCommunicatorInstantiator — creates it.
//   Home TU 059 "LDDWebCommunicator.cpp" (Mac 0x133720-0x138d3b; Win 0x632141-0x6357d2,
//   scattered). The TU also holds LDDWebObserver (LDDWebObserver.h), the TCPSocketNotifier and
//   HTTPCommunicator inline copies (HTTPCommunicator.h) and the destructors of Countries /
//   PaletteNames.
//
// RTTI: class LDDWebCommunicator : virtual Object. Mac vtable 0x70c56c {D1 0x133d80, D0
// 0x133e50}, Object-in 0x70c58c (Object at +0xf8, Mac sizeof 0x100); Win Object-in vtable
// 0x9d349c {sdd 0x635783, …} (Object at +0xe4). Instantiator Mac 0x765c94 {D1 0x133fd0, D0
// 0x1372e0, CreateObject 0x1372f0}, Win {sdd 0x63570b, CreateObject 0x634f92}.
// Every method and member name is inferred except the class names, the strings and the
// element / preference names.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "Brand.h"
#include "BasicMap.h"
#include "Version.h"
#include "ObjectRegistry.h"
#include "Countries.h"
#include "PaletteNames.h"

namespace LEGO {

struct InitializeDesignerData;
class XMLDOMNode;

// The collections of the server are a set of palette names (name, version): the version dump
// (BrickConnectSceneControl Mac 0x3b230) formats them with ToString(PaletteNameSet) (Mac
// 0x22b970).
typedef PaletteNameSet WebCollectionMap;

// Per-brand URLs. (name inferred)
typedef BasicMap<Brand, String> BrandURLMap;

class LDDWebCommunicator : public virtual Object {
public:
    enum { kRegistryId = 0x57454243 };   // 'WEBC'

    // Empty data; the brick-prices file URL from the preference "BrickPricesFileURL".
    // Mac 0x133820, Win 0x635457
    LDDWebCommunicator();
    virtual ~LDDWebCommunicator();                                          // Mac 0x133d80/0x133e50, Win sdd 0x635783

    // The registered communicator (created on first use). Mac 0x133f40, Win 0x634f92
    static LDDWebCommunicator* GetInstance();

    // Takes the service data of the InitializeDesigner reply (StartMode). Mac 0x133fe0, Win 0x635667
    void SetInitializeDesignerData(const InitializeDesignerData& data);

    // ---- accessors (Mac 0x1352f0-0x135420) ----
    const String&           GetCollectionURL() const;                       // Mac 0x1352f0
    const WebCollectionMap& GetCollections() const;                         // Mac 0x135300
    // The upload URL of the application's brand / of `brand` (an empty entry is created for
    // an unknown brand: map operator[]). Mac 0x135310 (Win 0x6356d9) / 0x135350
    const String&           GetUploadModelURL();
    const String&           GetUploadModelURL(Brand brand) const;
    const String&           GetGeometryURL() const;                         // Mac 0x135380
    const String&           GetInstallerURLPC() const;                      // Mac 0x135390
    const String&           GetInstallerURLMac() const;                     // Mac 0x1353a0
    const Countries&        GetCountries() const;                           // Mac 0x1353b0
    bool                    GetGenerateBGBeforeUpload() const;              // Mac 0x1353c0
    UInt32                  GetDefaultCountryIndex() const;                 // Mac 0x1353d0
    // The check-price URL of the application's brand / of `brand`. Mac 0x1353e0 / 0x135420
    const String&           GetCheckPriceURL();
    const String&           GetCheckPriceURL(Brand brand) const;
    // The name of `brand` as ASCII (UploadModelDialog, Mac 0x1e6b40). Mac 0x135450
    AString                 GetBrandName(Brand brand) const;

    // (accessors of the remaining data; no out-of-line copies located)
    const BrandURLMap&      GetOnlineHelpURLs() const       { return m_onlineHelpURLs; }
    const BrandURLMap&      GetBrickPricesFileURLs() const  { return m_brickPricesFileURLs; }
    const String&           GetBrickPricesFileURL() const   { return m_brickPricesFileURL; }

private:
    // "name|version,name|version,…" -> the collections. Mac 0x1340e0
    void SetCollections(const String& text);
    // "name|code[|default],…" -> the countries (an entry with a third field becomes the
    // default country). Mac 0x134340, Win 0x6333b5
    void SetCountries(const String& text);
    // <configURLs><brand name=… uploadModelURL=… onlineHelpURL=… checkPriceURL=…
    // brickPricesFileURL=…/>…. Mac 0x1346e0, Win 0x63503f
    void SetConfigURLs(const XMLDOMNode* node);
    // "code|currency,…" -> the countries' currencies. Mac 0x134da0, Win 0x63328b
    void SetCountryCurrencies(const XMLDOMNode* node);

    // Mac offsets
    String              m_collectionURL;           // +0x04 (ends with '/')
    PaletteNames        m_collections;             // +0x14
    mutable BrandURLMap m_uploadModelURLs;         // +0x30
    String              m_geometryURL;             // +0x4c (ends with '/')
    BrandURLMap         m_onlineHelpURLs;          // +0x5c
    BrandURLMap         m_brickPricesFileURLs;     // +0x78
    String              m_installerURLPC;          // +0x94
    String              m_installerURLMac;         // +0xa4
    Countries           m_countries;               // +0xb4
    bool                m_generateBGBeforeUpload;  // +0xc4 (initially true)
    UInt32              m_defaultCountryIndex;     // +0xc8
    mutable BrandURLMap m_checkPriceURLs;          // +0xcc
    String              m_brickPricesFileURL;      // +0xe8 preference "BrickPricesFileURL"
                                                   // Object +0xf8
};

class LDDWebCommunicatorInstantiator : public ObjectRegistryInstantiator {
public:
    LDDWebCommunicatorInstantiator() {}
    virtual ~LDDWebCommunicatorInstantiator() {}                             // Mac 0x133fd0/0x1372e0, Win sdd 0x63570b
    virtual RefPointer<Object> CreateObject() const;                         // Mac 0x1372f0
};

} // namespace LEGO
