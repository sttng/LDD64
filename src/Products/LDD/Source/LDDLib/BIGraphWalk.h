// LEGO Digital Designer — NOT original code: the iterative form of the recursive depth-first
// walks of the building-guide graphs (bug review M11). The original
// walks recurse once per primitive of the path (BIGraph::CollectComponent / ..Excluding /
// CountConnected, BIBrickPrimitive::CollectConnected, BIAnalyzerDirection::Collect), so a long
// chain of connected bricks overflows the stack. WalkDepthFirst keeps the call stack of those
// walks in an array instead: the same nodes are entered in the same order, the same tests are
// made at the same points, with the same results.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"

namespace LEGO {

// LDD_BI_WALK_TRACE (test switch, not original): tests/test_bi_graph.cpp defines BIWalkTrace and
// records every entered node (each call of the recursive original) to compare the visiting order
// of the walks with the recursive ones.
#if defined(LDD_BI_WALK_TRACE)
void BIWalkTrace(const void* node);
#define LDD_BI_WALK_ENTER(node) BIWalkTrace(node)
#else
#define LDD_BI_WALK_ENTER(node) ((void)0)
#endif

// One level of the walk: the position in the neighbours of an entered node (what the recursive
// call keeps in its loop).
template <class Iterator>
struct BIWalkFrame {
    Iterator m_next;
    Iterator m_end;
};

// The recursive walk
//     void Walk(Node node) { if (!Enter(node)) return;
//                            for (each neighbour n of node, in order) if (Descend(n)) Walk(n); }
// without recursion. The walker provides:
//     typedef … Node; typedef … Iterator;
//     bool Enter(Node)               the entry of the call (false: returns at once)
//     void GetRange(Node, Iterator& begin, Iterator& end)   the neighbours of an entered node
//     Node GetNode(const Iterator&)  the neighbour at the position
//     bool Descend(Node)             the test before the recursive call
// The neighbour ranges must not change during the walk (the walks only change their own sets).
template <class Walker>
void WalkDepthFirst(Walker& walker, typename Walker::Node start)
{
    typedef typename Walker::Iterator Iterator;
    LDD_BI_WALK_ENTER(start);
    if (!walker.Enter(start))
        return;
    BasicArray<BIWalkFrame<Iterator> > stack;
    BIWalkFrame<Iterator> frame;
    walker.GetRange(start, frame.m_next, frame.m_end);
    stack.PushBack(frame);
    while (!stack.IsEmpty()) {
        BIWalkFrame<Iterator>& top = stack.Back();
        if (top.m_next == top.m_end) {
            stack.PopBack();                     // the call returns
            continue;
        }
        typename Walker::Node node = walker.GetNode(top.m_next);
        ++top.m_next;                            // (the loop goes on here after the call)
        if (!walker.Descend(node))
            continue;
        LDD_BI_WALK_ENTER(node);                 // the recursive call
        if (!walker.Enter(node))
            continue;
        walker.GetRange(node, frame.m_next, frame.m_end);
        stack.PushBack(frame);                   // (`top` is not used after this)
    }
}

} // namespace LEGO
