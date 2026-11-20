// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsScene and the helper classes of its object file (Mac TU 435).
// See VortexPhysicsScene.h. The VortexConnectionAttributeDatas functions of this object are in
// VortexConnectionAttributeDatas.cpp.
// Not code: Mac 0x3b4230 = <iostream> + the two ProfileTimers below.
#include "VortexPhysicsScene.h"
#include "VortexBrickPhysics.h"
#include "MathUtil.h"
#include "VortexPhysicsRigidElement.h"
#include "VortexPhysicsSceneryElement.h"
#include "VortexPhysicsPrimitiveElement.h"
#include "VortexPhysicsPlacedGeometry.h"
#include "VortexPhysicsGeometry.h"
#include "VortexPhysicsFreeConnection.h"
#include "VortexMoveTogetherSolver.h"
#include "VortexAnchorSolver.h"
#include "VortexBrickPhysicsSimulationHingeSolver.h"
#include "VortexConversions.h"
#include "BaseBrickPhysicsPrimitiveElement.h"
#include "SimplePhysicsSceneDebugInfo.h"
#include "ProfileTimer.h"
#include "Color.h"

#include "Vx/VxUniverse.h"
#include "Vx/VxFrame.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxPart.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxCollisionPairRequest.h"
#include "Vx/VxGeometry.h"
#include "Vx/VxConstraint.h"
#include "Vx/VxConstraintFriction.h"
#include "Vx/VxHinge.h"
#include "Vx/VxSpring.h"
#include "Vx/VxLinear2.h"
#include "Vx/VxRPRO.h"
#include "Vx/VxMessage.h"
#include "Vinc/VincModel.h"
#include "Vinc/VincSpace.h"
#include <vector>

#include <float.h>
#include <math.h>

namespace LEGO {

namespace {

// The response model / collision type of the parts' models (Mac 0x644328 = 0, read through the
// non-lazy pointer 0x702500; Vx/VxPart.cpp kDefaultModelM80). (name inferred)
const int kPartCollisionType = 0;
// The coordinate of a VxHinge (Mac 0x64407c = 0 through 0x702504). (name inferred)
const int kHingeCoordinate = 0;
// The relative-transform identity tolerance of the collision queries (Mac 0x63fcc8 1e-14).
const Vx::VxReal kIdentityTolerance = 1e-14;
// The penetration the collectors tolerate (the doubles 0x3f8cac0840000000 / 0x3f8c779a80000000).
const double kInsertTolerance = (double)0.014f;
const double kElementTolerance = (double)0.0139f;

// The stiffness tables of the spring mirrors (rows: m_strength). Hinge (Mac 0x63fd30, 3 floats a
// row): maximum / minimum (and Adapt's near value) / Adapt's far value; ball (Mac 0x63fd48, 4
// floats a row): maximum, minimum, Adapt's aligned value, Adapt's other value. (names inferred)
const Real32 kHingeSpringStiffness[2][3] = {{8000.0f, 2000.0f, 160000.0f}, {1000000.0f, 100000.0f, 1000000.0f}};
const Real32 kBallSpringStiffness[2][4] = {{500.0f, 1000.0f, 100000.0f, 1000.0f}, {1000.0f, 1000.0f, 1000.0f, 1000.0f}};

// The world transform of a VxPart as a LEGO matrix (the float conversion inlined everywhere).
inline void ToMatrix3D(const Vx::VxTransform& tm, Matrix3D& matrix)
{
    for (int i = 0; i < 16; ++i)
        matrix.m[i] = (Real32)tm.m[i / 4][i % 4];
}

// A dynamic VxPart of mass 1 with the user mass flags (the scene's temporary bodies).
inline Vx::VxPart* NewTestPart()
{
    Vx::VxPart* part = new Vx::VxPart();
    part->setControl(Vx::VxEntity::kControlDynamic);
    part->setMass(1.0);
    part->mUserInertia = true;
    part->mUserMass = true;
    part->mUserCOM = true;
    return part;
}

// Sets the group / contact limits of the collision pair requests for a query and restores them
// (TestCollisions). Mac 0x3b31f0 / 0x3b3410 (not inlined: a local class of the original; name
// inferred)
class VortexPairRequestScope {
public:
    // Every pair of the types 0-5 and the frame's default request to one group / contact; the
    // pairs with type 5 (composites) to `groups` / `contacts`. Mac 0x3b31f0
    VortexPairRequestScope(Vx::VxUniverse* universe, int groups, int contacts) : m_universe(universe)
    {
        int* saved = m_saved;
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < i; ++j) {
                Vx::VxCollisionPairRequest* request = m_universe->getCollisionPairRequest(i, j);
                *saved++ = request->getGroupMaxCount();
                *saved++ = request->getContactMaxCount();
                request->setGroupMaxCount(1);
                request->setContactMaxCount(1);
            }
        Vx::VxCollisionPairRequest* request = Vx::VxFrame::currentInstance()->getDefaultCollisionPairRequest();
        m_defaultGroups = request->getGroupMaxCount();
        m_defaultContacts = request->getContactMaxCount();
        request->setGroupMaxCount(1);
        request->setContactMaxCount(1);
        for (int i = 0; i < 6; ++i) {
            request = m_universe->getCollisionPairRequest(i, 5);
            request->setGroupMaxCount(groups);
            request->setContactMaxCount(contacts);
        }
    }
    // NOTE: the pairs (i, 5) set above are not restored, only (5, j) for j < 5 (the original's).
    // Mac 0x3b3410
    ~VortexPairRequestScope()
    {
        const int* saved = m_saved;
        for (int i = 1; i < 6; ++i)
            for (int j = 0; j < i; ++j) {
                Vx::VxCollisionPairRequest* request = m_universe->getCollisionPairRequest(i, j);
                request->setGroupMaxCount(saved[0]);
                request->setContactMaxCount(saved[1]);
                saved += 2;
            }
        Vx::VxCollisionPairRequest* request = Vx::VxFrame::currentInstance()->getDefaultCollisionPairRequest();
        request->setGroupMaxCount(m_defaultGroups);
        request->setContactMaxCount(m_defaultContacts);
    }

private:
    int             m_saved[0x24];       // +0x00 (pairs (1,0) .. (5,4): 30 used)
    int             m_defaultGroups;     // +0x90
    int             m_defaultContacts;   // +0x94
    Vx::VxUniverse* m_universe;          // +0x98
};

// The universe's sleeping pairs take part in the intersection queries while it exists.
// Mac 0x3ae830 / 0x3ae850 (Win 0x524ac0; name inferred)
class SleepingPairsIntersectScope {
public:
    explicit SleepingPairsIntersectScope(Vx::VxUniverse* universe) : m_universe(universe)
    {
        m_universe->setSleepingPairsIntersect(true);
    }
    ~SleepingPairsIntersectScope() { m_universe->setSleepingPairsIntersect(false); }

private:
    Vx::VxUniverse* m_universe;
};

} // namespace

// The section timers (traced at exit). Mac 0x77f510 / 0x77f540
static ProfileTimer s_testInsertTimer(String("VortexPhysicsScene::TestInsertPrimitiveElements"), true);
static ProfileTimer s_hitTestTimer(String("VortexPhysicsScene::HitTest"), true);

// =============================================================================================
// VortexPhysicsScene
// =============================================================================================

// Mac 0x3aac30 (C1 0x3aaf80)
VortexPhysicsScene::VortexPhysicsScene(BaseBrickPhysics* physics, Vx::VxUniverse* universe)
    : BaseBrickPhysicsScene(physics), m_doCollide(true), m_universe(universe), m_mechanism(0),
      m_testBodyData(VortexRigidBodyUserData::KindTest), m_assemblyA(0),
      m_assemblyAShape(VortexShapeUserData::KindAssemblyA, &m_assemblyAFilter),
      m_assemblyABodyData(VortexRigidBodyUserData::KindAssemblyA), m_assemblyB(0),
      m_assemblyBShape(VortexShapeUserData::KindAssemblyB, &m_assemblyBFilter),
      m_assemblyBBodyData(VortexRigidBodyUserData::KindAssemblyB), m_dragPart(0), m_castGeometry(0)
{
    m_castGeometry = new Vx::VxCollisionGeometry(new Vx::VxSphere(0.03));
    m_mechanism = new Vx::VxMechanism();
    m_universe->addAssembly(m_mechanism);
}

// Mac 0x3af460 (D1 0x3af560, D0 0x3af570), Win 0x525190 (sdd 0x522240)
VortexPhysicsScene::~VortexPhysicsScene()
{
    if (m_assemblyA != 0)
        m_universe->removeAssembly(m_assemblyA);
    if (m_assemblyB != 0)
        m_universe->removeAssembly(m_assemblyB);
    delete m_castGeometry;
    Vx::VxFrame::instance()->removeUniverse(m_universe);
    delete m_universe;
}

// Mac 0x3b2cf0, Win 0x52f4f0
void* VortexPhysicsScene::GetNativeScene()
{
    return m_universe;
}

