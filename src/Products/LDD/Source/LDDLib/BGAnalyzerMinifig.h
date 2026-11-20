// LEGO Digital Designer — restored source
// LEGO::BGAnalyzerMinifig — building-guide analyzer: each torso with a head and legs next to it
//   becomes one substep holding the whole figure (legs + accessories, head + headwear, held
//   items, small attached parts), attribute 1 ("minifig", read by BGSubstepFilterMinifig).
//   Parts are recognised by "maingroupid" (BIBrick::HasMainGroup: 240 legs, 245 head,
//   250 headwear, 251 held items, 252 accessories) and design ID (3814 torso, 3678 dress slope
//   used as legs). Group meanings and all names inferred.
//
// Home TU 360 "BGAnalyzerMinifig.cpp" (Mac 0x343600-0x34473f, Win 0x59f180-0x59ffff).
// RTTI: class LEGO::BGAnalyzerMinifig : BIAnalyzer. Mac sizeof 0x10 + Object (no own data),
// vtable 0x74d13c {GetFacadeType 0x2230a0, GetName 0x3436e0, D1 0x344630, D0 0x344670,
// Analyze 0x343710, GetWeight 0x223110, GetAttributes 0x344590, idx 7-11: 0x343960, 0x344470,
// 0x344500, 0x344520, 0x344540}, base-object constructor 0x343600 (VTT), complete constructor
// 0x343650. Win vtable 0xa2c88c {0x5913d0, 0x59f1f0, 0x59f250, 0x5913e0, 0x59ffd0, 0x59f480,
// 0x59fe30, 0x59fed0, 0x59fef0, 0x59ff10} (same order), sdd 0x59f180.
// The non-virtual helpers are separate functions on Win; Mac inlined some (CollectMinifig, the
// neighbour tests) into the virtuals.
#pragma once
#include "BIAnalyzer.h"

namespace LEGO {

class BGAnalyzerMinifig : public BIAnalyzer {
public:
    typedef BasicSet<BIBrickPrimitive*> PrimitiveSet;

    // Mac 0x343650 (C1) / 0x343600 (C2), inlined on Win
    BGAnalyzerMinifig(BIProducer* producer, Real32 priority, Real32 weight);
    virtual ~BGAnalyzerMinifig();                                          // Mac 0x344630 / 0x344670 (base 0x344620), Win sdd 0x59f180

    // "Minifig". Mac 0x3436e0, Win 0x59f1f0
    virtual String GetName() const;                                        // idx 1
    // One substep (min 2 primitives) per torso of `graph` with a head and legs among its
    // neighbours; stops when the producer is cancelled. Mac 0x343710, Win 0x59f250
    virtual UInt32 Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps);   // idx 4
    // Sets attribute 1 (minifig). Mac 0x344590, Win 0x59ffd0
    virtual void GetAttributes(BIBrickSubstep::AttributeMap& attributes) const;     // idx 6
    // Adds the part `part` next to `torso` with what belongs to it to `result`.
    // Mac 0x343960, Win 0x59f480
    virtual void CollectPart(BIGraph* graph, BIBrickPrimitive* torso, BIBrickPrimitive* part,
                             PrimitiveSet& result);                        // idx 7
    // Any minifigure part. Mac 0x344470, Win 0x59fe30
    virtual bool IsMinifigPart(BIBrickPrimitive* primitive);               // idx 8
    // Main group 245. Mac 0x344500, Win 0x59fed0
    virtual bool IsHead(BIBrickPrimitive* primitive);                      // idx 9
    // Design 3814. Mac 0x344520, Win 0x59fef0
    virtual bool IsTorso(BIBrickPrimitive* primitive);                     // idx 10
    // Main group 240 (and not a torso) or design 3678. Mac 0x344540, Win 0x59ff10
    virtual bool IsLegs(BIBrickPrimitive* primitive);                      // idx 11

    // ---- non-virtual helpers (Mac addresses where out of line) -----------------------------
    // Main groups 250 / 251 / 252. Mac 0x344030 / 0x343cd0 / 0x343b80, Win 0x59ff70 /
    // 0x59ff90 / 0x59ffb0
    bool IsHeadwear(BIBrickPrimitive* primitive);
    bool IsHeldItem(BIBrickPrimitive* primitive);
    bool IsAccessory(BIBrickPrimitive* primitive);
    // A torso with a head and legs among its neighbours. Inlined on Mac, Win 0x59f370
    bool IsMinifig(BIGraph* graph, BIBrickPrimitive* torso);
    // A neighbour of the set is a head / torso / legs. Inlined on Mac, Win 0x59fcb0 /
    // 0x59fd30 / 0x59fdb0
    bool HasHead(const PrimitiveSet& neighbours);
    bool HasTorso(const PrimitiveSet& neighbours);
    bool HasLegs(const PrimitiveSet& neighbours);
    // The torso and CollectPart of each of its neighbours. Inlined on Mac, Win 0x59f3e0
    void CollectMinifig(BIGraph* graph, BIBrickPrimitive* torso, PrimitiveSet& result);
    // The legs and their accessory neighbours. Mac 0x343aa0, Win 0x59f560
    void CollectLegs(BIGraph* graph, BIBrickPrimitive* legs, PrimitiveSet& result);
    // The head and its headwear. Mac 0x343b40, Win 0x59f600
    void CollectHead(BIGraph* graph, BIBrickPrimitive* head, PrimitiveSet& result);
    // Headwear connected to `primitive`, recursively. Mac 0x343f30, Win 0x59f640
    void CollectHeadwear(BIGraph* graph, BIBrickPrimitive* primitive, PrimitiveSet& result);
    // An accessory with its small attachments, or the accessory alone. Mac 0x343ba0, Win 0x59f710
    void CollectAccessory(BIGraph* graph, BIBrickPrimitive* accessory, PrimitiveSet& result);
    // A held item with the held items connected to it (only held items and `torso` may touch
    // them), else CollectAttached. Mac 0x343cf0, Win 0x59f810
    void CollectHeldItem(BIGraph* graph, BIBrickPrimitive* item, BIBrickPrimitive* torso, PrimitiveSet& result);
    // A part with its small attachments (nothing when they get too big). Mac 0x343e10,
    // Win 0x59f9f0
    void CollectAttached(BIGraph* graph, BIBrickPrimitive* part, PrimitiveSet& result);
    // The held items reachable from `primitive`; false if another part than `torso` touches
    // them. Mac 0x3441c0, Win 0x59f900
    bool CollectHeldItems(BIGraph* graph, BIBrickPrimitive* primitive, BIBrickPrimitive* torso,
                          PrimitiveSet& parts);
    // The parts attached to `primitive` (not minifig parts at a torso) while their volume
    // stays <= 40; false when it gets larger. Mac 0x344050, Win 0x59fad0
    bool CollectSmallParts(BIGraph* graph, BIBrickPrimitive* primitive, Real32 volume, PrimitiveSet& parts);
    // A minifig part that is a torso or next to one. Mac 0x3442f0, Win 0x59fbe0
    bool IsAtTorso(BIGraph* graph, BIBrickPrimitive* primitive);
};

} // namespace LEGO
