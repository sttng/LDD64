// LEGO Digital Designer — restored source
// LEGO::BIBrickRepository — the BIBricks of the current scene for the building guide: one
//   BIBrickPrimitive per scene brick (per flexible part for wire assemblies), the substeps
//   created so far, the state of each brick and the primitive of every primitive element.
//   One global instance (Mac 0x77af08, reached through the non-lazy pointer 0x70232c).
//
// Home TU 214 "BIBrickRepository.cpp" (name inferred; Mac 0x25f870-0x2627cf, modmap guess
// BasicArray.cpp; RTTI BasicMap<BIBrick*, BIBrickRepository::BrickState, …>). Not an Object.
// Mac layout (0x48): +0x00 m_bricks, +0x10 m_states, +0x2c m_primitives.
// Destructor Mac 0x25f870 (body 0x2615c0), registered by the TU initialiser 0x2626d0.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include <unordered_map>
#include <vector>

namespace LEGO {

class Scene;
class Brick;
class Part;
class PrimitiveElement;
class BIBrick;
class BIBrickPrimitive;
class BIBrickSubstep;

class BIBrickRepository {
public:
    // m_states values (names inferred). Free bricks are the ones the generators may still use.
    enum BrickState {
        StateFree    = 0,
        StateUsed    = 1,
        StateRemoved = 2
    };
    typedef BasicMap<BIBrick*, BrickState> StateMap;

    BIBrickRepository() {}
    ~BIBrickRepository() {}                                                // Mac 0x25f870 (0x2615c0)

    // The global repository (Mac 0x77af08). (name inferred)
    static BIBrickRepository& GetInstance();

    // Rebuilds everything from the primitive elements of `scene`, then computes the
    // connections of every primitive. Always true. Mac 0x25f880
    bool Build(Scene* scene);
    // Appends `brick` with state free; returns it. Mac 0x260390
    BIBrick* AddBrick(const RefPointer<BIBrick>& brick);
    // The registered substep equal to `substep` that is not removed (marked free again), or
    // `substep` added as a new brick. Null for null. Mac 0x2605a0
    BIBrick* AddSubstep(const RefPointer<BIBrickSubstep>& substep);
    // Mac 0x260b10 / 0x260b90 (names inferred). Always true.
    bool SetUsed(BIBrick* brick);
    bool SetRemoved(BIBrick* brick);
    // The free registered substep with exactly these primitives, else a new substep
    // (group 0, weight 1) added through AddSubstep; null for fewer than two primitives.
    // Mac 0x260c10 (name inferred)
    BIBrick* GetSubstep(const BasicSet<BIBrickPrimitive*>& primitives);
    // The primitive of an element (null if unknown; with `freeOnly` also null if the
    // primitive is not free). Mac 0x260d40 (name inferred)
    BIBrickPrimitive* GetPrimitive(const PrimitiveElement* element, bool freeOnly);
    // Free bricks / free primitives / free substeps. Mac 0x260df0, 0x260eb0, 0x260f80
    // (names inferred)
    void GetFreeBricks(BasicSet<BIBrick*>& bricks);
    void GetFreePrimitives(BasicSet<BIBrickPrimitive*>& primitives);
    void GetFreeSubsteps(BasicArray<BIBrickSubstep*>& substeps);
    // Free bricks whose primitives are all in `primitives` but that are not exactly
    // `primitives`. Mac 0x261250 (name inferred)
    void GetFreeBricksWithin(const BasicSet<BIBrickPrimitive*>& primitives, BasicSet<BIBrick*>& bricks);

    const BasicArray<RefPointer<BIBrick> >& GetBricks() const { return m_bricks; }
    const StateMap& GetStates() const { return m_states; }

private:
    // One primitive element of the scene (with its world transform). Mac 0x25fd00
    void AddElement(PrimitiveElement* element, BasicMap<Brick*, BIBrickPrimitive*>& brickPrimitives,
                    BasicMap<Part*, BIBrickPrimitive*>& partPrimitives);

    BasicArray<RefPointer<BIBrick> >                     m_bricks;      // +0x00
    StateMap                                             m_states;      // +0x10
    BasicMap<const PrimitiveElement*, BIBrickPrimitive*> m_primitives;  // +0x2c
    // The substeps of m_bricks by a hash of their primitives (positions in m_bricks, in
    // order): AddSubstep and GetSubstep look at the substeps with the same hash instead of
    // walking every brick; the substeps with another hash have other primitives, which the
    // walks skip.
    static size_t HashPrimitives(const BasicSet<BIBrickPrimitive*>& primitives);
    void IndexBrick(UInt32 position);

    std::unordered_map<size_t, std::vector<UInt32> >     m_substepIndex;
};

} // namespace LEGO
