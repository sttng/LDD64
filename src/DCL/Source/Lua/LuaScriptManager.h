// LEGO Digital Designer — restored source
// LEGO::LuaScriptManager — loads the Lua/XML user interface (Scripts/LDD.xml and its includes)
// into the shared LuaPlus state and the layouts XMLDOM. Queue = one load request, Parser /
// XMLParser / LUAParser = one file of it, LuaScriptManagerLoadCallback(_script) = completion.
// TU 324 "LuaScriptManager.cpp": Mac 0x2d0ed0-0x2d80db, Win 0x452190-0x455d5f.
//
// Processing (Queue states, names inferred):
//   Load()    creates a Queue (kQueued) with a root Parser and hands it to the loader thread
//             (front of the list when synchronous).
//   Run()     (loader thread) takes the next queue (kParsing), resolves the root file (AddFile:
//             ".xml" -> XMLParser, ".lua" -> LUAParser, each file once unless `reload`), reads
//             every file and runs the parsers on the OperationQueue:
//               XMLParser: <Include file="…"/> adds a file (relative to this one), <Script>
//                 adds an embedded Lua chunk, every other element is a layout (attribute
//                 LuaScriptManagerSourceFile added);
//               LUAParser: compiles the chunk in a private state and keeps the bytecode
//                 (lua_dump; the precompiled-input check never matches, see Execute).
//             The Queue is an Operation depending on its parsers: Execute() collects them depth
//             first (includes first), kParsed. Run() records the files as loaded and wakes the
//             synchronous caller or queues the request for Update() (kReady). Errors: kFailed.
//   Update()  (main thread, SystemUpdater observer) moves the layouts into the layouts XMLDOM
//             (kApplyingLayouts), runs the bytecode (kRunningScripts), calls the callback
//             (kDone / kFailed), ~4 ms per call; every 101st call while kept attached: one
//             incremental GC step.
// Lua: global table ScriptManager {Load(file [, callback(success)] [, synchronous]),
// IsLoaded(file), AbortLoading()} (upvalue: the manager).
//
// RTTI: LuaScriptManager vmi {UpdateObserver (private) +0, Thread (private) +8, virtual Object};
// Mac vtables 0x749afc {D1 0x2d2df0, D0 0x2d2f30, OnFinalRelease 0x2d30a0, OnAttached 0x1e380,
// OnDetached 0x1e390, Update 0x2d49c0, Run 0x2d3170}, Thread-in 0x749b20 {0x2d2e50, 0x2d2fa0,
// 0xeb80, 0xeb90, Run thunk 0x2d49a0, 0xebb0, 0xebc0}; Win 0xa2138c {0x40be80, 0x40be80,
// Update 0x453b00}, Thread-in 0xa2139c {…, Run 0x453670, …}.
// Queue / Parser : Operation (Mac Queue 0x749bb4 {Execute 0x2d5540, D1 0x2d6fe0, D0 0x2d7040};
// Parser 0x749c08 {Execute 0x2d71a0 (empty), D1 0x2d71b0, D0 0x2d7210, Collect 0x2d5bf0};
// XMLParser 0x749c60 {0x2d6030, 0x2d66f0, 0x2d67b0, 0x2d5dd0}; LUAParser 0x749cc4 {0x2d6a60,
// 0x2d6e20, 0x2d6f30, 0x2d6830}). Class names: RTTI; member and method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicString.h"
#include "LEGO/String.h"
#include "Event.h"
#include "Thread.h"
#include "Updater.h"
#include "OperationManager.h"
#include "Path.h"
#include "XMLDOMNode.h"
#include "LuaPlus.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

class Directory;

// Told on the main thread when a request has finished. RTTI vmi, virtual base Object; Win vtable
// 0xa21370 {_purecall}. (method name inferred)
class LuaScriptManagerLoadCallback : public virtual Object {
public:
    // `request` = the handle Load() returned; `success` = every script ran without error.
    virtual void OnLoaded(Object* request, bool success) = 0;              // slot 0
    virtual ~LuaScriptManagerLoadCallback() {}                             // Win 0x452150 (sdd)
};

