// Vortex 5.0.0 build 800 "vortexldd41" - CxSpace: the broad-phase interface behind a VincSpace
// (restored; Mac 558d, Win 0x78dc10-0x790570).
//
// A pure interface: RTTI names it the only base of CxSmallSort, but neither binary has a CxSpace
// vtable (all 31 virtuals pure). No virtual destructor: slot 0 is destroy() (`delete this`). The
// VincSpace C API (Vinc/VincSpace.h) keeps the CxSpace* and handle in McdModel::space /
// spaceHandle; VxUniverse keeps the space at +0x60 (Mac) / +0x7c (Win).
// Slot order = declaration order (Mac/GCC); MSVC reverses overload groups, so the two
// setPairEnabled overloads are Mac slots 25 / 26, Win 26 / 25 (the only difference). Names inferred
// except where a VincSpace string names the operation ("VincSpaceBeginChanges",
// "VincSpaceEndChanges", "VincSpaceIsChanging", "VincSpacePathUpdate", VincError ids 9 / 11 / 24 /
// 25 / 36 / 37 / 38).
#ifndef CX_CXSPACE_H
#define CX_CXSPACE_H

#include "Me/MeMath.h"
#include "Me/MeCall.h"

struct McdModelPair;

// The bounds callback (VincModelUpdate for a VincSpace): AABB of `object` swept over dt.
typedef void (MEAPI *CxBoundsFn)(void* object, MeReal* minCorner, MeReal* maxCorner, MeReal dt);
// The bounding-sphere callback (VincModelGetBSphere for a VincSpace); CxSmallSort ignores it.
typedef void (MEAPI *CxSphereFn)(void* object, MeReal* center, MeReal* radius);

// A doubly linked list node / list head (sentinel). Every list of the broad phase is circular
// with a sentinel head. (name inferred)
struct CxLink {
    CxLink* next;                    // +0x00
    CxLink* prev;                    // +0x04
};

class CxSpace {
public:
    virtual void destroy() = 0;                                        //  0 Mac 0x53e7e6, Win 0x78e750
    virtual int getMaxObjectCount() = 0;                               //  1 Mac 0x53b638, Win 0x78e010
    virtual int getMaxPairCount() = 0;                                 //  2 Mac 0x53b62c, Win 0x78e000
    virtual int getFreeCount() = 0;                                    //  3 Mac 0x53b3de, Win 0x78dea0
    // a new handle for `object`, sorted into the axis lists unless `unsorted` (0 on failure)
    virtual int insert(void* object, bool unsorted) = 0;               //  4 Mac 0x53cfa2, Win 0x78f150
    virtual int getEnabledPairCount() = 0;                             //  5 Mac 0x53b8f6, Win 0x78df80
    virtual bool remove(int handle) = 0;                               //  6 Mac 0x53c628, Win 0x78e410
    // take the object out of the axis lists (its pairs end) / sort it in again
    virtual void setUnsorted(int handle, bool unsorted) = 0;           //  7 Mac 0x53cf66, Win 0x78f200
    virtual void setUserData(void* data) = 0;                          //  8 Mac 0x53b3ea, Win 0x78df00
    virtual void* getObject(int handle) = 0;                           //  9 Mac 0x53fbce, Win 0x790050
    virtual void* getUserData() = 0;                                   // 10 Mac 0x53b3fc, Win 0x78df10
    // active = not frozen: updateAll() updates only active objects
    virtual bool setActive(int handle, bool active) = 0;               // 11 Mac 0x53ba64, Win 0x78e020
    virtual bool isActive(int handle) = 0;                             // 12 Mac 0x53b98e, Win 0x78deb0
    virtual void setBoundsFn(CxBoundsFn fn) = 0;                       // 13 Mac 0x53b442, Win 0x78df60
    virtual void setSphereFn(CxSphereFn fn) = 0;                       // 14 Mac 0x53b454, Win 0x78df70
    // update one object inside its own begin/endChanges
    virtual bool updateNow(int handle, MeReal dt) = 0;                 // 15 Mac 0x53d350, Win 0x78eab0
    virtual bool beginChanges() = 0;                                   // 16 Mac 0x53b644, Win 0x78e4c0
    virtual bool endChanges() = 0;                                     // 17 Mac 0x53b45a, Win 0x78e0c0
    virtual bool update(int handle, MeReal dt, bool force) = 0;        // 18 Mac 0x53d072, Win 0x78eb20
    virtual bool updateAll(MeReal dt) = 0;                             // 19 Mac 0x53c700, Win 0x78eb80
    virtual bool isValid(int handle) = 0;                              // 20 Mac 0x53b8b6, Win 0x78e080
    // pair lists (CxLink heads; the VincModelPair follows the link at +0x10)
    virtual CxLink* getHelloList() = 0;                                // 21 Mac 0x53b40a, Win 0x78df20
    virtual CxLink* getStayingList() = 0;                              // 22 Mac 0x53b418, Win 0x78df30
    virtual CxLink* getGoodbyeList() = 0;                              // 23 Mac 0x53b426, Win 0x78df40
    virtual CxLink* getRemovedList() = 0;                              // 24 Mac 0x53b434, Win 0x78df50
    virtual bool setPairEnabled(int handle1, int handle2, bool enable) = 0; // 25 Mac 0x53d634, Win slot 26 0x78ec30
    virtual bool setPairEnabled(int handle, bool enable) = 0;         // 26 Mac 0x53b9e2, Win slot 25 0x78e0e0
    virtual bool isPairEnabled(int handle1, int handle2) = 0;          // 27 Mac 0x53fdde, Win 0x790070
    virtual bool isPairOverlapping(int handle1, int handle2) = 0;      // 28 Mac 0x53fd34, Win 0x7900b0
    virtual McdModelPair* getPair(int handle1, int handle2) = 0;       // 29 Mac 0x53c27e, Win 0x78e4f0
    virtual bool isChanging() = 0;                                     // 30 Mac 0x53fbe2, Win 0x790040
};

#endif
