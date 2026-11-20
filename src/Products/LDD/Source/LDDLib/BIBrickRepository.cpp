// LEGO Digital Designer — restored source
// TU 214 "BIBrickRepository.cpp" (name inferred; Mac 0x25f870-0x2627cf). See
// BIBrickRepository.h. Also holds BIBrickPrimitive::AddElement (Mac 0x2607f0).
#include "BIBrickRepository.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIBrickSubstep.h"
#include "BI.h"
#include "Scene.h"
#include "Brick.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include "Primitive.h"

namespace LEGO {

// Mac 0x77af08 (constructed by the TU initialiser 0x2626d0, destroyed through 0x25f870)
static BIBrickRepository g_brickRepository;

BIBrickRepository& BIBrickRepository::GetInstance()
{
    return g_brickRepository;
}

// Design that never joins the other elements of its brick (0xe30c = 58124). (name inferred)
static const UInt32 kSeparateDesignID = 0xe30c;

// Mac 0x25f880
bool BIBrickRepository::Build(Scene* scene)
{
    m_bricks.Clear();
    m_states.Clear();
    m_primitives.Clear();
    m_substepIndex.clear();
    BasicMap<Brick*, BIBrickPrimitive*> brickPrimitives;
    BasicMap<Part*, BIBrickPrimitive*> partPrimitives;
    BasicArray<RefPointer<PrimitiveElement> > elements(scene->m_primitiveElements);
    for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = elements.Begin(); it != elements.End(); ++it)
        AddElement(*it, brickPrimitives, partPrimitives);
    BasicSet<BIBrickPrimitive*> none;
    for (BasicArray<RefPointer<BIBrick> >::Iterator it = m_bricks.Begin(); it != m_bricks.End(); ++it) {
        BIBrick::ConnectionMap connections;
        BI::ComputeConnections(*it, connections, none);
        (*it)->m_connections = connections;
    }
    return true;
}

// Mac 0x25fd00
void BIBrickRepository::AddElement(PrimitiveElement* element, BasicMap<Brick*, BIBrickPrimitive*>& brickPrimitives,
                                   BasicMap<Part*, BIBrickPrimitive*>& partPrimitives)
{
    Matrix3D transform;
    element->GetTransform(transform);
    Part* part = element->GetPart();
    if (!BI::IsWire(part->GetBrick())) {
        if (element->GetPrimitive()->GetDesignID() == kSeparateDesignID) {
            RefPointer<BIBrickPrimitive> primitive = new BIBrickPrimitive;
            primitive->AddElement(element, transform);
            AddBrick(primitive.Get());
            m_primitives[element] = primitive;
            return;
        }
        BasicMap<Brick*, BIBrickPrimitive*>::Iterator found = brickPrimitives.Find(part->GetBrick());
        if (found != brickPrimitives.End()) {
            m_primitives[element] = found->second;
            found->second->AddElement(element, transform);
            return;
        }
        BIBrickPrimitive* primitive = new BIBrickPrimitive;
        primitive->AddElement(element, transform);
        brickPrimitives[part->GetBrick()] = primitive;
        m_primitives[element] = primitive;
        RefPointer<BIBrickPrimitive> reference = primitive;
        AddBrick(reference.Get());
    } else {
        // Wire assemblies: one primitive per flexible part, single-bone parts alone.
        if (part->GetElementCount() < 2) {
            RefPointer<BIBrickPrimitive> primitive = new BIBrickPrimitive;
            primitive->AddElement(element, transform);
            AddBrick(primitive.Get());
            m_primitives[element] = primitive;
            return;
        }
        BasicMap<Part*, BIBrickPrimitive*>::Iterator found = partPrimitives.Find(part);
        if (found != partPrimitives.End()) {
            m_primitives[element] = found->second;
            found->second->AddElement(element, transform);
            return;
        }
        BIBrickPrimitive* primitive = new BIBrickPrimitive;
        primitive->AddElement(element, transform);
        partPrimitives[part] = primitive;
        m_primitives[element] = primitive;
        RefPointer<BIBrickPrimitive> reference = primitive;
        AddBrick(reference.Get());
    }
}

// Mac 0x260390
BIBrick* BIBrickRepository::AddBrick(const RefPointer<BIBrick>& brick)
{
    if (brick.IsNull())
        return 0;
    m_bricks.PushBack(brick);
    IndexBrick(m_bricks.GetSize() - 1);
    m_states[brick.Get()] = StateFree;
    return brick.Get();
}

size_t BIBrickRepository::HashPrimitives(const BasicSet<BIBrickPrimitive*>& primitives)
{
    size_t hash = (size_t)primitives.GetSize() * (size_t)0x9e3779b9u;
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        size_t value = (size_t)(*it);
        hash ^= value + (size_t)0x9e3779b9u + (hash << 6) + (hash >> 2);
    }
    return hash;
}

void BIBrickRepository::IndexBrick(UInt32 position)
{
    BIBrick* brick = m_bricks[position];
    if (brick->GetType() == BIBrick::TypeSubstep)
        m_substepIndex[HashPrimitives(brick->GetPrimitiveSet())].push_back(position);
}

