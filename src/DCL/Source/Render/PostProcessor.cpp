// LEGO Digital Designer — restored source
// TU 372 "PostProcessor.cpp" (Mac 0x360d60-0x36193f; Win 0x5b6b40-0x5b725f). See PostProcessor.h.
// BasicArray<RefPointer<PostProcessingEffect> > instances (BasicArray.h): dtor Mac 0x360d60 /
// 0x361880, Win sdd 0x5b7220; PushBack Mac 0x361450, Win 0x5b7170.
#include "PostProcessor.h"
#include "PostProcessingEffect.h"
#include "PostProcessingEffectSSAO.h"
#include "PostProcessingEffectSSDO.h"
#include "PostProcessingEffectNone.h"

#include "Platform.h"
#include "Directory.h"
#include "LEGO/String.h"

namespace LEGO {

namespace {

// Keeps `effect` in `effects` when it initialises (the effect is referenced while it is tried).
void AddEffect(BasicArray<RefPointer<PostProcessingEffect> >& effects, PostProcessingEffect* effect)
{
    RefPointer<PostProcessingEffect> reference(effect);
    if (reference->Initialize())
        effects.PushBack(reference);
}

} // namespace

// Mac 0x360dc0
PostProcessor::PostProcessor(Render* render)
    : m_render(render)
{
}

// Mac 0x3616d0 (D1) / 0x361790 (D0) (body 0x361590), Win 0x5b7070 / sdd 0x5b6b40
PostProcessor::~PostProcessor()
{
}

// Mac 0x360e80, Win 0x5b6b70
bool PostProcessor::Initialize(bool roundEdges)
{
    RefPointer<Directory> assets = Platform::GetInstance()->GetSystemDirectory(9, true);
    if (assets.IsNull())
        return false;
    m_shaderDirectory = assets->GetDirectory(String("Shaders"), false);
    if (m_shaderDirectory.IsNull())
        return false;
    RefPointer<Directory> roundEdgeShaders;
    if (roundEdges) {
        roundEdgeShaders = m_shaderDirectory->GetDirectory(String("RoundEdge"), false);
        if (roundEdgeShaders.IsNull())
            return false;
    }
    AddEffect(m_effects, new PostProcessingEffectSSAO(m_render, m_shaderDirectory, roundEdges,
                                                      roundEdgeShaders));
    AddEffect(m_effects, new PostProcessingEffectSSDO(m_render, m_shaderDirectory, roundEdges,
                                                      roundEdgeShaders));
    AddEffect(m_extraEffects, new PostProcessingEffectNone(m_render, m_shaderDirectory, roundEdges,
                                                           roundEdgeShaders));
    AddEffect(m_extraEffects, new PostProcessingEffectSceneNormals(m_render, m_shaderDirectory,
                                                                   roundEdges, roundEdgeShaders));
    AddEffect(m_extraEffects, new PostProcessingEffectBlur(m_render, m_shaderDirectory, roundEdges,
                                                           roundEdgeShaders));
    AddEffect(m_extraEffects, new PostProcessingEffectEdgeDetection(m_render, m_shaderDirectory,
                                                                    roundEdges, roundEdgeShaders));
    return true;
}

// Mac 0x361570, Win 0x5b7050
const BasicArray<RefPointer<PostProcessingEffect> >& PostProcessor::GetEffects() const
{
    return m_effects;
}

// Mac 0x361580
const BasicArray<RefPointer<PostProcessingEffect> >& PostProcessor::GetExtraEffects() const
{
    return m_extraEffects;
}

} // namespace LEGO
