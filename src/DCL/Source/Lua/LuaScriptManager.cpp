// LEGO Digital Designer — restored source
// TU 324 "LuaScriptManager.cpp": Mac 0x2d0ed0-0x2d80db, Win 0x452190-0x455d5f.
// Processing model: LuaScriptManager.h.
#include "LuaScriptManager.h"
#include "LuaBindings.h"
#include "Directory.h"
#include "Platform.h"
#include "ReadFile.h"
#include "SystemTime.h"
#include "SystemUpdater.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using namespace LuaPlus;

namespace LEGO {

LuaScriptManager* LuaScriptManager::s_instance = 0;                       // Mac 0x77ed90, Win 0xb3ffe0

namespace {

// Lua ScriptManager.Load(file [, callback] [, synchronous]): a file already loaded only calls
// callback(true); otherwise an asynchronous (default) or synchronous request is started.
int LuaScriptManagerLoad(lua_State* L)                                     // Mac 0x2d0ed0, Win 0x4525b0
{
    LuaState* state = LuaState::CastState(L);
    LuaScriptManager* manager = (LuaScriptManager*)lua_touserdata(L, lua_upvalueindex(1));
    String name = CheckLuaString(L, 1);
    if (manager->IsLoaded(name)) {
        if (lua_type(L, 2) == LUA_TFUNCTION) {
            lua_pushvalue(L, 2);
            lua_pushboolean(L, 1);
            state->PCall(1, 0);
        }
    } else {
        RefPointer<LuaScriptManagerLoadCallback> callback;
        if (lua_type(L, 2) == LUA_TFUNCTION)
            callback = new LuaScriptManagerLoadCallback_script(LuaObject(state, 2));
        manager->Load(name, callback, OptLuaBoolean(L, 3, false), false);
    }
    return 0;
}

// Lua ScriptManager.IsLoaded(file) -> boolean.
int LuaScriptManagerIsLoaded(lua_State* L)                                 // Mac 0x2d13a0, Win 0x452770
{
    LuaScriptManager* manager = (LuaScriptManager*)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushboolean(L, manager->IsLoaded(CheckLuaString(L, 1)));
    return 1;
}

// Lua ScriptManager.AbortLoading().
int LuaScriptManagerAbortLoading(lua_State* L)                             // Mac 0x2d1430, Win 0x452800
{
    LuaScriptManager* manager = (LuaScriptManager*)lua_touserdata(L, lua_upvalueindex(1));
    manager->Abort();
    return 0;
}

const luaL_Reg kScriptManagerFunctions[] = {                               // (Win on the stack)
    { "Load", LuaScriptManagerLoad },
    { "IsLoaded", LuaScriptManagerIsLoaded },
    { "AbortLoading", LuaScriptManagerAbortLoading },
    { 0, 0 }
};

// std::find for the Path arrays (instance Mac 0x2d7eb0, Win 0x455d90).
bool ContainsPath(const BasicArray<Path>& paths, const Path& path)
{
    for (const Path* p = paths.Begin(); p != paths.End(); ++p) {
        if (*p == path)
            return true;
    }
    return false;
}

} // namespace

// =============================================================================================
// LuaScriptManager::Lock

LuaScriptManager::Lock::Lock()
{
#if defined(_WIN32)
    InitializeCriticalSection(&m_section);
#else
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_mutex, &attributes);
    pthread_mutexattr_destroy(&attributes);
#endif
}

LuaScriptManager::Lock::~Lock()
{
#if defined(_WIN32)
    DeleteCriticalSection(&m_section);
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}