// The callback of ScriptManager.Load(file, function): calls function(success).
// Mac sizeof 0x24 (LuaObject +0x04, Object +0x1c); Win 0x30 (LuaObject +0x08, Object +0x28).
class LuaScriptManagerLoadCallback_script : public LuaScriptManagerLoadCallback {
public:
    explicit LuaScriptManagerLoadCallback_script(const LuaPlus::LuaObject& function)
        : m_function(function) {}                                         // inlined (Mac 0x2d0fa7)
    virtual void OnLoaded(Object* request, bool success);                  // Mac 0x2d7360, Win 0x452470
    virtual ~LuaScriptManagerLoadCallback_script() {}                      // Mac 0x2d73f0/0x2d7450, Win 0x452500/0x452580

private:
    LuaPlus::LuaObject m_function;   // Mac +0x04, Win +0x08
};

class LuaScriptManager : private UpdateObserver, private Thread, public virtual Object {
public:
    class Queue;
    class XMLParser;
    class LUAParser;

    // One file of a request (the root of a request is a plain Parser naming the start file).
    // Mac sizeof 0x6c (Object +0x64), Win 0x60 (Object +0x58).
    class Parser : public Operation {
    public:
        Parser(LuaScriptManager* manager, const Path& path, Queue* queue);   // Mac 0x2d53a0 / 0x2d59d0 (C2), Win 0x4547f0
        virtual void Execute() {}                                            // Mac 0x2d71a0
        virtual ~Parser() {}                                                 // Mac 0x2d71b0/0x2d7210 (body 0x2d5c40), Win 0x4522b0/0x452390
        // Appends the XML and Lua parsers of this subtree, children first. (name inferred)
        virtual void Collect(BasicArray<const XMLParser*>& xml,
                             BasicArray<const LUAParser*>& lua) const;       // Mac 0x2d5bf0, Win 0x454960

        LuaScriptManager*                 m_manager;    // Mac +0x30, Win +0x30
        Path                              m_path;       // Mac +0x34, Win +0x34
        Queue*                            m_queue;      // Mac +0x4c, Win +0x40
        AString                           m_source;     // Mac +0x50, Win +0x44 file contents
        BasicArray<RefPointer<Parser> >   m_children;   // Mac +0x54, Win +0x48 included files / scripts
    };

    // A layout file. Mac sizeof 0x70, Win 0x64.
    class XMLParser : public Parser {
    public:
        XMLParser(LuaScriptManager* manager, const Path& path, Queue* queue)
            : Parser(manager, path, queue) {}                               // inlined (Mac 0x2d39f0), Win 0x454990
        virtual void Execute();                                              // Mac 0x2d6030, Win 0x454a90
        virtual ~XMLParser() {}                                              // Mac 0x2d66f0/0x2d67b0, Win 0x454e90/0x454a30
        virtual void Collect(BasicArray<const XMLParser*>& xml,
                             BasicArray<const LUAParser*>& lua) const;       // Mac 0x2d5dd0, Win 0x454a60

        RefPointer<XMLDOM> m_dom;   // Mac +0x64, Win +0x58 the parsed file (layouts as children)
    };

    // A Lua file or an embedded <Script>. Mac sizeof 0x80, Win 0x74.
    class LUAParser : public Parser {
    public:
        LUAParser(LuaScriptManager* manager, const Path& path, Queue* queue)
            : Parser(manager, path, queue), m_buffer(0), m_size(0), m_capacity(0), m_index(0),
              m_embedded(false) {}                                           // inlined (Mac 0x2d3d7d), Win 0x454f10
        virtual void Execute();                                              // Mac 0x2d6a60, Win 0x455000
        virtual ~LUAParser();                                                // Mac 0x2d6e20/0x2d6f30, Win 0x4551e0/0x454fb0
        virtual void Collect(BasicArray<const XMLParser*>& xml,
                             BasicArray<const LUAParser*>& lua) const;       // Mac 0x2d6830, Win 0x454fe0

