// Vortex 5.0.0 build 800 "vortexldd41" - VxBridge: the universe's link between collision
// (Vinc space, pair tests) and dynamics (Mdt contacts) (restored header).
// src/Vx/VxBridge.cpp = Mac TU 462 (0x432e8e-0x437019), Win 0x740750-0x742960.
//
// One per VxUniverse (VxUniverse::mIntersectManager, Mac +0x64 / Win +0x80; new(0x64) +
// VxBridge(materialTable, space, world, 0x401) in the VxUniverse ctor, then
// setPairContainerSize(maxPairCount)). Every step VxUniverse calls update(dt) (Mac 0x43454e):
// the broad-phase pairs come in chunks through a VincModelPairContainer
//   [0, goodbyeEnd)         pairs that disappeared   -> goodbye handlers, event 3
//   [helloFirst, middle)    new pairs                -> hello handlers (or VincHello)
//   [helloFirst, helloEnd)  new + still active pairs -> pair test, active handlers, events 0/2/3
//   [stayingFirst, stayingEnd) pairs not tested (frozen) -> event 1 subscribers only
// and the subscriber lists of the base class (keyed by response models) are notified.
//
// Mac vtable 0x75d468 (7 slots: D1 0x436194 (D2 0x435ec6), D0 0x435bec, 2 0x432f8c,
// 3 0x432f08, 4 0x432eda, 5/6 inherited); Win 0x742950 sdd, 0x740cf0, 0x740d40, 0x740da0.
// Mac 0x64 bytes, Win 0x68 (new(0x68) Win 0x709280). Method names inferred except the two in strings
// ("VxBridge::setContactBufferSize", "VxBridge::handleCollisions").
#ifndef VX_VXBRIDGE_H
#define VX_VXBRIDGE_H

#include "Vx/VxSubscriberListResponseHandler.h"
#include "Vx/VxResponse.h"
#include "Vx/VxMaterial.h"
#include "Vx/VxIntersect.h"
#include "Mdt/MdtTypes.h"

struct VinPool;
struct McdModelPair;
struct McdIntersectResult;
struct VincModelPairContainer;
typedef struct CxSpace CxSpace;

namespace Vx {

class VxCollisionGeometry;
class VxMaterialTable;
class VxContactMaterial;
class VxMaterial;
class VxPart;

class VxBridge : public VxSubscriberListResponseHandler {
public:
    // C1 Mac 0x436f74 -> C2 0x4361a8, Win 0x7416b0: response table 6 x 6 of empty entries,
    // pair-request table 7 x 7 with the per-type defaults (groups / contact counts), the
    // contact buffer pool ("bridge contact pool": 2 buffers of contactBufferSize contacts) and
    // a pair container of 1000 pairs
    VxBridge(VxMaterialTable* materials, CxSpace* space, MdtWorldID world, int contactBufferSize);
    virtual ~VxBridge();                                // Mac 0x436194 / 0x435bec, Win 0x741e20 / 0x742950

    // slots 2-4: the base behaviour plus the counts of event-1 and sensor-sensor subscriptions
    virtual void addSubscriber(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s, unsigned int index); // Mac 0x432f8c, Win 0x740cf0
    virtual bool removeSubscriber(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s); // Mac 0x432f08, Win 0x740d40
    virtual void removeAllSubscribers();                // Mac 0x432eda, Win 0x740da0

    // --- contact buffers ---
    int getContactBufferSize() const;                   // Mac 0x432e8e, Win 0x740840 (contacts per pool struct)
    int getContactBufferCount() const;                  // Mac 0x432eaa, Win 0x740850 (pool block count)
    // Mac 0x4333da, Win 0x740860 ("VxBridge::setContactBufferSize: some element are still in used")
    void setContactBufferSize(int contactCount, int bufferCount);
    void setPairContainerSize(unsigned int size);       // Mac 0x43339e, Win 0x7408b0 (grows only)

