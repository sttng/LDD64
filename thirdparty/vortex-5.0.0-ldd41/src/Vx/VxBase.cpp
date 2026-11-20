// Vortex 5.0.0 build 800 "vortexldd41" - VxRefCount, VxBase (restored).
// Mac TU 458 0x43046c-0x431ec4 (file guess Vx/VxBase.cpp); Win 0x72a0a0-0x72b800, 0x728eb0.
// The std::multimap / VxArray / VxSubscriberCollection template code of the TU (Mac
// 0x4316c4-0x431db0) is generated from the headers.
#include "Vx/VxBase.h"
#include "Vx/VxFrame.h"
#include <stdio.h>
#include <string.h>

// Win: the CRT's _snprintf (VS2008's CRT has no C99 snprintf; MinGW has both), as Me/MeMessage.c.
#if defined(_WIN32)
#define snprintf _snprintf
#endif

namespace Vx {

// ---------------------------------------------------------------------------------------------
// VxRefCount (the destructor is inline: Mac D1/D0 0x43176a/0x4316fc are weak copies)

// Mac 0x4304aa
int VxRefCount::getStaticClassType()
{
    return 0x102;
}

// Mac 0x4304b4, Win 0x72a220
bool VxRefCount::isOfClassType(int type) const
{
    return getStaticClassType() == type;
}

// Mac 0x4304ca, Win 0x72a0f0
bool VxRefCount::isOfType(int type) const
{
    return isOfClassType(type);
}

// ---------------------------------------------------------------------------------------------
// VxBase

int VxBase::sIDCounter = 0;                // Mac 0x7791ac, Win 0xaf1710
VxBase::Subscriber* VxBase::sCreationSubscriber = 0; // Mac 0x7791b0, Win 0xaf1714

// Mac 0x43046c
int VxBase::getStaticClassType()
{
    return 0x101;
}

// Mac 0x430476, Win 0x72a200
bool VxBase::isOfClassType(int type) const
{
    return getStaticClassType() == type;
}

// Mac 0x43048c, Win 0x72a0d0
bool VxBase::isOfType(int type) const
{
    return isOfClassType(type);
}

// Mac 0x4304e8, Win 0x72a100
void VxBase::resetIDCounter()
{
    sIDCounter = 0;
}

// operator delete Mac 0x430568 / Win 0x72a0a0, operator new Mac 0x43057f
VX_IMPLEMENT_ALLOCATOR(VxBase)

// Mac 0x430592, Win 0x72a110
void VxBase::setDefaultName(const char* prefix, const char* className, int id)
{
    char buf[256];
    strncpy(buf, prefix, 0xc5);
    buf[0xc5] = 0;
    size_t n = strlen(buf);
    buf[n] = '_';
    ++n;
    strncpy(buf + n, className, 0xf8 - n);
    buf[0xf8] = 0;
    n += strlen(buf + n);
    buf[n] = '_';
    ++n;
    if (id < 0)
        id = mID;
    snprintf(buf + n, 0xff - n, "%i", id);
    buf[0xff] = 0;
    setName(buf);
}

// Mac 0x4306b8, Win 0x72a240
const char* VxBase::getName() const
{
    if (mName.empty()) {
        VxBase* self = const_cast<VxBase*>(this);
        self->setDefaultName("default", getClassName(), -1);
    }
    return mName.c_str();
}

// Mac 0x430708, Win 0x72a310
const VxArray<int>& VxBase::getStaticClassTypes()
{
    static VxArray<int> sTypes(1, getStaticClassType());
    return sTypes;
}

// Mac 0x4307f0, Win 0x72a450
VxBase* VxBase::findFromName(const char* name, NameMap* nameMap)
{
    VxArray<VxBase*> found = findAllFromName(nameMap, name);
    int n = found.size();
    if (n > 1)
        VxWarning(0, "VxBase::findFromName found %d object named '%s'.\n", n, name);
    VxBase* result = 0;
    if (n != 0)
        result = found.begin()[0];
    return result;
}

// Mac 0x4308c4, Win 0x72a520
void VxBase::setName(const char* name, NameMap** nameMap)
{
    if (name == 0)
        name = "";
    std::string newName(name);
    if (nameMap == 0) {
        if (mName.compare(newName) != 0)
            mName = newName;
        return;
    }
    if (*nameMap != 0 && !mName.empty()) {
        NameMap::iterator last = (*nameMap)->upper_bound(mName);
        for (NameMap::iterator i = (*nameMap)->lower_bound(mName); i != last; ++i) {
            if (i->second == this) {
                (*nameMap)->erase(i);
                break;
            }
        }
    }
    if (mName.compare(newName) != 0)
        mName = newName;
    if (*nameMap == 0)
        *nameMap = new NameMap;
    if (!mName.empty())
        (*nameMap)->insert(NameMap::value_type(mName, this));
    if ((*nameMap)->empty()) {
        delete *nameMap;
        *nameMap = 0;
    }
}

// Mac 0x430d04, Win 0x72a6d0
void VxBase::setName(const char* name)
{
    setName(name, 0);
}

// Mac 0x430d26, Win 0x72a3b0
VxBase::VxBase()
    : mID(sIDCounter++)
{
    if (VxFrame::currentInstance()) {
        VxFrame::currentInstance()->addObject(this);
        if (sCreationSubscriber)
            sCreationSubscriber->notify(kEventCreated, this);
    }
}

// D2 Mac 0x4312b8 / Win 0x72a6f0 (D1 Mac 0x4316b0, D0 Mac 0x430eb6 / Win 0x72b800)
VxBase::~VxBase()
{
    mSubscribers.notify(kEventDestroyed, this);
    if (VxFrame::currentInstance())
        VxFrame::currentInstance()->removeObject(this);
    VxBase::setName((const char*)0);
}

// Mac 0x431a74, Win 0x72ace0.
// NOTE: the original dereferences a null map; it yields no object here.
VxArray<VxBase*> VxBase::findAllFromName(NameMap* nameMap, const char* name)
{
    VxArray<VxBase*> result;
    if (nameMap == 0)
        return result;
    std::string key(name);
    NameMap::iterator last = nameMap->upper_bound(key);
    for (NameMap::iterator i = nameMap->lower_bound(key); i != last; ++i)
        result.push_back(i->second);
    return result;
}

} // namespace Vx