// Mac 0x3aaf90, Win 0x522260
PhysicsSceneDebugInfo* VortexPhysicsScene::GetDebugInfo()
{
    SimplePhysicsSceneDebugInfo* info = new SimplePhysicsSceneDebugInfo();
    // the attachment frames of the constraints (primary axis x2, secondary x0.8)
    for (int i = 0; i < m_mechanism->getConstraintCount(); ++i) {
        Vx::VxConstraint* constraint = m_mechanism->getConstraint(i);
        Vx::VxPart* part = constraint->mParts[0];
        Vx::VxPart* otherPart = constraint->mParts[1];
        Vx::VxVector3 primary[2] = {Vx::VxVector3(0.0, 0.0, 0.0), Vx::VxVector3(0.0, 0.0, 0.0)};
        Vx::VxVector3 secondary[2] = {Vx::VxVector3(0.0, 0.0, 0.0), Vx::VxVector3(0.0, 0.0, 0.0)};
        Vx::VxVector3 position[2] = {Vx::VxVector3(0.0, 0.0, 0.0), Vx::VxVector3(0.0, 0.0, 0.0)};
        PhysicsSceneDebugInfo::Line lines[2];
        if ((part != 0 && part->mInternalControl == Vx::VxEntity::kControlDynamic) ||
            (otherPart != 0 && otherPart->mInternalControl == Vx::VxEntity::kControlDynamic)) {
            lines[0].m_color = Color::Magenta();
            lines[1].m_color = Color::Red();
        } else {
            lines[0].m_color = Color::Black();
            lines[1].m_color = Color::Black();
        }
        if (!constraint->mEnabled) {
            lines[0].m_color.a *= 0.5f;
            lines[1].m_color.a *= 0.5f;
        }
        for (int j = 0; j < 2; ++j) {
            PhysicsSceneDebugInfo::Line& line = lines[j];
            constraint->getPartAttachmentAxes(j, primary[j], secondary[j]);
            constraint->getPartAttachmentPosition(j, position[j]);
            const Vx::VxVector3& p = position[j];
            line.m_from = Vector3((Real32)p[0], (Real32)p[1], (Real32)p[2]);
            line.m_to = Vector3((Real32)(primary[j][0] + primary[j][0] + p[0]),
                                (Real32)(primary[j][1] + primary[j][1] + p[1]),
                                (Real32)(primary[j][2] + primary[j][2] + p[2]));
            info->m_lines.PushBack(line);
            line.m_to = Vector3((Real32)((secondary[j][0] + secondary[j][0]) * 0.8 + p[0]),
                                (Real32)((secondary[j][1] + secondary[j][1]) * 0.8 + p[1]),
                                (Real32)((secondary[j][2] + secondary[j][2]) * 0.8 + p[2]));
            info->m_lines.PushBack(line);
        }
    }
    // the collision boxes of the parts (dynamic green, frozen black, others yellow; half alpha)
    for (int i = 0; i < m_mechanism->getPartCount(); ++i) {
        Vx::VxPart* part = m_mechanism->getPart(i);
        for (int j = 0; j < part->getCollisionGeometryCount(); ++j) {
            const MeReal* box = part->getCollisionGeometry(j)->mModel->aabb;
            Real32 x0 = (Real32)box[0], x1 = (Real32)box[1];
            Real32 y0 = (Real32)box[2], y1 = (Real32)box[3];
            Real32 z0 = (Real32)box[4], z1 = (Real32)box[5];
            PhysicsSceneDebugInfo::Line line;
            if (part->mInternalControl == Vx::VxEntity::kControlDynamic)
                line.m_color = Color::Green();
            else if (part->mInternalControl == Vx::VxEntity::kControlFrozen)
                line.m_color = Color::Black();
            else
                line.m_color = Color::Yellow();
            line.m_color.a *= 0.5f;
            line.m_from = Vector3(x0, y0, z0);
            line.m_to = Vector3(x1, y0, z0);
            info->m_lines.PushBack(line);
            line.m_to = Vector3(x0, y1, z0);
            info->m_lines.PushBack(line);
            line.m_to = Vector3(x0, y0, z1);
            info->m_lines.PushBack(line);
            line.m_from = Vector3(x1, y1, z1);
            line.m_to = Vector3(x0, y1, z1);
            info->m_lines.PushBack(line);
            line.m_to = Vector3(x1, y0, z1);
            info->m_lines.PushBack(line);
            line.m_to = Vector3(x1, y1, z0);
            info->m_lines.PushBack(line);
        }
    }
    // the forces / torques of the constraints on their first dynamic part
    for (int i = 0; i < m_mechanism->getConstraintCount(); ++i) {
        Vx::VxConstraint* constraint = m_mechanism->getConstraint(i);
        Vx::VxVector3 primary(0.0, 0.0, 0.0);
        Vx::VxVector3 secondary(0.0, 0.0, 0.0);
        Vx::VxVector3 force(0.0, 0.0, 0.0);
        Vx::VxVector3 torque(0.0, 0.0, 0.0);
        Vx::VxVector3 position(0.0, 0.0, 0.0);
        constraint->getPartAttachmentAxes(0, primary, secondary);
        int index = constraint->mParts[0]->mInternalControl != Vx::VxEntity::kControlDynamic ? 1 : 0;
        constraint->getPartAttachmentPosition(index, position);
        constraint->getPartForce(index, force);
        constraint->getPartTorque(index, torque);
        double forceLength = sqrt(force[2] * force[2] + (force[1] * force[1] + force[0] * force[0])) * (double)0.001f;
        double torqueLength =
            sqrt(torque[2] * torque[2] + (torque[1] * torque[1] + torque[0] * torque[0])) * (double)0.001f;
        double forceScale = forceLength < 1000.0 ? forceLength : 1000.0;
        double torqueScale = torqueLength < 1000.0 ? torqueLength : 1000.0;
        PhysicsSceneDebugInfo::Line line;
        line.m_color = Color::DarkGray();
        line.m_from = Vector3((Real32)position[0], (Real32)position[1], (Real32)position[2]);
        line.m_to = Vector3((Real32)(secondary[0] * forceScale + position[0]),
                            (Real32)(secondary[1] * forceScale + position[1]),
                            (Real32)(secondary[2] * forceScale + position[2]));
        info->m_lines.PushBack(line);
        line.m_color = Color::LightGray();
        line.m_to = Vector3((Real32)(primary[0] * torqueScale + position[0]),
                            (Real32)(primary[1] * torqueScale + position[1]),
                            (Real32)(primary[2] * torqueScale + position[2]));
        info->m_lines.PushBack(line);
    }
    return info;
}

// Mac 0x3abd30, Win 0x522e10
void VortexPhysicsScene::Unknown5(bool on)
{
    Vx::VxArray<VortexPhysicsGeometry*>& geometries = static_cast<VortexBrickPhysics*>(m_physics)->m_geometries;
    if (!on) {
        for (VortexPhysicsGeometry** it = geometries.begin(); it != geometries.end(); ++it)
            (*it)->ResetSize();
    } else {
        for (VortexPhysicsGeometry** it = geometries.begin(); it != geometries.end(); ++it)
            (*it)->Inflate(-0.007f);
    }
}

// Mac 0x3af3d0, Win 0x525120
void VortexPhysicsScene::Unknown6()
{
    BaseBrickPhysicsScene::Unknown6();
}

// Mac 0x3af3e0, Win 0x525130
void VortexPhysicsScene::Unknown7()
{
    BaseBrickPhysicsScene::Unknown7();
    Unknown47();
    static_cast<VortexBrickPhysics*>(m_physics)->UpdateUniverse(m_universe);
}

// Mac 0x3abd90, Win 0x522fd0
bool VortexPhysicsScene::TestCollisions(UInt32 count, PhysicsPrimitiveElement* const* elements,
                                        const Matrix3D* const* transforms, PhysicsCollisionCallback* callback,
                                        PhysicsProgressCallback* progress)
{
    if (count == 0)
        return true;
    StopSolver(false);
    s_testInsertTimer.Start();
    Vx::VxInfo(0, "VortexPhysicsScene::TestInsertPrimitiveElements start, cg count = %d",
               m_universe->getCollisionGeometryCount());
    LEGOUniverseCollector collector(callback, kInsertTolerance);
    const int type = kPartCollisionType;
    m_universe->addIntersectSubscriber(type, type, 2, &collector, 0);
    m_universe->getInternalIDIntersectSubscriber();
    // the accepted elements (static, frozen) and the test body (dynamic)
    Vx::VxPart* accepted = new Vx::VxPart();
    accepted->setControl(Vx::VxEntity::kControlStatic);
    accepted->freeze(true);
    accepted->mUserInertia = true;
    accepted->mUserMass = true;
    accepted->mUserCOM = true;
    m_mechanism->addPart(accepted);
    // The accepted part gains a composite per 16 bricks, each disabling its pairs with all earlier
    // ones: quadratic in the bricks. As one broad-phase pair group (setPairGroup) the same pairs
    // are disabled without a step per pair.
    accepted->setPairGroup(true);
    Vx::VxPart* test = NewTestPart();
    test->setUserData(&m_testBodyData);
    m_mechanism->addPart(test);
    {
        VortexPairRequestScope requests(m_universe, 1, 0x900);
        // The test box moves from brick to brick through the search trees instead of the walk past
        // every box between (the same pairs; their order within a list does not matter here).
        struct Relocation {
            VincSpaceID space;
            explicit Relocation(VincSpaceID s) : space(s) { VincSpaceSetRelocateCrossings(space, 64); }
            ~Relocation() { VincSpaceSetRelocateCrossings(space, 0x7fffffff); }
        } relocation((VincSpaceID)(void*)m_universe->mSpace);
        Vx::VxFrame* frame = Vx::VxFrame::currentInstance();
        Vx::VxCompositeCollisionGeometry* composite = 0;
        for (UInt32 i = 0; i < count; ++i) {
            Vx::VxCollisionGeometry* geometry =
                static_cast<VortexPhysicsPrimitiveElement*>(elements[i])->CreateCollisionGeometry(true);
            if (geometry == 0)
                continue;
            Vx::VxTransform tm;
            ToVxTransform(*transforms[i], tm);
            if (!tm.isIdentity(kIdentityTolerance)) {
                const Vx::VxTransform* relative = geometry->getTransformRelativePtr();
                if (relative != 0)
                    tm.preMultiply(*relative);
                geometry->setTransformRelative(&tm);
            }
            test->addCollisionGeometry(geometry);
            const Vx::VxTransform* world = geometry->getTransformPtr();
            Vx::VxInfo(0, "VortexPhysicsScene::TestInsertPrimitiveElements collision with body at %.3f %.3f %.3f",
                       world->m[3][0], world->m[3][1], world->m[3][2]);
            frame->stepCollision();
            if (collector.m_collides) {
                Vx::VxInfo(0,
                           "IntersectionFound in VortexPhysicsScene::TestInsertPrimitiveElements found collision, "
                           "collector collides = %d",
                           1);
                break;
            }
            if (progress != 0 && !progress->OnProgress(i, count)) {
                Vx::VxInfo(0, "IntersectionFound in VortexPhysicsScene::TestInsertPrimitiveElements: stopped by "
                              "Physics progress call back.");
                break;
            }
            test->removeCollisionGeometry(geometry);
            if (count < 0x21) {
                accepted->addCollisionGeometry(geometry);
            } else {
                if (composite == 0 || composite->getCollisionGeometryCount() >= 16) {
                    composite = new Vx::VxCompositeCollisionGeometry(16);
                    VortexCollisionGeometryData::Create(composite);
                    accepted->addCollisionGeometry(composite);
                }
                composite->addCollisionGeometry(geometry);
                composite->update();
            }
        }
        s_testInsertTimer.Stop();
        Vx::VxInfo(0, "VortexPhysicsScene::TestInsertPrimitiveElements end, cg count = %d",
                   m_universe->getCollisionGeometryCount());
        m_mechanism->removePart(test);
        delete test;
        m_mechanism->removePart(accepted);
        accepted->setPairGroup(false);
        VortexCollisionGeometryData::DestroyAll(accepted);
        delete accepted;
        m_universe->removeIntersectSubscriber(type, type, 2, &collector);
    }
    return !collector.m_collides;
}

// Mac 0x3ac3a0, Win 0x523490
void VortexPhysicsScene::CollectGeometries(BaseBrickPhysicsScene* scene, Vx::VxPart* part)
{
    for (BaseBrickPhysicsRigidElement** body = scene->m_rigidElements.Begin(); body != scene->m_rigidElements.End();
         ++body) {
        Vx::VxCompositeCollisionGeometry* composite = 0;
        for (BaseBrickPhysicsPrimitiveElement** it = (*body)->m_primitives.Begin(); it != (*body)->m_primitives.End();
             ++it) {
            Vx::VxCollisionGeometry* geometry =
                static_cast<VortexPhysicsPrimitiveElement*>(*it)->CreateCollisionGeometry(false);
            if (geometry == 0)
                continue;
            Vx::VxTransform tm;
            ToVxTransform((*body)->m_transform, tm);
            if (!tm.isIdentity(kIdentityTolerance)) {
                const Vx::VxTransform* relative = geometry->getTransformRelativePtr();
                if (relative != 0)
                    tm.preMultiply(*relative);
                geometry->setTransformRelative(&tm);
            }
            if (composite == 0 || composite->getCollisionGeometryCount() >= 16) {
                composite = new Vx::VxCompositeCollisionGeometry(16);
                VortexCollisionGeometryData::Create(composite);
                part->addCollisionGeometry(composite);
            }
            composite->addCollisionGeometry(geometry);
        }
    }
}