    // --- tables ---
    // Mac 0x43305c / 0x4330fe (two copies), Win 0x7423a0 (+ thunks 0x740df0 / 0x740e00)
    VxResponseEntry& getResponseEntry(unsigned int model1, unsigned int model2);
    // Mac 0x434626, Win 0x742420 (+ thunks 0x740e10 / 0x740e20) (VxUniverse::getCollisionPairRequest)
    VxCollisionPairRequest& getPairRequest(unsigned int type1, unsigned int type2);
    // Mac 0x435148, Win 0x742640: entry (m1, m2) = e; a table too small grows to max(m1, m2) + 1
    // with every new entry = e
    void setResponseEntry(unsigned int model1, unsigned int model2, const VxResponseEntry& e);
    // Mac 0x4355e4, Win 0x741a60 (VxUniverse::registerResponseModel): a new model n = the old
    // size, its entries copies of (0, 0); returns n
    unsigned int registerResponseModel(unsigned int copyFrom);
    // Win 0x741bd0 (dead-stripped on the Mac): grows the response table to n + 1 empty entries
    bool setResponseModelCount(unsigned int n);
    // Mac 0x433890, Win 0x740e30: entries (m, m) and (m, n) share their active handler
    bool canChangeResponseModel(unsigned int oldModel, unsigned int newModel);
    // Mac 0x434886, Win 0x741c60 (VxUniverse::setCollisionGeometryTypeCount): grows the
    // pair-request table to n + 1 (new entries copies of (1, 1)) and sets the type defaults of
    // the new columns of rows 3, 4, 0, 2; false when it was big enough
    bool setCollisionGeometryTypeCount(unsigned int n);
    // Mac 0x4346c8, Win 0x741030: after a geometry's type ID changed, every pair of its model
    // with a dynamic part's geometry gets the request of the new type pair
    void updateCollisionGeometryType(VxCollisionGeometry* cg);
    VxContactMaterial* getContactMaterial(VxMaterial* m1, VxMaterial* m2); // Mac 0x433344, Win 0x7408f0

    // --- stepping ---
    // Mac 0x43454e, Win 0x741980: hello / goodbye processing and the pair tests of one step
    void update(VxReal dt);
    // Mac 0x433efc, Win 0x741b20: hello / goodbye processing only (pairs refreshed after
    // changes; VxIntersect::intersect, VxUniverse when removing geometries)
    void updatePairs();
    // Mac 0x43344e, Win 0x740960: the pair tests of one geometry (VxIntersect::intersect):
    // 0 = no contact, 1 = touched (and stopped), 2 = touched, the subscriber wanted more
    int intersect(VxCollisionGeometry* cg, VxReal dt, VxIntersect::IntersectSubscriber* subscriber);
    // Mac 0x433368, Win 0x740dc0: VxDynamicsContact::setEnabled(false) on the user contacts
    // (VxUniverse step end)
    void disableUserContacts();
    // Mac 0x433002, Win 0x741dc0: wake, disable and destroy the user contacts
    void destroyUserContacts();
    // Win 0x741ed0 (dead-stripped on the Mac): a user contact between two parts' bodies
    MdtContactID createUserContact(VxPart* part1, VxPart* part2, VxContactMaterial* material,
                                   const VxVector3& position, const VxVector3& normal, VxReal penetration);
    // Win 0x741b80 (dead-stripped on the Mac): removes and destroys one user contact
    void destroyUserContact(MdtContactID c);

    // Mac 0x4331a0, Win 0x740750: the pair filter (geometry filters, sensors, enabled / frozen
    // bodies)
    static bool canCollide(McdModel* m1, McdModel* m2, bool sensorSensor, bool sleeping);

    void setSleepingPairsIntersect(bool b) { mSleepingPairsIntersect = b; } // VxUniverse Mac 0x4808b8
    void setNotifyEnabled(bool b) { mNotifyEnabled = b; }                   // VxUniverse Mac 0x482b0a
    VxReal getSafeTime() const { return mSafeTime; }
    CxSpace* getSpace() const { return mSpace; }
    MdtWorldID getWorld() const { return mWorld; }

    // event 2 / 0 / 3 of one tested pair: active handler, then the subscribers (Mac 0x4338e2,
    // Win 0x741240)
    void dispatch(McdModelPair* pair, McdIntersectResult* result, int event);
    void handleGoodbyesAndHellos();                      // Mac 0x433acc, Win 0x740e70
    void handleCollisions(VxReal dt);                    // Mac 0x433f6a, Win 0x741360

    bool mHasStayingSubscribers;        // +0x14 event-1 subscriptions exist (name inferred)
    bool mHasSensorSensorSubscribers;   // +0x15 (1, 1) subscriptions exist (name inferred)
    bool mSleepingPairsIntersect;       // +0x16 (name inferred)
    int mStayingSubscriberCount;        // +0x18
    int mSensorSensorSubscriberCount;   // +0x1c
    VxMaterialTable* mMaterialTable;    // +0x20 (VxFrame's)
    VxSymmetricTable<VxResponseEntry> mResponseTable;        // +0x24
    VxSymmetricTable<VxCollisionPairRequest> mPairRequests;  // +0x2c
    VinPool* mContactPool;              // +0x34 contact buffers (operator new(0x34))
    unsigned int mPairContainerSize;    // +0x38
    VincModelPairContainer* mPairs;     // +0x3c
    CxSpace* mSpace;                    // +0x40
    MdtWorldID mWorld;                  // +0x44
    VxReal mSafeTime;                   // +0x48 smallest time of impact of the step (starts at dt)
    VxArray<MdtContactID> mUserContacts; // +0x50
    bool mNotifyEnabled;                // +0x60 (true; VxUniverse clears it around its own passes)
};

} // namespace Vx

#endif
