// LEGO Digital Designer — restored source
// LEGO::XMLDOMNode / XMLDOM (UTF-16 String tree), XMLDOMNodeAString / XMLDOMAString (8-bit UTF-8),
// XMLDOMNodeAStringTester (root-element veto). TU 171 (Mac 0x22bcd0-0x230f9f, Win 0x532690-0x535a9f).
// Expat (element / character-data handlers only): Mac 0x3c4ddc SetUserData, 0x3c4df9
// SetElementHandler, 0x3c4e0d SetCharacterDataHandler, 0x3c50c5 Parse, 0x3c523c StopParser,
// 0x3c618f ParserFree, 0x3cc070 ParserCreate; Win 0x53fe30 ParserCreate, 0x540370 Parse, 0x5401b0
// ParserFree (StopParser inlined). Win error strings point to 2.0.1 (thirdparty/expat-2.0.1).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "BasicArray.h"
#include "BasicMap.h"

namespace LEGO {

class File;
class Path;

// ---------------------------------------------------------------------------
// XMLDOMNode — RTTI LEGO::XMLDOMNode, Mac vtable 0x770d18 {D1 0x22f010, D0 0x22f020,
// Object 0x9250 0x9260 0x9280}, Win sdd 0x532740. Key function: the destructor.
// Mac sizeof 0x4c, Win 0x40.
// ---------------------------------------------------------------------------
class XMLDOMNode : public Object {
public:
    typedef Pair<String, String>             Attribute;
    typedef BasicArray<Attribute>            AttributeArray;
    typedef BasicArray<RefPointer<XMLDOMNode> > NodeArray;

    explicit XMLDOMNode(const String& name);                          // Mac 0x22c560, Win 0x532f70
    virtual ~XMLDOMNode();                                             // Mac 0x22f010/0x22f020 (body 0x230d50), Win 0x532690/0x532740

    const String&         GetName() const       { return m_name; }       // (accessor, inlined)
    void                  SetName(const String& name) { m_name = name; } // (accessor, inlined)
    const AttributeArray& GetAttributes() const { return m_attributes; } // (accessor, inlined)
    const NodeArray&      GetChildren() const   { return m_children; }   // (accessor, inlined)
    // Direct access to the child array: LuaScriptManager edits the parsed layouts in place
    // (erases <Include>/<Script>, moves layouts into its XMLDOM without AddChild). (accessor,
    // inlined; Mac 0x2d6030 / 0x2d2390)
    NodeArray&            GetChildren()         { return m_children; }
    XMLDOMNode*           GetParent() const     { return m_parent; }     // (accessor, inlined)

    // Replaces the value of an existing attribute or appends a new one. (name inferred)
    void SetAttribute(const String& name, const String& value);        // Mac 0x22bcd0, Win 0x5327d0
    // (name inferred)
    bool GetAttribute(const String& name, String& value) const;        // Mac 0x22bdd0, Win 0x532950
    // Appends `child` and makes this its parent. (name inferred)
    void AddChild(XMLDOMNode* child);                                   // Mac 0x22be30, Win 0x532a00
    // First child named `name` (case sensitive), or null. (name inferred)
    XMLDOMNode* FindChild(const String& name) const;                    // Mac 0x22bfc0, Win 0x532a90

    // Text lines joined with '\n'. (name inferred)
    String GetText() const;                                             // Mac 0x22c010, Win 0x532b30
    // Splits at '\n', trims each line and drops empty lines. (name inferred)
    void   SetText(const String& text);                                 // Mac 0x22c070, Win 0x532b90

    // Appends this element (tab indented by `depth`, "\r\n" line ends) to `out`. Fails for a
    // node without name. (name inferred)
    bool Write(String& out, UInt32 depth) const;                        // Mac 0x22c160, Win 0x532c90
    // XML declaration + Write(…, 0), encoded as UTF-8. (name inferred)
    bool ToAString(AString& out) const;                                 // Mac 0x22d160, Win 0x533aa0
    // Truncates `file` and writes ToAString(). (name inferred)
    bool Save(File* file) const;                                        // Mac 0x22d070, Win 0x5339b0
    // Opens (creates) `path` for writing and calls Save(File*). (name inferred)
    bool Save(const Path& path) const;                                  // Mac 0x22cfe0, Win 0x5338f0

    // Parses UTF-8 XML into this node (the root element becomes this node, its descendants new
    // children). On failure the name, attributes, children and text are cleared. (name inferred)
    bool Parse(const AString& xml);                                     // Mac 0x22c6c0, Win 0x5336f0
    bool Parse(const Path& path);                                       // Mac 0x22c620, Win 0x533590
    bool Parse(File* file);                                             // Mac 0x22c8f0, Win 0x533640

private:
    friend struct XMLDOMNodeParser;

    XMLDOMNode(const XMLDOMNode&);
    XMLDOMNode& operator=(const XMLDOMNode&);

