// LEGO Digital Designer — restored source
// LEGO::RenderVisualDebugger — the VisualDebugger (SceneListener in Scene.h, refcounted through
//   slots 0 / 1): debug shapes (points as three crossed lines, lines, triangle outlines) per
//   category and id, drawn as meshes. Base of BrickConnectSceneControl (Mac +0x1e0). Shapes from
//   other threads become meshes in the next QueueDebugMeshes on the main thread.
// TU 414 (Mac 0x391900-0x3943db, Win 0x5bb650-0x5bd00f).
// Vtables: Mac VisualDebugger 0x778950 {9 pure, D1 0x393ad0, D0 0x393ae0}, Win 0xa2cfa4
//   {9 _purecall, sdd 0x5bb650}; RenderVisualDebugger Mac 0x7574dc {AddRef 0x391900, Release
//   0x391950, AddPoint 0x3919a0, AddLine 0x3922a0, AddTriangle 0x392600, RemoveShape 0x3929e0,
//   NewShape 0x392c60, OnSceneEvent 0x392cd0, RemoveAllShapes 0x392db0, D1 0x3935c0, D0
//   0x393680, SetShapeMesh 0x392eb0, OnFinalRelease 0x393770}, Object-in 0x757528; Win 0xa2cfd0
//   {0x5bb670, 0x5bb690, 0x5bb6c0, 0x5bb7c0, 0x5bb8e0, 0x5bba30, 0x5bbb00, 0x5bbb70, 0x5bbbf0,
//   sdd 0x5bbfb0, 0x5bbc70}, Object-in 0xa2d000.
// Layout: Mac {vptr, RefPointer<Render> +0x04, RefPointer<Material> +0x08, categories +0x0c,
//   recursive mutex +0x1c (0x2c), Object +0x48}; Win {vptr, vbptr, render +0x08, material
//   +0x0c, categories +0x10, CRITICAL_SECTION +0x20, Object +0x3c}.
// Names except the RTTI names (RenderVisualDebugger, VisualDebugger, VDBShape) are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "Color.h"
#include "Scene.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

class RenderQueue;
class Render;
class Material;
class Mesh;

class RenderVisualDebugger : public VisualDebugger, public virtual Object {
public:
    // One shape (RTTI LEGO::RenderVisualDebugger::VDBShape, 0x44 bytes).
    struct VDBShape {
        enum Pending { kNone = 0, kPoint = 1, kLine = 2, kTriangle = 3, kRemove = 4 };
        VDBShape() : id(0), pending(kNone), size(1.0f) {}
        UInt32           id;        // +0x00
        RefPointer<Mesh> mesh;      // +0x04
        UInt32           pending;   // +0x08 Pending: work left for the main thread
        Vector3          a;         // +0x0c point / line start / first corner
        Vector3          b;         // +0x18
        Vector3          c;         // +0x24
        Color            color;     // +0x30 (white)
        Real32           size;      // +0x40 point size (1)
    };
    typedef Pair<UInt32, BasicArray<VDBShape> > Category;   // 0x14 bytes

    // Mac 0x393120 (C2 with VTT), Win 0x5bbea0: draws shapes with `material` (the scene
    // manager's silhouette material, BrickConnectSceneManager +0x44); `render` creates the
    // vertex buffers. Both are referenced.
    RenderVisualDebugger(Render* render, Material* material);

