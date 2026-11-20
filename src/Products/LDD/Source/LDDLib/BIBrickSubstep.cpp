// LEGO Digital Designer — restored source
// TU 355 "BIBrickSubstep.cpp" (Mac 0x3355e0-0x33686f). See BIBrickSubstep.h.
#include "BIBrickSubstep.h"
#include "BIBrickPrimitive.h"
#include "BIEvent.h"
#include "BIGraph.h"
#include "BI.h"

namespace LEGO {

// Mac 0x3355e0 (C2; C1 0x335820 jumps to it)
BIBrickSubstep::BIBrickSubstep(const BasicSet<BIBrickPrimitive*>& primitives, const BIAnalyzer* group, Real32 weight)
    : BIBrick(TypeSubstep), m_unknownDC(0), m_weight(weight)
{
    m_group = group;
    if (&m_primitives != &primitives)
        m_primitives = primitives;
    for (BasicSet<BIBrickPrimitive*>::Iterator it = m_primitives.Begin(); it != m_primitives.End(); ++it) {
        const BasicArray<PrimitiveElement*>& elements = (*it)->m_elements;
        m_elements.InsertRange(m_elements.End(), elements.Begin(), elements.End());
    }
    BasicSet<BIBrickPrimitive*> none;
    BI::ComputeConnections(this, m_connections, none);
    UpdateCategory();
}

// Mac 0x3361b0 / D0 0x336220, Win sdd 0x577d90
BIBrickSubstep::~BIBrickSubstep()
{
}

// Mac 0x3362a0, Win 0x577b60
UInt32 BIBrickSubstep::GetPrimitiveCount() const
{
    return m_primitives.GetSize();
}

// Mac 0x335890, Win 0x577f40
bool BIBrickSubstep::IsEqual(const BIBrick* brick) const
{
    if (GetPrimitiveCount() != brick->GetPrimitiveCount() || brick->GetType() != TypeSubstep)
        return false;
    BasicSet<BIBrickPrimitive*>::ConstIterator other = brick->GetPrimitiveSet().Begin();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it, ++other)
        if (*it != *other)
            return false;
    return true;
}

// Mac 0x335830, Win 0x577ea0
bool BIBrickSubstep::IsEqual(const BasicSet<BIBrickPrimitive*>& primitives) const
{
    if (primitives.GetSize() != GetPrimitiveCount())
        return false;
    BasicSet<BIBrickPrimitive*>::ConstIterator other = primitives.Begin();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it, ++other)
        if (*it != *other)
            return false;
    return true;
}

// Mac 0x3362b0, Win 0x577b80
Real32 BIBrickSubstep::GetWeight() const
{
    return m_weight;
}

// Mac 0x335aa0, Win 0x5781a0
bool BIBrickSubstep::Contains(const BIBrick* brick) const
{
    if (this == brick)
        return true;
    return brick->IsContainedIn(m_primitives);
}

// Mac 0x335ad0, Win 0x5781d0
bool BIBrickSubstep::IsContainedIn(const BasicSet<BIBrickPrimitive*>& primitives) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it)
        if (!primitives.Contains(*it))
            return false;
    return true;
}

// Mac 0x335b80
bool BIBrickSubstep::Intersects(const BIBrick* brick) const
{
    if (brick->GetType() == TypePrimitive)
        return m_primitives.Contains(static_cast<BIBrickPrimitive*>(const_cast<BIBrick*>(brick)));
    if (this == brick)
        return true;
    const BasicSet<BIBrickPrimitive*>& others = brick->GetPrimitiveSet();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = others.Begin(); it != others.End(); ++it)
        if (m_primitives.Contains(*it))
            return true;
    return false;
}

