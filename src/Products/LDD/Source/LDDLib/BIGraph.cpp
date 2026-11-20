// LEGO Digital Designer — restored source
// TU 215 "BIGraph.cpp" (Mac 0x2627d0-0x26509f): LEGO::BIGraph. See BIGraph.h.
// Template code of the TU (not restored separately): BasicMap<BIBrickPrimitive*,
// BasicSet<BIBrickPrimitive*> > (operator[] 0x2648d0 / insert 0x264a30 / 0x264bf0, node copy
// 0x264cb0, destructors 0x262c30 / 0x264820, erase 0x264870), BasicDeque<BIBrickPrimitive*>
// (0x263a90 / 0x264d90, map allocation 0x264e00, map growth 0x264f30).
#include "BIGraph.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BasicDeque.h"
#include "BIGraphWalk.h"

namespace LEGO {

namespace {

// The adjacent primitives of `primitive`. NOTE: the original reads find(primitive)->second
// without a check; for a primitive without an entry that reads through the map's header node
// (on Mac it aliases m_primitives). Every primitive of a graph has an entry (constructor), so
// this only guards misuse: a missing entry reads as no neighbours.
const BasicSet<BIBrickPrimitive*>& Neighbours(const BIGraph::Adjacency& adjacency, BIBrickPrimitive* primitive)
{
    static const BasicSet<BIBrickPrimitive*> s_none;
    BIGraph::Adjacency::ConstIterator it = adjacency.Find(primitive);
    return it == adjacency.End() ? s_none : it->second;
}

// The walkers of the depth-first walks below (WalkDepthFirst, BIGraphWalk.h; M11): the
// neighbours from the adjacency; Enter / Descend are the entry and the loop test of each
// recursive original.
struct GraphWalker {
    typedef BIBrickPrimitive* Node;
    typedef BasicSet<BIBrickPrimitive*>::ConstIterator Iterator;
    explicit GraphWalker(const BIGraph::Adjacency& adjacency) : m_adjacency(adjacency) {}
    void GetRange(Node node, Iterator& begin, Iterator& end) const
    {
        const BasicSet<BIBrickPrimitive*>& adjacent = Neighbours(m_adjacency, node);
        begin = adjacent.Begin();
        end = adjacent.End();
    }
    Node GetNode(const Iterator& it) const { return *it; }
    const BIGraph::Adjacency& m_adjacency;
};

// CollectComponent(primitive, visited)
struct ComponentWalker : GraphWalker {
    ComponentWalker(const BIGraph::Adjacency& adjacency, BasicSet<BIBrickPrimitive*>& visited)
        : GraphWalker(adjacency), m_visited(visited) {}
    bool Enter(Node node) { m_visited.Insert(node); return true; }
    bool Descend(Node node) const { return !m_visited.Contains(node); }
    BasicSet<BIBrickPrimitive*>& m_visited;
};

// CollectComponent(primitive, component, marked)
struct MarkedComponentWalker : GraphWalker {
    MarkedComponentWalker(const BIGraph::Adjacency& adjacency, BasicSet<BIBrickPrimitive*>& component,
                          BasicSet<BIBrickPrimitive*>& marked)
        : GraphWalker(adjacency), m_component(component), m_marked(marked) {}
    bool Enter(Node node)
    {
        if (m_marked.Contains(node))
            return false;
        m_component.Insert(node);
        m_marked.Insert(node);
        return true;
    }
    bool Descend(Node node) const { return !m_component.Contains(node); }
    BasicSet<BIBrickPrimitive*>& m_component;
    BasicSet<BIBrickPrimitive*>& m_marked;
};

// CollectComponentExcluding
struct ExcludingWalker : GraphWalker {
    ExcludingWalker(const BIGraph::Adjacency& adjacency, BasicSet<BIBrickPrimitive*>& visited,
                    const BasicSet<BIBrickPrimitive*>& excluded)
        : GraphWalker(adjacency), m_visited(visited), m_excluded(excluded) {}
    bool Enter(Node node) { m_visited.Insert(node); return true; }
    bool Descend(Node node) const { return !m_visited.Contains(node) && !m_excluded.Contains(node); }
    BasicSet<BIBrickPrimitive*>& m_visited;
    const BasicSet<BIBrickPrimitive*>& m_excluded;
};

// CountConnected: the count of every call (1 for a primitive of `within`) summed up
struct CountWalker : GraphWalker {
    CountWalker(const BIGraph::Adjacency& adjacency, BasicSet<BIBrickPrimitive*>& visited,
                const BasicSet<BIBrickPrimitive*>& within)
        : GraphWalker(adjacency), m_visited(visited), m_within(within), m_count(0) {}
    bool Enter(Node node)
    {
        if (m_within.Contains(node)) {
            m_visited.Insert(node);
            ++m_count;
        }
        return true;
    }
    bool Descend(Node node) const { return !m_visited.Contains(node) && m_within.Contains(node); }
    BasicSet<BIBrickPrimitive*>& m_visited;
    const BasicSet<BIBrickPrimitive*>& m_within;
    UInt32 m_count;
};

} // namespace

// Mac 0x2627d0 (C2) / 0x262c60 (C1), Win 0x595c80
BIGraph::BIGraph(const BasicSet<BIBrickPrimitive*>& primitives, GraphList* graphs)
    : m_graphs(graphs),
      m_primitives(primitives),
      m_articulationsValid(false)
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it)
        m_adjacency[*it] = BasicSet<BIBrickPrimitive*>();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it) {
        BIBrickPrimitive* primitive = *it;
        const BIBrick::ConnectionMap& connections = primitive->GetConnections();
        for (BIBrick::ConnectionMap::ConstIterator c = connections.Begin(); c != connections.End(); ++c) {
            BIBrickPrimitive* other = c->first;
            if (!m_primitives.Contains(other) || IsNeighbour(primitive, other))
                continue;
            m_adjacency[primitive].Insert(other);
            m_adjacency[other].Insert(primitive);
        }
    }
}

