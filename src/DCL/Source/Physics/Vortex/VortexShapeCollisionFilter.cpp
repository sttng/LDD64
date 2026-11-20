// LEGO Digital Designer — restored source
// LEGO::VortexShapeCollisionFilter (see LDDVortexCollisionFilter.h). The original emits these two
// functions in LDDVortexCollisionFilter.cpp (Mac TU 436).
#include "LDDVortexCollisionFilter.h"

namespace LEGO {

// Mac 0x3b4360 (Win inlined)
void VortexShapeCollisionFilter::Add(VortexShapeCollisionFilter* filter)
{
    m_filters[m_count++] = filter;
}

// Mac 0x3b4380 (Win inlined)
void VortexShapeCollisionFilter::Remove(VortexShapeCollisionFilter* filter)
{
    for (UInt32 i = 0; i < m_count; ++i) {
        if (m_filters[i] == filter) {
            m_filters[i] = m_filters[m_count - 1];
            --m_count;
            return;
        }
    }
}

} // namespace LEGO
