// Vortex 5.0.0 build 800 "vortexldd41" - VxColor (restored header).
// Known only from the VxData connection code (Mac 0x40ac74 / 0x40e9ce): four floats (16 bytes),
// compared per component, copied with a self-assignment check. Component names (r, g, b, a) inferred.
#ifndef VX_VXCOLOR_H
#define VX_VXCOLOR_H

namespace Vx {

class VxColor {
public:
    VxColor() { c[0] = c[1] = c[2] = c[3] = 0.0f; }
    VxColor(float r, float g, float b, float a) { c[0] = r; c[1] = g; c[2] = b; c[3] = a; }
    VxColor(const VxColor& o) { c[0] = o.c[0]; c[1] = o.c[1]; c[2] = o.c[2]; c[3] = o.c[3]; }
    VxColor& operator=(const VxColor& o)
    {
        if (this != &o) {
            c[0] = o.c[0];
            c[1] = o.c[1];
            c[2] = o.c[2];
            c[3] = o.c[3];
        }
        return *this;
    }
    float& operator[](int i) { return c[i]; }
    float operator[](int i) const { return c[i]; }

    bool operator==(const VxColor& o) const { return c[0] == o.c[0] && c[1] == o.c[1] && c[2] == o.c[2] && c[3] == o.c[3]; }
    bool operator!=(const VxColor& o) const { return !(*this == o); }

    float c[4]; // +0x00
};

} // namespace Vx

#endif
