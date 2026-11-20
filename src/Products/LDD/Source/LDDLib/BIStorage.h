// LEGO Digital Designer — restored source
// LEGO::BIStorage — the stored steps of a building guide: a name and a tree of events (steps
//   with nested steps, cameras and parts; a part is a list of scene primitive elements).
//   Converts from / to the <BuildingInstruction> of a StorageScene2 (Load / Save) and from /
//   to the BIEvent lists of the generators (GetEvents / SetEvents) through the global
//   BIBrickRepository.
//
// Home TU 358 "BIStorage.cpp" (Mac 0x33f030-0x34274f; modmap guess StorageScene2_BICamera.cpp).
// Mac layout (sizeof 0x28): Object, +0x08 m_name, +0x18 m_events. Mac vtable 0x777e90
// {D1 0x3418a0, D0 0x3418b0 (body 0x342660)}, Win vtable 0xa2b9b4 {sdd 0x55aa60, dtor 0x55a920}.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "StorageScene2.h"
#include "Scene.h"

namespace LEGO {

class PrimitiveElement;
class BIEvent;
class BIBrickPrimitive;
class BIBrickSubstep;

class BIStorage : public Object {
public:
    // Base of the stored events (RTTI BIStorage::Event; no vtable of its own on Mac).
    struct Event : public Object {
        enum Type {
            TypeStep   = 0,
            TypeCamera = 1,
            TypePart   = 2
        };
        explicit Event(UInt32 type) : m_type(type) {}
        UInt32 m_type;   // +0x08
    };

    // sizeof 0x2c. Mac vtable 0x777ebc {D1 0x3419c0, D0 0x341a60}, Win vtable 0xa2b9e4
    // {sdd 0x55a9d0, dtor 0x55a670}. Constructor Mac 0x3418f0.
    struct Step : public Event {
        Step() : Event(TypeStep), m_exploded(false) {}
        virtual ~Step() {}
        BasicArray<RefPointer<Event> > m_events;     // +0x0c
        bool                           m_exploded;   // +0x1c
        Vector3                        m_explode;    // +0x20
    };

    // sizeof 0x10. Mac vtable 0x777ef4 {D1 0x341b10, D0 0x341b70}, Win vtable 0xa2b9f8
    // {sdd 0x55aa00, dtor 0x55a750}. Constructor inlined.
    struct Camera : public Event {
        explicit Camera(StorageScene2::Camera* camera) : Event(TypeCamera), m_camera(camera) {}
        virtual ~Camera() {}
        RefPointer<StorageScene2::Camera> m_camera;   // +0x0c
    };

    // sizeof 0x60. Mac vtable 0x777f1c {D1 0x341cc0, D0 0x341d60}, Win vtable 0xa2ba0c
    // {sdd 0x55aa30, dtor 0x55a8b0}. Constructor Mac 0x341be0, Win 0x55a7c0.
    struct Part : public Event {
        Part() : Event(TypePart), m_exploded(false) {}
        virtual ~Part() {}
        // Appends `element` unless it is already there. Mac 0x33f8d0 (name inferred)
        void AddElement(PrimitiveElement* element);

        BasicArray<RefPointer<PrimitiveElement> > m_elements;        // +0x0c
        bool                                      m_exploded;        // +0x1c (never set)
        Matrix3D                                  m_transformation;  // +0x20 (never set)
    };

    typedef BasicArray<RefPointer<Event> > EventArray;
    // RTTI BasicMap<BIBrickSubstep*, BasicArray<RefPointer<BIEvent>>, Less<…>> (Mac vtable
    // 0x777fa8): the events of every substep; key 0 = the top level.
    typedef BasicMap<BIBrickSubstep*, BasicArray<RefPointer<BIEvent> > > SubstepMap;

    // Empty steps (new 0x28). Mac 0x33f030
    BIStorage();
    virtual ~BIStorage();                                                  // Mac 0x3418a0, Win 0x55a920

    // The steps of `instructions` with the scene objects of a Scene::Load map: the global
    // BIBrickRepository is rebuilt from `scene`; fails (and clears) if a part refers to an
    // element without a free primitive or the steps do not convert into BIEvents.
    // Mac 0x33f110
    bool Load(const StorageScene2::BI& instructions, Scene::StorageMap& map, Scene* scene);
    // Appends a <BuildingInstruction> with this storage's steps to `storage` (steps named
    // "<name>Step<n>", substeps "<prefix><n>Substep<m>"); false (nothing appended) without
    // elements in `map` or when a part element is not in `map`.
    // NOTE: BrickConnectSceneControl ignores the result.
    // Mac 0x33fa30, Win 0x55b040
    bool Save(StorageScene2& storage, Scene::SaveMap& map);
    // The steps as BIEvents (map[0] the top level, map[substep] each substep). Mac 0x33f5c0
    bool GetEvents(SubstepMap& substeps);
    // Replaces the steps by the BIEvents (inverse of GetEvents). Mac 0x340550
    bool SetEvents(const BasicArray<RefPointer<BIEvent> >& events, const SubstepMap& substeps);
    // Mac 0x33f750
    void Clear();

    const EventArray& GetSteps() const { return m_events; }               // Mac 0x341860 (out of line)
    const String& GetName() const { return m_name; }                       // Mac 0x341870
    void SetName(const String& name) { m_name = name; }                    // Mac 0x341880
    bool HasSteps() const { return !m_events.IsEmpty(); }

    String     m_name;     // +0x08
    EventArray m_events;   // +0x18

private:
    // Storage events -> events. Mac 0x33f250, Win 0x55ac20 (inlined parts)
    bool LoadEvents(const BasicArray<RefPointer<StorageScene2::BIEvent> >& source, Scene::StorageMap& map,
                    Scene* scene, EventArray& events);
    // Events -> storage events. Mac 0x33fcc0, Win 0x55b180
    bool SaveEvents(const EventArray& events, BasicArray<RefPointer<StorageScene2::BIEvent> >& target,
                    const String& prefix, Scene::SaveMap& map, StorageScene2& storage);
    // Events -> BIEvents. Mac 0x340b40 (top level), 0x340f90 (a step of parts), 0x341180
    // (primitives of a step tree), 0x341220 (a step with parts and steps)
    bool ConvertEvents(BasicArray<RefPointer<BIEvent> >& target, SubstepMap& substeps, const EventArray& events);
    bool ConvertParts(BasicArray<RefPointer<BIEvent> >& target, const EventArray& events);
    void CollectPrimitives(const EventArray& events, BasicSet<BIBrickPrimitive*>& primitives);
    bool ConvertMixed(BasicArray<RefPointer<BIEvent> >& target, SubstepMap& substeps, const EventArray& events);
    // BIEvents -> events. Mac 0x3405c0, 0x3409a0 (the parts of a primitive)
    bool FromEvents(const BasicArray<RefPointer<BIEvent> >& source, const SubstepMap& substeps, EventArray& events);
    void AddPrimitiveParts(const BIBrickPrimitive* primitive, EventArray& events);
};

} // namespace LEGO