// Mac 0x2605a0
BIBrick* BIBrickRepository::AddSubstep(const RefPointer<BIBrickSubstep>& substep)
{
    if (substep.IsNull())
        return 0;
    std::unordered_map<size_t, std::vector<UInt32> >::const_iterator found =
        m_substepIndex.find(HashPrimitives(substep->GetPrimitiveSet()));
    if (found != m_substepIndex.end()) {
        for (std::vector<UInt32>::const_iterator it = found->second.begin(); it != found->second.end(); ++it) {
            BIBrick* brick = m_bricks[*it];
            if (m_states[brick] == StateRemoved)
                continue;
            if (substep->IsEqual(brick)) {
                m_states[brick] = StateFree;
                return brick;
            }
        }
    }
    RefPointer<BIBrick> added = substep.Get();
    m_bricks.PushBack(added);
    IndexBrick(m_bricks.GetSize() - 1);
    m_states[substep.Get()] = StateFree;
    return substep.Get();
}

// Mac 0x2607f0 (BIBrickPrimitive, emitted in this TU)
void BIBrickPrimitive::AddElement(PrimitiveElement* element, const Matrix3D& transform)
{
    m_elements.PushBack(element);
    m_elementTransforms[element] = transform;
}

// Mac 0x260b10
bool BIBrickRepository::SetUsed(BIBrick* brick)
{
    m_states[brick] = StateUsed;
    return true;
}

// Mac 0x260b90
bool BIBrickRepository::SetRemoved(BIBrick* brick)
{
    m_states[brick] = StateRemoved;
    return true;
}

// Mac 0x260c10
BIBrick* BIBrickRepository::GetSubstep(const BasicSet<BIBrickPrimitive*>& primitives)
{
    if (primitives.GetSize() < 2)
        return 0;
    // (a primitive is equal to one primitive only: the candidates are the substeps)
    std::unordered_map<size_t, std::vector<UInt32> >::const_iterator found =
        m_substepIndex.find(HashPrimitives(primitives));
    static const std::vector<UInt32> none;
    const std::vector<UInt32>& candidates = found != m_substepIndex.end() ? found->second : none;
    for (std::vector<UInt32>::const_iterator it = candidates.begin(); it != candidates.end(); ++it) {
        if (!m_bricks[*it]->IsEqual(primitives))
            continue;
        BIBrick* brick = m_bricks[*it];
        StateMap::Iterator state = m_states.Find(brick);
        // (a brick without a state counts as not free: the original reads past the tree header)
        if (brick != 0 && state != m_states.End() && state->second == StateFree)
            return brick;
        break;
    }
    RefPointer<BIBrickSubstep> substep = new BIBrickSubstep(primitives, 0, 1.0f);
    return AddSubstep(substep);
}

// Mac 0x260d40
BIBrickPrimitive* BIBrickRepository::GetPrimitive(const PrimitiveElement* element, bool freeOnly)
{
    BasicMap<const PrimitiveElement*, BIBrickPrimitive*>::Iterator it = m_primitives.Find(element);
    if (it == m_primitives.End())
        return 0;
    BIBrickPrimitive* primitive = it->second;
    if (!freeOnly)
        return primitive;
    StateMap::Iterator state = m_states.Find(primitive);
    if (state != m_states.End() && state->second == StateFree)
        return primitive;
    return 0;
}

// Mac 0x260df0
void BIBrickRepository::GetFreeBricks(BasicSet<BIBrick*>& bricks)
{
    for (BasicArray<RefPointer<BIBrick> >::Iterator it = m_bricks.Begin(); it != m_bricks.End(); ++it) {
        BIBrick* brick = *it;
        StateMap::Iterator state = m_states.Find(brick);
        if (state != m_states.End() && state->second == StateFree)
            bricks.Insert(brick);
    }
}

// Mac 0x260eb0
void BIBrickRepository::GetFreePrimitives(BasicSet<BIBrickPrimitive*>& primitives)
{
    for (BasicArray<RefPointer<BIBrick> >::Iterator it = m_bricks.Begin(); it != m_bricks.End(); ++it) {
        BIBrick* brick = *it;
        StateMap::Iterator state = m_states.Find(brick);
        if (state != m_states.End() && state->second == StateFree && brick->GetType() == BIBrick::TypePrimitive)
            primitives.Insert(static_cast<BIBrickPrimitive*>(brick));
    }
}

// Mac 0x260f80
void BIBrickRepository::GetFreeSubsteps(BasicArray<BIBrickSubstep*>& substeps)
{
    for (BasicArray<RefPointer<BIBrick> >::Iterator it = m_bricks.Begin(); it != m_bricks.End(); ++it) {
        BIBrick* brick = *it;
        StateMap::Iterator state = m_states.Find(brick);
        if (state != m_states.End() && state->second == StateFree && brick->GetType() == BIBrick::TypeSubstep)
            substeps.PushBack(static_cast<BIBrickSubstep*>(brick));
    }
}

// Mac 0x261250
void BIBrickRepository::GetFreeBricksWithin(const BasicSet<BIBrickPrimitive*>& primitives, BasicSet<BIBrick*>& bricks)
{
    for (BasicArray<RefPointer<BIBrick> >::Iterator it = m_bricks.Begin(); it != m_bricks.End(); ++it) {
        BIBrick* brick = *it;
        StateMap::Iterator state = m_states.Find(brick);
        if (state != m_states.End() && state->second == StateFree && brick->IsContainedIn(primitives) &&
            !brick->IsEqual(primitives))
            bricks.Insert(brick);
    }
}

} // namespace LEGO
