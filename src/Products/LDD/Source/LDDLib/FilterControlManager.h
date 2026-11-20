// LEGO Digital Designer — restored source (declaration only)
// LEGO::FilterControlManager — interface of the owner of a brick filter (the brick palette):
//   the filter controls (FilterControl, material picker, search box) add / remove filters.
//
// RTTI: polymorphic class without bases. Mac vtable 0x761788 {5 × pure}, Win 0x9d760c {5 × purecall}
//   (no destructor slot). BrickPaletteControl implements it (Mac sub-object +0x17c, Win +0x190;
//   its thunks Mac 0x6b1b0 / 0x6b310 / 0x6b260 / 0x6b3c0 / 0x6ffd0).
// Slots used by EditMode (TU 085): 0 AddFilter (Win via +0x190 slot 0; Mac primary slot 36),
//   1 RemoveFilter (Win slot 1; Mac primary 38), 3 ClearFilter (Win slot 3; Mac primary 39).
// Names inferred (ClearFilter: Lua name).
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

class BrickFilter;
class CompositeBrickFilter;

class FilterControlManager {
public:
    // Adds `filter`, replacing a filter of the same type.
    virtual void AddFilter(const BrickFilter* filter) = 0;                    // 0
    // Removes the filters of the type of `filter`.
    virtual void RemoveFilter(const BrickFilter* filter) = 0;                 // 1
    // Removes exactly `filter`.
    virtual void RemoveFilterInstance(const BrickFilter* filter) = 0;         // 2
    virtual void ClearFilter() = 0;                                           // 3
    // The composite of the current filters.
    virtual CompositeBrickFilter* GetFilter() const = 0;                      // 4
};

} // namespace LEGO
