// LEGO Digital Designer — restored source
// LEGO::Behavior and the behavior helpers of LEGO::BehaviorElement.
//
// Original object file: Scene.cpp (Mac TU 025); this is sub-TU 025a,
// Mac 0x920c0-0x93260, Win 0x470d00-0x4713a0. The Mac range also holds:
//   0x920c0  HitBoundingBox (ray / box helper of BrickConnectSceneControl::PickElement)
//   0x92550 0x926f0 0x92c90 0x92650 0x93120 0x93170 0x931f0  template instances (BasicArray
//            PushBack / destructors, BehaviorElement VTT constructor / destructor)
//   0x92ed0 0x93010  Scene::AddBehavior / Scene::RemoveBehavior (+0x1a0 list; in Scene.cpp)
#include "Behavior.h"
#include "BehaviorElement.h"

namespace LEGO {

// Mac 0x920c0. Quadrants: 0 right (above max), 1 left (below min), 2 middle.
bool HitBoundingBox(const BoundingBox& box, const Vector3& origin, const Vector3& direction, Vector3& point)
{
    const Real32* o = &origin.x;
    const Real32* d = &direction.x;
    const Real32* lo = &box.m_min.x;
    const Real32* hi = &box.m_max.x;
    Real32* p = &point.x;
    bool inside = true;
    char quadrant[3];
    double candidate[3];
    for (int i = 0; i < 3; ++i) {
        if (o[i] < lo[i]) {
            quadrant[i] = 1;
            candidate[i] = lo[i];
            inside = false;
        } else if (o[i] > hi[i]) {
            quadrant[i] = 0;
            candidate[i] = hi[i];
            inside = false;
        } else {
            quadrant[i] = 2;
            candidate[i] = 0.0;
        }
    }
    if (inside) {
        point = origin;
        return true;
    }
    double maxT[3];
    for (int i = 0; i < 3; ++i)
        maxT[i] = (quadrant[i] != 2 && d[i] != 0.0f) ? (candidate[i] - (double)o[i]) / (double)d[i] : -1.0;
    int plane = maxT[0] < maxT[1] ? 1 : 0;
    if (maxT[plane] < maxT[2])
        plane = 2;
    if (maxT[plane] < 0.0)
        return false;
    for (int i = 0; i < 3; ++i) {
        if (i == plane) {
            p[i] = (Real32)candidate[i];
        } else {
            p[i] = (Real32)((double)d[i] * maxT[plane] + (double)o[i]);
            if (p[i] > hi[i] || p[i] < lo[i])
                return false;
        }
    }
    return true;
}

// Mac 0x92400, Win 0x470d30
void Behavior::AppendTo(BasicArray<StorageScene2::Behavior>& behaviors) const
{
    StorageScene2::Behavior behavior;
    behavior.m_name = m_name;
    behaviors.PushBack(behavior);

    BasicArray<StorageScene2::Attribute>& attributes = behaviors.Back().m_attributes;
    for (AttributeMap::ConstIterator it = m_attributes.Begin(); it != m_attributes.End(); ++it) {
        StorageScene2::Attribute attribute;
        attribute.m_key = it->first;
        attribute.m_value = it->second;
        attributes.PushBack(attribute);
    }
}

// Mac 0x929a0, Win 0x4710d0
void BehaviorElement::GetBehaviors(BasicArray<StorageScene2::Behavior>& behaviors) const
{
    for (BasicArray<RefPointer<Behavior> >::ConstIterator it = m_behaviors.Begin();
         it != m_behaviors.End(); ++it)
        (*it)->AppendTo(behaviors);
}

// Mac 0x929e0, Win 0x471110
void BehaviorElement::AddBehaviors(const BasicArray<StorageScene2::Behavior>& behaviors)
{
    for (BasicArray<StorageScene2::Behavior>::ConstIterator it = behaviors.Begin();
         it != behaviors.End(); ++it) {
        Behavior* behavior = new Behavior(it->m_name);
        // NOTE: both builds iterate over a copy of the attribute array.
        BasicArray<StorageScene2::Attribute> attributes(it->m_attributes);
        for (BasicArray<StorageScene2::Attribute>::ConstIterator a = attributes.Begin();
             a != attributes.End(); ++a)
            behavior->m_attributes[a->m_key] = a->m_value;
        RefPointer<Behavior> reference(behavior);
        m_behaviors.PushBack(reference);
    }
}

// Mac 0x92dd0, Win 0x471270
void BehaviorElement::CopyBehaviorsTo(BehaviorElement& target) const
{
    BasicArray<StorageScene2::Behavior> behaviors;
    GetBehaviors(behaviors);
    target.AddBehaviors(behaviors);
}

} // namespace LEGO
