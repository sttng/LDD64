// LEGO Digital Designer — restored source
// XMLDOMNode, XMLDOM, XMLDOMNodeAString — TU 171 "XMLDOMNode.cpp"
// (Mac 0x22bcd0-0x230f9f, Win 0x532690-0x535a9f). See XMLDOMNode.h.
//
// Not restored separately (library code instantiated here): BasicArray PushBack instances
// (Mac 0x22bea0 RefPointer<XMLDOMNode>, 0x22d5e0 Pair<AString,AString>, 0x230520
// RefPointer<XMLDOMNodeAString>, 0x230b90 AString), Pair ctors 0x230a10/0x230ad0, the
// BasicDeque/std::deque members 0x22f0b0-0x230990, and the `#include <iostream>` static
// initialiser 0x230f60.
#include "XMLDOMNode.h"
#include "LEGO/StdLib.h"
#include "StringConversion.h"
#include "BasicDeque.h"
#include "Path.h"
#include "File.h"
#include "ReadFile.h"

#if defined(_WIN32)
#ifndef XML_STATIC   // (the projects define it as well)
#define XML_STATIC   // Expat is linked statically into LDD.exe
#endif
#endif
#include "expat.h"

namespace LEGO {

// ===========================================================================
// XMLDOMNode
// ===========================================================================

// Mac 0x22c560, Win 0x532f70
XMLDOMNode::XMLDOMNode(const String& name)
    : m_name(name), m_parent(0)
{
}

// Mac D1 0x22f010 / D0 0x22f020 (body 0x230d50), Win 0x532690 / sdd 0x532740
XMLDOMNode::~XMLDOMNode()
{
}

// Mac 0x22bcd0, Win 0x5327d0
void XMLDOMNode::SetAttribute(const String& name, const String& value)
{
    for (AttributeArray::Iterator it = m_attributes.Begin(); it != m_attributes.End(); ++it) {
        if (it->first == name) {
            it->second = value;
            return;
        }
    }
    m_attributes.PushBack(Attribute(name, value));   // Mac 0x16ee30
}

// Mac 0x22bdd0, Win 0x532950
bool XMLDOMNode::GetAttribute(const String& name, String& value) const
{
    for (AttributeArray::ConstIterator it = m_attributes.Begin(); it != m_attributes.End(); ++it) {
        if (it->first == name) {
            value = it->second;
            return true;
        }
    }
    return false;
}

// Mac 0x22be30, Win 0x532a00
void XMLDOMNode::AddChild(XMLDOMNode* child)
{
    m_children.PushBack(RefPointer<XMLDOMNode>(child));
    child->m_parent = this;
}

// Mac 0x22bfc0, Win 0x532a90
XMLDOMNode* XMLDOMNode::FindChild(const String& name) const
{
    for (NodeArray::ConstIterator it = m_children.Begin(); it != m_children.End(); ++it) {
        if ((*it)->m_name == name)
            return *it;
    }
    return 0;
}

// Mac 0x22c010, Win 0x532b30
String XMLDOMNode::GetText() const
{
    String text;
    text.Join((UniChar16)'\n', m_text);
    return text;
}

// Mac 0x22c070, Win 0x532b90
void XMLDOMNode::SetText(const String& text)
{
    m_text.Clear();
    text.Split((UniChar16)'\n', m_text);
    // (performance) the empty lines leave in one pass; see XMLDOMNodeAString::SetText
    UInt32 kept = 0;
    for (UInt32 i = 0; i < m_text.GetSize(); ++i) {
        m_text[i].Trim();
        if (m_text[i].IsEmpty())
            continue;
        if (kept != i)
            m_text[kept] = m_text[i];
        ++kept;
    }
    m_text.Resize(kept);
}

// Mac 0x22c160, Win 0x532c90
bool XMLDOMNode::Write(String& out, UInt32 depth) const
{
    if (m_name.IsEmpty())
        return false;

    String indent(depth, (UniChar16)'\t');
    out << indent << "<" << m_name;
    for (AttributeArray::ConstIterator it = m_attributes.Begin(); it != m_attributes.End(); ++it)
        out << " " << it->first << "=\"" << it->second.ToXML() << "\"";

    if (m_children.IsEmpty()) {
        if (m_text.IsEmpty()) {
            out << "/>\r\n";
            return true;
        }
        if (m_text.GetSize() == 1) {
            out << ">" << m_text[0].ToXML() << "</" << m_name << ">\r\n";
            return true;
        }
    }

    out << ">\r\n";
    for (NodeArray::ConstIterator it = m_children.Begin(); it < m_children.End(); ++it) {
        if (!(*it)->Write(out, depth + 1))
            return false;
    }
    for (UInt32 i = 0; i < m_text.GetSize(); ++i)
        out << indent << "\t" << m_text[i].ToXML() << "\r\n";
    out << indent << "</" << m_name << ">\r\n";
    return true;
}

// Mac 0x22d160, Win 0x533aa0
bool XMLDOMNode::ToAString(AString& out) const
{
    String text;
    text.Clear();
    text << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\r\n";
    if (!Write(text, 0))
        return false;
    AString utf8 = EncodeString(kTextEncodingUTF8, text, 0, String::npos);
    const char* p = utf8.IsEmpty() ? 0 : utf8.GetBuffer();
    out.Assign(p, p ? StrLen(p) : 0);   // empty encoding -> released
    return true;
}

// Mac 0x22d070, Win 0x5339b0
bool XMLDOMNode::Save(File* file) const
{
    AString data;
    if (!ToAString(data))
        return false;
    if (!file->SetSize(0))
        return false;
    return file->Write(data.GetLength(), data.GetBuffer(), 0);
}

// Mac 0x22cfe0, Win 0x5338f0
bool XMLDOMNode::Save(const Path& path) const
{
    RefPointer<File> file = path.OpenFile(OpenMode(false, true), true);
    if (file.IsNull())
        return false;
    return Save(file);
}

// ---------------------------------------------------------------------------
// Expat callbacks (file-local; Mac 0x22c990 / 0x22cdf0 / 0x22cf60,
// Win 0x533110 / 0x5333e0 / 0x533510). (names inferred)
// ---------------------------------------------------------------------------

// (not in the original; H17) The deepest element nesting the DOM builders accept.
static const UInt32 kMaxXMLDepth = 256;

// Parse state handed to Expat as user data. Mac: root +0x00, stack +0x04 (BasicDeque),
// parser handle pointer +0x30. Mac ctor 0x22f0b0, Win 0x533020.
struct XMLDOMNodeParser {
    typedef Pair<RefPointer<XMLDOMNode>, String> Entry;   // element + collected character data

