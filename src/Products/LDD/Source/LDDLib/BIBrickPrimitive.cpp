// LEGO Digital Designer — restored source
// TU 354 "BIBrickPrimitive.cpp" (Mac 0x334c20-0x3355df). See BIBrickPrimitive.h.
#include "BIBrickPrimitive.h"
#include "BIBrickSubstep.h"
#include "BIEvent.h"
#include "BIGraphWalk.h"

namespace LEGO {

namespace {

// The walker of CollectConnected (WalkDepthFirst, BIGraphWalk.h): the neighbours are the
// connected primitives (the keys of the connection map).
struct ConnectedWalker {
    typedef BIBrickPrimitive* Node;
    typedef BIBrick::ConnectionMap::ConstIterator Iterator;
    ConnectedWalker(BasicSet<BIBrickPrimitive*>& result, const BasicSet<BIBrickPrimitive*>& within)
        : m_result(result), m_within(within) {}
    bool Enter(Node node) { m_result.Insert(node); return true; }
    void GetRange(Node node, Iterator& begin, Iterator& end) const
    {
        const BIBrick::ConnectionMap& connections = static_cast<const BIBrickPrimitive*>(node)->GetConnections();
        begin = connections.Begin();
        end = connections.End();
    }
    Node GetNode(const Iterator& it) const { return it->first; }
    bool Descend(Node node) const { return !m_result.Contains(node) && m_within.Contains(node); }
    BasicSet<BIBrickPrimitive*>& m_result;
    const BasicSet<BIBrickPrimitive*>& m_within;
};

} // namespace

// Mac 0x334c20
BIBrickPrimitive::BIBrickPrimitive()
    : BIBrick(TypePrimitive)
{
    m_primitives.Insert(this);
}

// Mac 0x3351e0 / D0 0x335250, Win sdd 0x58ab20
BIBrickPrimitive::~BIBrickPrimitive()
{
}

// Mac 0x3352d0, Win 0x58a960
UInt32 BIBrickPrimitive::GetPrimitiveCount() const
{
    return 1;
}

// Mac 0x3352e0, Win 0x58a970
bool BIBrickPrimitive::IsEqual(const BIBrick* brick) const
{
    return this == brick;
}

// Mac 0x3352f0, Win 0x58a990
bool BIBrickPrimitive::IsEqual(const BasicSet<BIBrickPrimitive*>& primitives) const
{
    if (primitives.GetSize() != 1)
        return false;
    return *primitives.Begin() == this;
}

// Mac 0x335310, Win 0x58a9e0
Real32 BIBrickPrimitive::GetWeight() const
{
    return 1.0f;
}

// Mac 0x335320, Win 0x58aa50
bool BIBrickPrimitive::Contains(const BIBrick* brick) const
{
    return this == brick;
}

// Mac 0x335330 (tail call), Win 0x58aa70
bool BIBrickPrimitive::IsContainedIn(const BasicSet<BIBrickPrimitive*>& primitives) const
{
    return Intersects(primitives);
}

// Mac 0x334cf0, Win 0x58abb0
bool BIBrickPrimitive::Intersects(const BIBrick* brick) const
{
    if (brick->GetType() == TypePrimitive)
        return this == brick;
    return brick->Contains(this);
}

// Mac 0x334d20, Win 0x578330
bool BIBrickPrimitive::Intersects(const BasicArray<BIBrick*>& bricks) const
{
    for (BasicArray<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        if (Intersects(*it))
            return true;
    return false;
}

// Mac 0x334d70, Win 0x5783a0
bool BIBrickPrimitive::Intersects(const BasicSet<BIBrick*>& bricks) const
{
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        if (Intersects(*it))
            return true;
    return false;
}

// Mac 0x334dc0, Win 0x578330
bool BIBrickPrimitive::Intersects(const BasicArray<BIBrickPrimitive*>& primitives) const
{
    for (BasicArray<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
        if (Intersects(*it))
            return true;
    return false;
}

// Mac 0x334e10, Win 0x5783a0
bool BIBrickPrimitive::Intersects(const BasicSet<BIBrickPrimitive*>& primitives) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
        if (Intersects(*it))
            return true;
    return false;
}

// Mac 0x334e60, Win 0x578330
bool BIBrickPrimitive::Intersects(const BasicArray<BIBrickSubstep*>& substeps) const
{
    for (BasicArray<BIBrickSubstep*>::ConstIterator it = substeps.Begin(); it != substeps.End(); ++it)
        if (Intersects(static_cast<const BIBrick*>(*it)))
            return true;
    return false;
}

// Mac 0x334eb0, Win 0x578410
bool BIBrickPrimitive::Intersects(const BasicArray<RefPointer<BIEvent> >& events) const
{
    for (BasicArray<RefPointer<BIEvent> >::ConstIterator it = events.Begin(); it != events.End(); ++it)
        if ((*it)->GetBrick() != 0 && Intersects((*it)->GetBrick()))
            return true;
    return false;
}

// Mac 0x334f20, Win 0x58abf0
bool BIBrickPrimitive::Split(const BIBrick* brick, BasicSet<BIBrickPrimitive*>& inside,
                             BasicSet<BIBrickPrimitive*>& outside) const
{
    BIBrickPrimitive* self = const_cast<BIBrickPrimitive*>(this);
    if (Intersects(brick))
        inside.Insert(self);
    else
        outside.Insert(self);
    return inside.GetSize() != 0;
}

// Mac 0x334f80, Win 0x58ac60
bool BIBrickPrimitive::Split(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& inside,
                             BasicSet<BIBrickPrimitive*>& outside) const
{
    BIBrickPrimitive* self = const_cast<BIBrickPrimitive*>(this);
    if (Intersects(events))
        inside.Insert(self);
    else
        outside.Insert(self);
    return inside.GetSize() != 0;
}

// Mac 0x335340, Win 0x58aa20
void BIBrickPrimitive::GetPrimitives(BasicArray<BIBrickPrimitive*>& primitives) const
{
    primitives.PushBack(const_cast<BIBrickPrimitive*>(this));
}

// Mac 0x335570, Win 0x58a9f0
void BIBrickPrimitive::GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const
{
    primitives.Insert(const_cast<BIBrickPrimitive*>(this));
}

// Mac 0x334fe0, Win 0x58acd0
bool BIBrickPrimitive::IsConnectedTo(const BIBrick* brick) const
{
    for (ConnectionMap::ConstIterator it = m_connections.Begin(); it != m_connections.End(); ++it)
        if (brick->Contains(it->first))
            return true;
    return false;
}

// Mac 0x335030, Win 0x58ad50
void BIBrickPrimitive::GetConnectedPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const
{
    for (ConnectionMap::ConstIterator it = m_connections.Begin(); it != m_connections.End(); ++it)
        primitives.Insert(it->first);
}

// Mac 0x335090
// NOTE: the original recurses once per primitive of the path, so a long chain of connected
// bricks overflows the stack; fixed: the same walk with an explicit stack (WalkDepthFirst).
void BIBrickPrimitive::CollectConnected(BasicSet<BIBrickPrimitive*>& result, const BasicSet<BIBrickPrimitive*>& within)
{
    ConnectedWalker walker(result, within);
    WalkDepthFirst(walker, this);
}

} // namespace LEGO
