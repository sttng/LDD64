// Vortex 5.0.0 build 800 "vortexldd41" - VxUniqueNameList (restored).
//
// A list of names that makes new names unique by appending "(1)", "(2)", ... (used by
// VxCompositeController::expose{Input,Output,Parameter}). Mac TU 522 0x47e3aa-0x47f528
// (src/Vx/VxUniqueNameList.cpp; no Win pairs: inlined or unpaired there). No RTTI, no strings:
// all names inferred. 16 bytes: one VxArray<std::string> (the ctor zeroes its three pointers).
#ifndef VX_VXUNIQUENAMELIST_H
#define VX_VXUNIQUENAMELIST_H

#include "Vx/VxArray.h"
#include <string>

namespace Vx {

class VxUniqueNameList {
public:
    VxUniqueNameList();                                      // C1 Mac 0x47e3f6 -> C2 0x47e3da
    ~VxUniqueNameList();                                     // D1 Mac 0x47e47a -> D2 0x47e440
    bool contains(const std::string& name) const;            // Mac 0x47e40a
    void add(const std::string& name);                       // Mac 0x47f0e8: as is, or addUnique if taken
    const std::string& addUnique(const std::string& name);   // Mac 0x47ef56: stores name or makeUniqueName(name)
    std::string makeUniqueName(const std::string& name) const; // Mac 0x47e7ce

    // "base(digits)" suffix helpers (static in the original file)
    static int findNumberSuffix(const std::string& name);    // Mac 0x47e48e: index of '(' or -1
    static void removeNumberSuffix(std::string& name);       // Mac 0x47e758
    static bool hasNumberSuffix(const std::string& name);    // Mac 0x47e7b4

    VxArray<std::string> mNames; // +0x00
};

} // namespace Vx

#endif
