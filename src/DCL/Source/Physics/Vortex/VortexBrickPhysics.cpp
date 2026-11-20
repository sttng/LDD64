// LEGO Digital Designer — restored source
// LEGO::VortexBrickPhysics and LEGO::CreateBrickPhysics (Mac TU 422, Win 0x51d730-0x51e36f).
// See VortexBrickPhysics.h.
#include "VortexBrickPhysics.h"
#include "VortexPhysicsScene.h"
#include "VortexPhysicsRigidElement.h"
#include "VortexPhysicsPrimitiveElement.h"
#include "VortexPhysicsSceneryElement.h"
#include "VortexPhysicsGeometry.h"
#include "VortexPhysicsGeometryConfigable.h"
#include "VortexPhysicsFreeConnection.h"
#include "VortexPhysicsHingeConnection.h"
#include "VortexPhysicsSlideHingeConnection.h"
#include "VortexPhysicsBallConnection.h"

#include "Vx/VxFrame.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxMessage.h"

#include <stdarg.h>
#include <stddef.h>

namespace LEGO {

VortexBrickPhysics* VortexBrickPhysics::s_instance = 0;   // Mac 0x77f4f8, Win 0xb4276c
bool VortexBrickPhysics::s_created = false;               // Mac 0x77f4fc, Win 0xb42765

namespace {

// The Vortex message handlers CreateBrickPhysics installs: messages are dropped. Mac 0x39e8c0
// (info) / 0x39e8d0 (warning) / 0x39e8e0 (fatal error); Win one body 0x520a60 (ICF).
// (names inferred)
void MEAPI IgnoreVortexInfo(const int, const char*, va_list) {}
void MEAPI IgnoreVortexWarning(const int, const char*, va_list) {}
void MEAPI IgnoreVortexFatalError(const int, const char*, va_list) {}

// The geometry joins the factory's list (VxArray<VortexPhysicsGeometry*>::insert at the end,
// Mac 0x39f220, Win 0x51e160).
inline void AddGeometry(Vx::VxArray<VortexPhysicsGeometry*>& geometries, VortexPhysicsGeometry* geometry)
{
    geometries.insert(geometries.end(), geometry);
}

} // namespace

// Sets up the frame (objects / geometries are not registered by name, the collision types,
// no Vortex messages), then the shared factory: made on the first call, else referenced.
// Mac 0x39e7c0, Win 0x51d730
BrickPhysics* CreateBrickPhysics(const char* /*xml*/, UInt32 /*size*/)
{
    Vx::VxFrame::instance()->setRegisterNewObjects(false);
    Vx::VxCollisionGeometry::setRegisterWithGeometry(false);
    Vx::VxFrame::registerCollisionTypes(Vx::VxFrame::instance());
    Vx::VxSetInfoLevel(0);
    Vx::VxSetWarningLevel(0);
    Vx::VxSetInfoHandler(IgnoreVortexInfo);
    Vx::VxSetWarningHandler(IgnoreVortexWarning);
    Vx::VxSetFatalErrorHandler(IgnoreVortexFatalError);
    if (VortexBrickPhysics::s_instance == 0) {
        VortexBrickPhysics::s_created = true;
        VortexBrickPhysics::s_instance = new VortexBrickPhysics;
    } else {
        VortexBrickPhysics::s_instance->AddRef();
    }
    return VortexBrickPhysics::s_instance;
}

// Mac D1 0x39e9a0 (body 0x39e900) / D0 0x39e9b0, Win 0x51d8a0 / sdd 0x51d880
VortexBrickPhysics::~VortexBrickPhysics()
{
    s_instance = 0;
    if (s_created)
        s_created = false;
    if (Vx::VxFrame::currentInstance() != 0)
        Vx::VxFrame::destroy();
    // (m_geometries: vxFree of its storage, Mac 0x39e950)
}

// Mac 0x39e9f0, Win 0x459790
void VortexBrickPhysics::Unknown3()
{
}

// Mac 0x39ea00, Win 0x459790
void VortexBrickPhysics::Unknown4()
{
}

// Mac 0x39ea10, Win 0x51d940
PhysicsScene* VortexBrickPhysics::CreatePhysicsScene(UInt32 flags)
{
    Vx::VxUniverse* universe = reinterpret_cast<Vx::VxUniverse*>((size_t)flags);
    if (universe == 0) {
        universe = new Vx::VxUniverse(10000, 100000, false);
        Vx::VxFrame::instance()->addUniverse(universe);
        universe->setTimeStep(0.0);
        universe->setAutoDisable(false);
    }
    return new VortexPhysicsScene(this, universe);
}

// Mac 0x39eb10, Win 0x51da50
PhysicsRigidElement* VortexBrickPhysics::CreateRigidElement()
{
    return new VortexPhysicsRigidElement();
}

// Mac 0x39eb50, Win 0x51dab0
PhysicsPrimitiveElement* VortexBrickPhysics::CreatePrimitiveElement(const PhysicsPlacedConfiguredGeometry* geometries,
                                                                    UInt32 count,
                                                                    const PhysicsMassProperties& properties)
{
    return new VortexPhysicsPrimitiveElement(geometries, count, properties);
}

// Mac 0x39ebb0, Win 0x51db30
PhysicsSceneryElement* VortexBrickPhysics::CreateSceneryElement(const PhysicsPlacedConfiguredGeometry* geometries,
                                                                UInt32 count,
                                                                const PhysicsMassProperties& properties)
{
    return new VortexPhysicsSceneryElement(geometries, count, properties);
}

// Mac 0x39ec10, Win 0x51dba0
PhysicsGeometryBox* VortexBrickPhysics::CreateBox(const Vector3& halfSize)
{
    VortexPhysicsGeometryBox* box = new VortexPhysicsGeometryBox(halfSize);
    AddGeometry(m_geometries, box);
    return box;
}

// Mac 0x39ec80, Win 0x51dc40
PhysicsGeometryCylinder* VortexBrickPhysics::CreateCylinder(Real32 radius, const Real32& length)
{
    VortexPhysicsGeometryCylinder* cylinder = new VortexPhysicsGeometryCylinder(radius, length);
    AddGeometry(m_geometries, cylinder);
    return cylinder;
}

// Mac 0x39ed00, Win 0x51dcf0
PhysicsGeometrySphere* VortexBrickPhysics::CreateSphere(Real32 radius)
{
    VortexPhysicsGeometrySphere* sphere = new VortexPhysicsGeometrySphere(radius);
    AddGeometry(m_geometries, sphere);
    return sphere;
}

// Mac 0x39ed70, Win 0x51dd90
PhysicsGeometryCapsule* VortexBrickPhysics::CreateCapsule(Real32 radius, Real32 length)
{
    VortexPhysicsGeometryCapsule* capsule = new VortexPhysicsGeometryCapsule(radius, length);
    AddGeometry(m_geometries, capsule);
    return capsule;
}

// Mac 0x39edf0, Win 0x51de40
PhysicsGeometryConfigable* VortexBrickPhysics::CreateConfigurableGeometry()
{
    return new VortexPhysicsGeometryConfigable();
}

// Mac 0x39ee30, Win 0x51dea0
PhysicsGeometrySampledHeightField* VortexBrickPhysics::CreateSampledHeightField(const UIntVector2& size,
                                                                                const Vector3& scale, bool flag,
                                                                                const Real32* heights)
{
    return new VortexPhysicsGeometrySampledHeightField(size.x, size.y, scale, flag, heights);
}

// Mac 0x39eea0, Win 0x424e90 (returns NULL)
void VortexBrickPhysics::Unknown15()
{
}

// Mac 0x39eeb0, Win 0x51df20 (returns NULL)
void VortexBrickPhysics::Unknown16()
{
}

// Mac 0x39f130, Win 0x51e0f0
PhysicsConnection* VortexBrickPhysics::CreateFixedConnection(PhysicsRigidElement* body, PhysicsRigidElement* otherBody)
{
    return new VortexPhysicsFreeConnection(static_cast<BaseBrickPhysicsRigidElement*>(body),
                                           static_cast<BaseBrickPhysicsRigidElement*>(otherBody));
}

// Mac 0x39eec0, Win 0x51df30
PhysicsConnection* VortexBrickPhysics::CreateHingeConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                             const Vector3& normal, const Vector3& position,
                                                             PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                             const Vector3& otherNormal, const Vector3& otherPosition)
{
    return new VortexPhysicsHingeConnection(static_cast<BaseBrickPhysicsRigidElement*>(body), axis, normal, position,
                                            static_cast<BaseBrickPhysicsRigidElement*>(otherBody), otherAxis,
                                            otherNormal, otherPosition);
}