    XMLDOMNodeParser(XMLDOMNode* root, XML_Parser* parser)
        : m_root(root), m_stack(BasicDeque<Entry>()), m_parser(parser)
    {
    }

    static void XMLCALL OnStartElement(void* userData, const XML_Char* name, const XML_Char** atts);
    static void XMLCALL OnEndElement(void* userData, const XML_Char* name);
    static void XMLCALL OnCharacterData(void* userData, const XML_Char* s, int len);

    RefPointer<XMLDOMNode> m_root;
    BasicDeque<Entry>      m_stack;
    XML_Parser*            m_parser;
};

static String DecodeUTF8(const char* s)
{
    return DecodeString(kTextEncodingUTF8, s, StrLen(s));
}

// Mac 0x22c990, Win 0x533110
void XMLCALL XMLDOMNodeParser::OnStartElement(void* userData, const XML_Char* name, const XML_Char** atts)
{
    XMLDOMNodeParser* state = static_cast<XMLDOMNodeParser*>(userData);
    // NOTE: the nesting is not limited (both builds): a deeply nested document builds a tree
    // that the recursive destructor and Write overflow the stack on; fixed: the parse fails
    // beyond kMaxXMLDepth levels (H17), here and in the AString builder.
    if (state->m_stack.GetSize() >= kMaxXMLDepth) {
        XML_StopParser(*state->m_parser, XML_FALSE);
        return;
    }
    RefPointer<XMLDOMNode> node;
    if (state->m_stack.IsEmpty()) {
        node = state->m_root;
    } else {
        node = new XMLDOMNode(String());
        RefPointer<XMLDOMNode> parent = state->m_stack.Back().first;
        // NOTE: the child is appended directly; unlike AddChild() its parent pointer stays
        // null in both builds.
        parent->m_children.PushBack(node);
    }
    node->m_name = DecodeUTF8(name);
    for (; *atts; atts += 2)
        node->SetAttribute(DecodeUTF8(atts[0]), DecodeUTF8(atts[1]));
    state->m_stack.PushBack(Entry(node, String()));
}

// Mac 0x22cdf0, Win 0x5333e0
void XMLCALL XMLDOMNodeParser::OnEndElement(void* userData, const XML_Char* name)
{
    XMLDOMNodeParser* state = static_cast<XMLDOMNodeParser*>(userData);
    Entry& top = state->m_stack.Back();
    if (top.first->m_name == DecodeUTF8(name)) {
        top.first->SetText(top.second);
        state->m_stack.PopBack();
    } else {
        XML_StopParser(*state->m_parser, XML_FALSE);   // Win: inlined
    }
}

// Mac 0x22cf60, Win 0x533510
void XMLCALL XMLDOMNodeParser::OnCharacterData(void* userData, const XML_Char* s, int len)
{
    XMLDOMNodeParser* state = static_cast<XMLDOMNodeParser*>(userData);
    state->m_stack.Back().second.Append(DecodeString(kTextEncodingUTF8, s, (UInt32)len));
}

// Mac 0x22c6c0, Win 0x5336f0
bool XMLDOMNode::Parse(const AString& xml)
{
    XML_Parser parser = XML_ParserCreate(0);
    XMLDOMNodeParser state(this, &parser);
    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, XMLDOMNodeParser::OnStartElement, XMLDOMNodeParser::OnEndElement);
    XML_SetCharacterDataHandler(parser, XMLDOMNodeParser::OnCharacterData);
    if (XML_Parse(parser, xml.GetBuffer(), (int)xml.GetLength(), 1) == XML_STATUS_ERROR) {
        XML_ParserFree(parser);
        m_name.Clear();
        m_attributes.Clear();
        m_children.Clear();
        m_text.Clear();
        return false;
    }
    XML_ParserFree(parser);
    return true;
}

