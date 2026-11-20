// LEGO Digital Designer — restored source
// TU 164 "BIAnalyzerDirection.cpp" (Mac 0x2232d0-0x2245f9): LEGO::BIAnalyzerDirection.
// Template code of the TU: BasicSet<UInt32> (operator[] of the visited map 0x223ba0, insert
// 0x223d00 / 0x223ec0, node copy 0x223f80 / 0x224060, erase 0x2241c0, Win 0x59cea0),
// BasicMap<UInt32, Vector3> (destructors 0x223a70 / 0x224220, Win sdd 0x56f320; erase
// 0x224270, Win 0x56f150; insert 0x2242c0 / 0x2244b0), BasicMap<const BIBrickPrimitive*,
// BasicSet<UInt32> > (destructors 0x223a40 / 0x224170, Win sdd 0x59cf20).
#include "BIAnalyzerDirection.h"
#include "BIGraph.h"
#include "BIBrickPrimitive.h"
#include <math.h>
#include "BIGraphWalk.h"

namespace LEGO {

BasicMap<UInt32, Vector3> BIAnalyzerDirection::s_directions;   // TU initialiser Mac 0x224560
UInt32 BIAnalyzerDirection::s_nextDirection = 0;

namespace {

// The connection from `primitive` to `other`. NOTE: the original reads
// find(other)->second unchecked; a neighbour without a connection from this side (the
// adjacency is symmetric, the connections need not be) reads the end node; fixed: no
// directions then.
const BIBrick::Connection* FindConnection(const BIBrickPrimitive* primitive, BIBrickPrimitive* other)
{
    const BIBrick::ConnectionMap& connections = primitive->GetConnections();
    BIBrick::ConnectionMap::ConstIterator it = connections.Find(other);
    return it == connections.End() ? 0 : it->second.Get();
}

// One level of the iterative Collect (M11): the position in the neighbours of the primitive
// and, while one is being looked at, in the directions of its connection.
struct DirectionFrame {
    BIBrickPrimitive*                         m_primitive;
    BasicSet<BIBrickPrimitive*>::ConstIterator m_neighbour;
    BasicSet<BIBrickPrimitive*>::ConstIterator m_end;
    const BIBrick::Connection*                m_connection;   // null: between two neighbours
    BasicArray<Vector3>::ConstIterator        m_direction;
};

} // namespace

// Mac 0x223ad0 / 0x223af0, Win 0x591bb0 / sdd 0x591c20
BIAnalyzerDirection::~BIAnalyzerDirection()
{
}

// Mac 0x223aa0, Win 0x591b50
String BIAnalyzerDirection::GetName() const
{
    return String("Direction");
}

// Mac 0x2234f0, Win 0x59cc40
UInt32 BIAnalyzerDirection::GetDirectionID(const Vector3& direction)
{
    for (BasicMap<UInt32, Vector3>::ConstIterator it = s_directions.Begin(); it != s_directions.End(); ++it) {
        const Vector3& known = it->second;
        if ((fabsf(known.x - direction.x) < 0.1f && fabsf(known.y - direction.y) < 0.1f &&
             fabsf(known.z - direction.z) < 0.1f) ||
            (fabsf(known.x + direction.x) < 0.1f && fabsf(known.y + direction.y) < 0.1f &&
             fabsf(known.z + direction.z) < 0.1f))
            return it->first;
    }
    s_directions[s_nextDirection] = direction;
    return s_nextDirection++;
}

// Mac 0x2232d0, Win 0x59c820
// NOTE: the original recurses once per primitive of the path, so a long chain of connected
// bricks overflows the stack; fixed: the same walk with an explicit stack (M11): the same
// primitives are entered and the same direction numbers looked up (GetDirectionID registers new
// directions) in the same order.
void BIAnalyzerDirection::Collect(BIBrickPrimitive* primitive, BIGraph* graph, BasicSet<BIBrickPrimitive*>& component,
                                  UInt32 direction, VisitedMap& visited)
{
    BasicArray<DirectionFrame> stack;
    BIBrickPrimitive* next = primitive;
    for (;;) {
        if (next != 0) {
            // the entry of the recursive call
            LDD_BI_WALK_ENTER(next);
            if (!visited[next].Contains(direction)) {
                component.Insert(next);
                visited[next].Insert(direction);
                const BasicSet<BIBrickPrimitive*>& neighbours = graph->GetNeighbours(next);
                DirectionFrame frame;
                frame.m_primitive = next;
                frame.m_neighbour = neighbours.Begin();
                frame.m_end = neighbours.End();
                frame.m_connection = 0;
                frame.m_direction = 0;
                stack.PushBack(frame);
            }
            next = 0;
        }
        if (stack.IsEmpty())
            break;
        DirectionFrame& top = stack.Back();
        if (top.m_connection == 0) {
            if (top.m_neighbour == top.m_end) {
                stack.PopBack();             // the call returns
                continue;
            }
            top.m_connection = FindConnection(top.m_primitive, *top.m_neighbour);
            if (top.m_connection == 0) {
                ++top.m_neighbour;
                continue;
            }
            top.m_direction = top.m_connection->m_directions.Begin();
        }
        if (top.m_direction == top.m_connection->m_directions.End()) {
            top.m_connection = 0;
            ++top.m_neighbour;
            continue;
        }
        UInt32 id = GetDirectionID(*top.m_direction);
        ++top.m_direction;                   // (the loop goes on here after the call)
        if (id == direction)
            next = *top.m_neighbour;         // Collect(*it, graph, component, id, visited)
    }
}

// Mac 0x2236a0, Win 0x59c990
UInt32 BIAnalyzerDirection::Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps)
{
    UInt32 count = 0;
    if (graph->GetPrimitiveCount() <= 2)
        return count;

    VisitedMap visited;
    const BasicSet<BIBrickPrimitive*>& primitives = graph->GetPrimitives();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        BIBrickPrimitive* primitive = *it;
        BasicSet<BIBrickPrimitive*> neighbours;
        graph->GetNeighbours(primitive, neighbours);
        for (BasicSet<BIBrickPrimitive*>::ConstIterator n = neighbours.Begin(); n != neighbours.End(); ++n) {
            const BIBrick::Connection* connection = FindConnection(primitive, *n);
            if (connection == 0)
                continue;
            for (BasicArray<Vector3>::ConstIterator d = connection->m_directions.Begin(); d != connection->m_directions.End(); ++d) {
                if (IsCancelled())
                    return count;
                BasicSet<BIBrickPrimitive*> component;
                Collect(primitive, graph, component, GetDirectionID(*d), visited);
                if (m_minSize <= component.GetSize() && component.GetSize() <= graph->GetPrimitiveCount() / 2) {
                    if (AddSubstep(component, substeps, 2, 0))
                        ++count;
                }
            }
        }
    }
    return count;
}

} // namespace LEGO
