// Vortex 5.0.0 build 800 "vortexldd41" - VxDynamicsResponseInput, VxDynamicsContactInput
// (restored header). Mac TU 483 0x446130-0x44664e (+ the type list Mac 0x44664e at
// the start of TU 484), Win 0x744c00-0x744f30. The input handed to contact handlers: VxPart's body
// contact callback (Mac 0x4681ca) wraps the Mdt contact in one; notifyIntersect gets one too.
#ifndef VX_VXDYNAMICSCONTACTINPUT_H
#define VX_VXDYNAMICSCONTACTINPUT_H

#include "Vx/VxBase.h"
#include "Mdt/MdtTypes.h"

namespace Vx {

// abstract; 4 bytes (vptr); class id 0x13e (names inferred)
class VxDynamicsResponseInput {
public:
    static int getStaticClassType() { return 0x13e; }  // (inline: Mac 0x44664e stores 0x13e)
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x44664e: {0x13e}
    virtual bool isOfType(int type) const = 0;          // slot 0
    virtual bool isOfClassType(int type) const = 0;     // slot 1
    virtual ~VxDynamicsResponseInput() {}               // slot 2/3 (inline)
    virtual const char* getClassName() const = 0;       // slot 4
};

// Mac vtable 0x75dc00, Win 0x744ef0.. ; 8 bytes; class id 0x114
class VxDynamicsContactInput : public VxDynamicsResponseInput {
public:
    static int getStaticClassType();                    // Mac 0x446130: 0x114
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x44622c: {0x114, 0x13e}
    explicit VxDynamicsContactInput(MdtContactID contact); // C1 Mac 0x446170 -> C2 0x446150
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x4465b4, Win 0x744ef0
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x44613a, Win 0x744c00
    virtual ~VxDynamicsContactInput();                  // slot 2/3 Mac 0x4461ce (D2 0x4461b4)/0x44618a, Win 0x744f30
    virtual const char* getClassName() const;           // slot 4 Mac 0x4465fa, Win 0x744f20 "VxDynamicsContactInput"
    MdtContactID getContact() const { return mContact; } // (name inferred)

    MdtContactID mContact;      // +0x04 (name inferred)
};

} // namespace Vx

#endif