// Mac 0x22c620, Win 0x533590
bool XMLDOMNode::Parse(const Path& path)
{
    AString xml;
    if (!ReadFile(path, xml))
        return false;
    return Parse(xml);
}

// Mac 0x22c8f0, Win 0x533640
bool XMLDOMNode::Parse(File* file)
{
    AString xml;
    if (!ReadFile(file, xml))
        return false;
    return Parse(xml);
}

// ===========================================================================
// XMLDOM
// ===========================================================================

// Mac 0x22ee40 (C1 0x22ef30 forwards), Win 0x534a60
XMLDOM::XMLDOM()
    : XMLDOMNode(String())
{
}

// Mac 0x22ef40, Win 0x534ae0
XMLDOM::XMLDOM(const String& name)
    : XMLDOMNode(name)
{
}

// Mac D1 0x22f060 / D0 0x22f070, Win 0x532760 / sdd 0x5327b0
XMLDOM::~XMLDOM()
{
}

// ===========================================================================
// XMLDOMNodeAString
// ===========================================================================

// AString == C string as the inlined comparisons do it (std::string::compare shape on
// StrNCmp; Win calls strncmp directly).
static bool EqualsAString(const AString& a, const char* b, UInt32 bLength)
{
    UInt32 aLength = a.GetLength();
    if (aLength == 0)
        return bLength == 0 || *b == '\0';
    UInt32 n = aLength < bLength ? aLength : bLength;
    return StrNCmp(a.GetBuffer(), b, n) == 0 && aLength == bLength;
}

static bool EqualsAString(const AString& a, const AString& b)
{
    return EqualsAString(a, b.GetBuffer(), b.GetLength());
}

static String DecodeAString(const AString& s)
{
    return DecodeString(kTextEncodingUTF8, s.GetBuffer(), s.GetLength());
}

// Splits at every `separator`; a trailing separator adds no empty part. (Mac 0x22de30,
// file-local; inlined on Win as 0x469a50) (name inferred)
static void SplitAString(const AString& s, char separator, BasicArray<AString>& out)
{
    out.Clear();
    UInt32 start = 0;
    UInt32 length = s.GetLength();
    while (start < length) {
        const char* p = static_cast<const char*>(MemChr(s.GetBuffer() + start, separator, length - start));
        if (!p)
            break;
        UInt32 pos = (UInt32)(p - s.GetBuffer());
        out.PushBack(AString(s.GetBuffer() + start, pos - start));
        start = pos + 1;
    }
    if (start < length)
        out.PushBack(AString(s.GetBuffer() + start, length - start));
}