// Mac 0x3ac5d0, Win 0x523620
bool VortexPhysicsScene::TestCollision(PhysicsScene* other, const Matrix3D& transform,
                                       PhysicsCollisionCallback* callback)
{
    StopSolver(false);
    Vx::VxPart* part = new Vx::VxPart();
    part->setControl(Vx::VxEntity::kControlDynamic);
    part->setMass(1.0);
    part->mUserInertia = true;
    part->mUserMass = true;
    part->mUserCOM = true;
    CollectGeometries(static_cast<BaseBrickPhysicsScene*>(other), part);
    if (part->getCollisionGeometryCount() == 0) {
        // NOTE: nothing to test counts as a collision (the original's)
        delete part;
        return false;
    }
    LEGOIndividualCollector collector(callback, kInsertTolerance);
    Vx::VxTransform tm;
    ToVxTransform(transform, tm);
    part->setTransform(tm);
    m_mechanism->addPart(part);
    part->setUserData(&m_testBodyData);
    part->updateCollisionGeometryTransforms();
    part->updateCollisionGeometries();
    if (Vx::VxIntersect::intersect(part, &collector, true))
        Vx::VxInfo(0, "IntersectionFound in VortexPhysicsScene::TestInsertScene found collision, collector collides = %d",
                   (int)collector.m_collides);
    m_mechanism->removePart(part);
    VortexCollisionGeometryData::DestroyAll(part);
    delete part;
    return !collector.m_collides;
}

// Mac 0x3ac850, Win 0x523800
bool VortexPhysicsScene::SetDragScene(PhysicsScene* dragScene)
{
    Vx::VxInfo(0, "VortexPhysicsScene::BeginHitTest, space contains %d cg", m_universe->getCollisionGeometryCount());
    StopSolver(false);
    m_dragPart = new Vx::VxPart();
    m_dragPart->setControl(Vx::VxEntity::kControlDynamic);
    m_dragPart->setMass(1.0);
    m_dragPart->mUserInertia = true;
    m_dragPart->mUserCOM = true;
    m_dragPart->mUserMass = true;
    CollectGeometries(static_cast<BaseBrickPhysicsScene*>(dragScene), m_dragPart);
    int count = m_dragPart->getCollisionGeometryCount();
    if (count != 0) {
        m_mechanism->addPart(m_dragPart);
        static_cast<VortexBrickPhysics*>(m_physics)->UpdateUniverse(m_universe);
    }
    return count != 0;
}

// LDD64: a carried group of at least kLargeDragPart boxes jumps through the broad phase
// (CxSmallSort::takeOut) instead of walking past every box between; the same boxes overlap after.
// Drag test time step 0 only (bounds without a sweep). False: nothing done.
bool VortexPhysicsScene::MoveLargeDragPart(const Vx::VxTransform& tm)
{
    const int kLargeDragPart = 64;
    const int count = m_dragPart->getCollisionGeometryCount();
    if (count < kLargeDragPart || m_universe->getTimeStep() != 0.0)
        return false;
    std::vector<McdModelID> models;
    models.reserve((size_t)count);
    for (int i = 0; i < count; ++i) {
        Vx::VxCollisionGeometry* geometry = m_dragPart->getCollisionGeometry(i);
        McdModelID model = geometry->getCollisionModel();
        if (geometry->isEnabled() && model->space != 0)
            models.push_back(model);
    }
    if (models.empty() || !VincSpaceTakeOutModels(&models[0], (int)models.size()))
        return false;
    m_dragPart->setTransform(tm);
    VincSpacePutBackModels(&models[0], (int)models.size());
    return true;
}

// Mac 0x3ac980, Win 0x523930
bool VortexPhysicsScene::TestDragCollision(const Matrix3D& transform, PhysicsCollisionCallback* callback)
{
    s_hitTestTimer.Start();
    Vx::VxTransform tm;
    ToVxTransform(transform, tm);
    if (!MoveLargeDragPart(tm))
    {
        m_dragPart->setTransform(tm);
        m_dragPart->updateCollisionGeometries();
    }
    LEGOIndividualCollector collector(callback, kInsertTolerance);
    Vx::VxInfo(0, "VortexPhysicsRigidElement::HitTest at: %.3f %.3f %.3f", tm.m[3][0], tm.m[3][1], tm.m[3][2]);
    if (Vx::VxIntersect::intersect(m_dragPart, &collector, true))
        Vx::VxInfo(0, "IntersectionFound in VortexPhysicsScene::HitTest found collision, collector collides = %d",
                   (int)collector.m_collides);
    bool collides = collector.m_collides;
    s_hitTestTimer.Stop();
    return !collides;
}

// Mac 0x3acb50, Win 0x523ab0
bool VortexPhysicsScene::ClearDragScene()
{
    m_mechanism->removePart(m_dragPart);
    VortexCollisionGeometryData::DestroyAll(m_dragPart);
    delete m_dragPart;
    m_dragPart = 0;
    Vx::VxInfo(0, "VortexPhysicsScene::EndHitTest, space contains %d cg", m_universe->getCollisionGeometryCount());
    return true;
}

// Mac 0x3acbe0, Win 0x523be0
bool VortexPhysicsScene::CastDragScene(const Matrix3D& from, const Vector3& to, Real32& distance, bool terrainOnly)
{
    if (terrainOnly) {
        TerrainRayCastCollector collector;
        Vx::VxLineSegResult result;
        result.mCollisionGeometry = 0;
        result.mPosition = Vx::VxVector3(0.0, 0.0, 0.0);
        result.mNormal = Vx::VxVector3(0.0, 0.0, 0.0);
        result.mDistance = DBL_MAX;
        result.mElement = -1;
        Vx::VxVector3 start = ToVxVector3(from.GetTranslation());
        Vx::VxVector3 end = ToVxVector3(to);
        bool hit = Vx::VxLineSegIntersect(m_universe, start, end, false, &result, &collector);
        if (hit)
            distance = (Real32)result.mDistance;
        return hit;
    }
    Vx::VxTransform tm;
    ToVxTransform(from, tm);
    m_dragPart->setTransform(tm);
    Vx::VxVector3 velocity = ToVxVector3(to - from.GetTranslation());
    m_dragPart->setSweepEnabled(true);
    m_dragPart->setLinearVelocity(velocity);
    CastIndividualCollector collector;
    g_lddVortexCollisionFilter.m_mode = LDDVortexCollisionFilter::ModeTerrain;
    const Vx::VxTransform& world = *m_dragPart->mTransformPtr;
    Vx::VxInfo(0, "VortexPhysicsScene::TerrainCastScene collision with body at %.3f %.3f %.3f, at vel %.3f %.3f %.3f",
               world.m[3][0], world.m[3][1], world.m[3][2], velocity[0], velocity[1], velocity[2]);
    m_universe->setTimeStep(1.0);
    m_dragPart->updateCollisionGeometries();
    if (Vx::VxIntersect::intersect(m_dragPart, &collector, true))
        Vx::VxInfo(0, "IntersectionFound in VortexPhysicsScene::TerrainCastScene found collision, collector collides = %d",
                   (int)collector.m_hit);
    m_universe->setTimeStep(0.0);
    bool result = true;
    if (!collector.m_hit) {
        double fraction = 1.0;
        result = false;
        if (1.0 > collector.m_timeOfImpact) {
            Vx::VxInfo(0, "IntersectionFound in VortexPhysicsScene::TerrainCastScene: time of impact found at %.3f",
                       collector.m_timeOfImpact);
            result = collector.m_hit;
            fraction = collector.m_timeOfImpact;
        }
        // NOTE: written only when nothing was hit (the original's)
        distance = (Real32)(sqrt(velocity[2] * velocity[2] + (velocity[1] * velocity[1] + velocity[0] * velocity[0])) *
                            fraction);
    }
    g_lddVortexCollisionFilter.m_mode = LDDVortexCollisionFilter::ModeDefault;
    return result;
}

// Mac 0x3ad030, Win 0x523f70
bool VortexPhysicsScene::TestElementCollision(PhysicsPrimitiveElement* element, const Vector3& position,
                                              PhysicsCollisionCallback* callback)
{
    StopSolver(false);
    VortexPhysicsPrimitiveElement* primitive = static_cast<VortexPhysicsPrimitiveElement*>(element);
    Vx::VxCollisionGeometry* geometry = primitive->CreateCollisionGeometry(true);
    if (geometry == 0)
        return true;
    Vx::VxPart* part = new Vx::VxPart();
    part->setControl(Vx::VxEntity::kControlDynamic);
    part->setMass(1.0);
    part->mUserInertia = true;
    part->mUserMass = true;
    part->mUserCOM = true;
    m_mechanism->addPart(part);
    Matrix3D placement(primitive->m_owner->m_transform, primitive->m_transform);
    Vx::VxTransform tm;
    ToVxTransform(placement, tm);
    part->addCollisionGeometry(geometry);
    part->setTransform(tm);
    part->setUserData(&m_testBodyData);
    Vx::VxVector3 velocity((double)position.x - tm.m[3][0], (double)position.y - tm.m[3][1],
                           (double)position.z - tm.m[3][2]);
    part->setSweepEnabled(true);
    part->setLinearVelocity(velocity);
    CastIndividualCollector collector;
    const Vx::VxTransform& world = *part->mTransformPtr;
    Vx::VxInfo(0,
               "VortexPhysicsScene::TestCastPrimitiveElement collision with body at %.3f %.3f %.3f, at vel %.3f %.3f %.3f",
               world.m[3][0], world.m[3][1], world.m[3][2], velocity[0], velocity[1], velocity[2]);
    m_universe->setTimeStep(1.0);
    {
        // The test box enters the broad phase where the part was made and moves to the element's
        // swept box; the search trees move it instead of the walk past every box between (the same
        // pairs).
        VincSpaceID space = (VincSpaceID)(void*)m_universe->mSpace;
        VincSpaceSetRelocateCrossings(space, 64);
        part->updateCollisionGeometries();
        VincSpaceSetRelocateCrossings(space, 0x7fffffff);
    }
    g_lddVortexCollisionFilter.m_mode = LDDVortexCollisionFilter::ModeCallback;
    g_lddVortexCollisionFilter.m_callback = callback;
    if (Vx::VxIntersect::intersect(part, &collector, true))
        Vx::VxInfo(0,
                   "IntersectionFound in VortexPhysicsScene::TestCastPrimitiveElement found collision, collector collides = %d",
                   (int)collector.m_hit);
    m_universe->setTimeStep(0.0);
    VortexCollisionGeometryData::DestroyAll(part);
    m_mechanism->removePart(part);
    delete part;
    g_lddVortexCollisionFilter.m_callback = 0;
    g_lddVortexCollisionFilter.m_mode = LDDVortexCollisionFilter::ModeDefault;
    return !collector.m_hit;
}

