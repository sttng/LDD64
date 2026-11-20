// LEGO Digital Designer — restored source
// LEGO::BrickUpdateLoaderXML — reads the "update.xml" of a brick database patch
//   (BricksPatchFileSaver):
//     <update version="x.y.z.w">
//       <add><file path="a/b"/>…</add> <overwrite><file path=…/>…</overwrite>
//       <remove><file path=…/>…</remove>
//     </update>
//   TU 359 "BrickUpdateLoaderXML.cpp" (Mac 0x342750-0x3435ff, Win 0x695a71-0x696084).
//
// RTTI: polymorphic, no bases. Mac vtable 0x74d110 {D1 0x342770, D0 0x342780, Load 0x342790,
// Load 0x342b90}; inline constructor (copy Mac 0x342750). Win vtable 0x9ddeb4 {sdd 0x695b31,
// Load(4 args) 0x695b04, Load(5 args) 0x695f9d} (MSVC lists the overloads in reverse order).
// Method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "Version.h"

namespace LEGO {

class File;

class BrickUpdateLoaderXML {
public:
    BrickUpdateLoaderXML() {}                                                // Mac 0x342750
    virtual ~BrickUpdateLoaderXML() {}                                       // Mac 0x342770/0x342780

    // Reads `file` from the start: the version of <update> into `version`, the paths of the
    // <add> / <overwrite> / <remove> files appended to `added` / `modified` / `deleted`.
    // False when the file cannot be read or does not parse. Mac 0x342790, Win 0x695f9d
    virtual bool Load(File* file, Version& version, BasicArray<String>& added,
                      BasicArray<String>& modified, BasicArray<String>& deleted);
    // The same without the version. Mac 0x342b90, Win 0x695b04
    virtual bool Load(File* file, BasicArray<String>& added, BasicArray<String>& modified,
                      BasicArray<String>& deleted);
};

} // namespace LEGO
