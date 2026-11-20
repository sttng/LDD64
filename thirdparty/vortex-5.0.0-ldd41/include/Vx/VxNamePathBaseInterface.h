// Vortex 5.0.0 build 800 "vortexldd41" - VxNamePathBaseInterface and the interface registry
// (restored header; bodies in src/Vx/VxNamePathAssemblyInterface.cpp =
// Mac TU 505, the other interfaces in Vx/VxNamePathAssemblyInterface.h).
//
// A name-path interface tells the name-path code ("base://" paths) the parent and the children
// of an object of one class. The interfaces are registered per class in one global registry
// (Mac guard 0x779168, VxArray at 0x779170: {type_info*, interface getter, type test}); each
// registering TU adds its entry in its static initialiser and removes it at exit
// ("Could not find registered factory for type %s\r\n" when missing).
#ifndef VX_VXNAMEPATHBASEINTERFACE_H
#define VX_VXNAMEPATHBASEINTERFACE_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxMessage.h"
#include <typeinfo>

namespace Vx {

// VxNamePathBaseInterface (Mac 4 slots; Win 3) (method names inferred)
class VxNamePathBaseInterface {
public:
    virtual ~VxNamePathBaseInterface();                                         // slot 0/1 Mac 0x4646d6 / 0x464692 (D2 0x4646bc), Win 0x7fa410 (ICF)
    virtual VxBase* getParent(VxBase* object) const;                            // slot 2 Mac 0x464682, Win 0x822710 (0)
    virtual void getChildren(VxBase* object, VxArray<VxBase*>& children) const; // slot 3 Mac 0x46468c, Win 0x822720 (none)
};

// one registry entry (Mac 12 bytes) (names inferred)
struct VxNamePathInterfaceEntry {
    const std::type_info* type;
    VxNamePathBaseInterface* (*getInterface)();
    bool (*isOfType)(VxBase* object);
};

// the registry (a function-local static shared by all registering TUs) (name inferred)
inline VxArray<VxNamePathInterfaceEntry>& vxNamePathInterfaces()
{
    static VxArray<VxNamePathInterfaceEntry> sEntries;
    return sEntries;
}

// The registered getter of interface I: a function-local static I (one function per interface
// class, shared by every type registered with it: VxMechanism and VxAssemblyBase both use the
// assembly interface's, Mac 0x466319) (V4.6; name inferred)
template <class I>
VxNamePathBaseInterface* vxNamePathInterfaceInstance()
{
    static I sInterface;
    return &sInterface;
}

// The registered type test of class T: a dynamic_cast from VxBase (one function per type, e.g.
// Mac 0x466632 / Win 0x822f00 for VxFrame) (V4.6; name inferred)
template <class T>
bool vxNamePathIsOfType(VxBase* object)
{
    return object != 0 && dynamic_cast<T*>(object) != 0;
}

// Static registrar for the interface I of class T (one static object per registration; the
// original inlines the push_back into the TU's static initialiser and registers the removal
// with atexit, e.g. Mac 0x46667d / 0x4650a6) (name inferred).
template <class T, class I>
class VxNamePathInterfaceRegistration {
public:
    VxNamePathInterfaceRegistration()
    {
        VxNamePathInterfaceEntry e;
        e.type = &typeid(T);
        e.getInterface = &vxNamePathInterfaceInstance<I>;
        e.isOfType = &vxNamePathIsOfType<T>;
        vxNamePathInterfaces().push_back(e);
    }
    ~VxNamePathInterfaceRegistration()
    {
        VxArray<VxNamePathInterfaceEntry>& l = vxNamePathInterfaces();
        for (VxNamePathInterfaceEntry* i = l.begin(); i != l.end(); ++i) {
            if (*i->type == typeid(T) && i->getInterface == &vxNamePathInterfaceInstance<I>) {
                for (VxNamePathInterfaceEntry* j = i + 1; j != l.end(); ++j)
                    *(j - 1) = *j;
                l.pop_back();
                return;
            }
        }
        VxWarning(1, "Could not find registered factory for type %s\r\n", typeid(T).name());
    }
    static VxNamePathBaseInterface* getInterface() { return vxNamePathInterfaceInstance<I>(); }
    static bool isOfType(VxBase* object) { return vxNamePathIsOfType<T>(object); }
};

} // namespace Vx

#endif