// Mac 0x262d40 (body 0x262c70) / 0x262d50, Win 0x595f70 / sdd 0x595f40
BIGraph::~BIGraph()
{
}

// Mac 0x262d90, Win 0x596000
void BIGraph::AddPrimitive(BIBrickPrimitive* primitive)
{
    m_primitives.Insert(primitive);
    const BIBrick::ConnectionMap& connections = primitive->GetConnections();
    for (BIBrick::ConnectionMap::ConstIterator c = connections.Begin(); c != connections.End(); ++c) {
        BIBrickPrimitive* other = c->first;
        if (!m_primitives.Contains(other) || IsNeighbour(primitive, other))
            continue;
        m_adjacency[primitive].Insert(other);
        m_adjacency[other].Insert(primitive);
    }
    m_articulationsValid = false;
}

// Mac 0x262f50, Win 0x596120
void BIGraph::RemovePrimitive(BIBrickPrimitive* primitive)
{
    const BasicSet<BIBrickPrimitive*>& neighbours = Neighbours(m_adjacency, primitive);
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it)
        m_adjacency[*it].Erase(primitive);
    m_adjacency.Erase(primitive);
    m_primitives.Erase(primitive);
    m_articulationsValid = false;
}

// Mac 0x263130, Win 0x5961e0
bool BIGraph::IsContainedIn(const BasicSet<BIBrickPrimitive*>& primitives) const
{
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it) {
        if (!(*it)->IsContainedIn(primitives))
            return false;
    }
    return true;
}

// Mac 0x263180, Win 0x596260
const BasicSet<BIBrickPrimitive*>& BIGraph::GetNeighbours(BIBrickPrimitive* primitive) const
{
    return Neighbours(m_adjacency, primitive);
}

// Mac 0x2631d0, Win 0x596290
void BIGraph::GetNeighbours(const BIBrick* brick, BasicSet<BIBrickPrimitive*>& neighbours) const
{
    BasicSet<BIBrickPrimitive*> primitives;
    brick->GetPrimitives(primitives);
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        const BasicSet<BIBrickPrimitive*>& adjacent = Neighbours(m_adjacency, *it);
        for (BasicSet<BIBrickPrimitive*>::ConstIterator n = adjacent.Begin(); n != adjacent.End(); ++n)
            neighbours.Insert(*n);
    }
}

