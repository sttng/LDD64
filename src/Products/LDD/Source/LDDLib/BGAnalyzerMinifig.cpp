// LEGO Digital Designer — restored source
// TU 360 "BGAnalyzerMinifig.cpp" (Mac 0x343600-0x34473f): LEGO::BGAnalyzerMinifig.
#include "BGAnalyzerMinifig.h"
#include "BIBrickPrimitive.h"
#include "BIGraph.h"

namespace LEGO {

// Mac 0x343650 (C1) / 0x343600 (C2)
BGAnalyzerMinifig::BGAnalyzerMinifig(BIProducer* producer, Real32 priority, Real32 weight)
    : BIAnalyzer(producer, priority, weight)
{
}

// Mac 0x344630 / 0x344670 (base-object 0x344620, thunks 0x344650 / 0x3446b0), Win sdd 0x59f180
BGAnalyzerMinifig::~BGAnalyzerMinifig()
{
}

// Mac 0x3436e0, Win 0x59f1f0
String BGAnalyzerMinifig::GetName() const
{
    return String("Minifig");
}

// Mac 0x343710, Win 0x59f250
UInt32 BGAnalyzerMinifig::Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps)
{
    UInt32 count = 0;
    const PrimitiveSet& primitives = graph->GetPrimitives();
    for (PrimitiveSet::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        if (IsCancelled())
            return count;
        BIBrickPrimitive* torso = *it;
        if (!IsMinifig(graph, torso))
            continue;
        PrimitiveSet figure;
        CollectMinifig(graph, torso, figure);
        count += AddSubstep(figure, substeps, 2, 0) ? 1 : 0;
    }
    return count;
}

// Mac 0x344590, Win 0x59ffd0
void BGAnalyzerMinifig::GetAttributes(BIBrickSubstep::AttributeMap& attributes) const
{
    attributes[1] = true;
}

// Mac 0x343960, Win 0x59f480
void BGAnalyzerMinifig::CollectPart(BIGraph* graph, BIBrickPrimitive* torso, BIBrickPrimitive* part,
                                    PrimitiveSet& result)
{
    if (IsLegs(part))
        CollectLegs(graph, part, result);
    else if (IsHead(part))
        CollectHead(graph, part, result);
    else if (IsAccessory(part))
        CollectAccessory(graph, part, result);
    else if (IsHeldItem(part))
        CollectHeldItem(graph, part, torso, result);
    else
        CollectAttached(graph, part, result);
}

// Mac 0x344470, Win 0x59fe30
bool BGAnalyzerMinifig::IsMinifigPart(BIBrickPrimitive* primitive)
{
    return IsHead(primitive) || IsTorso(primitive) || IsLegs(primitive) || IsHeadwear(primitive) ||
           IsHeldItem(primitive) || IsAccessory(primitive);
}

// Mac 0x344500, Win 0x59fed0
bool BGAnalyzerMinifig::IsHead(BIBrickPrimitive* primitive)
{
    return primitive->HasMainGroup(245);
}

// Mac 0x344520, Win 0x59fef0
bool BGAnalyzerMinifig::IsTorso(BIBrickPrimitive* primitive)
{
    return primitive->ContainsDesign(3814);
}

// Mac 0x344540, Win 0x59ff10
bool BGAnalyzerMinifig::IsLegs(BIBrickPrimitive* primitive)
{
    if (primitive->HasMainGroup(240) && !IsTorso(primitive))
        return true;
    return primitive->ContainsDesign(3678);
}

// Mac 0x344030, Win 0x59ff70
bool BGAnalyzerMinifig::IsHeadwear(BIBrickPrimitive* primitive)
{
    return primitive->HasMainGroup(250);
}

// Mac 0x343cd0, Win 0x59ff90
bool BGAnalyzerMinifig::IsHeldItem(BIBrickPrimitive* primitive)
{
    return primitive->HasMainGroup(251);
}

// Mac 0x343b80, Win 0x59ffb0
bool BGAnalyzerMinifig::IsAccessory(BIBrickPrimitive* primitive)
{
    return primitive->HasMainGroup(252);
}

// Inlined on Mac (0x343710), Win 0x59f370
bool BGAnalyzerMinifig::IsMinifig(BIGraph* graph, BIBrickPrimitive* torso)
{
    if (!IsTorso(torso))
        return false;
    const PrimitiveSet& neighbours = graph->GetNeighbours(torso);
    return HasHead(neighbours) && HasLegs(neighbours);
}

// Inlined on Mac, Win 0x59fcb0
bool BGAnalyzerMinifig::HasHead(const PrimitiveSet& neighbours)
{
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it) {
        if (IsHead(*it))
            return true;
    }
    return false;
}

// Inlined on Mac (0x3442f0), Win 0x59fd30
bool BGAnalyzerMinifig::HasTorso(const PrimitiveSet& neighbours)
{
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it) {
        if (IsTorso(*it))
            return true;
    }
    return false;
}

// Inlined on Mac, Win 0x59fdb0
bool BGAnalyzerMinifig::HasLegs(const PrimitiveSet& neighbours)
{
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it) {
        if (IsLegs(*it))
            return true;
    }
    return false;
}

