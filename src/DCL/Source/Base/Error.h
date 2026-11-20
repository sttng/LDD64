// LEGO Digital Designer — restored source
// Error reporting: ReportedError tree, per-thread ErrorRoot, error handlers.
//
// Restored from TU "TraceErrorHandler.cpp" (Mac 0x5cf0-0x6bb0, Win 0x42bde0-0x42cb60).
// File/directory names are inferred, not confirmed.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicList.h"

namespace LEGO {

// ---------------------------------------------------------------------------
// 64-bit error code layout (decoded from TraceErrorHandler::Dump):
//   bits  0..15  value
//   bits 16..47  ident     (usually a FourCC, printed as 'abcd' when 4 printable chars)
//   bits 48..55  type      (ErrorType, unsigned byte; 0xFF = other)
//   bits 56..63  severity  (ErrorSeverity, unsigned byte; 0xFF = other)
//   "facility" = bits 16..55 (ident + type)
// ---------------------------------------------------------------------------
typedef UInt64 ErrorCode;

enum ErrorSeverity {
    errSeverityOther   = 0xFF,   // compared as unsigned byte in both builds
    errSeverityWarning = 0,
    errSeverityError   = 1
};

enum ErrorType {
    errTypeOther       = 0xFF,
    errTypeGeneric     = 0,
    errTypeSystem      = 1,
    errTypeEngine      = 2,
    errTypeHandle      = 3,
    errTypeInterface   = 4,
    errTypeHook        = 5,
    errTypeDCL         = 6,
    errTypeInternal    = 7,
    errTypeExternal    = 8,
    errTypeApplication = 9
};

inline UInt16 ErrorGetValue(ErrorCode c)    { return (UInt16)c; }
inline UInt32 ErrorGetIdent(ErrorCode c)    { return (UInt32)(c >> 16); }
inline UInt8  ErrorGetType(ErrorCode c)     { return (UInt8)(c >> 48); }
inline UInt8  ErrorGetSeverity(ErrorCode c) { return (UInt8)(c >> 56); }
inline UInt64 ErrorGetFacility(ErrorCode c) { return (c >> 16) & 0xFFFFFFFFFFULL; }

// ---------------------------------------------------------------------------
// ReportedError — abstract node interface.
// Vtable order (Mac slots after D1/D0; Win after scalar deleting dtor):
// ---------------------------------------------------------------------------
class ReportedError {
public:
    virtual ~ReportedError() {}
    virtual ErrorCode            GetCode() const = 0;                 // Mac 0x6390
    virtual const String&        GetDescription() const = 0;          // Mac 0x63a0
    virtual bool                 IsHandled() const = 0;               // Mac 0x63b0
    virtual bool                 AreChildrenHandled() const = 0;      // Mac 0x63e0
    virtual void                 SetHandled() = 0;                    // Mac 0x6420
    virtual void                 VisitChildren(class VisitFunction& f) = 0; // Mac 0x6430
    virtual bool                 HasChildren() const = 0;             // Mac 0x6470
    virtual bool                 HasParent() const = 0;               // Mac 0x64a0
    virtual bool                 HasNext() const = 0;                 // Mac 0x64b0
    virtual bool                 HasPrevious() const = 0;             // Mac 0x64c0
    virtual ReportedError*       GetFirstChild() const = 0;           // Mac 0x64d0
    virtual ReportedError*       GetParent() const = 0;               // Mac 0x6510
    virtual ReportedError*       GetNext() const = 0;                 // Mac 0x6520
    virtual ReportedError*       GetPrevious() const = 0;             // Mac 0x6530
};

class VisitFunction {
public:
    virtual ~VisitFunction() {}
    virtual void Visit(ReportedError& error) = 0;
};

class ErrorHandler {
public:
    virtual ~ErrorHandler() {}                                        // Win 0x42bde0
    virtual void HandleError(ReportedError& error) = 0;
};

// ---------------------------------------------------------------------------
// ErrorTree — concrete node. sizeof == 0x34 (Mac).
// Children form a doubly linked sibling list; new errors are inserted at the front.
// "Group" nodes (m_isGroup) are containers: they are never visited themselves and
// count as handled once all their children are handled.
// ---------------------------------------------------------------------------
class ErrorTree : public ReportedError {
public:
    ErrorTree();
    ErrorTree(ErrorCode code, const String& description);
    virtual ~ErrorTree();                                             // Mac 0x6280/0x6300

    virtual ErrorCode       GetCode() const;
    virtual const String&   GetDescription() const;
    virtual bool            IsHandled() const;
    virtual bool            AreChildrenHandled() const;
    virtual void            SetHandled();
    virtual void            VisitChildren(VisitFunction& f);
    virtual bool            HasChildren() const;
    virtual bool            HasParent() const;
    virtual bool            HasNext() const;
    virtual bool            HasPrevious() const;
    virtual ReportedError*  GetFirstChild() const;
    virtual ReportedError*  GetParent() const;
    virtual ReportedError*  GetNext() const;
    virtual ReportedError*  GetPrevious() const;

    void DeleteFollowingSiblings();                                   // Mac 0x6540

    ErrorTree* m_parent;       // +0x04
    ErrorTree* m_firstChild;   // +0x08
    ErrorTree* m_next;         // +0x0c
    ErrorTree* m_previous;     // +0x10
    ErrorCode  m_code;         // +0x14
    String     m_description;  // +0x1c
    bool       m_handled;      // +0x2c
    bool       m_isGroup;      // +0x2d
    UInt32     m_scopeCount;   // +0x30  non-zero while an error scope is open (name inferred)
};

// ---------------------------------------------------------------------------
// TraceErrorHandler — writes the error tree to the trace log.
// ---------------------------------------------------------------------------
class TraceErrorHandler : public ErrorHandler, public VisitFunction {
public:
    TraceErrorHandler() : m_depth(0) {}
    virtual ~TraceErrorHandler() {}                                   // Mac 0x6910/0x6920
    virtual void HandleError(ReportedError& error);                   // Mac 0x5cf0  Win 0x42bff0
    virtual void Visit(ReportedError& error);                         // Mac 0x6250  Win 0x42c0a0

private:
    void Dump(ReportedError& error);                                  // Mac 0x5da0  Win 0x42c0c0

    UInt32 m_depth;            // +0x08
};

// ---------------------------------------------------------------------------
// ErrorRoot — one per thread (lives inside the ThreadHook at +4).
// sizeof == 0x24 (Mac).
// ---------------------------------------------------------------------------
class ErrorRoot : public VisitFunction {
public:
    ErrorRoot();                                                      // Mac 0x65d0
    virtual ~ErrorRoot();                                             // Mac 0x6960/0x69f0
    virtual void Visit(ReportedError& error);                         // Mac 0x6880

    void AddHandler(ErrorHandler* handler);                           // Mac 0x66a0
    void EnableTrace();                                               // Mac 0x66e0 (name inferred)
    void Report(ErrorCode code, const String& description);           // Mac 0x6750

private:
    ErrorTree*                 m_root;      // +0x04
    ErrorTree*                 m_current;   // +0x08
    BasicList<ErrorHandler*>   m_handlers;  // +0x0c
    TraceErrorHandler          m_trace;     // +0x18
};

ErrorRoot& GetErrorRoot();                                            // Mac 0x68c0
void ReportError(ErrorCode code, const String& description);         // Mac 0x68e0

} // namespace LEGO