// Mac 0x3ad500, Win 0x5242d0
PhysicsSnapshot* VortexPhysicsScene::CreateSnapshot(PhysicsRigidElement* const* bodies, UInt32 count)
{
    VortexPhysicsSnapshot* snapshot = new VortexPhysicsSnapshot(this);
    snapshot->m_entries.reserve((int)count);
    for (UInt32 i = count; i != 0; --i) {
        VortexPhysicsSnapshot::Entry entry;
        entry.m_element = 0;
        snapshot->m_entries.push_back(entry);
        VortexPhysicsSnapshot::Entry& back = snapshot->m_entries.end()[-1];
        VortexPhysicsRigidElement* body = static_cast<VortexPhysicsRigidElement*>(*bodies);
        const Vx::VxTransform* tm = body->m_part->mTransformPtr;
        if (&back.m_partTransform != tm)
            back.m_partTransform = *tm;
        body->GetEngineTransform(back.m_transform);
        back.m_element = body;
        ++bodies;
    }
    return snapshot;
}

// Mac 0x3adaf0, Win 0x524410
PhysicsSolver* VortexPhysicsScene::SolveMoveTogether(PhysicsRigidElement* body, const Vector3& position, UInt32,
                                                     const Vector3& axis, PhysicsRigidElement* otherBody,
                                                     const Vector3& otherPosition, UInt32, const Vector3& otherAxis,
                                                     PhysicsCollisionCallback* callback,
                                                     const PhysicsGeometryIdentifier* features, UInt32 featureCount,
                                                     IUnknown* context)
{
    StopSolver(false);
    VortexMoveTogetherSolver* solver =
        new VortexMoveTogetherSolver(this, static_cast<PhysicsSolverCallback*>(context));
    solver->AddRef();   // the COM AddRef (Mac slot 13)
    Unknown6();
    // (`position` / `otherPosition` are the feature axes, see BrickPhysics.h)
    BaseBrickPhysicsRigidElement* first = static_cast<BaseBrickPhysicsRigidElement*>(body);
    BaseBrickPhysicsRigidElement* second = static_cast<BaseBrickPhysicsRigidElement*>(otherBody);
    VortexMoveTogetherHinge* hinge = new VortexMoveTogetherHinge(
        this, first, position, position.GetPerpendicular(), axis, second, otherPosition,
        otherPosition.GetPerpendicular(), otherAxis, callback, features, featureCount, m_castGeometry,
        &solver->m_hinges);
    solver->m_hinge = hinge;
    Unknown7();
    Vx::VxInfo(0, "VortexPhysicsScene::MoveTogether: created");
    CollectHingeGroup(solver->m_hinge.Get(), first, solver->m_connections, solver->m_elements);
    // The second body leaves the moving bodies: Mac 0x3add53 calls the erase-by-value helper
    // 0x258140 (std::find by body + erase by swap, 0x25a870) with a TransformedElement key.
    {
        ActiveRigidElement* it = solver->m_elements.Begin();
        while (it != solver->m_elements.End() && it->m_element != second)
            ++it;
        if (it != solver->m_elements.End()) {
            ActiveRigidElement* last = solver->m_elements.End() - 1;
            if (last != it)
                *it = *last;
            solver->m_elements.PopBack();
        }
    }
    BaseBrickPhysicsConnectionHelper* moveTogether = solver->m_hinge.Get();
    PhysicsArrayRemove(solver->m_connections, moveTogether);
    SetSolver(solver);
    solver->AddRef();   // the COM AddRef (Mac slot 13)
    static_cast<Object*>(solver)->Release();
    return static_cast<MoveTogetherPhysicsSolver*>(solver);
}

// Mac 0x3ade30, Win 0x524710
IUnknown* VortexPhysicsScene::AddSolverCallback(PhysicsSolverCallback* callback)
{
    StopSolver(false);
    VortexAnchorSolver* solver = new VortexAnchorSolver(this, callback);
    solver->AddRef();   // the COM AddRef (Mac slot 13)
    SetSolver(solver);
    solver->AddRef();   // the COM AddRef (Mac slot 13)
    static_cast<Object*>(solver)->Release();
    return static_cast<AnchorPhysicsSolver*>(solver);
}

// Mac 0x3adee0, Win 0x5247d0
void VortexPhysicsScene::Unknown29(void* object)
{
    Vx::VxMechanism* assembly = static_cast<Vx::VxMechanism*>(object);
    if (m_assemblyB == assembly)
        return;
    if (m_assemblyB != 0)
        m_universe->removeAssembly(m_assemblyB);
    m_assemblyB = assembly;
    if (assembly == 0)
        return;
    for (int i = 0; i < assembly->getPartCount(); ++i) {
        Vx::VxPart* part = assembly->getPart(i);
        part->setUserData(&m_assemblyBBodyData);
        for (int j = 0; j < part->getCollisionGeometryCount(); ++j)
            SetShape(part->getCollisionGeometry(j), &m_assemblyBShape);
    }
    m_universe->addAssembly(m_assemblyB);
}

// Mac 0x3adfe0, Win 0x524830
void* VortexPhysicsScene::Unknown30()
{
    return m_assemblyB;
}

// Mac 0x3adff0, Win 0x524840
void VortexPhysicsScene::Unknown31(void* object)
{
    Vx::VxMechanism* assembly = static_cast<Vx::VxMechanism*>(object);
    if (m_assemblyA == assembly)
        return;
    if (m_assemblyA != 0)
        m_universe->removeAssembly(m_assemblyA);
    m_assemblyA = assembly;
    if (assembly == 0)
        return;
    for (int i = 0; i < assembly->getPartCount(); ++i) {
        Vx::VxPart* part = assembly->getPart(i);
        part->setUserData(&m_assemblyABodyData);
        for (int j = 0; j < part->getCollisionGeometryCount(); ++j)
            SetShape(part->getCollisionGeometry(j), &m_assemblyAShape);
    }
    m_universe->addAssembly(m_assemblyA);
}

// Mac 0x3ae0f0, Win 0x5294e0
void* VortexPhysicsScene::Unknown32()
{
    return m_assemblyA;
}

// Mac 0x3af700 (ECX = geometry, EDX = shape), Win 0x525440
void VortexPhysicsScene::SetShape(Vx::VxCollisionGeometry* geometry, VortexShapeUserData* shape)
{
    VortexCollisionGeometryData::SetShape(geometry, shape);
    if (!geometry->isOfClassType(Vx::VxCompositeCollisionGeometry::getStaticClassType()))
        return;
    Vx::VxCompositeCollisionGeometry* composite = static_cast<Vx::VxCompositeCollisionGeometry*>(geometry);
    for (int i = 0; i < composite->getCollisionGeometryCount(); ++i)
        SetShape(composite->getCollisionGeometry(i), shape);
}

// Mac 0x3ae100, Win 0x5248a0
void VortexPhysicsScene::AddSceneryElement(PhysicsSceneryElement* element, const Matrix3D& transform, UInt32 flags)
{
    VortexPhysicsSceneryElement* scenery = static_cast<VortexPhysicsSceneryElement*>(element);
    m_sceneryElements.PushBack(scenery);
    scenery->AddToScene(this, transform, flags);
    if (m_solver != 0)
        m_solver->OnAddSceneryElement(element, transform, flags);
}

// Mac 0x3ae380, Win 0x5248f0
void VortexPhysicsScene::RemoveSceneryElement(PhysicsSceneryElement* element)
{
    VortexPhysicsSceneryElement* scenery = static_cast<VortexPhysicsSceneryElement*>(element);
    if (m_solver != 0)
        m_solver->OnRemoveSceneryElement(element);
    scenery->RemoveFromScene();
    PhysicsArrayRemove(m_sceneryElements, scenery);
}

// Mac 0x3ae5d0, Win 0x524950
UInt32 VortexPhysicsScene::GetSceneryElementCount(UInt32 flags)
{
    UInt32 count = 0;
    for (VortexPhysicsSceneryElement** it = m_sceneryElements.Begin(); it != m_sceneryElements.End(); ++it)
        if (((*it)->m_flags & flags) != 0)
            ++count;
    return count;
}

// Mac 0x3ae6a0, Win 0x524990
PhysicsSceneryElement* VortexPhysicsScene::GetSceneryElement(UInt32 index, UInt32 flags)
{
    UInt32 n = 0;
    for (VortexPhysicsSceneryElement** it = m_sceneryElements.Begin(); it != m_sceneryElements.End(); ++it)
        if (((*it)->m_flags & flags) != 0) {
            if (n == index)
                return *it;
            ++n;
        }
    return 0;
}

// Mac 0x3ae6f0, Win 0x5249d0
void VortexPhysicsScene::SetGravity(const Vector3& gravity)
{
    m_universe->setGravity((Vx::VxReal)gravity.x, (Vx::VxReal)gravity.y, (Vx::VxReal)gravity.z);
}

// Mac 0x3ae740, Win 0x524a00
void VortexPhysicsScene::EnableSimulation(BaseBrickPhysicsRigidElement* body)
{
    static_cast<VortexPhysicsRigidElement*>(body)->m_part->freeze(false);
}

// Mac 0x3ae760, Win 0x524a00 (the same body)
void VortexPhysicsScene::EnableManualMotion(BaseBrickPhysicsRigidElement* body)
{
    static_cast<VortexPhysicsRigidElement*>(body)->m_part->freeze(false);
}

// Mac 0x3ae7b0, Win 0x524a50
void VortexPhysicsScene::DisableSimulation(BaseBrickPhysicsRigidElement* body)
{
    Vx::VxPart* part = static_cast<VortexPhysicsRigidElement*>(body)->m_part;
    part->setControl(Vx::VxEntity::kControlDynamic);
    part->setAngularVelocity(0.0, 0.0, 0.0);
    part->setLinearVelocity(0.0, 0.0, 0.0);
    part->freeze(true);
}

// Mac 0x3ae780, Win 0x524a20
void VortexPhysicsScene::Unknown37(BaseBrickPhysicsRigidElement* body)
{
    Vx::VxPart* part = static_cast<VortexPhysicsRigidElement*>(body)->m_part;
    part->setControl(Vx::VxEntity::kControlStatic);
    part->freeze(false);
}

// Mac 0x3ae810, Win 0x40be80
void VortexPhysicsScene::Unknown38(void*)
{
}

// Mac 0x3ae820, Win 0x40be80
void VortexPhysicsScene::Unknown39(void*)
{
}

