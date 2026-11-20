// Vortex 5.0.0 build 800 "vortexldd41" - collision responses (restored header).
// src/Vx/VxResponse.cpp = Mac TU 514 (0x476a32-0x477fa7, Win 0x74c2b0-0x74cf80),
// src/Vx/VxResponseTypes.cpp = Mac TU 515 (the out-of-line destructors, 0x477fa8-0x478500).
//
// VxBridge keeps a symmetric table of VxResponseEntry indexed by the two geometries' response
// models (model m80; VxCollisionSensor::getResponseModel). An entry has the Vinc pair request
// of that pair of models and three refcounted handlers (VxSmartPtr):
//   hello   - a broad-phase pair appears (default without one: VincHello)
//   active  - after every pair test of a pair whose models are both enabled (event 0 / 2 / 3)
//   goodbye - the pair disappears or one model got disabled
// VxUniverse's constructor (Mac 0x4848fa) fills the table:
//   (0, 0) body-body: StartBody_Body / Body_Body / EndBody_Body
//   (1, 0) sensor-body: Start_Sensor_Body; (1, 1) sensor-sensor: Start_Sensor_Sensor;
//   (2, 1) particle system-sensor: Start_Sensor_PS; contact max count 0 for (0, 1) and (1, 1).
// Response model 0 = body (part), 1 = sensor, 2 = particle system (names inferred from the
// class names). All class names from RTTI; method names inferred.
#ifndef VX_VXRESPONSE_H
#define VX_VXRESPONSE_H

#include "Vx/VxBase.h"
#include "Vx/VxSmartPtr.h"
#include "Vx/VxCollisionPairRequest.h"
#include "Mdt/MdtTypes.h"
#include "Vinc/VincTypes.h"

#include <stdlib.h>

struct McdModelPair;
struct McdIntersectResult;
typedef struct CxSpace CxSpace;

namespace Vx {

class VxBridge;
struct VxResponseEntry;

// Response model constants (Mac 0x64432c = 1 is read by VxBridge for the sensor-sensor test)
// (names inferred)
enum VxResponseModel {
    kVxResponseModelBody = 0,
    kVxResponseModelSensor = 1,
    kVxResponseModelParticleSystem = 2
};

// --------------------------------------------------------------------------------------
// The three handler interfaces (Mac: VxRefCount slots 0/1, D1/D0, 4 = respond; Win: 0/1, sdd,
// 3 = respond). Their destructors are inline (emitted with VxUniverse, Mac 0x48618a /
// 0x4860a6 / 0x48561c). Mac vtables 0x75eda0 / 0x75edc8 / 0x75edf0.
class VxResponseHello : public VxRefCount {
public:
    virtual ~VxResponseHello() {}
    virtual void respond(VxBridge* bridge, McdModelPair* pair, CxSpace* space, MdtWorldID world,
                         VxResponseEntry* entry) = 0;
};

class VxResponseActive : public VxRefCount {
public:
    virtual ~VxResponseActive() {}
    virtual void respond(VxBridge* bridge, McdModelPair* pair, McdIntersectResult* result, CxSpace* space,
                         MdtWorldID world, VxResponseEntry* entry) = 0;
};

class VxResponseGoodbye : public VxRefCount {
public:
    virtual ~VxResponseGoodbye() {}
    virtual void respond(VxBridge* bridge, McdModelPair* pair, CxSpace* space, MdtWorldID world,
                         VxResponseEntry* entry) = 0;
};

// --------------------------------------------------------------------------------------
// 16 bytes on both (VxBridge: vxAlloc(n * 0x10)) (name inferred). Copying keeps the original
// order: the request copy-constructed first, assignment releases each old handler before it
// takes the new one and copies the request last (VxBridge Mac 0x435148).
struct VxResponseEntry {
    VxResponseEntry() {}
    VxResponseEntry(const VxResponseEntry& o) : mRequest(o.mRequest), mHello(o.mHello), mActive(o.mActive), mGoodbye(o.mGoodbye) {}
    // the handlers are released in field order, then the request (Mac 0x435bec)
    ~VxResponseEntry()
    {
        release(mHello);
        release(mActive);
        release(mGoodbye);
    }
    template <class T> static void release(VxSmartPtr<T>& p)
    {
        if (p.mPtr)
            p.mPtr->release();
        p.mPtr = 0;
    }
    VxResponseEntry& operator=(const VxResponseEntry& o)
    {
        if (this != &o) {
            assign(mHello, o.mHello);
            assign(mActive, o.mActive);
            assign(mGoodbye, o.mGoodbye);
            mRequest = o.mRequest;
        }
        return *this;
    }
    template <class T> static void assign(VxSmartPtr<T>& d, const VxSmartPtr<T>& s)
    {
        if (d.mPtr)
            d.mPtr->release();
        d.mPtr = s.mPtr;
        if (d.mPtr)
            d.mPtr->addRef();
    }

