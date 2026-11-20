// Vortex 5.0.0 build 800 "vortexldd41" - Vortex heap allocation (restored header).
// Every Vx class has its own out-of-line operator new / delete (e.g. VxHinge: new Mac 0x45a2a1,
// delete Mac 0x45a1c7) forwarding to vxAlloc / vxFree (the MeMemoryAPI table); VxArray growth
// (inlined in LEGO code, e.g. Mac TU 435 0x3abd90) uses them too. Win: operator new is a 16-byte
// thunk "push ebp; mov ebp,esp; pop ebp; jmp 0x7075c0" (e.g. VxPart 0x71b8e0, VxMechanism 0x71eac0).
#ifndef VX_VXALLOCATOR_H
#define VX_VXALLOCATOR_H

#include <stddef.h>

namespace Vx {

// Mac 0x463c0a, Win 0x7075c0: MeMemoryAPI.create(size) (name inferred) (src/Vx/VxMessage.cpp)
void* vxAlloc(size_t size);
// Mac 0x463c2c, Win 0x7c24e0: MeMemoryAPI.destroy(p) (name inferred)
void vxFree(void* p);

} // namespace Vx

// Declares the per-class allocation operators; VX_IMPLEMENT_ALLOCATOR defines them as the Mac
// bodies are: "FUN_00463c0a(size)" and "if (p) FUN_00463c2c(p)".
#define VX_DECLARE_ALLOCATOR()               \
    static void* operator new(size_t size);  \
    static void operator delete(void* p)

// The matching definitions, for the class's .cpp.
#define VX_IMPLEMENT_ALLOCATOR(Class)                                    \
    void* Class::operator new(size_t size) { return Vx::vxAlloc(size); } \
    void Class::operator delete(void* p)                                 \
    {                                                                    \
        if (p)                                                           \
            Vx::vxFree(p);                                               \
    }

#endif
