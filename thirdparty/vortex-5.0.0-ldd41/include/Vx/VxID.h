// Vortex 5.0.0 build 800 "vortexldd41" - VxID: a name plus its CRC-32 (restored header).
//
//
// Evidence: Win strings "VxID checksum specified for %s doesn't seem to be valid." / "VxID %s is
// already registered." name the class; every VxData field name is one (VxData::FieldBase +4).
// Layout: { std::string mName; int mChecksum; } = Mac 8 bytes, Win 32 bytes.
// The checksum is the standard reflected CRC-32 (polynomial 0xedb88320) of the characters, -1 for
// the empty name. Code: Mac TU 495 (0x45b3d2-0x45b585) + the CRC routines of Mac TU 468
// (0x437abe-0x437bac); Win 0x7634e0, 0x7cc5a0, 0x7cc600, 0x81b2c0, 0x81b300.
#ifndef VX_VXID_H
#define VX_VXID_H

#include <string>

namespace Vx {

// Mac 0x437bac, Win 0x81b300: crc = update(0, 0, 0); return update(crc, data, size) (the zlib
// "crc32(0L, Z_NULL, 0)" idiom) (name inferred)
unsigned int vxCrc32(const void* data, unsigned int size);
// Mac 0x437b6e (register arguments), Win 0x81b2c0: c = ~crc; table step per byte; return ~c
// (name inferred)
unsigned int vxCrc32Update(unsigned int crc, const void* data, unsigned int size);
// Mac 0x437abe: the 256-entry table, built on first use (name inferred)
const unsigned int* vxCrc32Table();

class VxID {
public:
    // Mac C2 0x45b3d2 / C1 0x45b3f2: empty name, checksum -1
    VxID();
    // Mac C2 0x45b438 / C1 0x45b4c2, Win 0x7cc600
    VxID(const std::string& name);
    // Win 0x7634e0 (out of line); inlined on Mac (e.g. 0x445b4e "__Invalid__"): a null pointer
    // gives the empty name (Win: 0xa1f6ac = "")
    VxID(const char* name);

    // inline (Mac 0x445550: "if (this != &o) { assign; copy checksum }")
    VxID& operator=(const VxID& o)
    {
        if (this != &o) {
            mName = o.mName;
            mChecksum = o.mChecksum;
        }
        return *this;
    }

    const std::string& getName() const { return mName; }
    const char* c_str() const { return mName.c_str(); }
    int getChecksum() const { return mChecksum; }
    bool isValid() const { return mChecksum != -1; } // (name inferred)

    // fields and containers compare names by checksum only (Mac 0x4440e2)
    bool operator==(const VxID& o) const { return mChecksum == o.mChecksum; }
    bool operator!=(const VxID& o) const { return mChecksum != o.mChecksum; }

    // the static empty id (Mac 0x77cdf4, constructed by the TU 495 initialiser 0x45b537,
    // destroyed by 0x45b4dc) (name inferred)
    static const VxID kNull;

    std::string mName; // Mac +0x00, Win +0x00
    int mChecksum;     // Mac +0x04, Win +0x1c

private:
    // Mac 0x45b406, Win 0x7cc5a0: -1 for the empty name, else vxCrc32(name) (name inferred)
    void computeChecksum();
};

} // namespace Vx

#endif