// Mac 0x3ae870, Win 0x524ad0
bool VortexPhysicsScene::TestElementCollisions(const BasicArray<ActiveRigidElement>& elements)
{
    LEGOIndividualCollector collector(0, kElementTolerance);
    SleepingPairsIntersectScope scope(m_universe);
    for (const ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it) {
        Vx::VxPart* part = static_cast<VortexPhysicsRigidElement*>(it->m_element)->m_part;
        part->updateCollisionGeometryTransforms();
        part->updateCollisionGeometries();
    }
    bool update = true;
    for (const ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it) {
        Vx::VxIntersect::intersect(static_cast<VortexPhysicsRigidElement*>(it->m_element)->m_part, &collector, update);
        if (collector.m_collides) {
            Vx::VxInfo(0, "VortexPhysicsScene::CheckCollision: collision found with parent.");
            return true;
        }
        for (const TransformedElement* child = it->m_children.Begin(); child != it->m_children.End(); ++child) {
            Vx::VxIntersect::intersect(static_cast<VortexPhysicsRigidElement*>(child->m_element)->m_part, &collector,
                                       update);
            if (collector.m_collides) {
                Vx::VxInfo(0, "VortexPhysicsScene::CheckCollision: collision found with child.");
                return true;
            }
        }
        Vx::VxInfo(0, "VortexPhysicsScene::CheckCollision: no collision, cgcount=%d.",
                   m_universe->getCollisionGeometryCount());
        update = false;
    }
    return false;
}

// Mac 0x3aea60, Win 0x524cb0
void VortexPhysicsScene::RestoreSnapshotImpl(PhysicsSnapshot* snapshot)
{
    VortexPhysicsSnapshot* vortex = static_cast<VortexPhysicsSnapshot*>(snapshot);
    for (int i = 0; i < vortex->m_entries.size(); ++i) {
        VortexPhysicsSnapshot::Entry& entry = vortex->m_entries[i];
        entry.m_element->SetEngineTransform(entry.m_transform);
    }
    m_universe->updateIntersections(false);
    if (m_solver != 0)
        m_solver->OnSnapshotRestored();
}

// Mac 0x3af290, Win 0x525030
void VortexPhysicsScene::StepPhysics(Real32 time)
{
    static_cast<VortexBrickPhysics*>(m_physics)->UpdateUniverse(m_universe);
    m_universe->setTimeStep((Vx::VxReal)time);
    Vx::VxFrame::instance()->stepStart();
    if (m_doCollide)
        Vx::VxFrame::instance()->stepCollision();
    Vx::VxFrame::instance()->stepDynamics();
    Vx::VxFrame::instance()->stepEnd();
    m_universe->setTimeStep(0.0);
    if (2.0 < m_universe->getTime2() + m_universe->getTime1())
        Vx::VxInfo(0, "VortexPhysicsScene::StepPhysics: dt=%.3f, dynTime=%gms, collTime=%gms.", (double)time,
                   m_universe->getTime2(), m_universe->getTime1());
    ++m_stepCount;
}