    // VisualDebugger (SceneListener): the Object reference count.
    using Object::AddRef;
    using Object::Release;
    virtual void AddRef() { Object::AddRef(); }                             // 0 Mac 0x391900 (thunk 0x391920), Win 0x5bb670
    virtual void Release() { Object::Release(); }                           // 1 Mac 0x391950 (thunk 0x391970), Win 0x5bb690
    // A point: three crossed lines of half length `size` through `position`.
    virtual UInt32 AddPoint(UInt32 category, UInt32 id, const Vector3& position, const Color& color,
                            Real32 size);                                   // 2 Mac 0x3919a0, Win 0x5bb6c0
    virtual UInt32 AddLine(UInt32 category, UInt32 id, const Vector3& from, const Vector3& to,
                           const Color& color);                             // 3 Mac 0x3922a0, Win 0x5bb7c0
    // The outline of the triangle (a line strip a b c a; no material).
    virtual UInt32 AddTriangle(UInt32 category, UInt32 id, const Vector3& a, const Vector3& b,
                               const Vector3& c, const Color& color);       // 4 Mac 0x392600, Win 0x5bb8e0
    virtual void RemoveShape(UInt32 category, UInt32 id);                   // 5 Mac 0x3929e0, Win 0x5bba30
    virtual UInt32 NewShape(UInt32 category);                               // 6 Mac 0x392c60, Win 0x5bbb00
    // Removes every shape of category `code`.
    virtual void OnSceneEvent(UInt32 code);                                 // 7 Mac 0x392cd0, Win 0x5bbb70
    virtual void RemoveAllShapes();                                         // 8 Mac 0x392db0, Win 0x5bbbf0
    virtual ~RenderVisualDebugger();                                        // Mac 0x3935c0/0x393680 (body 0x3933b0), Win 0x5bbfe0 / sdd 0x5bbfb0
    // Shape `id` of `category` (a new one for id 0) shows `mesh`. Returns the shape's id.
    virtual UInt32 SetShapeMesh(UInt32 category, UInt32 id, Mesh* mesh);    // 11 Mac 0x392eb0, Win 0x5bbc70
    // Under the debugger's mutex: builds the meshes of shapes reported from
    // other threads, removes the shapes marked for removal and adds every shape's mesh to
    // `queue` at `transform` (BrickConnectSceneControl::Draw). Mac 0x392f60, Win 0x5bbcf0
    void QueueDebugMeshes(RenderQueue* queue, const Matrix3D& transform);

    // (tests) the shapes of every category.
    const BasicArray<Category>& GetCategories() const { return m_categories; }

protected:
    // Empty (Object::Release deletes the object afterwards).
    virtual void OnFinalRelease();                                          // Mac 0x393770 (thunk 0x393780)

    // Shape `id` of `category`, appended (with the category) when missing; id 0: a new shape
    // with the next id (largest + 1). Mac 0x391a70, Win 0x5bcbf0
    VDBShape* GetShape(UInt32 category, UInt32 id);
    // The meshes (one vertex colour, lines). Mac 0x391cd0 / 0x392370 / 0x3926f0,
    // Win 0x5bc140 / 0x5bc6a0 / 0x5bc8d0
    void BuildPointMesh(VDBShape& shape, const Vector3& position, const Color& color, Real32 size);
    void BuildLineMesh(VDBShape& shape, const Vector3& from, const Vector3& to, const Color& color);
    void BuildTriangleMesh(VDBShape& shape, const Vector3& a, const Vector3& b, const Vector3& c,
                           const Color& color);

    // Embedded recursive mutex with inline ctor / dtor (as ObjectRegistry's).
    class RecursiveMutex {
    public:
        RecursiveMutex()
        {
#if defined(_WIN32)
            InitializeCriticalSection(&m_cs);
#else
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&m_mutex, &attr);
            pthread_mutexattr_destroy(&attr);
#endif
        }
        ~RecursiveMutex()
        {
#if defined(_WIN32)
            DeleteCriticalSection(&m_cs);
#else
            pthread_mutex_destroy(&m_mutex);
#endif
        }
        void Lock()
        {
#if defined(_WIN32)
            EnterCriticalSection(&m_cs);
#else
            pthread_mutex_lock(&m_mutex);
#endif
        }
        void Unlock()
        {
#if defined(_WIN32)
            LeaveCriticalSection(&m_cs);
#else
            pthread_mutex_unlock(&m_mutex);
#endif
        }
    private:
        RecursiveMutex(const RecursiveMutex&);
        RecursiveMutex& operator=(const RecursiveMutex&);
#if defined(_WIN32)
        CRITICAL_SECTION m_cs;      // 0x18 bytes
#else
        pthread_mutex_t  m_mutex;   // 0x2c bytes
#endif
    };

    RefPointer<Render>     m_render;       // Mac +0x04, Win +0x08
    RefPointer<Material>   m_material;     // Mac +0x08, Win +0x0c
    BasicArray<Category>   m_categories;   // Mac +0x0c, Win +0x10
    RecursiveMutex         m_mutex;        // Mac +0x1c, Win +0x20

private:
    RenderVisualDebugger(const RenderVisualDebugger&);
    RenderVisualDebugger& operator=(const RenderVisualDebugger&);
};

} // namespace LEGO