    String             m_name;        // Mac +0x08, Win +0x08
    AttributeArray     m_attributes;  // Mac +0x18, Win +0x0c
    NodeArray          m_children;    // Mac +0x28, Win +0x1c
    XMLDOMNode*        m_parent;      // Mac +0x38, Win +0x2c (set by AddChild only)
    BasicArray<String> m_text;        // Mac +0x3c, Win +0x30 (trimmed, non-empty lines)
};

// ---------------------------------------------------------------------------
// XMLDOM — the document node. RTTI LEGO::XMLDOM, Mac vtable 0x770d34 {D1 0x22f060,
// D0 0x22f070, …}, Win sdd 0x5327b0. No members of its own.
// ---------------------------------------------------------------------------
class XMLDOM : public XMLDOMNode {
public:
    XMLDOM();                                                           // Mac 0x22ee40 (C1 0x22ef30), Win 0x534a60
    explicit XMLDOM(const String& name);                                // Mac 0x22ef40, Win 0x534ae0
    virtual ~XMLDOM();                                                  // Mac 0x22f060/0x22f070, Win 0x532760/0x5327b0
};

class XMLDOMNodeAString;

// ---------------------------------------------------------------------------
// XMLDOMNodeAStringTester — abstract; LXFML5Tester (Mac vtable 0x777448, Test 0x3242f0,
// Win 0x4a42a0) checks that the root is <LXFML versionMajor="5">. Held by RefPointer while
// parsing. No vtable of its own is emitted.
// ---------------------------------------------------------------------------
class XMLDOMNodeAStringTester : public Object {
public:
    // Called once for the root element (name + attributes set, no children yet); returning
    // false stops the parser, so Parse() fails. (name inferred) Mac slot 5, Win slot 4.
    virtual bool Test(const XMLDOMNodeAString* root) = 0;
};

// ---------------------------------------------------------------------------
// XMLDOMNodeAString — RTTI LEGO::XMLDOMNodeAString, Mac vtable 0x770da0 {D1 0x88ce0,
// D0 0x88cf0, Object …}, Win sdd 0x4a3fe0. No user-declared constructor or destructor:
// the parser value-initialises new nodes (`new XMLDOMNodeAString()`: Mac 0x22e5b0 zero-fills
// the 0x3c bytes before running the member constructors) and the destructor (body Mac
// 0x88f60, Win 0x4a3f10) is emitted as a weak copy in its first user (TU 024).
// No parent pointer. Mac/Win sizeof 0x3c.
// ---------------------------------------------------------------------------
class XMLDOMNodeAString : public Object {
public:
    typedef Pair<AString, AString>                     Attribute;
    typedef BasicArray<Attribute>                      AttributeArray;
    typedef BasicArray<RefPointer<XMLDOMNodeAString> > NodeArray;

    const AString&        GetName() const       { return m_name; }       // (accessor, inlined)
    const AttributeArray& GetAttributes() const { return m_attributes; } // (accessor, inlined)
    const NodeArray&      GetChildren() const   { return m_children; }   // (accessor, inlined)

    // Name decoded from UTF-8. (name inferred)
    void GetName(String& out) const;                                    // Mac 0x22d2f0
    // Decodes a UTF-8 AString (does not use `this`). (name inferred)
    void Decode(const AString& utf8, String& out) const;                // Mac 0x22d380, Win 0x534130

    void SetAttribute(const AString& name, const AString& value);       // Mac 0x22d410, Win 0x533be0
    bool GetAttribute(const AString& name, AString& value) const;       // Mac 0x22d730, Win 0x533d70
    // Value decoded from UTF-8.
    bool GetAttribute(const AString& name, String& value) const;        // Mac 0x22d820, Win 0x533e20
    // First child named `name`, or null. (names inferred)
    XMLDOMNodeAString* FindChild(const AString& name) const;           // Mac 0x22d970, Win 0x533ed0

    // Text lines joined with '\n' (no '\n' is added while the result is still empty).
    AString GetText() const;                                            // Mac 0x22db50, Win 0x533fe0
    void    GetText(String& out) const;                                 // Mac 0x22da60, Win 0x533f70
    // Splits at '\n', trims each line (isspace) and drops empty lines.
    void    SetText(const AString& text);                               // Mac 0x22dce0, Win 0x534050

    // Parses UTF-8 XML into this node; see XMLDOMNode::Parse. `tester` (may be null) can
    // reject the document after its root element was read. (names inferred)
    bool Parse(const AString& xml, XMLDOMNodeAStringTester* tester);    // Mac 0x22e200, Win 0x534850
    bool Parse(const Path& path, XMLDOMNodeAStringTester* tester);      // Mac 0x22e150, Win 0x5346f0
    bool Parse(File* file, XMLDOMNodeAStringTester* tester);            // Mac 0x22e500, Win 0x5347a0

    // Public only so the implicit constructor/destructor stay implicit (members are not
    // touched by any caller outside this TU).
    AString             m_name;        // +0x08
    AttributeArray      m_attributes;  // +0x0c
    NodeArray           m_children;    // +0x1c
    BasicArray<AString> m_text;        // +0x2c
};

// ---------------------------------------------------------------------------
// XMLDOMAString — the 8-bit document node. RTTI LEGO::XMLDOMAString, Mac vtable 0x761d00
// {D1 0x88b80, D0 0x88b90, …} (weak, emitted in TU 024), Win sdd 0x4a4170. Header-only.
// ---------------------------------------------------------------------------
class XMLDOMAString : public XMLDOMNodeAString {
};

} // namespace LEGO
