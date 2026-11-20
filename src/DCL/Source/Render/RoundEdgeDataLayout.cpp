// LEGO Digital Designer — restored source
// TU 393 (Mac 0x37dab0-0x37db2f; file name inferred, no class, no strings): the layout helpers
// of the round-edge data texture. Declared in RoundEdgeDataTextureCreator.h. Inlined on Win.
#include "RoundEdgeDataTextureCreator.h"

namespace LEGO {

// Mac 0x37dab0
UInt32 GetRoundEdgeDataTextureMinWidth()
{
    return 0x40;
}

// Mac 0x37dac0: (bytes / 4) rounded up to a multiple of 64, times 4.
UInt32 AlignRoundEdgeDataSize(UInt32 bytes)
{
    UInt32 values = bytes >> 2;
    UInt32 pad = 0;
    if ((values & 0x3f) != 0)
        pad = 0x40 - (values & 0x3f);
    return (pad + values) * 4;
}

} // namespace LEGO
