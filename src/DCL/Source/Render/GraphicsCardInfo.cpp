// LEGO Digital Designer — restored source
// TU 419 "GraphicsCardInfo.cpp": Mac 0x399290-0x39965f, Win 0x503850-0x503a70. See
// GraphicsCardInfo.h. The static initialiser (Mac 0x399620) is only <iostream>.
#include "GraphicsCardInfo.h"

namespace LEGO {

// Mac 0x399290 (Win: inlined into OpenGLGraphicsCardInfo's constructor)
GraphicsCardInfo::GraphicsCardInfo()
    : m_memoryMB(0)
{
}

// Mac 0x399530 (D2), 0x3995d0 (D1), 0x3995e0 (D0); Win 0x5039e0, 0x503850 (sdd)
GraphicsCardInfo::~GraphicsCardInfo()
{
}

// Mac 0x399360, Win 0x503870
String GraphicsCardInfo::GetDescription() const
{
    String text("\n\nGRAPHICS CARD INFO:\n\n");
    text << "Vendor: " << m_vendor << "\n";
    text << "ProductName: " << m_productName << "\n";
    text << "MB memory: " << m_memoryMB << "\n";
    text << "Installed drivers: " << m_installedDrivers << "\n";
    String version;
    ToString(m_driverVersion, version);
    text << "Driver version: " << version << "\n";
    text << "Driver date: " << m_driverDate << "\n";
    return text;
}

} // namespace LEGO
