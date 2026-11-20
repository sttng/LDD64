// LEGO Digital Designer — restored source
// LEGO::ProxyFactory<T> — SpecificObjectFactory forwarding CreateObject for typeid(T) to another
//   ObjectFactory (RenderControlManager: Texture / VertexBuffer / Font / TextLayoutRender).
// Instances in TU 065: Mac 0x157b60-0x15809f, vtables 0x767e60 / 0x767eac / 0x767ef0 / 0x767f30;
//   Win CreateObject 0x50f855 / 0x50f8a1 / 0x50f8ed / 0x50f939, dtors 0x50f869 / 0x50f8b5 /
//   0x50f901 / 0x50f94d, sdd 0x50f9c3 / 0x50f9df / 0x50f9fb / 0x50fa17.
// Layout: target ObjectFactory* +0x10 (referenced), sizeof 0x14. Name from RTTI, rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include <typeinfo>

namespace LEGO {

template <class T>
class ProxyFactory : public SpecificObjectFactory {
public:
    explicit ProxyFactory(ObjectFactory* target) : m_target(target) {}
    virtual ~ProxyFactory() {}                                        // Mac 0x157b60 … (releases m_target)

    // m_target->CreateObject(typeid(T), args). Mac 0x157c80 …, Win 0x50f855 …
    virtual Object* CreateObject(const ObjectFactoryArgs& args)
    {
        return m_target->CreateObject(typeid(T), args);
    }

private:
    RefPointer<ObjectFactory> m_target;   // +0x10
};

} // namespace LEGO
