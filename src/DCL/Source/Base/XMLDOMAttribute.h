// LEGO Digital Designer — restored source
// GetXMLAttribute — parses an 8-bit XML node's attribute with the value type's FromString
// (names inferred). Weak instances in TU 024 (Win: one stack argument, `ret 4`):
//   T = UInt32 Mac 0x720a0 Win 0x4ad850 (also ModelIOLXFML5 0x316110); UInt8 Mac 0x72180 Win
//   0x4c6fe0; MaterialManual::MaterialType Mac 0x72260 Win 0x4c70b0.
#pragma once
#include "XMLDOMNode.h"
#include "StringStream.h"

namespace LEGO {

// false when the attribute is missing or does not parse; `value` is then left as the parser
// leaves it.
template <class T>
inline bool GetXMLAttribute(const XMLDOMNodeAString& node, const AString& name, T& value)
{
    AString text;
    if (!node.GetAttribute(name, text))
        return false;
    return FromString(value, String(text));
}

} // namespace LEGO