// Inlined on Mac (0x343710), Win 0x59f3e0
void BGAnalyzerMinifig::CollectMinifig(BIGraph* graph, BIBrickPrimitive* torso, PrimitiveSet& result)
{
    result.Insert(torso);
    const PrimitiveSet& neighbours = graph->GetNeighbours(torso);
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it)
        CollectPart(graph, torso, *it, result);
}

// Mac 0x343aa0, Win 0x59f560
void BGAnalyzerMinifig::CollectLegs(BIGraph* graph, BIBrickPrimitive* legs, PrimitiveSet& result)
{
    result.Insert(legs);
    const PrimitiveSet& neighbours = graph->GetNeighbours(legs);
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it) {
        if (IsAccessory(*it))
            result.Insert(*it);
    }
}

// Mac 0x343b40, Win 0x59f600
void BGAnalyzerMinifig::CollectHead(BIGraph* graph, BIBrickPrimitive* head, PrimitiveSet& result)
{
    result.Insert(head);
    CollectHeadwear(graph, head, result);
}

// Mac 0x343f30, Win 0x59f640
void BGAnalyzerMinifig::CollectHeadwear(BIGraph* graph, BIBrickPrimitive* primitive, PrimitiveSet& result)
{
    const PrimitiveSet& neighbours = graph->GetNeighbours(primitive);
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it) {
        BIBrickPrimitive* neighbour = *it;
        if (IsHeadwear(neighbour) && !result.Contains(neighbour)) {
            result.Insert(neighbour);
            CollectHeadwear(graph, neighbour, result);
        }
    }
}

// Mac 0x343ba0, Win 0x59f710
void BGAnalyzerMinifig::CollectAccessory(BIGraph* graph, BIBrickPrimitive* accessory, PrimitiveSet& result)
{
    PrimitiveSet parts;
    parts.Insert(accessory);
    if (!CollectSmallParts(graph, accessory, accessory->GetVolume(), parts)) {
        result.Insert(accessory);
    } else {
        for (PrimitiveSet::Iterator it = parts.Begin(); it != parts.End(); ++it)
            result.Insert(*it);
    }
}

// Mac 0x343cf0, Win 0x59f810
void BGAnalyzerMinifig::CollectHeldItem(BIGraph* graph, BIBrickPrimitive* item, BIBrickPrimitive* torso,
                                        PrimitiveSet& result)
{
    PrimitiveSet parts;
    parts.Insert(item);
    if (!CollectHeldItems(graph, item, torso, parts)) {
        CollectAttached(graph, item, result);
    } else {
        for (PrimitiveSet::Iterator it = parts.Begin(); it != parts.End(); ++it)
            result.Insert(*it);
    }
}

// Mac 0x343e10, Win 0x59f9f0
void BGAnalyzerMinifig::CollectAttached(BIGraph* graph, BIBrickPrimitive* part, PrimitiveSet& result)
{
    PrimitiveSet parts;
    parts.Insert(part);
    if (CollectSmallParts(graph, part, part->GetVolume(), parts)) {
        for (PrimitiveSet::Iterator it = parts.Begin(); it != parts.End(); ++it)
            result.Insert(*it);
    }
}

// Mac 0x3441c0, Win 0x59f900
bool BGAnalyzerMinifig::CollectHeldItems(BIGraph* graph, BIBrickPrimitive* primitive, BIBrickPrimitive* torso,
                                         PrimitiveSet& parts)
{
    const PrimitiveSet& neighbours = graph->GetNeighbours(primitive);
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it) {
        BIBrickPrimitive* neighbour = *it;
        if (neighbour == torso)
            continue;
        if (!IsHeldItem(neighbour))
            return false;
        if (!parts.Contains(neighbour)) {
            parts.Insert(neighbour);
            if (!CollectHeldItems(graph, neighbour, torso, parts))
                return false;
        }
    }
    return true;
}

// Mac 0x344050, Win 0x59fad0
bool BGAnalyzerMinifig::CollectSmallParts(BIGraph* graph, BIBrickPrimitive* primitive, Real32 volume,
                                          PrimitiveSet& parts)
{
    const PrimitiveSet& neighbours = graph->GetNeighbours(primitive);
    for (PrimitiveSet::ConstIterator it = neighbours.Begin(); it != neighbours.End(); ++it) {
        BIBrickPrimitive* neighbour = *it;
        if (parts.Contains(neighbour) || IsAtTorso(graph, neighbour))
            continue;
        volume += neighbour->GetVolume();
        parts.Insert(neighbour);
        if (volume > 40.0f)
            return false;
        if (!CollectSmallParts(graph, neighbour, volume, parts))
            return false;
    }
    return true;
}

// Mac 0x3442f0, Win 0x59fbe0
bool BGAnalyzerMinifig::IsAtTorso(BIGraph* graph, BIBrickPrimitive* primitive)
{
    if (!IsMinifigPart(primitive))
        return false;
    PrimitiveSet neighbours(graph->GetNeighbours(primitive));
    if (IsTorso(primitive))
        return true;
    return HasTorso(neighbours);
}

} // namespace LEGO