// NOTE: NULL in the shipped build. Mac 0x39ef80, Win 0x51dfc0
PhysicsConnection* VortexBrickPhysics::CreateSlideHingeConnection(PhysicsRigidElement*, const Vector3&, const Vector3&,
                                                                  const Vector3&, PhysicsRigidElement*, const Vector3&,
                                                                  const Vector3&, const Vector3&)
{
    return 0;
}

// Mac 0x39ef90, Win 0x51dfc0 (returns NULL)
void VortexBrickPhysics::Unknown20()
{
}

// Mac 0x39efa0, Win 0x51dfd0
PhysicsConnection* VortexBrickPhysics::CreateSlidingHingeConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                                    const Vector3& normal, const Vector3& position,
                                                                    PhysicsRigidElement* otherBody,
                                                                    const Vector3& otherAxis, const Vector3& otherNormal,
                                                                    const Vector3& otherPosition)
{
    return new VortexPhysicsSlideHingeConnection(static_cast<BaseBrickPhysicsRigidElement*>(body), axis, normal,
                                                 position, static_cast<BaseBrickPhysicsRigidElement*>(otherBody),
                                                 otherAxis, otherNormal, otherPosition);
}

// Mac 0x39f060, Win 0x51e060
PhysicsConnection* VortexBrickPhysics::CreateBallConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                            const Vector3& normal, const Vector3& position,
                                                            PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                            const Vector3& otherNormal, const Vector3& otherPosition)
{
    return new VortexPhysicsBallConnection(static_cast<BaseBrickPhysicsRigidElement*>(body), axis, normal, position,
                                           static_cast<BaseBrickPhysicsRigidElement*>(otherBody), otherAxis,
                                           otherNormal, otherPosition);
}

// Mac 0x39e8f0 (empty), Win inlined
void VortexBrickPhysics::UpdateUniverse(Vx::VxUniverse*)
{
}

} // namespace LEGO