    VxCollisionPairRequest mRequest;        // +0x00
    VxSmartPtr<VxResponseHello> mHello;     // +0x04
    VxSmartPtr<VxResponseActive> mActive;   // +0x08
    VxSmartPtr<VxResponseGoodbye> mGoodbye; // +0x0c
};

// --------------------------------------------------------------------------------------
// The handlers (8 bytes each; the VxUniverse ctor news them). Mac vtables: StartBody_Body
// 0x75e728, Body_Body 0x75e708, EndBody_Body 0x75e6e8, Start_Sensor_Body 0x75e6c8,
// Start_Sensor_Sensor 0x75e688, Start_Sensor_PS 0x75e6a8. Win: every dtor is the shared sdd
// 0x70c810 (ICF).

// hello of two bodies: the pair request of the geometry type IDs (model m7c), no contacts yet
class VxResponseStartBody_Body : public VxResponseHello {
public:
    virtual ~VxResponseStartBody_Body();                // Mac 0x4781ea / 0x4782ce
    virtual void respond(VxBridge* bridge, McdModelPair* pair, CxSpace* space, MdtWorldID world,
                         VxResponseEntry* entry);       // Mac 0x476d4c, Win 0x74c2b0
};

// active of two bodies: keeps the pair's Mdt contacts (pair +0x24) in step with the Vinc
// contacts (reusing them and their friction state), calls the bodies' contact callback
class VxResponseBody_Body : public VxResponseActive {
public:
    virtual ~VxResponseBody_Body();                     // Mac 0x478180 / 0x478106
    virtual void respond(VxBridge* bridge, McdModelPair* pair, McdIntersectResult* result, CxSpace* space,
                         MdtWorldID world, VxResponseEntry* entry); // Mac 0x476df0, Win 0x74c6e0
};

// goodbye of two bodies: destroys the pair's Mdt contacts (waking the bodies)
class VxResponseEndBody_Body : public VxResponseGoodbye {
public:
    virtual ~VxResponseEndBody_Body();                  // Mac 0x47809c / 0x478022
    virtual void respond(VxBridge* bridge, McdModelPair* pair, CxSpace* space, MdtWorldID world,
                         VxResponseEntry* entry);       // Mac 0x476cf0, Win 0x74c5c0
};

// hellos of sensor pairs: the entry's request, no contacts
class VxResponseStart_Sensor_Body : public VxResponseHello {
public:
    virtual ~VxResponseStart_Sensor_Body();             // Mac 0x4783b2 / 0x478254
    virtual void respond(VxBridge* bridge, McdModelPair* pair, CxSpace* space, MdtWorldID world,
                         VxResponseEntry* entry);       // Mac 0x476cc8, Win 0x74c610
};
class VxResponseStart_Sensor_Sensor : public VxResponseHello {
public:
    virtual ~VxResponseStart_Sensor_Sensor();           // Mac 0x47841c / 0x477fa8
    virtual void respond(VxBridge* bridge, McdModelPair* pair, CxSpace* space, MdtWorldID world,
                         VxResponseEntry* entry);       // Mac 0x476da0, Win 0x74c650
};
class VxResponseStart_Sensor_PS : public VxResponseHello {
public:
    virtual ~VxResponseStart_Sensor_PS();               // Mac 0x478348 / 0x478486
    virtual void respond(VxBridge* bridge, McdModelPair* pair, CxSpace* space, MdtWorldID world,
                         VxResponseEntry* entry);       // Mac 0x476dc8, Win 0x74c630
};

// Body_Body keeps the friction state of up to 99 old contacts (Win: the global byte 0xabc5d8,
// initialised 1 and never written; the Mac build folded it to the constant) (name inferred)
extern bool gVxResponseKeepAllContacts;

// Random helpers emitted (weak, coalesced) into Mac TU 514; used by the fluid volumes (
// Mac TU 510) (names inferred). The divisor is the C library's RAND_MAX: 2147483647 on the Mac,
// 32767 on Win (Win 0x7e3b60 / 0x7e3b80 divide by 32767.0).
inline VxReal vxRandom(VxReal max)                     // Mac 0x476a32, Win 0x7e3b60
{
    return ((VxReal)rand() / (VxReal)RAND_MAX) * max;
}
inline VxReal vxRandomSymmetric(VxReal half)           // Mac 0x476a6d, Win 0x7e3b80: [-half, half]
{
    return vxRandom(half + half) - half;
}

} // namespace Vx

#endif
