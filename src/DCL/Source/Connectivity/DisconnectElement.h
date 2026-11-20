// LEGO Digital Designer — restored source
// LEGO::DisconnectElement — frees a primitive element for pulling: every connection of the
// element is disconnected (Scene::Disconnect) except the internal joints of its own part (the
// connections to elements of the same part through fields of kind >= 499500, the flexible
// part's bone joints). Called by the StringPullTool and the ClickPullTool on the picked element.
//
// TU 353 (Mac 0x334640-0x334c1f, file name unknown: only this function and the
// BasicArray<PrimitiveConnection*> instance it uses; Win: inlined into the tools, Win
// 0x66c534 / 0x66cbe4). (function and file names inferred)
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

class PrimitiveElement;

// NOTE: a connection to an element of the same part is listed once per field connection of
// kind < 499500 (so possibly several times). Mac 0x334640
void DisconnectElement(PrimitiveElement* element);

} // namespace LEGO
