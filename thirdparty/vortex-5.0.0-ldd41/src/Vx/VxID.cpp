// Vortex 5.0.0 build 800 "vortexldd41" - VxID and the CRC-32 it uses (restored).
// Mac: VxID = TU 495 0x45b3d2-0x45b585 (file name unknown), CRC-32 = part of TU 468
// (0x437abe-0x437bac); together here because VxData field names are VxIDs. The rest of those TUs
// (VxInternalIDIntersectSubscriber ctor 0x45b59c.., the geometry helpers 0x437bd4..) is V4.4 / V4.5.
#include "Vx/VxID.h"

namespace Vx {

namespace {
unsigned int sCrcTable[256];     // Mac 0x77f840
bool sCrcTableDirty = true;      // Mac 0x7791b4 (initialised to 1)
}

// Mac 0x437abe (name inferred). Builds the reflected table by the doubling trick:
// for i = 128, 64, .., 1: c = next CRC step; table[i + j] = table[j] ^ c for j = 0, 2i, 4i, ..
const unsigned int* vxCrc32Table()
{
    if (sCrcTableDirty) {
        sCrcTable[0] = 0;
        unsigned int c = 1;
        unsigned int i = 0x80;
        for (int n = 0; n != 8; ++n) {
            c = (-(c & 1) & 0xedb88320u) ^ (c >> 1);
            for (unsigned int j = 0; j < 0x100; j += i * 2)
                sCrcTable[i + j] = c ^ sCrcTable[j];
            i >>= 1;
        }
        sCrcTableDirty = false;
    }
    return sCrcTable;
}

// Mac 0x437b6e (eax = crc, edx = data, ecx = size), Win 0x81b2c0 (name inferred)
unsigned int vxCrc32Update(unsigned int crc, const void* data, unsigned int size)
{
    unsigned int c = ~crc;
    if (size != 0) {
        const unsigned char* p = (const unsigned char*)data;
        const unsigned char* e = p + size;
        do {
            const unsigned int* t = vxCrc32Table();
            c = (c >> 8) ^ t[(unsigned char)(*p ^ c)];
            ++p;
        } while (p != e);
    }
    return ~c;
}

// Mac 0x437bac, Win 0x81b300 (name inferred)
unsigned int vxCrc32(const void* data, unsigned int size)
{
    unsigned int crc = vxCrc32Update(0, 0, 0);
    return vxCrc32Update(crc, data, size);
}

// Mac 0x77cdf4 (initialiser 0x45b537 -> 0x45b3f2, atexit 0x45b4dc) (name inferred)
const VxID VxID::kNull;

// Mac C2 0x45b3d2, C1 0x45b3f2
VxID::VxID()
    : mName(), mChecksum(-1)
{
}

// Mac C2 0x45b438, C1 0x45b4c2, Win 0x7cc600
VxID::VxID(const std::string& name)
    : mName(name), mChecksum(-1)
{
    computeChecksum();
}

// Win 0x7634e0 (inlined on Mac)
VxID::VxID(const char* name)
    : mName(name ? name : ""), mChecksum(-1)
{
    computeChecksum();
}

// Mac 0x45b406, Win 0x7cc5a0 (name inferred)
void VxID::computeChecksum()
{
    if (mName.size() == 0)
        mChecksum = -1;
    else
        mChecksum = (int)vxCrc32(mName.data(), (unsigned int)mName.size());
}

} // namespace Vx