// Removes leading and trailing IsSpace() characters. (Mac 0x22e0a0, Win 0x468100) (name inferred)
static void TrimAString(AString& s)
{
    UInt32 length = s.GetLength();
    UInt32 lead = 0;
    if (length != 0 && IsSpace(s[0])) {
        lead = 1;
        while (lead < length && IsSpace(s[lead]))
            ++lead;
    }
    s.Erase(0, lead);
    length = s.GetLength();
    if (length != 0) {
        UInt32 end = length;
        while (end != 1 && IsSpace(s[end - 1]))
            --end;
        s.Erase(end, AString::npos);
    }
}

// Mac 0x22d2f0
void XMLDOMNodeAString::GetName(String& out) const
{
    out = DecodeAString(m_name);
}

// Mac 0x22d380, Win 0x534130
void XMLDOMNodeAString::Decode(const AString& utf8, String& out) const
{
    out = DecodeAString(utf8);
}

// Mac 0x22d410, Win 0x533be0
void XMLDOMNodeAString::SetAttribute(const AString& name, const AString& value)
{
    for (AttributeArray::Iterator it = m_attributes.Begin(); it != m_attributes.End(); ++it) {
        if (EqualsAString(it->first, name)) {
            it->second = value;
            return;
        }
    }
    m_attributes.PushBack(Attribute(name, value));
}

// Mac 0x22d730, Win 0x533d70
bool XMLDOMNodeAString::GetAttribute(const AString& name, AString& value) const
{
    for (AttributeArray::ConstIterator it = m_attributes.Begin(); it != m_attributes.End(); ++it) {
        if (EqualsAString(it->first, name)) {
            value = it->second;
            return true;
        }
    }
    return false;
}

// Mac 0x22d820, Win 0x533e20
bool XMLDOMNodeAString::GetAttribute(const AString& name, String& value) const
{
    for (AttributeArray::ConstIterator it = m_attributes.Begin(); it != m_attributes.End(); ++it) {
        if (EqualsAString(it->first, name)) {
            value = DecodeAString(it->second);
            return true;
        }
    }
    return false;
}

// Mac 0x22d970, Win 0x533ed0
XMLDOMNodeAString* XMLDOMNodeAString::FindChild(const AString& name) const
{
    for (NodeArray::ConstIterator it = m_children.Begin(); it != m_children.End(); ++it) {
        if (EqualsAString((*it)->m_name, name))
            return *it;
    }
    return 0;
}

// Mac 0x22db50, Win 0x533fe0
AString XMLDOMNodeAString::GetText() const
{
    AString text;
    BasicArray<AString>::ConstIterator it = m_text.Begin();
    if (it != m_text.End()) {
        for (;;) {
            text.Append(*it);
            ++it;
            if (it == m_text.End())
                break;
            // (Append throws RangeException* when the text is 0xfffffffe characters long, as in
            // the original.)
            if (!text.IsEmpty())
                text.Append("\n", 1);
        }
    }
    return text;
}

// Mac 0x22da60, Win 0x533f70
void XMLDOMNodeAString::GetText(String& out) const
{
    out = DecodeAString(GetText());
}

// Mac 0x22dce0, Win 0x534050
void XMLDOMNodeAString::SetText(const AString& text)
{
    m_text.Clear();
    // (performance) The original erases empty lines one by one, quadratic in the line count (a
    // 131,072-brick model's <Bricks> text has ~800,000 lines: over two minutes). Whitespace-only
    // text gives no lines; otherwise the empty lines leave in one pass. The same lines either way.
    {
        const char* p = text.GetBuffer();
        const char* end = p + text.GetLength();
        while (p != end && (*p == '\n' || IsSpace(*p)))
            ++p;
        if (p == end)
            return;
    }
    SplitAString(text, '\n', m_text);
    UInt32 kept = 0;
    for (UInt32 i = 0; i < m_text.GetSize(); ++i) {
        TrimAString(m_text[i]);
        if (m_text[i].IsEmpty())
            continue;
        if (kept != i)
            m_text[kept] = m_text[i];
        ++kept;
    }
    m_text.Resize(kept);
}

// Parse state (Mac ctor 0x22fc00, Win 0x5341c0): root +0x00, stack +0x04, parser handle
// pointer +0x30, tester +0x34 (RefPointer), "next element is the root" flag +0x38.
struct XMLDOMNodeAStringParser {
    typedef Pair<RefPointer<XMLDOMNodeAString>, AString> Entry;

    XMLDOMNodeAStringParser(XMLDOMNodeAString* root, XML_Parser* parser, XMLDOMNodeAStringTester* tester)
        : m_root(root), m_stack(BasicDeque<Entry>()), m_parser(parser), m_tester(tester), m_first(true)
    {
    }