        const char* m_buffer;     // Mac +0x64, Win +0x58 bytecode (or m_source when precompiled/empty)
        UInt32      m_size;       // Mac +0x68, Win +0x5c
        UInt32      m_capacity;   // Mac +0x6c, Win +0x60 0 = m_buffer is not owned
        UInt32      m_index;      // Mac +0x70, Win +0x64 1-based position among the parent's children
        bool        m_embedded;   // Mac +0x74, Win +0x68 a <Script> of an XML file

    private:
        // lua_dump writer: appends to m_buffer. (name inferred)
        static int Write(lua_State* L, const void* data, size_t size, void* parser); // Mac 0x2d6d80, Win 0x455260
    };

    // One load request. Mac sizeof 0xe0 (Object +0xd8), Win 0x98 (Object +0x90).
    class Queue : public Operation {
    public:
        enum State {                                   // (names inferred)
            kQueued          = 0,
            kParsing         = 1,
            kParsed          = 2,
            kReady           = 3,
            kApplyingLayouts = 4,
            kRunningScripts  = 5,
            kDone            = 6,
            kFailed          = 7
        };
        Queue(const Path& path, LuaScriptManager* manager, LuaScriptManagerLoadCallback* callback,
              bool synchronous, bool reload);                                // Mac 0x2d1f70, Win 0x4545b0
        // All parsers have finished: collects them (kParsed) unless the request failed.
        virtual void Execute();                                              // Mac 0x2d5540, Win 0x454790
        virtual ~Queue() {}                                                  // Mac 0x2d6fe0/0x2d7040 (body 0x2d76c0), Win 0x452190/0x452350

        UInt32                                   m_state;        // Mac +0x30, Win +0x30 State
        UInt32                                   m_count;        // Mac +0x34, Win +0x34 parsing: XML files (low 16 bits) and
                                                                 //   Lua chunks (high 16 bits); Update(): progress index
        BasicArray<Path>                         m_files;        // Mac +0x38, Win +0x38 files of this request
        BasicArray<Parser*>                      m_toRead;       // Mac +0x48, Win +0x48 files not read yet
        RefPointer<Parser>                       m_root;         // Mac +0x58, Win +0x58
        BasicArray<const XMLParser*>             m_xmlParsers;   // Mac +0x5c, Win +0x5c
        BasicArray<const LUAParser*>             m_luaParsers;   // Mac +0x6c, Win +0x6c
        RefPointer<LuaScriptManagerLoadCallback> m_callback;     // Mac +0x7c, Win +0x7c
        bool                                     m_synchronous;  // Mac +0x80, Win +0x80
        Event                                    m_done;         // Mac +0x84, Win +0x84 parsed (synchronous requests)
        bool                                     m_reload;       // Mac +0xd0, Win +0x88 load files again
        LuaScriptManager*                        m_manager;      // Mac +0xd4, Win +0x8c
    };

    // `state` / `layouts` null: own ones are created. `keepAttached`: stay attached to the
    // SystemUpdater (else only while requests are pending). Starts the loader thread.
    LuaScriptManager(LuaPlus::LuaState* state, XMLDOM* layouts, bool keepAttached); // Mac 0x2d1530, Win 0x452820
    virtual ~LuaScriptManager();                                             // Mac 0x2d2df0/0x2d2f30 (body 0x2d28f0), Win 0x453430/0x452c60

    // The instance created last (null after its destruction). (name inferred)
    static RefPointer<LuaScriptManager> GetInstance();                       // Mac 0x2d1bd0, Win 0x452c90
    LuaPlus::LuaState* GetLuaState() const { return m_luaState; }           // Mac 0x2d1c10, Win 0x452d10
    XMLDOM*            GetLayouts() const  { return m_layouts; }            // Mac 0x2d1c20, Win 0x452d20
    LuaPlus::LuaObject GetGlobals() const;                                  // Mac 0x2d28c0, Win 0x4533d0

