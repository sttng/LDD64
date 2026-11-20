// LEGO Digital Designer — restored source
// LEGO::GraphicsCardInfo — graphics hardware description for the system report ("GRAPHICS CARD
// INFO"), filled by OpenGLGraphicsCardInfo (TU 420). TU 419: Mac 0x399290-0x39965f,
// Win 0x503850-0x503a70. Vtable Mac 0x757f70 {D1 0x3995d0, D0 0x3995e0, .., GetDescription
// 0x399360}, Win 0xa27d78 {sdd 0x503850, .., GetDescription 0x503870}. sizeof Mac 0x5c, Win 0x2c.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Version.h"

namespace LEGO {

class GraphicsCardInfo : public Object {
public:
    GraphicsCardInfo();                                               // Mac 0x399290 (Win inlined in the subclass ctor)
    virtual ~GraphicsCardInfo();                                      // Mac 0x399530 (D2) 0x3995d0/0x3995e0, Win 0x5039e0/0x503850

    // Multi-line report: vendor, product name, memory, drivers, driver version and date.
    virtual String GetDescription() const;                            // Mac 0x399360, Win 0x503870

protected:
    String  m_productName;        // Mac +0x08, Win +0x08
    String  m_vendor;             // Mac +0x18, Win +0x0c
    String  m_driverDate;         // Mac +0x28, Win +0x10
    Version m_driverVersion;      // Mac +0x38, Win +0x14
    UInt32  m_memoryMB;           // Mac +0x48, Win +0x24
    String  m_installedDrivers;   // Mac +0x4c, Win +0x28
};

} // namespace LEGO
