// LEGO Digital Designer — restored source
// TU 216 (Mac 0x2650a0-0x26624f; file name inferred, modmap guess BasicSet.cpp): the functions
// that keep a list of BIGraphs (the connected groups of a building-guide scene, held by
// BGAutomaticScene +0x3c and BGAutomaticApplication) up to date. Declared as static members of
// BIGraph (BIGraph.h). Template code of the TU: BasicArray<RefPointer<BIGraph> > (constructor
// 0x2650a0, Clear 0x265270, PushBack 0x265500, unordered Remove 0x265980, destructors
// 0x265fc0 / 0x266020, Win sdd 0x56da30), BasicSet<BIGraph*> (insert 0x266140, destructors
// 0x265e40 / 0x2660a0, erase 0x2660f0; Win 0x590d40, 0x590db0, sdd 0x590e30).
#include "BIGraph.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"

namespace LEGO {

namespace {

// BasicArray<RefPointer<BIGraph> > unordered removal (Mac 0x265980): the found element is
// replaced by the last one, the array shrinks by one.
void RemoveGraph(BIGraph::GraphList& graphs, BIGraph* graph)
{
    for (BIGraph::GraphList::Iterator it = graphs.Begin(); it != graphs.End(); ++it) {
        if (it->Get() == graph) {
            *it = graphs.Back();
            graphs.PopBack();
            return;
        }
    }
}

} // namespace

// Mac 0x2652c0, Win 0x590640
void BIGraph::Build(GraphList& graphs, const BasicSet<BIBrickPrimitive*>& primitives)
{
    BasicSet<BIBrickPrimitive*> remaining(primitives);
    while (remaining.GetSize() != 0) {
        BasicSet<BIBrickPrimitive*> component;
        (*remaining.Begin())->CollectConnected(component, remaining);
        RefPointer<BIGraph> graph = new BIGraph(component, &graphs);
        graphs.PushBack(graph);
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = component.Begin(); it != component.End(); ++it)
            remaining.Erase(*it);
    }
}

// Mac 0x2650d0, Win 0x5905d0
BIGraph* BIGraph::FindGraph(const GraphList& graphs, BIBrickPrimitive* primitive)
{
    for (GraphList::ConstIterator it = graphs.Begin(); it != graphs.End(); ++it) {
        if ((*it)->m_primitives.Contains(primitive))
            return *it;
    }
    return 0;
}

// Mac 0x265160, Win 0x590550
BIGraph* BIGraph::FindBrickGraph(const GraphList& graphs, BIBrick* brick)
{
    // NOTE: a substep is looked up by its first primitive (m_primitives.Begin(), read inline).
    BIBrickPrimitive* primitive = brick->GetType() == BIBrick::TypePrimitive
                                      ? static_cast<BIBrickPrimitive*>(brick)
                                      : *brick->GetPrimitiveSet().Begin();
    return FindGraph(graphs, primitive);
}

// Mac 0x265af0, Win 0x590a60
void BIGraph::AddPrimitive(GraphList& graphs, BIBrickPrimitive* primitive)
{
    // the graphs the primitive is connected to
    BasicSet<BIGraph*> connected;
    const BIBrick::ConnectionMap& connections = primitive->GetConnections();
    for (BIBrick::ConnectionMap::ConstIterator c = connections.Begin(); c != connections.End(); ++c) {
        BIGraph* graph = FindGraph(graphs, c->first);
        if (graph != 0)
            connected.Insert(graph);
    }
    if (!connections.IsEmpty() && connected.GetSize() == 1) {
        (*connected.Begin())->AddPrimitive(primitive);
        return;
    }

    // none or several: one new graph of the primitive and all those graphs
    BasicSet<BIBrickPrimitive*> merged;
    merged.Insert(primitive);
    for (BasicSet<BIGraph*>::ConstIterator it = connected.Begin(); it != connected.End(); ++it) {
        const BasicSet<BIBrickPrimitive*>& primitives = (*it)->m_primitives;
        for (BasicSet<BIBrickPrimitive*>::ConstIterator p = primitives.Begin(); p != primitives.End(); ++p)
            merged.Insert(*p);
        RemoveGraph(graphs, *it);
    }
    RefPointer<BIGraph> graph = new BIGraph(merged, &graphs);
    graphs.PushBack(graph);
}

// Mac 0x265620, Win 0x5907c0
void BIGraph::RemovePrimitive(GraphList& graphs, BIBrickPrimitive* primitive)
{
    RefPointer<BIGraph> graph = FindGraph(graphs, primitive);
    // NOTE: the original dereferences the graph unchecked (a primitive of no graph crashes).
    if (graph.IsNull())
        return;
    if (graph->m_primitives.GetSize() == 1) {
        RemoveGraph(graphs, graph);
        return;
    }
    if (!graph->IsArticulation(primitive)) {
        graph->RemovePrimitive(primitive);
        return;
    }
    // an articulation point: every part it held together becomes a graph of its own
    BIBrickPrimitive* neighbour;
    while ((neighbour = graph->GetFirstNeighbour(primitive)) != 0) {
        BasicSet<BIBrickPrimitive*> component;
        BasicSet<BIBrickPrimitive*> marked;
        component.Insert(primitive);
        graph->CollectComponent(neighbour, component, marked);
        component.Erase(primitive);
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = component.Begin(); it != component.End(); ++it)
            graph->RemovePrimitive(*it);
        RefPointer<BIGraph> part = new BIGraph(component, &graphs);
        graphs.PushBack(part);
    }
    graph->RemovePrimitive(primitive);
    RemoveGraph(graphs, graph);
}

// Mac 0x265e70, Win 0x590cd0
bool BIGraph::IsArticulation(const GraphList& graphs, BIBrickPrimitive* primitive)
{
    BIGraph* graph = FindGraph(graphs, primitive);
    // NOTE: unchecked in the original.
    return graph != 0 && graph->IsArticulation(primitive);
}

// Mac 0x265f10, Win 0x590d00
bool BIGraph::IsSeparating(const GraphList& graphs, const BasicSet<BIBrickPrimitive*>& primitives)
{
    // NOTE: the graph of the first primitive, unchecked in the original.
    BIGraph* graph = primitives.IsEmpty() ? 0 : FindGraph(graphs, *primitives.Begin());
    return graph != 0 && graph->IsSeparating(primitives);
}

} // namespace LEGO