// Mac 0x3af5b0, Win 0x5252b0
BaseBrickPhysicsBaseHingeSolver* VortexPhysicsScene::CreateHingeManualSolver(
    BaseBrickPhysicsConnectionHelper* hinge, bool first, const BasicArray<ActiveRigidElement>& elements,
    const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
{
    Vx::VxInfo(0, "VortexPhysicsScene::CreateHingeManualSolver.");
    return new VortexBrickPhysicsManualHingeSolver(this, hinge, first, elements, connections);
}

// Mac 0x3af680, Win 0x525340
BaseBrickPhysicsBaseHingeSolver* VortexPhysicsScene::CreateHingeSimulateSolver(
    BaseBrickPhysicsConnectionHelper* hinge, bool first, const BasicArray<ActiveRigidElement>& elements,
    const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
{
    Vx::VxInfo(0, "VortexPhysicsScene::CreateHingeSimulateSolver.");
    return new VortexBrickPhysicsSimulationHingeSolver(this, hinge, first, elements, connections);
}

// Mac 0x3af420, Win 0x459790
void VortexPhysicsScene::Unknown47()
{
}

// Mac 0x3af430, Win 0x525160
void VortexPhysicsScene::RebuildFreeConnection(BaseBrickPhysicsConnectionHelper* connection)
{
    VortexPhysicsFreeConnection* free = static_cast<VortexPhysicsFreeConnection*>(connection);
    if (free->m_constraint != 0)
        free->DestroyConstraint();
    free->UpdateConstraint();
}

// Mac 0x3aeb30, Win 0x524d30
VortexHingeSolverHinge* VortexPhysicsScene::BuildHingeSolverHinge(
    bool, BaseBrickPhysicsRigidElement* body, const Vector3& axis, const Vector3& normal, const Vector3& position,
    BaseBrickPhysicsRigidElement* otherBody, const Vector3& otherAxis, const Vector3& otherNormal,
    const Vector3& otherPosition, BasicArray<BaseBrickPhysicsConnectionHelper*>* hinges)
{
    LockDirty();
    VortexHingeSolverHinge* hinge = new VortexHingeSolverHinge(this, body, axis, normal, position, otherBody,
                                                               otherAxis, otherNormal, otherPosition, hinges);
    UnlockDirty(false);
    Vx::VxInfo(0, "VortexPhysicsScene::BuildHingeSolverHinge.");
    return hinge;
}

// Mac 0x3aec50, Win 0x524df0
void VortexPhysicsScene::BuildConnectionSpringMirrors(UInt32 strength,
                                                      const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections,
                                                      BasicArray<BaseBrickPhysicsConnectionHelper*>& springs,
                                                      BaseBrickPhysicsConnectionHelper* exclude)
{
    for (BaseBrickPhysicsConnectionHelper* const* it = connections.Begin(); it != connections.End(); ++it) {
        BaseBrickPhysicsConnectionHelper* connection = *it;
        if (connection->Unknown3())
            continue;
        UInt32 type = connection->GetType();
        if (type == 5) {
            BaseBrickPhysicsConnectionHelper* spring =
                new VortexPhysicsConstraintBallSpringHelper(strength, *connection, connection != exclude);
            springs.PushBack(spring);
            connection->DestroyConstraint();
            Vx::VxInfo(0, "VortexPhysicsScene::BuildConnectionSpringMirrors: VortexPhysicsConstraintBallSpringHelper "
                          "created.");
        } else if (type == 1) {
            BaseBrickPhysicsConnectionHelper* spring =
                new VortexPhysicsConstraintHingeSpringHelper(strength, *connection, connection != exclude);
            springs.PushBack(spring);
            Vx::VxInfo(0, "VortexPhysicsScene::BuildConnectionSpringMirrors: VortexPhysicsConstraintHingeSpringHelper "
                          "created.");
        }
    }
}

// Mac 0x3af1c0
void VortexPhysicsScene::DestroyConnectionSpringMirrors(const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections,
                                                        BasicArray<BaseBrickPhysicsConnectionHelper*>& springs)
{
    for (BaseBrickPhysicsConnectionHelper** it = springs.Begin(); it != springs.End(); ++it) {
        BaseBrickPhysicsConnectionHelper* spring = *it;
        spring->Detach();
        delete spring;
    }
    springs.Clear();
    for (BaseBrickPhysicsConnectionHelper* const* it = connections.Begin(); it != connections.End(); ++it)
        if (!(*it)->HasNativeConstraint())
            (*it)->UpdateConstraint();
}

// Mac 0x3af250
void VortexPhysicsScene::ResetForces(const BasicArray<ActiveRigidElement>& elements)
{
    for (const ActiveRigidElement* it = elements.Begin(); it != elements.End(); ++it)
        static_cast<VortexPhysicsRigidElement*>(it->m_element)->ResetForces();
}

// =============================================================================================
// VortexPhysicsSnapshot
// =============================================================================================

// Mac 0x3ad3d0, Win 0x524240
void VortexPhysicsSnapshot::Update()
{
    for (int i = 0; i < m_entries.size(); ++i) {
        Entry& entry = m_entries[i];
        Vx::VxPart* part = entry.m_element->m_part;
        ToMatrix3D(*part->mTransformPtr, entry.m_transform);
        part->getTransform(entry.m_partTransform);
    }
}

// Mac 0x3b2b30, Win 0x5219a0
UInt32 VortexPhysicsSnapshot::GetCount() const
{
    return (UInt32)m_entries.size();
}

// Mac 0x3b2b50, Win 0x5219c0
PhysicsRigidElement* VortexPhysicsSnapshot::GetRigidElement(UInt32 index) const
{
    return m_entries[(int)index].m_element;
}

// Mac 0x3b2ba0, Win 0x5219e0
void VortexPhysicsSnapshot::Clear()
{
    m_entries.mEnd = m_entries.mBegin;
}

// =============================================================================================
// VortexHingeSolverHinge
// =============================================================================================

// Mac 0x3b28d0, Win 0x521770
UInt32 VortexHingeSolverHinge::GetType() const
{
    return 7;
}

// Mac 0x3b28e0, Win 0x521850
void* VortexHingeSolverHinge::GetNativeConstraint()
{
    if (m_constraint != 0)
        return m_constraint->getUserData();
    return 0;
}

// Mac 0x3b2900, Win 0x521860
bool VortexHingeSolverHinge::HasNativeConstraint()
{
    return m_constraint != 0;
}

// Mac 0x3b2910, Win 0x4aede0
bool VortexHingeSolverHinge::Unknown3()
{
    return false;
}

// Mac 0x3b1c00, Win 0x527d90
UInt32 VortexHingeSolverHinge::CheckSeparation(const PhysicsSeparationLimits&)
{
    Vx::VxReal position = m_constraint->getCoordinatePosition(0);
    Vx::VxReal lock = m_constraint->getLockPosition(0);
    if (fabs(position) < fabs(lock) * (double)0.016f)
        m_constraint->setControl(0, Vx::VxConstraint::kControlMotorized);
    return (double)0.001f <= fabs(position) ? 1 : 0;
}

// Mac 0x3b1ca0, Win 0x527e00
bool VortexHingeSolverHinge::UpdateConstraint()
{
    Vx::VxVector3 position = ToVxVector3(m_position);
    Vx::VxVector3 otherPosition = ToVxVector3(m_otherPosition);
    Vx::VxVector3 axis = ToVxVector3(m_axis);
    Vx::VxVector3 otherAxis = ToVxVector3(m_otherAxis);
    Vx::VxVector3 normal = ToVxVector3(m_normal);
    Vx::VxVector3 otherNormal = ToVxVector3(m_otherNormal);
    Vx::VxHinge* hinge = new Vx::VxHinge();
    m_constraint = hinge;
    hinge->setPositionEquationReferencePart(1);
    m_constraint->setName("toolJoint");
    m_constraint->setPartAndAttachmentRel(0, static_cast<VortexPhysicsRigidElement*>(m_body)->m_part, position, axis,
                                          normal);
    m_constraint->setPartAndAttachmentRel(1, static_cast<VortexPhysicsRigidElement*>(m_otherBody)->m_part,
                                          otherPosition, otherAxis, otherNormal);
    m_constraint->setRelaxationParameters(0, 10.0, 1.0, 0.0, true);
    m_constraint->setUserData(static_cast<BaseBrickPhysicsConnectionHelper*>(this));
    m_vortexScene->m_mechanism->addConstraint(m_constraint);
    Vx::VxReal current = m_constraint->getCoordinateCurrentPosition(0);
    m_constraint->setCoordinatePosition(0, current);
    m_constraint->setControl(0, Vx::VxConstraint::kControlLocked);
    Real32 lockSpeed;
    if (current > 0.0) {
        lockSpeed = -m_lockSpeed;
        m_constraint->setLimitPositions(0, 0.0, current);
    } else {
        lockSpeed = m_lockSpeed;
        m_constraint->setLimitPositions(0, current, 0.0);
    }
    double lock = (double)lockSpeed;
    m_constraint->setLimitsActive(0, true);
    double nearRange = (double)(m_lockSpeed * 0.016f);
    if (fabs(current) < nearRange)
        lock = lock * ((fabs(current) / nearRange) / 5.0);
    m_constraint->setLockParameters(0, lock, 1e7, 0.0);
    return true;
}

// Mac 0x3b2010, Win 0x528090
void VortexHingeSolverHinge::DestroyConstraint()
{
    if (m_constraint == 0)
        return;
    m_vortexScene->m_mechanism->removeConstraint(m_constraint);
    delete m_constraint;
    m_constraint = 0;
}

// Mac 0x3b2060 (thunk 0x3b20b0), Win 0x5280d0
UInt32 VortexHingeSolverHinge::Step(Real32)
{
    for (BaseBrickPhysicsConnectionHelper** it = m_hinges->Begin(); it != m_hinges->End(); ++it)
        static_cast<VortexPhysicsConstraintSpringHelper*>(*it)->Adapt();
    return 1;
}

// =============================================================================================
// VortexMoveTogetherHinge
// =============================================================================================

// Mac 0x3b2670, Win 0x521770
UInt32 VortexMoveTogetherHinge::GetType() const
{
    return 7;
}

// Mac 0x3b2680, Win 0x521780
void* VortexMoveTogetherHinge::GetNativeConstraint()
{
    if (m_constraint != 0)
        return m_constraint->getUserData();
    return 0;
}

// Mac 0x3b26a0, Win 0x521790
bool VortexMoveTogetherHinge::HasNativeConstraint()
{
    return m_constraint != 0;
}

// Mac 0x3b26b0, Win 0x4aede0
bool VortexMoveTogetherHinge::Unknown3()
{
    return false;
}

// Mac 0x3b0cb0, Win 0x527140
UInt32 VortexMoveTogetherHinge::CheckSeparation(const PhysicsSeparationLimits& limits)
{
    Matrix3D transform;
    Matrix3D otherTransform;
    m_body->GetEngineTransform(transform);
    m_otherBody->GetEngineTransform(otherTransform);
    Vector3 axis = LEGO::Rotate(m_axis, transform);
    Vector3 otherAxis = LEGO::Rotate(m_otherAxis, otherTransform);
    Vector3 position = m_position * transform;
    Vector3 otherPosition = m_otherPosition * otherTransform;
    Real32 alignment = (Real32)fabs(axis * otherAxis);
    bool result = true;
    if (limits.m_alignment < alignment)
        result = limits.m_distance <= position.GetDistanceSquared(otherPosition);
    return result ? 1 : 0;
}

// Mac 0x3b0dc0, Win 0x527300
bool VortexMoveTogetherHinge::UpdateConstraint()
{
    Vx::VxVector3 position = ToVxVector3(m_position);
    Vx::VxVector3 otherPosition = ToVxVector3(m_otherPosition);
    Vx::VxPart* part = static_cast<VortexPhysicsRigidElement*>(m_body)->m_part;
    Vx::VxPart* otherPart = static_cast<VortexPhysicsRigidElement*>(m_otherBody)->m_part;
    Vx::VxVector3 axis = ToVxVector3(m_axis);
    Vx::VxVector3 otherAxis = ToVxVector3(m_otherAxis);
    Vx::VxVector3 normal = ToVxVector3(m_normal);
    Vx::VxVector3 otherNormal = ToVxVector3(m_otherNormal);
    if (m_attached) {
        Vx::VxHinge* hinge = new Vx::VxHinge();
        m_constraint = hinge;
        hinge->setPositionEquationReferencePart(1);
        m_constraint->setPartAndAttachmentRel(0, part, position, axis, normal);
        m_constraint->setPartAndAttachmentRel(1, otherPart, otherPosition, otherAxis, otherNormal);
        m_constraint->setRelaxationParameters(0, 0.1, 1.0, 0.0, true);
        m_constraint->setUserData(static_cast<BaseBrickPhysicsConnectionHelper*>(this));
        m_vortexScene->m_mechanism->addConstraint(m_constraint);
        return true;
    }
    // the first position in world space, relative to the second body (its frame)
    const Vx::VxTransform& tm = *part->mTransformPtr;
    const Vx::VxTransform& otherTm = *otherPart->mTransformPtr;
    double wx = ((tm.m[0][0] * position[0] + tm.m[1][0] * position[1]) + tm.m[2][0] * position[2]) + tm.m[3][0];
    double wy = ((tm.m[0][1] * position[0] + tm.m[1][1] * position[1]) + tm.m[2][1] * position[2]) + tm.m[3][1];
    double wz = ((tm.m[0][2] * position[0] + tm.m[1][2] * position[1]) + tm.m[2][2] * position[2]) + tm.m[3][2];
    double dx = wx - otherTm.m[3][0];
    double dy = wy - otherTm.m[3][1];
    double dz = wz - otherTm.m[3][2];
    Vx::VxSpring* spring = new Vx::VxSpring();
    m_constraint = spring;
    spring->setPartAndAttachmentRel(0, part, position, axis, normal);
    m_constraint->setPartAndAttachmentRel(1, otherPart, otherPosition, otherAxis, otherNormal);
    m_constraint->setUserData(static_cast<BaseBrickPhysicsConnectionHelper*>(this));
    m_vortexScene->m_mechanism->addConstraint(m_constraint);
    Vx::VxLinear2* linear = new Vx::VxLinear2();
    double lx = (dy * otherTm.m[0][1] + dx * otherTm.m[0][0]) + dz * otherTm.m[0][2];
    double ly = (dy * otherTm.m[1][1] + dx * otherTm.m[1][0]) + dz * otherTm.m[1][2];
    double lz = (otherTm.m[2][0] * dx + otherTm.m[2][1] * dy) + otherTm.m[2][2] * dz;
    m_linear = linear;
    linear->setPartAndAttachmentRel(0, part, position, axis, normal);
    Vx::VxVector3 linearAxis(otherPosition[0] - lx, otherPosition[1] - ly, otherPosition[2] - lz);
    // NOTE: the primary axis is the second position minus the first one (body space) and the
    // VxLinear2 is never added to the mechanism (the original's)
    m_linear->setPartAndAttachmentRel(1, otherPart, otherPosition, linearAxis, otherAxis);
    m_linear->setMdtFlag2(1);
    m_linear->setRelaxationStiffnessAndDamping(0, 0.01, 0.01);
    m_linear->setRelaxationStiffnessAndDamping(0, 0.01, 0.01);
    Vx::VxVector3 attachment(0.0, 0.0, 0.0);
    Vx::VxVector3 otherAttachment(0.0, 0.0, 0.0);
    m_constraint->getPartAttachmentPosition(0, attachment);
    m_constraint->getPartAttachmentPosition(1, otherAttachment);
    m_constraint->setLimitsActive(0, true);
    m_constraint->setLimitPosition(0, 0, 0.0);
    spring->setNaturalLength(0.0);
    // Win divides on the x87 by the double 60.0 without rounding to float (Win 0x5276ca); the Mac
    // in single precision (divss).
    double slack = (double)((EvalReal)m_maxForce / (EvalReal)60.0f);
    spring->setMotorMaximumForce(0, 1000.0 * slack * (double)0.1f);
    double ax = attachment[0] - otherAttachment[0];
    double ay = attachment[1] - otherAttachment[1];
    double az = attachment[2] - otherAttachment[2];
    m_constraint->setUpperLimit(0, sqrt(az * az + (ay * ay + ax * ax)) + slack, (double)(-m_maxForce), 0.0, 1000.0,
                                10.0);
    return true;
}

// Mac 0x3b14e0, Win 0x527860
void VortexMoveTogetherHinge::DestroyConstraint()
{
    Vx::VxMechanism* mechanism = m_vortexScene->m_mechanism;
    if (m_constraint != 0) {
        mechanism->removeConstraint(m_constraint);
        delete m_constraint;
        m_constraint = 0;
    }
    if (m_linear != 0) {
        mechanism->removeConstraint(m_linear);
        delete m_linear;
        m_linear = 0;
    }
}

// Mac 0x3b1560 (thunk 0x3b15b0), Win 0x5278d0
void VortexMoveTogetherHinge::Start()
{
    for (const PhysicsGeometryIdentifier* it = m_features; it < m_features + m_featureCount; ++it)
        static_cast<VortexPhysicsPrimitiveElement*>(it->m_element)->SetGeometryEnabled((Int32)it->m_feature, false);
}

// Mac 0x3b1600 (thunk 0x3b1a60), Win 0x527910
void VortexMoveTogetherHinge::Stop(bool& result)
{
    BasicArray<BaseBrickPhysicsRigidElement*> bodies;
    for (const PhysicsGeometryIdentifier* it = m_features; it < m_features + m_featureCount; ++it) {
        VortexPhysicsPrimitiveElement* element = static_cast<VortexPhysicsPrimitiveElement*>(it->m_element);
        element->SetGeometryEnabled((Int32)it->m_feature, true);
        BaseBrickPhysicsRigidElement* body = element->m_owner;
        BaseBrickPhysicsRigidElement** found = bodies.Begin();
        while (found != bodies.End() && *found != body)
            ++found;
        if (found == bodies.End())
            bodies.PushBack(body);
    }
    if (!result)
        return;
    LEGOIndividualCollector collector(m_callback, kElementTolerance);
    if (bodies.IsEmpty()) {
        result = true;
        return;
    }
    for (BaseBrickPhysicsRigidElement** it = bodies.Begin(); it != bodies.End(); ++it)
        if (Vx::VxIntersect::intersect(static_cast<VortexPhysicsRigidElement*>(*it)->m_part, &collector, false))
            break;
    result = !collector.m_collides;
}

// Mac 0x3b1a80 (thunk 0x3b1be0), Win 0x527a80
UInt32 VortexMoveTogetherHinge::Step(Real32)
{
    if (!m_attached) {
        for (BaseBrickPhysicsConnectionHelper** it = m_hinges->Begin(); it != m_hinges->End(); ++it)
            static_cast<VortexPhysicsConstraintSpringHelper*>(*it)->ResetStiffness();
        Matrix3D transform;
        Matrix3D otherTransform;
        m_body->GetEngineTransform(transform);
        m_otherBody->GetEngineTransform(otherTransform);
        Vector3 position = m_position * transform;
        Vector3 otherPosition = m_otherPosition * otherTransform;
        if (Distance(position, otherPosition) < 0.1f) {
            DestroyConstraint();
            m_attached = true;
            UpdateConstraint();
        }
    } else {
        for (BaseBrickPhysicsConnectionHelper** it = m_hinges->Begin(); it != m_hinges->End(); ++it)
            static_cast<VortexPhysicsConstraintSpringHelper*>(*it)->Adapt();
        Vx::VxConstraint* hinge = m_constraint;
        double stiffness = hinge->getRelaxationStiffness(0);
        hinge->setRelaxationStiffness(0, stiffness * 1.1);
    }
    return 0;
}

// =============================================================================================
// VortexPhysicsConstraintHingeSpringHelper
// =============================================================================================

// Mac 0x3b0100, Win 0x525f80
VortexPhysicsConstraintHingeSpringHelper::VortexPhysicsConstraintHingeSpringHelper(
    UInt32 strength, const BaseBrickPhysicsConnectionHelper& hinge, bool motor)
    : VortexPhysicsConstraintSpringHelper(strength, hinge), m_targetAngle(0.0f), m_motor(motor)
{
    m_position += m_axis * 1.0f;
    m_otherPosition += m_otherAxis * 1.0f;
    m_targetAngle = -GetAngle();
}

// Mac 0x3b25f0, Win 0x51e780
UInt32 VortexPhysicsConstraintHingeSpringHelper::GetType() const
{
    return 1;
}

// Mac 0x3afa00, Win 0x52a700
void* VortexPhysicsConstraintHingeSpringHelper::GetNativeConstraint()
{
    if (m_constraint != 0)
        return m_constraint->getUserData();
    return 0;
}

// Mac 0x3b2600, Win 0x51cf20
bool VortexPhysicsConstraintHingeSpringHelper::HasNativeConstraint()
{
    return m_constraint != 0;
}

// Mac 0x3b2610, Win 0x4aede0
bool VortexPhysicsConstraintHingeSpringHelper::Unknown3()
{
    return false;
}

// Mac 0x3af8b0, Win 0x5255e0
UInt32 VortexPhysicsConstraintHingeSpringHelper::CheckSeparation(const PhysicsSeparationLimits& limits)
{
    Matrix3D transform;
    Matrix3D otherTransform;
    m_body->GetEngineTransform(transform);
    m_otherBody->GetEngineTransform(otherTransform);
    Vector3 axis = LEGO::Rotate(m_axis, transform);
    Vector3 otherAxis = LEGO::Rotate(m_otherAxis, otherTransform);
    Vector3 position = m_position * transform;
    Vector3 otherPosition = m_otherPosition * otherTransform;
    Real32 alignment = (Real32)fabs(axis * otherAxis);
    Real32 distance = position.GetDistanceSquared(otherPosition);
    if (limits.m_alignment < alignment && distance < limits.m_distance) {
        m_separationCount = 0;
        return 0;
    }
    if (limits.m_minAlignment < alignment && distance < limits.m_maxDistance) {
        m_separationCount = 0;
        return 1;
    }
    ++m_separationCount;
    return limits.m_breakCount <= m_separationCount ? 2 : 1;
}

// Mac 0x3afa20, Win 0x5257f0
bool VortexPhysicsConstraintHingeSpringHelper::UpdateConstraint()
{
    Vx::VxVector3 position = ToVxVector3(m_position);
    Vx::VxVector3 otherPosition = ToVxVector3(m_otherPosition);
    Vx::VxVector3 axis = ToVxVector3(m_axis);
    Vx::VxVector3 otherAxis = ToVxVector3(m_otherAxis);
    Vx::VxVector3 normal = ToVxVector3(m_normal);
    Vx::VxVector3 otherNormal = ToVxVector3(m_otherNormal);
    Vx::VxHinge* hinge = new Vx::VxHinge();
    m_constraint = hinge;
    hinge->setPositionEquationReferencePart(1);
    static_cast<Vx::VxHinge*>(m_constraint)->setPositionEquationReferencePart(1);
    m_constraint->setPartAndAttachmentRel(0, static_cast<VortexPhysicsRigidElement*>(m_body)->m_part, position, axis,
                                          normal);
    m_constraint->setPartAndAttachmentRel(1, static_cast<VortexPhysicsRigidElement*>(m_otherBody)->m_part,
                                          otherPosition, otherAxis, otherNormal);
    static_cast<VortexPhysicsScene*>(m_body->m_scene)->m_mechanism->addConstraint(m_constraint);
    if (m_motor) {
        m_constraint->setControl(kHingeCoordinate, Vx::VxConstraint::kControlMotorized);
        m_constraint->setMotorDesiredVelocity(kHingeCoordinate, (double)m_targetAngle);
        SetStiffness((double)kHingeSpringStiffness[m_strength][0]);
    }
    m_constraint->setUserData(static_cast<BaseBrickPhysicsConnectionHelper*>(this));
    return true;
}

// Mac 0x3afd70, Win 0x525b00
void VortexPhysicsConstraintHingeSpringHelper::DestroyConstraint()
{
    if (m_constraint == 0)
        return;
    static_cast<VortexPhysicsScene*>(m_body->m_scene)->m_mechanism->removeConstraint(m_constraint);
    delete m_constraint;
    m_constraint = 0;
}

// Mac 0x3afc40, Win 0x5259f0
void VortexPhysicsConstraintHingeSpringHelper::SetStiffness(double stiffness)
{
    Real32 target = m_targetAngle;
    Vx::VxReal current = m_constraint->getCoordinatePosition(kHingeCoordinate);
    double maximum;
    if (m_strength == 1) {
        maximum = 1.0;
    } else {
        maximum = 0.1745329350233078;   // 10 degrees
        double range = 0.1745329350233078 - fabs((double)target - current);
        if (range < (double)0.01f)
            range = (double)0.01f;
        stiffness = stiffness * range;
    }
    m_constraint->setMotorStiffnessAndDamping(kHingeCoordinate, stiffness, (double)0.1f * stiffness);
    m_constraint->setMotorMaximumForce(kHingeCoordinate, maximum * stiffness);
    m_constraint->getCoordinateFriction(kHingeCoordinate)->setMaxForce(stiffness * 0.0003490658709779382);
    m_constraint->getCoordinateFriction(kHingeCoordinate)->setScale(1000.0);
}

// Mac 0x3afdc0, Win 0x525b40
void VortexPhysicsConstraintHingeSpringHelper::Stiffen()
{
    if (m_constraint == 0)
        return;
    Real32 k = (Real32)m_constraint->getMotorStiffness(kHingeCoordinate) * 1.5f;
    if (kHingeSpringStiffness[m_strength][0] <= k)
        k = kHingeSpringStiffness[m_strength][0];
    SetStiffness((double)k);
}

// Mac 0x3afe30, Win 0x525bb0
void VortexPhysicsConstraintHingeSpringHelper::Soften()
{
    if (m_constraint == 0)
        return;
    Real32 k = (Real32)m_constraint->getMotorStiffness(kHingeCoordinate) * 0.5f;
    if (k <= kHingeSpringStiffness[m_strength][1])
        k = kHingeSpringStiffness[m_strength][1];
    SetStiffness((double)k);
}

// Mac 0x3afea0, Win 0x525c20
void VortexPhysicsConstraintHingeSpringHelper::ResetStiffness()
{
    if (m_constraint == 0)
        return;
    SetStiffness((double)kHingeSpringStiffness[m_strength][0]);
}

// Mac 0x3afee0, Win 0x525c50
void VortexPhysicsConstraintHingeSpringHelper::Adapt()
{
    if (m_constraint == 0)
        return;
    Real32 angle = GetAngle();
    Real32 k;
    if (0.017453292f <= (Real32)fabs(-angle - m_targetAngle))
        k = kHingeSpringStiffness[m_strength][1];
    else
        k = kHingeSpringStiffness[m_strength][2];
    SetStiffness((double)k);
}

// Mac 0x3aff60, Win 0x525ce0
Real32 VortexPhysicsConstraintHingeSpringHelper::GetAngle() const
{
    Matrix3D transform;
    Matrix3D otherTransform;
    m_body->GetEngineTransform(transform);
    m_otherBody->GetEngineTransform(otherTransform);
    Vector3 axis = LEGO::Rotate(m_axis, transform);
    Vector3 normal = LEGO::Rotate(m_normal, transform);
    Vector3 otherNormal = LEGO::Rotate(m_otherNormal, otherTransform);
    Vector3 cross = normal ^ otherNormal;
    if (cross.IsAlmostZero()) {
        if (normal.IsEqual(otherNormal, 1e-05f))
            return 0.0f;
        return -3.1415927f;
    }
    Real32 side = cross * axis;
    Real32 c = normal * otherNormal;
    Real32 angle = 0.0f;
    if (!(c > 0.99999f)) {
        if (-0.99999f > c)
            angle = 3.1415927f;
        else
            angle = (Real32)acos((double)c);
    }
    return side < 0.0f ? -angle : angle;
}

// =============================================================================================
// VortexPhysicsConstraintBallSpringHelper
// =============================================================================================

// Mac 0x3b0b70, Win 0x526d30
VortexPhysicsConstraintBallSpringHelper::VortexPhysicsConstraintBallSpringHelper(
    UInt32 strength, const BaseBrickPhysicsConnectionHelper& ball, bool strong)
    : VortexPhysicsConstraintSpringHelper(strength, ball), m_strong(strong)
{
    Matrix3D transform;
    Matrix3D otherTransform;
    m_body->GetEngineTransform(transform);
    m_otherBody->GetEngineTransform(otherTransform);
    Vector3 axis = LEGO::Rotate(m_axis, transform);
    Vector3 normal = LEGO::Rotate(m_normal, transform);
    transform.SetInverse(otherTransform);
    m_otherAxis = LEGO::Rotate(axis, transform);
    m_otherNormal = LEGO::Rotate(normal, transform);
}

// Mac 0x3b2d00, Win 0x475ed0
UInt32 VortexPhysicsConstraintBallSpringHelper::GetType() const
{
    return 5;
}

// Mac 0x3b02c0, Win 0x52a700
void* VortexPhysicsConstraintBallSpringHelper::GetNativeConstraint()
{
    if (m_constraint != 0)
        return m_constraint->getUserData();
    return 0;
}

// Mac 0x3b2d10, Win 0x51cf20
bool VortexPhysicsConstraintBallSpringHelper::HasNativeConstraint()
{
    return m_constraint != 0;
}

// Mac 0x3b2d20, Win 0x4aede0
bool VortexPhysicsConstraintBallSpringHelper::Unknown3()
{
    return false;
}

// Mac 0x3b01e0, Win 0x526060
UInt32 VortexPhysicsConstraintBallSpringHelper::CheckSeparation(const PhysicsSeparationLimits& limits)
{
    Matrix3D transform;
    Matrix3D otherTransform;
    m_body->GetEngineTransform(transform);
    m_otherBody->GetEngineTransform(otherTransform);
    Vector3 position = m_position * transform;
    Vector3 otherPosition = m_otherPosition * otherTransform;
    Real32 distance = position.GetDistanceSquared(otherPosition);
    if (distance < limits.m_distance) {
        m_separationCount = 0;
        return 0;
    }
    if (distance < limits.m_maxDistance) {
        m_separationCount = 0;
        return 1;
    }
    ++m_separationCount;
    return limits.m_breakCount <= m_separationCount ? 2 : 1;
}

// Mac 0x3b02e0, Win 0x526150
bool VortexPhysicsConstraintBallSpringHelper::UpdateConstraint()
{
    Vx::VxVector3 position = ToVxVector3(m_position);
    Vx::VxVector3 otherPosition = ToVxVector3(m_otherPosition);
    Vx::VxVector3 axis = ToVxVector3(m_axis);
    Vx::VxVector3 otherAxis = ToVxVector3(m_otherAxis);
    Vx::VxVector3 normal = ToVxVector3(m_normal);
    Vx::VxVector3 otherNormal = ToVxVector3(m_otherNormal);
    Vx::VxRPRO* rpro = new Vx::VxRPRO();
    m_constraint = rpro;
    rpro->setPartAndAttachmentRel(0, static_cast<VortexPhysicsRigidElement*>(m_body)->m_part, position, axis, normal);
    m_constraint->setPartAndAttachmentRel(1, static_cast<VortexPhysicsRigidElement*>(m_otherBody)->m_part,
                                          otherPosition, otherAxis, otherNormal);
    static_cast<VortexPhysicsScene*>(m_body->m_scene)->m_mechanism->addConstraint(m_constraint);
    m_constraint->setUserData(static_cast<BaseBrickPhysicsConnectionHelper*>(this));
    double angular = m_strong ? (double)kBallSpringStiffness[m_strength][0] : 10.0;
    rpro->setAngularStrength(Vx::VxVector3(angular, angular, angular));
    rpro->setLinearStrength(Vx::VxVector3(1e10, 1e10, 1e10));
    return true;
}

// Mac 0x3b04f0, Win 0x525b00
void VortexPhysicsConstraintBallSpringHelper::DestroyConstraint()
{
    if (m_constraint == 0)
        return;
    static_cast<VortexPhysicsScene*>(m_body->m_scene)->m_mechanism->removeConstraint(m_constraint);
    delete m_constraint;
    m_constraint = 0;
}

// NOTE: Stiffen / Soften first set the angular strength to zero and scale that zero (the
// original calls the setter where it means the getter), so the RPRO ends with strength 0
// (Stiffen) or the row's minimum (Soften). Mac 0x3b0540, Win 0x526380
void VortexPhysicsConstraintBallSpringHelper::Stiffen()
{
    Vx::VxRPRO* rpro = static_cast<Vx::VxRPRO*>(m_constraint);
    Vx::VxVector3 strength(0.0, 0.0, 0.0);
    rpro->setAngularStrength(strength);
    strength[0] = strength[0] * 1.5;
    strength[1] = strength[1] * 1.5;
    strength[2] = strength[2] * 1.5;
    double maximum = (double)kBallSpringStiffness[m_strength][0];
    for (int i = 0; i < 3; ++i)
        if (maximum <= strength[i])
            strength[i] = maximum;
    rpro->setAngularStrength(strength);
}

// Mac 0x3b05e0, Win 0x526430
void VortexPhysicsConstraintBallSpringHelper::Soften()
{
    Vx::VxRPRO* rpro = static_cast<Vx::VxRPRO*>(m_constraint);
    Vx::VxVector3 strength(0.0, 0.0, 0.0);
    rpro->setAngularStrength(strength);
    double scaled[3] = {strength[0] * 0.5, strength[1] * 0.5, strength[2] * 0.5};
    double minimum = (double)kBallSpringStiffness[m_strength][1];
    for (int i = 0; i < 3; ++i)
        strength[i] = minimum <= scaled[i] ? scaled[i] : minimum;
    rpro->setAngularStrength(strength);
}

// NOTE: the original also rotates the frames and takes three dot products it never uses.
// Mac 0x3b0690, Win 0x5264e0
void VortexPhysicsConstraintBallSpringHelper::ResetStiffness()
{
    const Real32 dampingRatio = 0.1f;
    for (int equation = 3; equation <= 5; ++equation) {
        Real32 k = kBallSpringStiffness[m_strength][0];
        m_constraint->setRelaxationStiffnessAndDamping(equation, (double)k, (double)(k * dampingRatio));
    }
}

// Mac 0x3b08a0, Win 0x5268e0
void VortexPhysicsConstraintBallSpringHelper::Adapt()
{
    Matrix3D transform;
    Matrix3D otherTransform;
    m_body->GetEngineTransform(transform);
    m_otherBody->GetEngineTransform(otherTransform);
    Vector3 axis = LEGO::Rotate(m_axis, transform);
    Vector3 otherAxis = LEGO::Rotate(m_otherAxis, otherTransform);
    Vector3 normal = LEGO::Rotate(m_normal, transform);
    Vector3 otherNormal = LEGO::Rotate(m_otherNormal, otherTransform);
    Vector3 cross = m_axis ^ m_normal;
    Vector3 otherCross = m_axis ^ m_normal;
    Real32 sum = axis * otherAxis;
    sum = sum + normal * otherNormal;
    Real32 alignment = (sum + cross * otherCross) / 3.0f;
    if (alignment > 0.996f) {
        const Real32 dampingRatio = 20.0f;
        for (int equation = 3; equation <= 5; ++equation) {
            Real32 k = kBallSpringStiffness[m_strength][2];
            m_constraint->setRelaxationStiffnessAndDamping(equation, (double)k, (double)(k * dampingRatio));
        }
    } else {
        for (int equation = 3; equation <= 5; ++equation) {
            double k = (double)kBallSpringStiffness[m_strength][3];
            m_constraint->setRelaxationStiffnessAndDamping(equation, k, k);
        }
    }
}

// =============================================================================================
// PointGravitationAction
// =============================================================================================

// Mac 0x3af770, Win 0x5254b0
void PointGravitationAction::update(Vx::VxReal)
{
    double x = (double)m_point.x;
    double y = (double)m_point.y;
    double z = (double)m_point.z;
    Vx::VxPart* part = static_cast<VortexPhysicsRigidElement*>(m_element)->m_part;
    const Vx::VxTransform& tm = *part->mTransformPtr;
    double dx = (double)m_target.x - (((tm.m[1][0] * y + tm.m[0][0] * x) + tm.m[2][0] * z) + tm.m[3][0]);
    double dy = (double)m_target.y - (((tm.m[1][1] * y + tm.m[0][1] * x) + tm.m[2][1] * z) + tm.m[3][1]);
    double dz = (double)m_target.z - (((tm.m[1][2] * y + tm.m[0][2] * x) + tm.m[2][2] * z) + tm.m[3][2]);
    double distance = sqrt(dz * dz + (dy * dy + dx * dx));
    if (!(1e-14 < distance))
        return;
    double inverse = 1.0 / distance;
    double scale = (distance < 3.0 ? distance : 3.0) * 3.0 * m_strength;
    Vx::VxVector3 impulse(scale * (dx * inverse), scale * (dy * inverse), scale * (inverse * dz));
    part->addImpulse(impulse);
}

// =============================================================================================
// collectors
// =============================================================================================

// Mac 0x3b2db0, Win 0x424ec0
void LEGOUniverseCollector::notifyDisjoint(int, Vx::VxIntersectResult*)
{
}

// NOTE (both brick collectors): the original also builds a red / yellow debug line per contact
// (position, normal) that it never stores.
// Mac 0x3b2dc0, Win 0x521b60
void LEGOUniverseCollector::notifyIntersect(int, Vx::VxIntersectResult* result, Vx::VxDynamicsResponseInput*)
{
    if (m_collides)
        return;
    Vx::VxIntersectResult::ContactIterator end = result->contactEnd();
    for (Vx::VxIntersectResult::ContactIterator it = result->contactBegin(); it != end; it++) {
        Vx::VxIntersectResult::Contact contact = *it;
        if (!(m_tolerance < contact.getPenetration()))
            continue;
        VortexShapeUserData* shape = VortexCollisionGeometryData::GetShape(contact.getCollisionGeometry1());
        VortexShapeUserData* otherShape = VortexCollisionGeometryData::GetShape(contact.getCollisionGeometry2());
        if (shape != 0 && otherShape != 0 && shape->m_kind == VortexShapeUserData::KindBrick &&
            otherShape->m_kind == VortexShapeUserData::KindBrick &&
            m_callback->OnCollision(static_cast<VortexPhysicsBrickPlacedGeometry*>(shape)->m_identifier,
                                    static_cast<VortexPhysicsBrickPlacedGeometry*>(otherShape)->m_identifier) !=
                PhysicsCollisionCallback::Collision)
            continue;
        m_collides = true;
        return;
    }
}

// Mac 0x3b3720, Win 0x521e00
int LEGOIndividualCollector::notifyIntersect(Vx::VxCollisionGeometry*)
{
    if (m_collides)
        return 2;
    Vx::VxIntersectResult::ContactIterator end = mResult->contactEnd();
    for (Vx::VxIntersectResult::ContactIterator it = mResult->contactBegin(); it != end; it++) {
        Vx::VxIntersectResult::Contact contact = *it;
        if (!(m_tolerance < contact.getPenetration()))
            continue;
        VortexShapeUserData* shape = VortexCollisionGeometryData::GetShape(contact.getCollisionGeometry1());
        VortexShapeUserData* otherShape = VortexCollisionGeometryData::GetShape(contact.getCollisionGeometry2());
        if (otherShape != 0 && shape != 0 && m_callback != 0 && shape->m_kind == VortexShapeUserData::KindBrick &&
            otherShape->m_kind == VortexShapeUserData::KindBrick &&
            m_callback->OnCollision(static_cast<VortexPhysicsBrickPlacedGeometry*>(shape)->m_identifier,
                                    static_cast<VortexPhysicsBrickPlacedGeometry*>(otherShape)->m_identifier) !=
                PhysicsCollisionCallback::Collision)
            continue;
        m_collides = true;
        return 2;
    }
    return 0;
}

// Mac 0x3b3bc0, Win 0x523b20
int CastIndividualCollector::notifyIntersect(Vx::VxCollisionGeometry*)
{
    double time = mResult->getTimeOfImpact();
    m_timeOfImpact = time > m_timeOfImpact ? m_timeOfImpact : mResult->getTimeOfImpact();
    m_hit = true;
    return 2;
}

// Mac 0x3b3b80, Win 0x523b90
int TerrainRayCastCollector::filter(Vx::VxCollisionGeometry* geometry)
{
    VortexShapeUserData* shape = VortexCollisionGeometryData::GetShape(geometry);
    if (shape == 0 || shape->m_kind != VortexShapeUserData::KindTerrain)
        return 1;
    return 0;
}

} // namespace LEGO
