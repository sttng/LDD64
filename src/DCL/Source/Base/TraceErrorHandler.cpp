// LEGO Digital Designer — restored source
// TU "TraceErrorHandler.cpp" (name inferred from the main class): Mac 0x5cf0-0x6bb0, Win
// 0x42bde0-0x42cb60. Tested by tests/test_error.cpp.
#include "Error.h"
#include "LEGO/String.h"
#include "Trace.h"
#include "ThreadHook.h"

#include <string.h>

namespace LEGO {

// ===========================================================================
// TraceErrorHandler
// ===========================================================================

// Mac 0x5cf0, Win 0x42bff0
void TraceErrorHandler::HandleError(ReportedError& error)
{
    Trace(String("-------------------Begin----------------------"), 1);
    Dump(error);
    Trace(String("--------------------End----------------------"), 1);
}

// Mac 0x6250 (thunk 0x6260 for the VisitFunction sub-object), Win 0x42c0a0
void TraceErrorHandler::Visit(ReportedError& error)
{
    Dump(error);
}

// Mac 0x5da0, Win 0x42c0c0
void TraceErrorHandler::Dump(ReportedError& error)
{
    String s;
    for (UInt32 i = 0; i < m_depth; ++i)
        s << "+";

    s << " ThreadID: " << GetCurrentThreadID();
    s << " Fac: " << ErrorGetFacility(error.GetCode());
    s << " Value: " << ErrorGetValue(error.GetCode());

    s << " Severity: ";
    switch (ErrorGetSeverity(error.GetCode())) {
    case errSeverityOther:   s << "errSeverityOther";   break;
    case errSeverityWarning: s << "errSeverityWarning"; break;
    case errSeverityError:   s << "errSeverityError";   break;
    }

    s << " Type: ";
    switch (ErrorGetType(error.GetCode())) {
    case errTypeOther:       s << "errTypeOther";       break;
    case errTypeGeneric:     s << "errTypeGeneric";     break;
    case errTypeSystem:      s << "errTypeSystem";      break;
    case errTypeEngine:      s << "errTypeEngine";      break;
    case errTypeHandle:      s << "errTypeHandle";      break;
    case errTypeInterface:   s << "errTypeInterface";   break;
    case errTypeHook:        s << "errTypeHook";        break;
    case errTypeDCL:         s << "errTypeDCL";         break;
    case errTypeInternal:    s << "errTypeInternal";    break;
    case errTypeExternal:    s << "errTypeExternal";    break;
    case errTypeApplication: s << "errTypeApplication"; break;
    default:                 s << (Int32)ErrorGetType(error.GetCode()); break;   // Mac 0x28a140
    }

    // The ident is printed as a four-character code when it is one.
    // NOTE: the original passes a non-terminated char[4] to strlen (reads past the array in
    // both builds); fixed: a 5th byte ends it.
    UInt32 ident = ErrorGetIdent(error.GetCode());
    char fourcc[5];
    fourcc[4] = 0;
    fourcc[0] = (char)(ident >> 24);
    fourcc[1] = (char)(ident >> 16);
    fourcc[2] = (char)(ident >> 8);
    fourcc[3] = (char)(ident);
    if (strlen(fourcc) == 4)
        s << " Ident: '" << String(fourcc, 4) << "'";
    else
        s << " Ident: " << ident;

    s << " Handled: " << (error.IsHandled() ? "true" : "false");
    s << " Description: " << error.GetDescription();
    Trace(s, 1);

    ++m_depth;
    error.VisitChildren(*this);
    --m_depth;
    error.SetHandled();
}

// ===========================================================================
// ErrorTree
// ===========================================================================

ErrorTree::ErrorTree()
    : m_parent(0), m_firstChild(0), m_next(0), m_previous(0),
      m_code(0), m_description(),
      m_handled(false), m_isGroup(true), m_scopeCount(0)
{
}

ErrorTree::ErrorTree(ErrorCode code, const String& description)
    : m_parent(0), m_firstChild(0), m_next(0), m_previous(0),
      m_code(code), m_description(description),
      m_handled(false), m_isGroup(false), m_scopeCount(0)
{
}

// Mac 0x6280 (complete) / 0x6300 (deleting)
ErrorTree::~ErrorTree()
{
    if (m_firstChild) {
        m_firstChild->DeleteFollowingSiblings();
        delete m_firstChild;
        m_firstChild = 0;
    }
}

// Mac 0x6540 — recursive: deletes the whole sibling chain after this node
void ErrorTree::DeleteFollowingSiblings()
{
    if (m_next) {
        m_next->DeleteFollowingSiblings();
        delete m_next;
        m_next = 0;
    }
}

ErrorCode ErrorTree::GetCode() const                { return m_code; }          // 0x6390
const String& ErrorTree::GetDescription() const     { return m_description; }   // 0x63a0

// Mac 0x63b0
bool ErrorTree::IsHandled() const
{
    if (m_handled)
        return true;
    if (m_isGroup)
        return AreChildrenHandled();
    return false;
}

// Mac 0x63e0
bool ErrorTree::AreChildrenHandled() const
{
    for (ErrorTree* c = m_firstChild; c; c = c->m_next)
        if (!c->IsHandled())
            return false;
    return true;
}

void ErrorTree::SetHandled() { m_handled = true; }                              // 0x6420

// Mac 0x6430 — group nodes are skipped
void ErrorTree::VisitChildren(VisitFunction& f)
{
    for (ErrorTree* c = m_firstChild; c; c = c->m_next)
        if (!c->m_isGroup)
            f.Visit(*c);
}

// Mac 0x6470
bool ErrorTree::HasChildren() const
{
    for (ErrorTree* c = m_firstChild; c; c = c->m_next)
        if (!c->m_isGroup)
            return true;
    return false;
}

bool ErrorTree::HasParent() const   { return m_parent != 0; }                  // 0x64a0
bool ErrorTree::HasNext() const     { return m_next != 0; }                    // 0x64b0
bool ErrorTree::HasPrevious() const { return m_previous != 0; }                // 0x64c0

// Mac 0x64d0
ReportedError* ErrorTree::GetFirstChild() const
{
    for (ErrorTree* c = m_firstChild; c; c = c->m_next)
        if (!c->m_isGroup)
            return c;
    return 0;
}

ReportedError* ErrorTree::GetParent() const   { return m_parent; }             // 0x6510
ReportedError* ErrorTree::GetNext() const     { return m_next; }               // 0x6520
ReportedError* ErrorTree::GetPrevious() const { return m_previous; }           // 0x6530

// ===========================================================================
// ErrorRoot
// ===========================================================================

// Mac 0x65d0
ErrorRoot::ErrorRoot()
    : m_root(new ErrorTree()), m_current(0)
{
    m_current = m_root;
}

// Mac 0x6960
ErrorRoot::~ErrorRoot()
{
    delete m_root;
}

// Mac 0x6880 — dispatch an error to every registered handler
void ErrorRoot::Visit(ReportedError& error)
{
    for (BasicList<ErrorHandler*>::Iterator it = m_handlers.Begin(); it != m_handlers.End(); ++it)
        (*it)->HandleError(error);
}

// Mac 0x66a0
void ErrorRoot::AddHandler(ErrorHandler* handler)
{
    m_handlers.PushBack(handler);
}

// Mac 0x66e0 — make sure the trace handler is registered exactly once, at the end
void ErrorRoot::EnableTrace()
{
    m_handlers.Remove(&m_trace);
    m_handlers.PushBack(&m_trace);
}

// Mac 0x6750
void ErrorRoot::Report(ErrorCode code, const String& description)
{
    ErrorTree* parent = m_current;
    ErrorTree* e = new ErrorTree(code, description);

    // insert as first child
    e->m_next = parent->m_firstChild;
    e->m_previous = 0;
    e->m_parent = parent;
    if (parent->m_firstChild)
        parent->m_firstChild->m_previous = e;
    parent->m_firstChild = e;

    // Outside of any error scope: hand the new errors to the handlers right away,
    // then drop every top-level error that got handled.
    if (m_current == m_root && m_current->m_scopeCount == 0) {
        m_current->VisitChildren(*this);

        ErrorTree* root = m_root;
        ErrorTree* next;
        for (ErrorTree* c = root->m_firstChild; c; c = next) {
            next = c->m_next;
            if (!c->IsHandled() || c->m_parent != root)
                continue;
            // unlink
            if (root->m_firstChild == c) {
                root->m_firstChild = next;
                if (next)
                    next->m_previous = 0;
            } else {
                c->m_previous->m_next = next;
                if (c->m_next)
                    c->m_next->m_previous = c->m_previous;
            }
            delete c;
        }
    }
}

// ===========================================================================
// Free functions
// ===========================================================================

// Mac 0x68c0
ErrorRoot& GetErrorRoot()
{
    return Thread::GetCurrent()->m_errorRoot;
}

// Mac 0x68e0
void ReportError(ErrorCode code, const String& description)
{
    GetErrorRoot().Report(code, description);
}

} // namespace LEGO
