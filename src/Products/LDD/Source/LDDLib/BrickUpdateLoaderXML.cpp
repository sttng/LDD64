// LEGO Digital Designer — restored source
// TU 359 "BrickUpdateLoaderXML.cpp" (Mac 0x342750-0x3435ff, Win 0x695a71-0x696084). See
// BrickUpdateLoaderXML.h for the format.
//
// Template instances emitted here: the BasicDeque of handler pairs (Mac 0x342bd0 is the context
// constructor with the deque map init inlined). Static initialiser Mac 0x3435c0: <iostream>.
// NOTE (both builds): the "Unhandled tag" message is built and dropped (no trace call).
// NOTE (both builds): the start handler of an unknown element installs nothing for its
// children, but their end handler is the popping one: a child of an unknown element pops the
// handler pair of its parent's level (an unknown element nested two deep unbalances the stack).
#include "BrickUpdateLoaderXML.h"

#include "LEGO/StdLib.h"
#include "BasicDeque.h"
#include "StringConversion.h"
#include "File.h"
#include "ReadFile.h"

#ifndef XML_STATIC   // (the projects define it as well)
#define XML_STATIC   // Expat is linked statically into both programs
#endif
#include "expat.h"

#include <iostream>

namespace LEGO {

namespace {

// The Expat user data of one parse (constructor Mac 0x342bd0). (name inferred)
struct UpdateContext {
    typedef void (*StartHandler)(void* userData, const char* name, const char** atts);
    typedef void (*EndHandler)(void* userData, const char* name);
    typedef Pair<StartHandler, EndHandler> Handlers;

    UpdateContext(XML_Parser* parser, Version* version, BasicArray<String>* added,
                  BasicArray<String>* modified, BasicArray<String>* deleted)
        : m_parser(parser), m_added(added), m_modified(modified), m_deleted(deleted),
          m_version(version)
    {
    }

    // Mac 0x342980
    void PushHandlers(StartHandler start, EndHandler end)
    {
        m_handlers.PushBack(Handlers(start, end));
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    // (inlined in every popping end handler)
    void PopHandlers()
    {
        m_handlers.PopBack();
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }

    XML_Parser*          m_parser;     // +0x00
    BasicArray<String>*  m_added;      // +0x04
    BasicArray<String>*  m_modified;   // +0x08
    BasicArray<String>*  m_deleted;    // +0x0c
    Version*             m_version;    // +0x10
    BasicDeque<Handlers> m_handlers;   // +0x14
};

inline UpdateContext& C(void* userData) { return *static_cast<UpdateContext*>(userData); }

String Decode(const char* text)
{
    return DecodeString(kTextEncodingUTF8, text, StrLen(text));
}

// Mac 0x342f90: an element nobody handles (installs no handlers, see the NOTE above).
void OnUnhandledStart(void*, const char* name, const char**)
{
    String message("BrickUpdateLoaderXML: Unhandled tag '");
    message << String(name) << String("' found");                        // NOTE: dropped
}

// Mac 0x343060
void OnPopEnd(void* userData, const char*)
{
    C(userData).PopHandlers();
}

// <file path=…> below <add> / <overwrite> / <remove>: the path goes to `files`; another element
// is unhandled. Mac 0x3430e0 / 0x343280 / 0x343420
void ReadFileElement(void* userData, const char* name, const char** atts, BasicArray<String>* files)
{
    if (StrCmp(name, "file") != 0) {
        C(userData).PushHandlers(OnUnhandledStart, OnPopEnd);
        return;
    }
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "path") == 0)
            files->PushBack(Decode(atts[1]));
    }
}

void OnAddChildStart(void* userData, const char* name, const char** atts)
{
    ReadFileElement(userData, name, atts, C(userData).m_added);
}
void OnOverwriteChildStart(void* userData, const char* name, const char** atts)
{
    ReadFileElement(userData, name, atts, C(userData).m_modified);
}
void OnRemoveChildStart(void* userData, const char* name, const char** atts)
{
    ReadFileElement(userData, name, atts, C(userData).m_deleted);
}

// Mac 0x3431d0 / 0x343370 / 0x343510: back to the <update> level at the list's end tag.
void OnAddChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "add") == 0)
        C(userData).PopHandlers();
}
void OnOverwriteChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "overwrite") == 0)
        C(userData).PopHandlers();
}
void OnRemoveChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "remove") == 0)
        C(userData).PopHandlers();
}

// Children of <update>. Mac 0x342e30
void OnUpdateChildStart(void* userData, const char* name, const char**)
{
    UpdateContext& context = C(userData);
    if (StrCmp(name, "add") == 0)
        context.PushHandlers(OnAddChildStart, OnAddChildEnd);
    else if (StrCmp(name, "overwrite") == 0)
        context.PushHandlers(OnOverwriteChildStart, OnOverwriteChildEnd);
    else if (StrCmp(name, "remove") == 0)
        context.PushHandlers(OnRemoveChildStart, OnRemoveChildEnd);
    else
        context.PushHandlers(OnUnhandledStart, OnPopEnd);
}

// Mac 0x342ee0
void OnUpdateChildEnd(void* userData, const char* name)
{
    if (StrCmp(name, "update") == 0)
        C(userData).PopHandlers();
}

// <update version>. Mac 0x342a60
void OnRootStart(void* userData, const char* name, const char** atts)
{
    UpdateContext& context = C(userData);
    if (StrCmp(name, "update") != 0) {
        context.PushHandlers(OnUnhandledStart, OnPopEnd);
        return;
    }
    for (; atts[0]; atts += 2) {
        if (StrCmp(atts[0], "version") == 0) {
            UInt32 position = 0;
            FromString(*context.m_version, Decode(atts[1]), position);
        }
    }
    context.PushHandlers(OnUpdateChildStart, OnUpdateChildEnd);
}

// Mac 0x342b80 (empty)
void OnRootEnd(void*, const char*)
{
}

} // namespace

// Mac 0x342790, Win 0x695f9d
bool BrickUpdateLoaderXML::Load(File* file, Version& version, BasicArray<String>& added,
                                BasicArray<String>& modified, BasicArray<String>& deleted)
{
    file->Seek(0);
    AString data;
    if (!ReadFile(file, data))
        return false;
    XML_Parser parser = XML_ParserCreate(0);
    UpdateContext context(&parser, &version, &added, &modified, &deleted);
    XML_SetUserData(parser, &context);
    context.PushHandlers(OnRootStart, OnRootEnd);
    const bool ok = XML_Parse(parser, data.GetBuffer(), (int)data.GetLength(), 1) != XML_STATUS_ERROR;
    XML_ParserFree(parser);
    return ok;
}

// Mac 0x342b90, Win 0x695b31 (?)
bool BrickUpdateLoaderXML::Load(File* file, BasicArray<String>& added, BasicArray<String>& modified,
                                BasicArray<String>& deleted)
{
    Version version;
    return Load(file, version, added, modified, deleted);
}

} // namespace LEGO