    // Loads `name` (relative to the Assets directory). Returns the request handle (null when
    // the path cannot be resolved). NOTE: a synchronous request is processed before the call
    // returns but stays in the request list, so IsLoading(handle) remains true (both builds).
    RefPointer<Object> Load(const String& name, LuaScriptManagerLoadCallback* callback,
                            bool synchronous, bool reload);                  // Mac 0x2d1260, Win 0x452d30
    RefPointer<Object> Load(const Path& path, LuaScriptManagerLoadCallback* callback,
                            bool synchronous, bool reload);                  // Mac 0x2d1c30, Win 0x452de0
    // The file was loaded (and is not part of the request being executed).
    bool IsLoaded(const String& name);                                       // Mac 0x2d1140, Win 0x453050
    bool IsLoaded(const Path& path);                                         // Mac 0x2d2510, Win 0x453100
    // `request` (a Load() handle) is still pending; null: any request is pending.
    bool IsLoading(Object* request);                                         // Mac 0x2d25d0, Win 0x453210
    bool HasFailed(Object* request);                                         // Mac 0x2d2710, Win 0x453300
    // Stops the loader thread and drops every pending request.
    void Abort();                                                            // Mac 0x2d2750, Win 0x453320

    // (loader side, called by the parsers; names inferred)
    // Adds file `name` relative to `base` to the request being parsed, as a child of `parent`.
    bool AddFile(const String& name, const Path& base, Parser* parent);      // Mac 0x2d3920, Win 0x453e80
    // Adds an embedded <Script> chunk of `parent`.
    void AddScript(const AString& source, Parser* parent);                   // Mac 0x2d50c0, Win 0x454310

protected:
    virtual void OnFinalRelease();                                           // Mac 0x2d30a0, Win 0x453630
    virtual void Update();                                                   // Mac 0x2d49c0, Win 0x453b00
    virtual bool Run();                                                      // Mac 0x2d3170 (thunk 0x2d49a0), Win 0x453670

private:
    LuaScriptManager(const LuaScriptManager&);
    LuaScriptManager& operator=(const LuaScriptManager&);

    // Moves the layouts of a parsed file into m_layouts. (name inferred)
    void AddLayouts(const XMLParser* parser);                                // Mac 0x2d2390, Win 0x454480

    // Embedded recursive lock (pthread recursive mutex / CRITICAL_SECTION, inlined). (name inferred)
    class Lock {
    public:
        Lock();
        ~Lock();
        void Enter();
        void Leave();
    private:
        Lock(const Lock&);
        Lock& operator=(const Lock&);
#if defined(_WIN32)
        CRITICAL_SECTION m_section;
#else
        pthread_mutex_t  m_mutex;
#endif
    };

    LuaPlus::LuaState*                 m_luaState;         // Mac +0x78, Win +0x7c
    bool                               m_ownState;         // Mac +0x7c, Win +0x80
    RefPointer<XMLDOM>                 m_layouts;          // Mac +0x80, Win +0x84
    RefPointer<Directory>              m_directory;        // Mac +0x84, Win +0x88 Assets
    BasicArray<Path>                   m_loadedFiles;      // Mac +0x88, Win +0x8c
    BasicArray<RefPointer<Queue> >     m_queues;           // Mac +0x98, Win +0x9c every unfinished request
    BasicArray<RefPointer<Queue> >     m_pendingQueues;    // Mac +0xa8, Win +0xac waiting for the loader thread
    BasicArray<RefPointer<Queue> >     m_readyQueues;      // Mac +0xb8, Win +0xbc waiting for Update()
    RefPointer<Queue>                  m_parsingQueue;     // Mac +0xc8, Win +0xcc
    RefPointer<Queue>                  m_executingQueue;   // Mac +0xcc, Win +0xd0
    RefPointer<OperationQueue>         m_operations;       // Mac +0xd0, Win +0xd4
    Lock                               m_lock;             // Mac +0xd4, Win +0xd8
    Event                              m_wakeUp;           // Mac +0x100, Win +0xf0 wakes the loader thread
    volatile bool                      m_quit;             // Mac +0x14c, Win +0xf4
    bool                               m_keepAttached;     // Mac +0x14d, Win +0xf5
    UInt32                             m_updateCount;      // Mac +0x150, Win +0xf8

    static LuaScriptManager* s_instance;                                     // Mac 0x77ed90, Win 0xb3ffe0
};

} // namespace LEGO
