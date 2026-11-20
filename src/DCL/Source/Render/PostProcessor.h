// LEGO Digital Designer — restored source
// LEGO::PostProcessor — 3D view post-processing: builds the effects on the "Shaders" assets and
//   keeps those that initialise. LDDApplication creates it in LoadAssets (+0x178); GetEffects
//   feed the preferences / scene control, GetExtraEffects LDDPostProcessor's debug list.
// TU 372 (Mac 0x360d60-0x36193f; Win 0x5b6b40-0x5b725f).
// Mac: vtable 0x74feac {Initialize 0x360e80, D1 0x3616d0, D0 0x361790}, Object-in 0x74fed0;
//   layout {vptr, Render* +0x04, RefPointer<Directory> shaders +0x08, UIntVector2 +0x0c (never
//   used), effects +0x14, extra effects +0x24, virtual base Object +0x34} sizeof 0x3c.
// Win: vtable 0xa2cda0 {Initialize 0x5b6b70}, Object-in 0xa2cda8 {sdd 0x5b6b40, …};
//   {vptr, vbptr, render +0x08, shaders +0x0c, …, Object +0x38}.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"

namespace LEGO {

class Render;
class Directory;
class PostProcessingEffect;

class PostProcessor : public virtual Object {
public:
    explicit PostProcessor(Render* render);                                  // Mac 0x360dc0 (Win inlined in LDDApplication)
    // Opens <assets>/Shaders (and Shaders/RoundEdge when `roundEdges`: LDDApplication passes
    // UseRoundEdges) and creates the effects: SSAO and SSDO into GetEffects, None,
    // SceneNormals, Blur and EdgeDetection into GetExtraEffects — each only when its
    // Initialize succeeds. False when a directory is missing. NOTE (both builds): a second
    // call appends the effects again.
    virtual bool Initialize(bool roundEdges);                                // 0 Mac 0x360e80, Win 0x5b6b70
    virtual ~PostProcessor();                                                // Mac 0x3616d0/0x361790, Win dtor 0x5b7070 / sdd 0x5b6b40

    // The effects (+0x14). Mac 0x361570, Win 0x5b7050
    const BasicArray<RefPointer<PostProcessingEffect> >& GetEffects() const;
    // (additive) The debug effects (+0x24; LDDApplication Mac 0x17d300 hands them
    // to LDDPostProcessor). Mac 0x361580 (Win: one body with GetEffects' neighbours, inlined)
    // (name inferred)
    const BasicArray<RefPointer<PostProcessingEffect> >& GetExtraEffects() const;

protected:
    Render*                                     m_render;            // +0x04
    RefPointer<Directory>                       m_shaderDirectory;   // +0x08
    UIntVector2                                 m_vector0c;          // +0x0c (not used)
    BasicArray<RefPointer<PostProcessingEffect> > m_effects;         // +0x14
    BasicArray<RefPointer<PostProcessingEffect> > m_extraEffects;    // +0x24

private:
    PostProcessor(const PostProcessor&);
    PostProcessor& operator=(const PostProcessor&);
};

} // namespace LEGO
