// LEGO Digital Designer — restored source
// LEGO::BIGraph — one connected group of BIBrickPrimitives of the building-guide scene: the
//   primitives, their adjacency (one has a BIBrick::Connection to the other) and, on demand,
//   the articulation points (primitives whose removal splits the group). The static members
//   keep a scene's graph list up to date while primitives are added / removed (split, merge).
//
// Home TU 215 "BIGraph.cpp" (Mac 0x2627d0-0x26509f); the list functions are TU 216 (Mac
// 0x2650a0-0x26624f, modmap guess BasicSet.cpp; restored in BIGraphList.cpp, name inferred).
// RTTI: class LEGO::BIGraph : Object (not virtual).
//   Mac sizeof 0x64: vptr +0, refcount +4, m_graphs +0x08, m_adjacency +0x0c (BasicMap,
//       0x1c), m_primitives +0x28, m_articulations +0x44 (BasicSet, 0x1c each),
//       m_articulationsValid +0x60. Vtable 0x736ee0 {D1 0x262d40, D0 0x262d50, Object slots}.
//       Constructor C2 0x2627d0 / C1 0x262c60.
//   Win sizeof 0x7c (VC9 maps / sets are 0x24 bytes): m_graphs +0x08, m_adjacency +0x0c,
//       m_primitives +0x30, m_articulations +0x54, m_articulationsValid +0x78. Vtable 0xa2c790
//       {sdd 0x595f40}, destructor 0x595f70, constructor 0x595c80.
// The articulation search keeps its state in the bricks (BIBrick::m_dfsNumber, m_dfsLow,
// m_dfsParent, m_dfsChildren).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"

namespace LEGO {

class BIBrick;
class BIBrickPrimitive;

class BIGraph : public Object {
public:
    typedef BasicArray<RefPointer<BIGraph> > GraphList;   // (name inferred)
    typedef BasicMap<BIBrickPrimitive*, BasicSet<BIBrickPrimitive*> > Adjacency;

    // A graph of `primitives` (adjacency from their connections, restricted to the set);
    // `graphs` is the list it will be kept in. Mac 0x2627d0 (C2) / 0x262c60 (C1), Win 0x595c80
    BIGraph(const BasicSet<BIBrickPrimitive*>& primitives, GraphList* graphs);
    virtual ~BIGraph();                                                    // Mac 0x262d40 (body 0x262c70) / 0x262d50, Win 0x595f70 / sdd 0x595f40

    // Adds `primitive` with its edges to the primitives of the graph. Mac 0x262d90, Win 0x596000
    // (name inferred)
    void AddPrimitive(BIBrickPrimitive* primitive);
    // Removes `primitive` and its edges. Mac 0x262f50, Win 0x596120 (name inferred)
    void RemovePrimitive(BIBrickPrimitive* primitive);
    // Every primitive of the graph is in `primitives` (BIBrick::IsContainedIn). Mac 0x263130,
    // Win 0x5961e0 (name inferred)
    bool IsContainedIn(const BasicSet<BIBrickPrimitive*>& primitives) const;
    // The primitives adjacent to `primitive` (which must be in the graph). Mac 0x263180,
    // Win 0x596260 (name inferred)
    const BasicSet<BIBrickPrimitive*>& GetNeighbours(BIBrickPrimitive* primitive) const;
    // Adds the primitives adjacent to any primitive of `brick` (these may include primitives of
    // the brick itself). Mac 0x2631d0, Win 0x596290 (name inferred)
    void GetNeighbours(const BIBrick* brick, BasicSet<BIBrickPrimitive*>& neighbours) const;
    // The first adjacent primitive (by address) or null. Mac 0x263330, Win 0x5963b0 (name inferred)
    BIBrickPrimitive* GetFirstNeighbour(BIBrickPrimitive* primitive) const;
    // `a` and `b` are adjacent (inlined on Mac). Win 0x596430 (name inferred)
    bool IsNeighbour(BIBrickPrimitive* a, BIBrickPrimitive* b) const;
    // Fills m_articulations (iterative depth-first search from the first primitive, Hopcroft /
    // Tarjan low points). Mac 0x263390, Win 0x5964d0 (name inferred)
    void ComputeArticulationPoints();
    // For an articulation point: the largest of the components that remain when it is removed
    // which has at most half the primitives of the graph, plus the articulation point itself,
    // added to `result`; returns result's size. 0 (nothing added) for other primitives.
    // Mac 0x263b00, Win 0x596910 (name inferred)
    UInt32 GetSmallSide(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& result);
    // Mac 0x263ed0, Win 0x596bc0 (name inferred)
    bool IsArticulation(BIBrickPrimitive* primitive);
    // Removing the primitives splits the graph (one primitive: IsArticulation). Mac 0x264200,
    // Win 0x596c10 (name inferred)
    bool IsSeparating(const BasicSet<BIBrickPrimitive*>& primitives);
    // The number of primitives of `within` reachable from `primitive` through primitives of
    // `within` (the start counts only if it is in `within`); also used by BIAnalyzerGroups.
    // Mac 0x264570, Win 0x596f50
    UInt32 CountConnected(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& visited,
                          const BasicSet<BIBrickPrimitive*>& within);
    // The primitives are connected through primitives of the set only. Mac 0x264770,
    // Win 0x597080 (name inferred)
    bool IsConnected(const BasicSet<BIBrickPrimitive*>& primitives);

