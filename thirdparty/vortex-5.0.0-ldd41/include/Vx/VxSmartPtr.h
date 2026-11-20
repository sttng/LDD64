// Vortex 5.0.0 build 800 "vortexldd41" - VxSmartPtr<T> (restored header).
// Intrusive pointer (one T*) to a VxRefCount object, using VxRefCount's inline addRef / release
// (deleting dtor = Mac slot 3). Seen inlined in Vortex (VxAssemblyBase Mac 0x427a68 / 0x42acd8) and
// LEGO code (VxGeometry refcount at Mac +0x24, Mac 0x3a7080).
// Name from MAK VR-Engage's "#include <Vx/VxSmartPtr.h>" (later Vortex); template body inferred.
#ifndef VX_VXSMARTPTR_H
#define VX_VXSMARTPTR_H

namespace Vx {

template <class T>
class VxSmartPtr {
public:
    VxSmartPtr() : mPtr(0) {}
    VxSmartPtr(T* p) : mPtr(p) { if (mPtr) mPtr->addRef(); }
    VxSmartPtr(const VxSmartPtr& o) : mPtr(o.mPtr) { if (mPtr) mPtr->addRef(); }
    ~VxSmartPtr() { if (mPtr) mPtr->release(); }
    VxSmartPtr& operator=(const VxSmartPtr& o) { return *this = o.mPtr; }
    VxSmartPtr& operator=(T* p)
    {
        if (mPtr != p) {
            T* old = mPtr;
            mPtr = p;
            if (mPtr)
                mPtr->addRef();
            if (old)
                old->release();
        }
        return *this;
    }
    T* get() const { return mPtr; }
    T* operator->() const { return mPtr; }
    T& operator*() const { return *mPtr; }
    operator T*() const { return mPtr; }
    bool operator==(const VxSmartPtr& o) const { return mPtr == o.mPtr; }
    bool operator!=(const VxSmartPtr& o) const { return mPtr != o.mPtr; }
    bool operator<(const VxSmartPtr& o) const { return mPtr < o.mPtr; }

    T* mPtr; // +0x00
};

} // namespace Vx

#endif