// Mac 0x263330, Win 0x5963b0
BIBrickPrimitive* BIGraph::GetFirstNeighbour(BIBrickPrimitive* primitive) const
{
    const BasicSet<BIBrickPrimitive*>& adjacent = Neighbours(m_adjacency, primitive);
    return adjacent.IsEmpty() ? 0 : *adjacent.Begin();
}

// inlined on Mac, Win 0x596430
bool BIGraph::IsNeighbour(BIBrickPrimitive* a, BIBrickPrimitive* b) const
{
    Adjacency::ConstIterator it = m_adjacency.Find(a);
    return it != m_adjacency.End() && it->second.Contains(b);
}

// Mac 0x263390, Win 0x5964d0
void BIGraph::ComputeArticulationPoints()
{
    m_articulations.Clear();
    m_articulationsValid = true;
    if (m_primitives.GetSize() == 0)
        return;

    // NOTE: the original copy-constructs the stack from a default-constructed BasicDeque.
    BasicDeque<BIBrickPrimitive*> stack;
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it) {
        BIBrickPrimitive* primitive = *it;
        primitive->m_dfsNumber = 0xffffffff;
        primitive->m_dfsLow = 0;
        primitive->m_dfsParent = 0;
        primitive->m_dfsChildren.Clear();
    }
    BIBrickPrimitive* root = *m_primitives.Begin();
    root->m_dfsNumber = 0;
    stack.PushBack(root);
    root->m_dfsLow = root;

    UInt32 number = 1;
    while (!stack.IsEmpty()) {
        BIBrickPrimitive* current = stack.Back();
        const BasicSet<BIBrickPrimitive*>& adjacent = Neighbours(m_adjacency, current);

        // descend into the first unvisited neighbour
        BIBrickPrimitive* next = 0;
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = adjacent.Begin(); it != adjacent.End(); ++it) {
            if ((*it)->m_dfsNumber == 0xffffffff) {
                next = *it;
                break;
            }
        }
        if (next != 0) {
            next->m_dfsNumber = number;
            stack.PushBack(next);
            next->m_dfsParent = current;
            current->m_dfsChildren.Insert(next);
            ++number;
            next->m_dfsLow = next;
            continue;
        }

        // every neighbour is visited: leave the node and update the low points
        stack.PopBack();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = adjacent.Begin(); it != adjacent.End(); ++it) {
            BIBrickPrimitive* other = *it;
            if (other == current->m_dfsParent || current->m_dfsNumber <= other->m_dfsNumber) {
                if (current == other->m_dfsParent) {
                    if (other->m_dfsLow->m_dfsNumber < current->m_dfsLow->m_dfsNumber)
                        current->m_dfsLow = other->m_dfsLow;
                    if (current->m_dfsParent != 0 && current->m_dfsNumber <= other->m_dfsLow->m_dfsNumber)
                        m_articulations.Insert(current);
                }
            } else if (other->m_dfsNumber < current->m_dfsLow->m_dfsNumber) {
                current->m_dfsLow = other;   // back edge
            }
        }
    }
    if (root->m_dfsChildren.GetSize() > 1)
        m_articulations.Insert(root);
}

// Mac 0x263b00, Win 0x596910
UInt32 BIGraph::GetSmallSide(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& result)
{
    if (!m_articulationsValid)
        ComputeArticulationPoints();
    if (!m_articulations.Contains(primitive))
        return 0;

    BasicSet<BIBrickPrimitive*> best;
    const BasicSet<BIBrickPrimitive*>& adjacent = Neighbours(m_adjacency, primitive);
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = adjacent.Begin(); it != adjacent.End(); ++it) {
        BasicSet<BIBrickPrimitive*> side;
        side.Insert(primitive);
        CollectComponent(*it, side);
        if (side.GetSize() <= m_primitives.GetSize() / 2) {
            // NOTE: the original counts the neighbours inside and outside the side here and
            // never uses the counts (Win 0x596910; the Mac build keeps only the lookups).
            UInt32 inside = 0, outside = 0;
            for (BasicSet<BIBrickPrimitive*>::ConstIterator n = adjacent.Begin(); n != adjacent.End(); ++n) {
                if (side.Contains(*n))
                    ++inside;
                else
                    ++outside;
            }
            (void)inside;
            (void)outside;
            if (best.GetSize() < side.GetSize())
                best = side;
        }
    }
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = best.Begin(); it != best.End(); ++it)
        result.Insert(*it);
    return result.GetSize();
}

