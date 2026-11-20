// Vortex 5.0.0 build 800 "vortexldd41" - VxIntersectSubscriberList, VxSubscriberListResponse,
// VxSubscriberListResponseHandler (restored header: Mac TU 500, Win
// 0x7428c0-0x742f90).
//
// The handler keeps one subscriber list per (type1, type2, event) - types are collision
// response models (VxCollisionGeometry::getResponseModel, the model's m80), events
// VxUniverse::IntersectSubscriber events (0 = still touching, 1 = staying (no test this step),
// 2 = first touch, 3 = separated). A list is itself a VxUniverse::IntersectSubscriber that
// forwards to its subscribers in order. Base of VxBridge (offset 0) and VxAssemblyBase (Mac
// +0x20, Win +0x38).
//
// Handler: Mac vtable 7 slots: 0/1 D1/D0 (0x45cb46 -> body 0x45cab4, 0x45ca1a; pure in
// VxAssemblyBase, which declares a pure virtual destructor), 2 0x45cc86, 3 0x45c7ac, 4 0x45c88e,
// 5 0x45c8c2, 6 0x45d318; Win 6 (0x742f70 sdd, 0x742eb0, 0x742c50, 0x742e60, 0x742cd0,
// 0x7428c0). 0x14 bytes on both. Ctor Mac 0x45c67e, Win 0x742be0. (method names inferred)
#ifndef VX_VXSUBSCRIBERLISTRESPONSEHANDLER_H
#define VX_VXSUBSCRIBERLISTRESPONSEHANDLER_H

#include "Vx/VxUniverse.h"
#include "Vx/VxArray.h"

namespace Vx {

// The optional filter of a VxSubscriberListResponse: Mac slots D1, D0, 2 = accept(result); Win
// sdd, 1 = accept. (class and method names inferred; the RTTI name of the base is
// Vx::VxAcceptNotificationBase.) Slot 3 = clone. The one implementation is
// VxAssemblyBase's (anonymous) AcceptIntersectPair (Mac vtable 0x75d178), set by
// VxAssemblyBase::createSubscriberList.
class VxIntersectResultFilter {
public:
    virtual ~VxIntersectResultFilter() {}
    virtual bool accept(VxIntersectResult* result) = 0;
    virtual VxIntersectResultFilter* clone() const = 0;  // slot 3 (V4.1)
};

// --------------------------------------------------------------------------------------
// VxIntersectSubscriberList (Mac vtable 0x75e048: D1 0x45c992 (body 0x45c926), D0 0x45c9a6,
// 2 0x45c56e, 3 0x45c5aa; Win 0x742f40 sdd, 0x742980, 0x7429b0). 0x14 bytes on both
// (Win new(0x14) 0x743500). Ctor Mac 0x45c540, Win 0x742b10. (method names inferred)
class VxIntersectSubscriberList : public VxUniverse::IntersectSubscriber {
public:
    VxIntersectSubscriberList() {}                      // Mac 0x45c540, Win 0x742b10 (out of line)
    virtual ~VxIntersectSubscriberList();               // Mac 0x45c992 / 0x45c9a6, Win 0x742d10 / 0x742f40
    // slot 2 / 3: every subscriber in order (Mac 0x45c56e / 0x45c5aa, Win 0x742980 / 0x7429b0)
    virtual void notifyDisjoint(int event, VxIntersectResult* result);
    virtual void notifyIntersect(int event, VxIntersectResult* result, VxDynamicsResponseInput* response);

    bool contains(VxUniverse::IntersectSubscriber* s) const;  // Mac 0x45c824, Win 0x742bb0
    bool remove(VxUniverse::IntersectSubscriber* s);          // Mac 0x45c748, Win 0x742b30
    int indexOf(VxUniverse::IntersectSubscriber* s) const;    // Win 0x742b70 (-1; dead-stripped on the Mac)
    // Mac 0x45cb5a, Win 0x742e70: before `index` (the end when index > size)
    void insert(VxUniverse::IntersectSubscriber* s, unsigned int index);
    bool empty() const { return mSubscribers.empty(); }

    VxArray<VxUniverse::IntersectSubscriber*> mSubscribers; // +0x04
};

// --------------------------------------------------------------------------------------
// VxSubscriberListResponse (Mac vtable 0x75e030: D1 0x45d2c4, D0 0x45d268, 2 0x45c5ee,
// 3 0x45c632; Win 0x725740 dtor, 0x7257b0 sdd, 0x7429f0, 0x742a30). 0x24 bytes: the list of the
// handler, keyed by (type1, type2, event), with an optional filter (deleted with the list).
class VxSubscriberListResponse : public VxIntersectSubscriberList {
public:
    VxSubscriberListResponse(int type1, int type2, int event)
        : mType1(type1), mType2(type2), mEvent(event), mFilter(0) {}
    virtual ~VxSubscriberListResponse();
    // slot 2 / 3: only when no filter is set or the filter accepts the result's geometries
    virtual void notifyDisjoint(int event, VxIntersectResult* result);
    virtual void notifyIntersect(int event, VxIntersectResult* result, VxDynamicsResponseInput* response);

    int mType1;                     // +0x14
    int mType2;                     // +0x18
    int mEvent;                     // +0x1c
    VxIntersectResultFilter* mFilter; // +0x20 (name inferred; set by VxAssemblyBase::createSubscriberList, V4.1)
};

// --------------------------------------------------------------------------------------
class VxSubscriberListResponseHandler {
public:
    VxSubscriberListResponseHandler() {}                // Mac 0x45c67e, Win 0x742be0
    virtual ~VxSubscriberListResponseHandler();         // slot 0/1 (D2 Mac 0x45cab4, Win 0x742df0)
    // slot 2 Mac 0x45cc86, Win 0x742eb0: the list for (type1, type2, event) (created through
    // slot 6 when missing), then the subscriber inserted at `index` unless already there
    virtual void addSubscriber(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s, unsigned int index);
    // slot 3 Mac 0x45c7ac, Win 0x742c50: true when it was in the list; an emptied list is
    // removed (slot 5) and deleted
    virtual bool removeSubscriber(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s);
    virtual void removeAllSubscribers();                // slot 4 Mac 0x45c88e (-> 0x45c85a), Win 0x742e60 (-> 0x742da0)
    virtual bool removeSubscriberList(VxSubscriberListResponse* l); // slot 5 Mac 0x45c8c2, Win 0x742cd0
    // slot 6 Mac 0x45d318, Win 0x7428c0: new VxSubscriberListResponse (new(0x24))
    virtual VxSubscriberListResponse* createSubscriberList(int type1, int type2, int event);

    // Mac 0x45c6ac, Win 0x742aa0: the list of (type1, type2) in either order and that event
    VxSubscriberListResponse* findSubscriberList(int type1, int type2, int event) const;
    // Win 0x742c00 (dead-stripped on the Mac): index of s in that list, -1
    int getSubscriberIndex(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s) const;
    bool hasSubscribers() const { return !mLists.empty(); }

    VxArray<VxSubscriberListResponse*> mLists;          // +0x04
};

} // namespace Vx

#endif
