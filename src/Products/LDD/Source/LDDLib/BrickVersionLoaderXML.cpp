// LEGO Digital Designer — restored source
// TU 103 "BrickVersionLoaderXML.cpp" (Mac 0x1b8360-0x1b93ff, Win 0x686db2 + 0x641c3a): the
// reader of the server's BrickVersion.xml:
//   <BrickSets>
//     <BrickSet version="x.y.z.w" fullUpdate="true|false"><File name="archive.zip"/></BrickSet>…
//   </BrickSets>
// See BrickVersionDownloaderCaller.h.
//
// Template instances emitted here: the BasicDeque of handler pairs (Mac 0x1b8840 is the context
// constructor with the deque map init inlined), the BrickVersionMap helpers (0x1b9010-0x1b9310).
// Static initialiser Mac 0x1b93c0: <iostream>.
// NOTE (both builds): the start handler of an unknown element installs nothing, but its end
// handler pops (see BrickUpdateLoaderXML.cpp for the same pattern).
#include "BrickVersionDownloaderCaller.h"

#include "LEGO/StdLib.h"
#include "BasicDeque.h"
#include "StringConversion.h"
#include "StringStream.h"
#include "File.h"
#include "ReadFile.h"

#ifndef XML_STATIC   // (the projects define it as well)
#define XML_STATIC   // Expat is linked statically into both programs
#endif
#include "expat.h"

#include <iostream>

namespace LEGO {

namespace {

// The Expat user data of one parse (constructor Mac 0x1b8840, Mac sizeof 0x68). (name inferred)
struct VersionContext {
    typedef void (*StartHandler)(void* userData, const char* name, const char** atts);
    typedef void (*EndHandler)(void* userData, const char* name);
    typedef Pair<StartHandler, EndHandler> Handlers;

    VersionContext(XML_Parser* parser, BrickVersionMap* versions, const Version& current)
        : m_parser(parser), m_current(current), m_fullUpdate(false), m_versions(versions)
    {
    }

    // Mac 0x1b86b0
    void PushHandlers(StartHandler start, EndHandler end)
    {
        m_handlers.PushBack(Handlers(start, end));
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    void PopHandlers()
    {
        m_handlers.PopBack();
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }

    XML_Parser*          m_parser;       // +0x00
    Version              m_current;      // +0x04 the installed version
    Version              m_setVersion;   // +0x14 of the <BrickSet> being read
    bool                 m_fullUpdate;   // +0x24 of the <BrickSet> being read
    BrickVersionMap*     m_versions;     // +0x28
    Version              m_lastFull;     // +0x2c the newest full update seen
    BasicDeque<Handlers> m_handlers;     // +0x3c
};

inline VersionContext& C(void* userData) { return *static_cast<VersionContext*>(userData); }

String Decode(const char* text)
{
    return DecodeString(kTextEncodingUTF8, text, StrLen(text));
}

// Mac 0x1b8d60 (empty: no handlers for the children, see the NOTE above)
void OnIgnoredStart(void*, const char*, const char**)
{
}

// Mac 0x1b8d70
void OnIgnoredEnd(void* userData, const char*)
{
    C(userData).PopHandlers();
}

// Children of a <BrickSet> newer than the installed version: <File name>. Mac 0x1b8df0
void OnBrickSetChildStart(void* userData, const char* name, const char** atts)
{
    VersionContext& context = C(userData);
    if (!(context.m_setVersion > context.m_current))
        return;
    if (StrCmp(name, "File") != 0) {
        context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
        return;
    }
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "name") != 0)
            continue;
        if (context.m_fullUpdate && context.m_setVersion > context.m_lastFull)
            context.m_lastFull = context.m_setVersion;
        (*context.m_versions)[context.m_setVersion] = Decode(atts[1]);
    }
}

// Mac 0x1b8f60
void OnBrickSetChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "BrickSet") == 0)
        C(userData).PopHandlers();
}

// Children of <BrickSets>: <BrickSet version fullUpdate>. Mac 0x1b8ae0
void OnBrickSetsChildStart(void* userData, const char* name, const char** atts)
{
    VersionContext& context = C(userData);
    if (StrCmp(name, "BrickSet") != 0) {
        context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
        return;
    }
    context.m_setVersion = Version();
    context.m_fullUpdate = false;
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "version") == 0) {
            UInt32 position = 0;
            FromString(context.m_setVersion, Decode(atts[1]), position);
        } else if (StrCmp(atts[0], "fullUpdate") == 0) {
            FromString(context.m_fullUpdate, Decode(atts[1]));
        }
    }
    context.PushHandlers(OnBrickSetChildStart, OnBrickSetChildEnd);
}

// Mac 0x1b8cb0
void OnBrickSetsChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "BrickSets") == 0)
        C(userData).PopHandlers();
}

// Mac 0x1b8790
void OnRootStart(void* userData, const char* name, const char**)
{
    VersionContext& context = C(userData);
    if (StrCmp(name, "BrickSets") != 0) {
        context.PushHandlers(OnIgnoredStart, OnIgnoredEnd);
        return;
    }
    context.m_setVersion = Version();
    context.PushHandlers(OnBrickSetsChildStart, OnBrickSetsChildEnd);
}

// Mac 0x1b8810 (empty)
void OnRootEnd(void*, const char*)
{
}

} // namespace

// Mac 0x1b8360, Win 0x686db2
bool BrickVersionLoaderXML::Load(File* file, BrickVersionMap& versions, bool& fullUpdate,
                                 Version current)
{
    file->Seek(0);
    AString data;
    if (!ReadFile(file, data))
        return false;
    XML_Parser parser = XML_ParserCreate(0);
    VersionContext context(&parser, &versions, current);
    XML_SetUserData(parser, &context);
    context.PushHandlers(OnRootStart, OnRootEnd);
    if (XML_Parse(parser, data.GetBuffer(), (int)data.GetLength(), 1) == XML_STATUS_ERROR) {
        XML_ParserFree(parser);
        return false;
    }
    fullUpdate = false;
    if (context.m_lastFull > Version()) {
        fullUpdate = true;
        // Everything older than the newest full update is not needed. NOTE (both builds):
        // erases up to find(), so everything goes when that version is not a key.
        versions.Erase(versions.Begin(), versions.Find(context.m_lastFull));
    }
    XML_ParserFree(parser);
    return true;
}

} // namespace LEGO