    static void XMLCALL OnStartElement(void* userData, const XML_Char* name, const XML_Char** atts);
    static void XMLCALL OnEndElement(void* userData, const XML_Char* name);
    static void XMLCALL OnCharacterData(void* userData, const XML_Char* s, int len);

    RefPointer<XMLDOMNodeAString>       m_root;
    BasicDeque<Entry>                   m_stack;
    XML_Parser*                         m_parser;
    RefPointer<XMLDOMNodeAStringTester> m_tester;
    bool                                m_first;
};

// Mac 0x22e5b0, Win 0x5342d0
void XMLCALL XMLDOMNodeAStringParser::OnStartElement(void* userData, const XML_Char* name, const XML_Char** atts)
{
    XMLDOMNodeAStringParser* state = static_cast<XMLDOMNodeAStringParser*>(userData);
    if (state->m_stack.GetSize() >= kMaxXMLDepth) {   // (H17, see XMLDOMNodeParser)
        XML_StopParser(*state->m_parser, XML_FALSE);
        return;
    }
    RefPointer<XMLDOMNodeAString> node;
    if (state->m_stack.IsEmpty()) {
        node = state->m_root;
    } else {
        node = new XMLDOMNodeAString();
        RefPointer<XMLDOMNodeAString> parent = state->m_stack.Back().first;
        parent->m_children.PushBack(node);
    }
    if (name)
        node->m_name.Assign(name, StrLen(name));
    else
        node->m_name.Clear();
    for (; *atts; atts += 2) {
        AString key(atts[0], StrLen(atts[0]));
        AString value;
        if (atts[1])
            value.Assign(atts[1], StrLen(atts[1]));
        node->SetAttribute(key, value);
    }
    if (state->m_first && !state->m_tester.IsNull() && !state->m_tester->Test(node))
        XML_StopParser(*state->m_parser, XML_FALSE);   // Win: inlined
    state->m_first = false;
    state->m_stack.PushBack(Entry(node, AString()));
}

// Mac 0x22ebb0, Win 0x5345a0
void XMLCALL XMLDOMNodeAStringParser::OnEndElement(void* userData, const XML_Char* name)
{
    XMLDOMNodeAStringParser* state = static_cast<XMLDOMNodeAStringParser*>(userData);
    Entry& top = state->m_stack.Back();
    if (EqualsAString(top.first->m_name, name, StrLen(name))) {
        top.first->SetText(top.second);
        state->m_stack.PopBack();
    } else {
        XML_StopParser(*state->m_parser, XML_FALSE);
    }
}

// Mac 0x22ed50, Win 0x534670
void XMLCALL XMLDOMNodeAStringParser::OnCharacterData(void* userData, const XML_Char* s, int len)
{
    XMLDOMNodeAStringParser* state = static_cast<XMLDOMNodeAStringParser*>(userData);
    AString data;
    if (s)
        data.Assign(s, (UInt32)len);
    state->m_stack.Back().second.Append(data);
}

// Mac 0x22e200, Win 0x534850
bool XMLDOMNodeAString::Parse(const AString& xml, XMLDOMNodeAStringTester* tester)
{
    XML_Parser parser = XML_ParserCreate(0);
    XMLDOMNodeAStringParser state(this, &parser, tester);
    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, XMLDOMNodeAStringParser::OnStartElement,
                          XMLDOMNodeAStringParser::OnEndElement);
    XML_SetCharacterDataHandler(parser, XMLDOMNodeAStringParser::OnCharacterData);
    if (XML_Parse(parser, xml.GetBuffer(), (int)xml.GetLength(), 1) == XML_STATUS_ERROR) {
        XML_ParserFree(parser);
        m_name.Clear();
        m_attributes.Clear();
        m_children.Clear();
        m_text.Clear();
        return false;
    }
    XML_ParserFree(parser);
    return true;
}

// Mac 0x22e150, Win 0x5346f0
bool XMLDOMNodeAString::Parse(const Path& path, XMLDOMNodeAStringTester* tester)
{
    AString xml;
    if (!ReadFile(path, xml))
        return false;
    return Parse(xml, tester);
}

// Mac 0x22e500, Win 0x5347a0
bool XMLDOMNodeAString::Parse(File* file, XMLDOMNodeAStringTester* tester)
{
    AString xml;
    if (!ReadFile(file, xml))
        return false;
    return Parse(xml, tester);
}

} // namespace LEGO
