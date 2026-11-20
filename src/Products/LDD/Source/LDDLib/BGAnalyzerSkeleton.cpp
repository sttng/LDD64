// LEGO Digital Designer — restored source
// TU 365 "BGAnalyzerSkeleton.cpp" (Mac 0x34c7d0-0x34ce4f): LEGO::BGAnalyzerSkeleton.
#include "BGAnalyzerSkeleton.h"
#include "BIBrickPrimitive.h"
#include "BIGraph.h"

namespace LEGO {

// Mac 0x34c7d0
BGAnalyzerSkeleton::BGAnalyzerSkeleton(BIProducer* producer, Real32 priority, Real32 weight)
    : BGAnalyzerMinifig(producer, priority, weight)
{
}

// Mac 0x34cc60 / 0x34cd20 (thunks 0x34ccc0 / 0x34cd90), Win sdd 0x5a0970
BGAnalyzerSkeleton::~BGAnalyzerSkeleton()
{
}

// Mac 0x34c860, Win 0x5a09e0
void BGAnalyzerSkeleton::CollectPart(BIGraph* graph, BIBrickPrimitive* torso, BIBrickPrimitive* part,
                                     PrimitiveSet& result)
{
    if (IsLegs(part))
        CollectLegs(graph, part, result);
    else if (IsSkeletonArm(part))
        CollectSkeletonArm(graph, part, result);
    else if (IsHead(part))
        CollectHead(graph, part, result);
    else if (IsAccessory(part))
        CollectAccessory(graph, part, result);
    else if (IsHeldItem(part))
        CollectHeldItem(graph, part, torso, result);
    else
        CollectAttached(graph, part, result);
}

// Mac 0x34ca20, Win 0x5a0bb0
bool BGAnalyzerSkeleton::IsMinifigPart(BIBrickPrimitive* primitive)
{
    return IsHead(primitive) || IsTorso(primitive) || IsLegs(primitive) || IsHeadwear(primitive) ||
           IsHeldItem(primitive) || IsAccessory(primitive) || IsSkeletonArm(primitive);
}

// Mac 0x34cb10, Win 0x5a0c60
bool BGAnalyzerSkeleton::IsHead(BIBrickPrimitive* primitive)
{
    return primitive->HasMainGroup(245) || primitive->ContainsDesign(85946) || primitive->ContainsDesign(30529);
}

// Mac 0x34cb60, Win 0x5a0cb0
bool BGAnalyzerSkeleton::IsTorso(BIBrickPrimitive* primitive)
{
    return primitive->ContainsDesign(30375) || primitive->ContainsDesign(6260) || primitive->ContainsDesign(60115) ||
           primitive->ContainsDesign(54275) || primitive->ContainsDesign(53988);
}

// Mac 0x34cbe0, Win 0x5a0d30
bool BGAnalyzerSkeleton::IsLegs(BIBrickPrimitive* primitive)
{
    return primitive->ContainsDesign(6266) || primitive->ContainsDesign(30530) || primitive->ContainsDesign(30376) ||
           primitive->ContainsDesign(53984) || primitive->ContainsDesign(54276);
}

// Inlined on Mac (0x34c860 / 0x34ca20), Win 0x5a0db0
bool BGAnalyzerSkeleton::IsSkeletonArm(BIBrickPrimitive* primitive)
{
    return primitive->ContainsDesign(6265) || primitive->ContainsDesign(30377) || primitive->ContainsDesign(59230) ||
           primitive->ContainsDesign(53989);
}

// Mac 0x34c970, Win 0x5a0af0
void BGAnalyzerSkeleton::CollectSkeletonArm(BIGraph* graph, BIBrickPrimitive* arm, PrimitiveSet& result)
{
    result.Insert(arm);
    const PrimitiveSet& neighbours = graph->GetNeighbours(arm);
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it) {
        if (IsHeldItem(*it))
            CollectHeldItem(graph, *it, arm, result);
        else
            CollectAttached(graph, *it, result);
    }
}

} // namespace LEGO