// Mac 0x335c90, Win 0x578330
bool BIBrickSubstep::Intersects(const BasicArray<BIBrick*>& bricks) const
{
    for (BasicArray<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        if (Intersects(*it))
            return true;
    return false;
}

// Mac 0x335ce0, Win 0x5783a0
bool BIBrickSubstep::Intersects(const BasicSet<BIBrick*>& bricks) const
{
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        if (Intersects(*it))
            return true;
    return false;
}

// Mac 0x335d30, Win 0x578330
bool BIBrickSubstep::Intersects(const BasicArray<BIBrickPrimitive*>& primitives) const
{
    for (BasicArray<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
        if (Intersects(static_cast<const BIBrick*>(*it)))
            return true;
    return false;
}

// Mac 0x335d80, Win 0x5783a0
bool BIBrickSubstep::Intersects(const BasicSet<BIBrickPrimitive*>& primitives) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
        if (Intersects(static_cast<const BIBrick*>(*it)))
            return true;
    return false;
}

// Mac 0x335dd0, Win 0x578330
bool BIBrickSubstep::Intersects(const BasicArray<BIBrickSubstep*>& substeps) const
{
    for (BasicArray<BIBrickSubstep*>::ConstIterator it = substeps.Begin(); it != substeps.End(); ++it)
        if (Intersects(static_cast<const BIBrick*>(*it)))
            return true;
    return false;
}

// Mac 0x335e20, Win 0x578410
bool BIBrickSubstep::Intersects(const BasicArray<RefPointer<BIEvent> >& events) const
{
    for (BasicArray<RefPointer<BIEvent> >::ConstIterator it = events.Begin(); it != events.End(); ++it)
        if ((*it)->GetBrick() != 0 && Intersects((*it)->GetBrick()))
            return true;
    return false;
}

// Mac 0x335e90, Win 0x5784c0
bool BIBrickSubstep::Split(const BIBrick* brick, BasicSet<BIBrickPrimitive*>& inside,
                           BasicSet<BIBrickPrimitive*>& outside) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it)
        (*it)->Split(brick, inside, outside);
    return inside.GetSize() != 0;
}

// Mac 0x335ef0, Win 0x578550
bool BIBrickSubstep::Split(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& inside,
                           BasicSet<BIBrickPrimitive*>& outside) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it) {
        // NOTE: the inner loop only counts the events; the same split is repeated once per
        // event (both builds).
        for (UInt32 i = 0; i < events.GetSize(); ++i)
            (*it)->Split(events, inside, outside);
    }
    return inside.GetSize() != 0;
}

// Mac 0x3362c0, Win 0x577ba0
void BIBrickSubstep::GetPrimitives(BasicArray<BIBrickPrimitive*>& primitives) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it)
        primitives.PushBack(*it);
}

// Mac 0x336520, Win 0x577c10
void BIBrickSubstep::GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it)
        primitives.Insert(*it);
}

// Mac 0x335f60, Win 0x578620
bool BIBrickSubstep::IsConnectedTo(const BIBrick* brick) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it)
        if ((*it)->IsConnectedTo(brick))
            return true;
    return false;
}

// Mac 0x335fb0, Win 0x5786a0
void BIBrickSubstep::GetConnectedPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it)
        (*it)->GetConnectedPrimitives(primitives);
}

// Mac 0x335900, Win 0x577ff0
BIBrickSubstep* BIBrickSubstep::Subtract(const BasicSet<BIBrickPrimitive*>& primitives, BIGraph* graph) const
{
    BasicSet<BIBrickPrimitive*> all(m_primitives);
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
        all.Erase(*it);
    BIBrickSubstep* result = 0;
    UInt32 size = all.GetSize();
    if (size > 1 && size != m_primitives.GetSize() && graph->IsConnected(all))
        result = new BIBrickSubstep(all, m_group, 0.9f / (Real32)size);
    return result;
}

// Mac 0x336000
bool BIBrickSubstep::GetAttribute(Attribute attribute) const
{
    AttributeMap::ConstIterator it = m_attributes.Find(attribute);
    if (it == m_attributes.End())
        return false;
    return it->second;
}

// Mac 0x3360b0
void BIBrickSubstep::SetAttributes(const AttributeMap& attributes)
{
    for (AttributeMap::ConstIterator it = attributes.Begin(); it != attributes.End(); ++it)
        m_attributes[it->first] = it->second;
}

} // namespace LEGO