    // The primitives of the graph (+0x28; read inline by BIGenerator). (name inferred)
    const BasicSet<BIBrickPrimitive*>& GetPrimitives() const { return m_primitives; }
    UInt32 GetPrimitiveCount() const { return m_primitives.GetSize(); }
    const Adjacency& GetAdjacency() const { return m_adjacency; }
    const BasicSet<BIBrickPrimitive*>& GetArticulations() const { return m_articulations; }
    GraphList* GetGraphList() const { return m_graphs; }

    // ---- the graph list (TU 216, BIGraphList.cpp; names inferred) --------------------------
    // Splits `primitives` into connected components (BIBrickPrimitive::CollectConnected) and
    // appends one graph per component. Mac 0x2652c0, Win 0x590640
    static void Build(GraphList& graphs, const BasicSet<BIBrickPrimitive*>& primitives);
    // The graph that holds the primitive, else null. Mac 0x2650d0, Win 0x5905d0
    static BIGraph* FindGraph(const GraphList& graphs, BIBrickPrimitive* primitive);
    // The graph that holds the brick (a primitive, else the substep's first primitive).
    // Mac 0x265160, Win 0x590550
    static BIGraph* FindBrickGraph(const GraphList& graphs, BIBrick* brick);
    // Adds the primitive: to the one graph it is connected to, else a new graph that merges
    // every graph it is connected to (none: a graph of the primitive alone).
    // Mac 0x265af0, Win 0x590a60
    static void AddPrimitive(GraphList& graphs, BIBrickPrimitive* primitive);
    // Removes the primitive from its graph; the parts it held together become graphs of their
    // own and a graph left empty is removed. Mac 0x265620, Win 0x5907c0
    static void RemovePrimitive(GraphList& graphs, BIBrickPrimitive* primitive);
    // FindGraph(primitive)->IsArticulation(primitive). Mac 0x265e70, Win 0x590cd0
    static bool IsArticulation(const GraphList& graphs, BIBrickPrimitive* primitive);
    // The graph of the first primitive ->IsSeparating(primitives). Mac 0x265f10, Win 0x590d00
    static bool IsSeparating(const GraphList& graphs, const BasicSet<BIBrickPrimitive*>& primitives);

private:
    // Depth-first collection of the primitives reachable from `primitive` that are not in
    // `visited` (inserted there). Mac 0x263f40, Win 0x596db0
    void CollectComponent(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& visited);
    // The same, collected into `component` (also the visited set) and marked in `marked`;
    // returns at once for a primitive already in `marked`. Mac 0x264070, Win 0x596e60
    void CollectComponent(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& component,
                          BasicSet<BIBrickPrimitive*>& marked);
    // The same, not entering primitives of `excluded`. Mac 0x2643d0, Win 0x597120
    void CollectComponentExcluding(BIBrickPrimitive* primitive, BasicSet<BIBrickPrimitive*>& visited,
                                   const BasicSet<BIBrickPrimitive*>& excluded);

public:
    GraphList*                  m_graphs;              // +0x08 (name inferred; never read here)
    Adjacency                   m_adjacency;           // +0x0c
    BasicSet<BIBrickPrimitive*> m_primitives;          // +0x28
    BasicSet<BIBrickPrimitive*> m_articulations;       // +0x44
    bool                        m_articulationsValid;  // +0x60
};

} // namespace LEGO
