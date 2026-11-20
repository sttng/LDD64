// LEGO Digital Designer — restored source
// The selection filters of the selection tools (SelectionTool::Filter): LEGO::
// IncludeSceneOnlyFilter (every part of the scene), IncludeModelOnlyFilter (every part of the
// picked part's model: its HingedElement), IncludeDesignOnlyFilter (keeps the parts with the
// picked design), IncludeMaterialOnlyFilter (keeps the parts with the picked part's colour).
//
// Mac TU 083 "IncludeModelOnlyFilter.cpp" (0x181270-0x18182b); Win 0x627789-0x627dda,
// 0x66026a-0x6603d2.
// RTTI (all : SelectionTool::Filter, sizeof 8): vtables Mac 0x7148e0 / 0x71490c / 0x71493c /
// 0x71496c {D1, D0, Object slots, Apply}; Win 0x9d03e8 / 0x9d080c / 0x9d0418 / 0x9d0400.
// Class names from RTTI; method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "SelectionTool.h"

namespace LEGO {

class IncludeSceneOnlyFilter : public SelectionTool::Filter {
public:
    IncludeSceneOnlyFilter() {}                                                // Win 0x6277b5
    virtual ~IncludeSceneOnlyFilter() {}                                       // Mac 0x181670 / 0x181680, Win 0x6277e1 / sdd 0x627805
    // Mac 0x181270, Win 0x66026a
    virtual void Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const;
};

class IncludeModelOnlyFilter : public SelectionTool::Filter {
public:
    IncludeModelOnlyFilter() {}
    virtual ~IncludeModelOnlyFilter() {}                                       // Mac 0x1816c0 / 0x1816d0, Win 0x627db6 / sdd 0x627dda
    // Mac 0x181330, Win 0x66033d
    virtual void Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const;
};

class IncludeDesignOnlyFilter : public SelectionTool::Filter {
public:
    IncludeDesignOnlyFilter() {}                                               // Win 0x62788d
    virtual ~IncludeDesignOnlyFilter() {}                                      // Mac 0x181710 / 0x181720, Win 0x6278b9 / sdd 0x6278dd
    // Mac 0x181490, Win 0x6602b4
    virtual void Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const;
};

class IncludeMaterialOnlyFilter : public SelectionTool::Filter {
public:
    IncludeMaterialOnlyFilter() {}                                             // Win 0x627821
    virtual ~IncludeMaterialOnlyFilter() {}                                    // Mac 0x181760 / 0x181770, Win 0x62784d / sdd 0x627871
    // Mac 0x181570, Win 0x6602f4
    virtual void Apply(const PrimitiveElement* picked, BasicArray<RefPointer<Part> >& parts) const;
};

} // namespace LEGO