// Mac 0x263ed0, Win 0x596bc0
bool BIGraph::IsArticulation(BIBrickPrimitive* primitive)
{
    if (!m_articulationsValid)
        ComputeArticulationPoints();
    return m_articulations.Contains(primitive);
}

// Mac 0x263f40, Win 0x596db0
// NOTE: the original recurses once per primitive of the path, so a long chain of connected
// bricks overflows the stack; fixed: the same walk with an explicit stack (WalkDepthFirst, M11).
void BIGraph::CollectComponent(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& visited)
{
    ComponentWalker walker(m_adjacency, visited);
    WalkDepthFirst(walker, primitive);
}

// Mac 0x264070, Win 0x596e60
// NOTE: the original recurses once per primitive of the path, so a long chain of connected
// bricks overflows the stack; fixed: the same walk with an explicit stack (WalkDepthFirst, M11).
void BIGraph::CollectComponent(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& component,
                               BasicSet<BIBrickPrimitive*>& marked)
{
    MarkedComponentWalker walker(m_adjacency, component, marked);
    WalkDepthFirst(walker, primitive);
}

// Mac 0x264200, Win 0x596c10
bool BIGraph::IsSeparating(const BasicSet<BIBrickPrimitive*>& primitives)
{
    if (!m_articulationsValid)
        ComputeArticulationPoints();
    if (primitives.GetSize() == 1)
        return IsArticulation(*primitives.Begin());

    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_primitives.Begin(); it != m_primitives.End(); ++it) {
        BIBrickPrimitive* start = *it;
        // NOTE: a linear scan of the set (no lookup, no early exit), as the original
        for (BasicSet<BIBrickPrimitive*>::ConstIterator p = primitives.Begin(); p != primitives.End(); ++p) {
            if (*it == *p)
                start = 0;
        }
        if (start != 0) {
            BasicSet<BIBrickPrimitive*> reached;
            CollectComponentExcluding(start, reached, primitives);
            return reached.GetSize() != m_primitives.GetSize() - primitives.GetSize();
        }
    }
    return false;
}

// Mac 0x2643d0, Win 0x597120
// NOTE: the original recurses once per primitive of the path, so a long chain of connected
// bricks overflows the stack; fixed: the same walk with an explicit stack (WalkDepthFirst, M11).
void BIGraph::CollectComponentExcluding(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& visited,
                                        const BasicSet<BIBrickPrimitive*>& excluded)
{
    ExcludingWalker walker(m_adjacency, visited, excluded);
    WalkDepthFirst(walker, primitive);
}

// Mac 0x264570, Win 0x596f50
// NOTE: the original recurses once per primitive of the path, so a long chain of connected
// bricks overflows the stack; fixed: the same walk with an explicit stack (WalkDepthFirst, M11).
UInt32 BIGraph::CountConnected(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& visited,
                               const BasicSet<BIBrickPrimitive*>& within)
{
    CountWalker walker(m_adjacency, visited, within);
    WalkDepthFirst(walker, primitive);
    return walker.m_count;
}

// Mac 0x264770, Win 0x597080
bool BIGraph::IsConnected(const BasicSet<BIBrickPrimitive*>& primitives)
{
    // NOTE: the original starts at *primitives.Begin() without checking for an empty set
    // (it would read the set's header node); fixed: an empty set counts as connected.
    if (primitives.IsEmpty())
        return true;
    BasicSet<BIBrickPrimitive*> visited;
    UInt32 count = CountConnected(*primitives.Begin(), visited, primitives);
    return count == primitives.GetSize();
}

} // namespace LEGO
