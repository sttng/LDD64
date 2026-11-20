// LEGO Digital Designer — restored source
// Whole-file read helpers (TU 181, Mac 0x2366c0-0x23698f; file and function names inferred —
// the TU has no class and no strings). Used by the XML DOM parsers (XMLDOMNode::Parse,
// XMLDOMNodeAString::Parse), BrickDatabase, Lua scripts, BrickPalette loaders …
// Win twins: 0x538e70 (Path), 0x538f30 (Directory + name), 0x539000 (File).
#pragma once
#include "LEGO/Types.h"
#include "BasicString.h"
#include "LEGO/String.h"

namespace LEGO {

class File;
class Directory;
class Path;

// Opens `path` for reading (OpenMode read + share read) and reads it completely.
bool ReadFile(const Path& path, AString& out);                                  // Mac 0x2366c0, Win 0x538e70
// Reads the whole file from position 0 (File::GetSize bytes).
bool ReadFile(File* file, AString& out);                                        // Mac 0x236750, Win 0x539000
// Opens `name` below `directory` for reading and reads it completely.
bool ReadFile(Directory* directory, const String& name, AString& out);          // Mac 0x2368b0, Win 0x538f30

} // namespace LEGO