void LuaScriptManager::Lock::Enter()
{
#if defined(_WIN32)
    EnterCriticalSection(&m_section);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void LuaScriptManager::Lock::Leave()
{
#if defined(_WIN32)
    LeaveCriticalSection(&m_section);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

// =============================================================================================
// LuaScriptManagerLoadCallback_script

void LuaScriptManagerLoadCallback_script::OnLoaded(Object* /*request*/, bool success) // Mac 0x2d7360, Win 0x452470
{
    LuaCall call(m_function);
    call << success << LuaRun();
}

// =============================================================================================
// Parser

LuaScriptManager::Parser::Parser(LuaScriptManager* manager, const Path& path, Queue* queue) // Mac 0x2d53a0, Win 0x4547f0
    : m_manager(manager), m_queue(queue)
{
    m_path.Assign(path);
}

void LuaScriptManager::Parser::Collect(BasicArray<const XMLParser*>& xml,
                                       BasicArray<const LUAParser*>& lua) const // Mac 0x2d5bf0, Win 0x454960
{
    for (const RefPointer<Parser>* child = m_children.Begin(); child != m_children.End(); ++child)
        (*child)->Collect(xml, lua);
}

// =============================================================================================
// XMLParser

void LuaScriptManager::XMLParser::Collect(BasicArray<const XMLParser*>& xml,
                                          BasicArray<const LUAParser*>& lua) const // Mac 0x2d5dd0, Win 0x454a60
{
    Parser::Collect(xml, lua);
    xml.PushBack(this);
}

void LuaScriptManager::XMLParser::Execute()                                // Mac 0x2d6030, Win 0x454a90
{
    m_dom = new XMLDOM;
    if (!m_dom->Parse(m_source)) {
        // NOTE: the messages of this TU are built and discarded (both builds).
        String message("Failed to parse XML layout file: ");
        message << m_path.GetPath(true);
        m_queue->m_state = Queue::kFailed;
        m_queue->m_manager->m_wakeUp.Signal();
        return;
    }
    if (IsCancelled())
        return;
    String path = m_path.GetPath(true);
    XMLDOMNode::NodeArray& nodes = m_dom->GetChildren();
    UInt32 i = 0;
    while (i < nodes.GetSize()) {
        XMLDOMNode* node = nodes[i];
        if (node->GetName() == String("Include")) {
            String file;
            if (!node->GetAttribute(String("file"), file)) {
                String message("XML layout file: ");
                message << path << " has <Include> tag but no file attribute.";
            } else {
                Path base;
                base.Assign(m_path);
                base.ToParent();
                m_manager->AddFile(file, base, this);
            }
            nodes.Erase(i);
        } else if (node->GetName() == String("Script")) {
            m_manager->AddScript(node->GetText().ToUTF8(), this);
            nodes.Erase(i);
        } else {
            node->SetAttribute(String("LuaScriptManagerSourceFile"), path);
            ++i;
        }
    }
}

// =============================================================================================
// LUAParser

LuaScriptManager::LUAParser::~LUAParser()                                  // Mac 0x2d6e20, Win 0x4551e0
{
    if (m_capacity && m_buffer)
        delete[] m_buffer;
}

void LuaScriptManager::LUAParser::Collect(BasicArray<const XMLParser*>& /*xml*/,
                                          BasicArray<const LUAParser*>& lua) const // Mac 0x2d6830, Win 0x454fe0
{
    if (m_size)
        lua.PushBack(this);
}

int LuaScriptManager::LUAParser::Write(lua_State* /*L*/, const void* data, size_t size, void* userData) // Mac 0x2d6d80, Win 0x455260
{
    LUAParser* parser = (LUAParser*)userData;
    // NOTE: the capacity is doubled once (at least 2 KB) whatever the size of the piece (both
    // builds): lua_dump writes a long string constant in one piece, so a script with one longer
    // than the new capacity overflows the buffer; fixed: it doubles until the piece fits.
    if (parser->m_capacity < parser->m_size + size) {
        UInt32 capacity = parser->m_capacity * 2;
        if (capacity < 0x800)
            capacity = 0x800;
        if ((UInt64)parser->m_size + size > 0x40000000u)
            return 1;                                      // lua_dump stops on a non-zero result
        while (capacity < parser->m_size + size)
            capacity *= 2;
        parser->m_capacity = capacity;
        char* buffer = new char[capacity];
        memcpy(buffer, parser->m_buffer, parser->m_size);
        delete[] parser->m_buffer;
        parser->m_buffer = buffer;
    }
    memcpy((char*)parser->m_buffer + parser->m_size, data, size);
    parser->m_size += (UInt32)size;
    return 0;
}

void LuaScriptManager::LUAParser::Execute()                                // Mac 0x2d6a60, Win 0x455000
{
    // NOTE: compares 5 bytes, the terminator of "\x1bLua" included, so a precompiled chunk
    // (version byte 0x51) is never recognised and goes through the compiler too (both builds).
    if (!m_source.IsEmpty() && memcmp(m_source.c_str(), "\x1bLua", 5) != 0) {
        LuaState* state = CreateLuaState();
        AString chunkName = m_path.GetPath(false).ToUTF8();
        if (m_embedded) {
            char number[128];
            sprintf(number, "%u", (unsigned)m_index);                      // Win _itoa(n, buf, 10)
            chunkName.Append(" # ");
            chunkName.Append(number);
        }
        if (!state->LoadBuffer(m_source.c_str(), m_source.GetLength(), chunkName.c_str())) {
            String message("Failed to parse LUA file: ");
            message << m_path.GetPath(true);
            LuaState::Destroy(state);
            m_queue->m_state = Queue::kFailed;
            m_queue->m_manager->m_wakeUp.Signal();
        } else {
            if (!IsCancelled())
                state->Dump(Write, this, 0, '=');
            LuaState::Destroy(state);
        }
        return;
    }
    m_buffer = m_source.c_str();
    m_size = m_source.GetLength();
}

// =============================================================================================
// Queue

LuaScriptManager::Queue::Queue(const Path& path, LuaScriptManager* manager,
                               LuaScriptManagerLoadCallback* callback, bool synchronous,
                               bool reload)                                 // Mac 0x2d1f70, Win 0x4545b0
    : m_state(kQueued), m_count(0), m_callback(callback), m_synchronous(synchronous),
      m_reload(reload), m_manager(manager)
{
    m_root = new Parser(manager, path, this);
}

void LuaScriptManager::Queue::Execute()                                    // Mac 0x2d5540, Win 0x454790
{
    if (m_state != kFailed) {
        m_xmlParsers.Reserve(m_count & 0xffff);
        m_luaParsers.Reserve(m_count >> 16);
        m_root->Collect(m_xmlParsers, m_luaParsers);
        m_count = 0;
        m_state = kParsed;
    }
    m_manager->m_wakeUp.Signal();
}

// =============================================================================================
// LuaScriptManager

LuaScriptManager::LuaScriptManager(LuaState* state, XMLDOM* layouts, bool keepAttached) // Mac 0x2d1530, Win 0x452820
    : m_luaState(state), m_ownState(false), m_layouts(layouts),
      m_operations(new OperationQueue), m_quit(false), m_keepAttached(keepAttached),
      m_updateCount(0)
{
    s_instance = this;
    if (!m_luaState) {
        m_luaState = CreateLuaState();
        m_ownState = true;
    }
    if (!m_layouts)
        m_layouts = new XMLDOM;
    m_directory = Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);

    LuaObject globals = m_luaState->GetGlobals();
    LuaObject table = globals.CreateTable("ScriptManager", 0, 0);
    lua_pushlightuserdata(m_luaState->GetCState(), this);
    table.Register(kScriptManagerFunctions, 1);
    if (m_keepAttached) {
        RefPointer<Updater> updater = SystemUpdater::GetInstance();
        UpdateObserver::Attach(updater);
    }
    Start(0);
}

LuaScriptManager::~LuaScriptManager()                                      // Mac 0x2d28f0, Win 0x453430
{
    s_instance = 0;
    if (m_ownState)
        LuaState::Destroy(m_luaState);
}

RefPointer<LuaScriptManager> LuaScriptManager::GetInstance()               // Mac 0x2d1bd0, Win 0x452c90
{
    return RefPointer<LuaScriptManager>(s_instance);
}

LuaObject LuaScriptManager::GetGlobals() const                             // Mac 0x2d28c0, Win 0x4533d0
{
    return m_luaState->GetGlobals();
}

RefPointer<Object> LuaScriptManager::Load(const String& name, LuaScriptManagerLoadCallback* callback,
                                          bool synchronous, bool reload)    // Mac 0x2d1260, Win 0x452d30
{
    Path path;
    path.SetElement(m_directory);
    if (!path.Append(name))
        return RefPointer<Object>();
    return Load(path, callback, synchronous, reload);
}

RefPointer<Object> LuaScriptManager::Load(const Path& path, LuaScriptManagerLoadCallback* callback,
                                          bool synchronous, bool reload)    // Mac 0x2d1c30, Win 0x452de0
{
    RefPointer<Queue> queue = new Queue(path, this, callback, synchronous, reload);
    m_lock.Enter();
    m_queues.PushBack(queue);
    if (synchronous) {
        m_pendingQueues.Insert(m_pendingQueues.Begin(), 1, queue);
        m_lock.Leave();
        m_wakeUp.Signal();
        queue->m_done.Wait();
        if (queue->m_state == Queue::kParsed) {
            for (const XMLParser** xml = queue->m_xmlParsers.Begin(); xml != queue->m_xmlParsers.End(); ++xml)
                AddLayouts(*xml);
            bool success = true;
            for (const LUAParser** lua = queue->m_luaParsers.Begin(); lua != queue->m_luaParsers.End(); ++lua) {
                m_luaState->LoadBuffer((*lua)->m_buffer, (*lua)->m_size, 0);
                success &= m_luaState->PCall(0, 0);
            }
            queue->m_state = success ? Queue::kDone : Queue::kFailed;
        }
        if (queue->m_callback)
            queue->m_callback->OnLoaded(queue, queue->m_state == Queue::kDone);
    } else {
        m_pendingQueues.PushBack(queue);
        m_lock.Leave();
        m_wakeUp.Signal();
        if (!GetSubject()) {
            RefPointer<Updater> updater = SystemUpdater::GetInstance();
            UpdateObserver::Attach(updater);
        }
    }
    return RefPointer<Object>(queue.Get());
}

bool LuaScriptManager::IsLoaded(const String& name)                        // Mac 0x2d1140, Win 0x453050
{
    Path path;
    path.SetElement(m_directory);
    if (!path.Append(name))
        return false;
    return IsLoaded(path);
}

bool LuaScriptManager::IsLoaded(const Path& path)                          // Mac 0x2d2510, Win 0x453100
{
    m_lock.Enter();
    bool loaded;
    if (!ContainsPath(m_loadedFiles, path))
        loaded = false;
    else if (!m_executingQueue)
        loaded = true;
    else
        loaded = !ContainsPath(m_executingQueue->m_files, path);
    m_lock.Leave();
    return loaded;
}

bool LuaScriptManager::IsLoading(Object* request)                          // Mac 0x2d25d0, Win 0x453210
{
    if (!request)
        return !m_queues.IsEmpty();
    RefPointer<Queue> queue = dynamic_cast<Queue*>(request);
    for (RefPointer<Queue>* it = m_queues.Begin(); it != m_queues.End(); ++it) {
        if (it->Get() == queue.Get())
            return true;
    }
    return false;
}

bool LuaScriptManager::HasFailed(Object* request)                          // Mac 0x2d2710, Win 0x453300
{
    // NOTE: no null check after the dynamic_cast (both builds).
    return dynamic_cast<Queue*>(request)->m_state == Queue::kFailed;
}

void LuaScriptManager::Abort()                                             // Mac 0x2d2750, Win 0x453320
{
    m_quit = true;
    m_wakeUp.Signal();
    if (IsRunning())
        Thread::Wait();
    UpdateObserver::Detach();
    m_executingQueue = 0;
    m_readyQueues.Clear();
    m_pendingQueues.Clear();
    m_queues.Clear();
}

void LuaScriptManager::OnFinalRelease()                                    // Mac 0x2d30a0, Win 0x453630
{
    Abort();
    // Observer::OnFinalRelease: leaves the updater, then Object::OnFinalRelease.
    UpdateObserver::OnFinalRelease();
}

bool LuaScriptManager::AddFile(const String& name, const Path& base, Parser* parent) // Mac 0x2d3920, Win 0x453e80
{
    if (m_quit)
        return true;
    m_lock.Enter();
    Path path;
    path.Assign(base);
    if (!name.IsEmpty() && !path.Append(name)) {
        String message("Failed to find file: ");
        message << path.GetPath(true) << "/" << name;
        m_parsingQueue->m_state = Queue::kFailed;
        m_parsingQueue->m_manager->m_wakeUp.Signal();
        m_lock.Leave();
        return false;
    }
    String fileName = path.GetPath(false);
    if (!m_parsingQueue->m_reload) {
        if (ContainsPath(m_loadedFiles, path) || ContainsPath(m_parsingQueue->m_files, path)) {
            m_lock.Leave();
            return true;
        }
    }
    m_parsingQueue->m_files.PushBack(path);
    if (fileName.CompareNoCase(fileName.GetLength() - 4, 4, String(".XML")) == 0) {
        RefPointer<Parser> parser = new XMLParser(this, path, m_parsingQueue);
        parent->m_children.PushBack(parser);
        m_parsingQueue->AddDependency(parser);
        m_parsingQueue->m_toRead.PushBack(parser);
        m_parsingQueue->m_count += 1;
    } else if (fileName.CompareNoCase(fileName.GetLength() - 4, 4, String(".LUA")) == 0) {
        RefPointer<Parser> parser = new LUAParser(this, path, m_parsingQueue);
        parent->m_children.PushBack(parser);
        m_parsingQueue->AddDependency(parser);
        m_parsingQueue->m_toRead.PushBack(parser);
        m_parsingQueue->m_count += 0x10000;
    } else {
        String message("Unknown file type: ");
        message << path.GetPath(true);
        m_parsingQueue->m_state = Queue::kFailed;
        m_parsingQueue->m_manager->m_wakeUp.Signal();
        m_lock.Leave();
        return false;
    }
    m_wakeUp.Signal();
    m_lock.Leave();
    return true;
}

void LuaScriptManager::AddScript(const AString& source, Parser* parent)    // Mac 0x2d50c0, Win 0x454310
{
    if (m_quit)
        return;
    m_lock.Enter();
    RefPointer<LUAParser> parser = new LUAParser(this, parent->m_path, m_parsingQueue);
    parent->m_children.PushBack(RefPointer<Parser>(parser));
    m_parsingQueue->AddDependency(parser);
    parser->m_index = parent->m_children.GetSize();
    parser->m_embedded = true;
    m_parsingQueue->m_count += 0x10000;
    parser->m_source = source;
    m_operations->Add(parser);
    m_lock.Leave();
}

void LuaScriptManager::AddLayouts(const XMLParser* parser)                 // Mac 0x2d2390, Win 0x454480
{
    XMLDOM* dom = parser->m_dom;
    XMLDOMNode::NodeArray& layouts = m_layouts->GetChildren();
    layouts.Reserve(dom->GetChildren().GetSize() + layouts.GetSize());
    const XMLDOMNode::NodeArray& nodes = dom->GetChildren();
    for (const RefPointer<XMLDOMNode>* node = nodes.Begin(); node != nodes.End(); ++node) {
        if (m_layouts->FindChild((*node)->GetName())) {
            // NOTE: built and discarded; the old layout is not replaced either (the new one is appended
            // after it, so FindChild keeps returning the old one; both builds).
            String message("XML layout file: ");
            message << parser->m_path.GetPath(true) << " has a layout named: " << (*node)->GetName()
                    << " that clash with the existing layout, the old layout is overwritten.";
        }
        layouts.PushBack(*node);
    }
}

bool LuaScriptManager::Run()                                               // Mac 0x2d3170, Win 0x453670
{
    while (!m_quit) {
        if (!m_parsingQueue) {
            m_lock.Enter();
            if (m_pendingQueues.IsEmpty()) {
                m_lock.Leave();
                m_wakeUp.Wait();
                continue;
            }
            m_parsingQueue = m_pendingQueues[0];
            m_pendingQueues.Erase(0);
            m_parsingQueue->m_state = Queue::kParsing;
            m_lock.Leave();
            Parser* root = m_parsingQueue->m_root;
            AddFile(String(), root->m_path, root);
            m_operations->Add(m_parsingQueue);
        }

        Queue* queue = m_parsingQueue;
        if (queue->m_state == Queue::kParsed || queue->m_state == Queue::kFailed) {
            if (queue->m_state == Queue::kParsed) {
                m_lock.Enter();
                if (!queue->m_reload) {
                    for (const Path* file = queue->m_files.Begin(); file != queue->m_files.End(); ++file)
                        m_loadedFiles.PushBack(*file);
                } else {
                    for (const Path* file = queue->m_files.Begin(); file != queue->m_files.End(); ++file) {
                        if (!ContainsPath(m_loadedFiles, *file))
                            m_loadedFiles.PushBack(*file);
                    }
                }
                if (!queue->m_synchronous)
                    queue->m_state = Queue::kReady;
            } else {
                m_operations->CancelAll();
                m_operations->Wait();
                m_lock.Enter();
            }
            if (queue->m_synchronous)
                queue->m_done.Signal();
            else
                m_readyQueues.PushBack(m_parsingQueue);
            m_parsingQueue = 0;
            m_lock.Leave();
        } else {
            m_lock.Enter();
            if (queue->m_toRead.IsEmpty()) {
                m_lock.Leave();
                m_wakeUp.Wait();
                continue;
            }
            RefPointer<Parser> parser = queue->m_toRead[0];
            queue->m_toRead.Erase(0);
            m_lock.Leave();
            AString data;
            if (!ReadFile(parser->m_path, data)) {
                String message("Failed to load file: ");
                message << parser->m_path.GetPath(true);
                queue->m_state = Queue::kFailed;
                queue->m_manager->m_wakeUp.Signal();
            } else {
                parser->m_source = data;
                m_operations->Add(parser);
            }
        }
    }
    m_operations->CancelAll();
    m_operations->Wait();
    if (m_parsingQueue) {
        m_lock.Enter();
        m_readyQueues.PushBack(m_parsingQueue);
        m_parsingQueue = 0;
        m_lock.Leave();
    }
    return true;
}

void LuaScriptManager::Update()                                            // Mac 0x2d49c0, Win 0x453b00
{
#if defined(_WIN32)
    // NOTE: 32-bit timeGetTime arithmetic: within 4 ms of the wrap the loop only ends through
    // the empty-queue exit (Win 0x453e30).
    UInt32 start = (UInt32)GetMilliseconds();
#else
    Int64 start = GetMilliseconds();
#endif
    for (;;) {
        if (!m_executingQueue) {
            m_lock.Enter();
            if (m_readyQueues.IsEmpty()) {
                if (!m_keepAttached && !m_parsingQueue && m_pendingQueues.IsEmpty())
                    UpdateObserver::Detach();
                m_lock.Leave();
                break;
            }
            m_executingQueue = m_readyQueues[0];
            m_readyQueues.Erase(0);
            m_lock.Leave();
        }
        Queue* queue = m_executingQueue;
        switch (queue->m_state) {
        case Queue::kReady:
            queue->m_count = 0;
            queue->m_state = Queue::kApplyingLayouts;
            // fall through
        case Queue::kApplyingLayouts:
            for (; queue->m_count < queue->m_xmlParsers.GetSize(); ++queue->m_count)
                AddLayouts(queue->m_xmlParsers[queue->m_count]);
            queue->m_count = 0;
            queue->m_state = Queue::kRunningScripts;
            // fall through
        case Queue::kRunningScripts: {
            bool success = true;
            for (; queue->m_count < queue->m_luaParsers.GetSize(); ++queue->m_count) {
                const LUAParser* lua = queue->m_luaParsers[queue->m_count];
                m_luaState->LoadBuffer(lua->m_buffer, lua->m_size, 0);
                success &= m_luaState->PCall(0, 0);
            }
            queue->m_state = success ? Queue::kDone : Queue::kFailed;
        }
            // fall through
        case Queue::kDone:
        case Queue::kFailed: {
            if (queue->m_callback)
                queue->m_callback->OnLoaded(queue, queue->m_state == Queue::kDone);
            for (UInt32 i = 0; i < m_queues.GetSize(); ++i) {
                if (m_queues[i].Get() == queue) {
                    m_queues.Erase(i);
                    break;
                }
            }
            m_executingQueue = 0;
            break;
        }
        default:
            break;
        }
#if defined(_WIN32)
        if (start <= 0xfffffffbu && (UInt32)GetMilliseconds() >= start + 4)
            break;
#else
        if (GetMilliseconds() >= start + 4)
            break;
#endif
    }
    if (m_keepAttached) {
        if (m_updateCount <= 100) {
            ++m_updateCount;
        } else {
            m_luaState->GC(LUA_GCSTEP, 5);
            m_updateCount = 0;
        }
    }
}

} // namespace LEGO
