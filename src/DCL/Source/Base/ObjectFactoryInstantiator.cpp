// LEGO Digital Designer — restored source
// TU "ObjectFactoryInstantiator.cpp"  Mac 0x9470-0xaecf, Win 0x41c940-0x41d4ff (+ COMDATs)
// Templates emitted here (Mac): BasicMap<type_info*, RefPointer<AbstractObjectFactory>> D1/D0
//   0x9fb0/0xa030, _M_erase 0xa080, operator[] 0xa5b0 (+ 0xa6a0, 0xa8a0, 0xa980);
//   BasicMap<.., RefPointer<SpecificObjectFactory>> 0xa0e0/0xa110, 0xa160, 0xaa00 (+ 0xaaf0,
//   0xacf0, 0xadd0); registry map insert 0xa3e0, 0xa4a0; inline ObjectRegistry::Get / Register
//   0x9550 / 0xa280.
// NOTE: the Mac static initialiser (0xae50) also zero-fills a 24-byte header-defined static
// shared by most TUs; not part of this module.
#include "ObjectFactory.h"

namespace LEGO {

// ---------------------------------------------------------------------------
// ObjectFactory
// ---------------------------------------------------------------------------

// Mac 0x9470, Win 0x41cd50
RefPointer<ObjectFactory> ObjectFactory::GetInstance()
{
    ObjectFactoryInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryId, instantiator);
    return RefPointer<ObjectFactory>(dynamic_cast<ObjectFactory*>(object.Get()));
}

// Mac 0x9ee0, Win 0x41c940
ObjectFactory::ObjectFactory()
{
    m_mutex = new Mutex;
}

// Mac 0x9fe0 (D1) -> 0xa1c0 (D2), 0x9ff0 (D0); Win 0x41ca00, 0x41caa0 (sdd)
ObjectFactory::~ObjectFactory()
{
}

// Mac 0x96b0, Win 0x41ce30
Object* ObjectFactory::CreateObject(const std::type_info& type, const ObjectFactoryArgs& args)
{
    MutexLocker lock(m_mutex);

    SpecificObjectFactoryMap::Iterator specific = m_specificFactories.Find(&type);
    if (specific != m_specificFactories.End())
        return specific->second->CreateObject(args);

    AbstractFactoryMap::Iterator abstract = m_abstractFactories.Find(&type);
    if (abstract != m_abstractFactories.End())
        return abstract->second->CreateObject(type, args);

    return 0;
}

// Mac 0x9810, Win 0x41cfd0
bool ObjectFactory::RegisterFactory(const std::type_info& type, SpecificObjectFactory* factory)
{
    MutexLocker lock(m_mutex);

    if (m_specificFactories.Contains(&type))
        return false;
    if (m_abstractFactories.Contains(&type))
        return false;
    if (!factory->AddRegistration(m_mutex))
        return false;

    m_specificFactories[&type] = factory;
    return true;
}

// Mac 0x99d0, Win 0x41d180
bool ObjectFactory::RegisterFactory(const std::type_info& type, SpecificObjectFactory* factory,
                                    const std::type_info& abstractType)
{
    MutexLocker lock(m_mutex);

    if (m_specificFactories.Contains(&type))
        return false;
    if (m_abstractFactories.Contains(&type))
        return false;
    if (m_specificFactories.Contains(&abstractType))
        return false;
    if (!factory->AddRegistration(m_mutex))
        return false;

    AbstractObjectFactory* abstractFactory;
    AbstractFactoryMap::Iterator it = m_abstractFactories.Find(&abstractType);
    if (it == m_abstractFactories.End()) {
        RefPointer<AbstractObjectFactory>& slot = m_abstractFactories[&abstractType];
        slot = new AbstractObjectFactory;
        abstractFactory = slot;
    } else {
        abstractFactory = it->second;
    }

    // The concrete type is reachable through the abstract factory as well.
    m_abstractFactories[&type] = abstractFactory;
    abstractFactory->SetFactory(type, factory);
    return true;
}

// ---------------------------------------------------------------------------
// AbstractObjectFactory
// ---------------------------------------------------------------------------

// Mac 0x9e30 (D1) -> 0xa520 (D2), 0x9e40 (D0); Win 0x41cb40, 0x41cbd0 (sdd)
AbstractObjectFactory::~AbstractObjectFactory()
{
}

// Mac 0x9d60, Win 0x41d420
Object* AbstractObjectFactory::CreateObject(const std::type_info& type, const ObjectFactoryArgs& args)
{
    SpecificObjectFactoryMap::Iterator it = m_factories.Find(&type);
    if (it != m_factories.End())
        return it->second->CreateObject(args);

    for (it = m_factories.Begin(); it != m_factories.End(); ++it) {
        Object* object = it->second->CreateObject(args);
        if (object)
            return object;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// ObjectFactoryInstantiator
// ---------------------------------------------------------------------------

// Mac 0x96a0 (D1), 0x9e80 (D0); Win 0x41cbf0, 0x41cce0 (sdd)
ObjectFactoryInstantiator::~ObjectFactoryInstantiator()
{
}

// Mac 0x9e90, Win 0x41cc40
RefPointer<Object> ObjectFactoryInstantiator::CreateObject() const
{
    return new ObjectFactory;
}

} // namespace LEGO
