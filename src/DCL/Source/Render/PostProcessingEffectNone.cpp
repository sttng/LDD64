// LEGO Digital Designer — restored source
// TU 405 "PostProcessingEffectNone.cpp" (Mac 0x38cba0-0x38ccff; Win 0x5bfcd0-0x5bfe0f).
#include "PostProcessingEffectNone.h"

namespace LEGO {

// Mac 0x38cba0, Win 0x5bfcd0
PostProcessingEffectNone::PostProcessingEffectNone(Render* render, Directory* shaders,
                                                   bool roundEdges, Directory* roundEdgeShaders)
    : PostProcessingEffect(render, shaders, roundEdges, roundEdgeShaders)
{
}

// Mac 0x38cc70 / 0x38cc80, Win 0x5bfdc0 / sdd 0x5bfd30
PostProcessingEffectNone::~PostProcessingEffectNone()
{
}

// Mac 0x38cbe0, Win 0x4f5b70
bool PostProcessingEffectNone::Initialize()
{
    return true;
}

// Mac 0x38cbf0, Win 0x424ec0
void PostProcessingEffectNone::Resize(const Vector2&, const Vector2&)
{
}

// Mac 0x38cc00, Win 0x459790
void PostProcessingEffectNone::Deactivate()
{
}

// Mac 0x38cc10, Win 0x40be80
void PostProcessingEffectNone::Begin(const Color&)
{
}

// Mac 0x38cc20, Win 0x5bfd50
void PostProcessingEffectNone::Apply(const Vector2&, RenderQueue*, Texture*, const Light*,
                                     const Light*, const Light*, const Matrix3D&, Real32, Real32,
                                     Real32, Real32)
{
}

// Mac 0x38cc30, Win 0x4f5b70
bool PostProcessingEffectNone::LoadShaders()
{
    return true;
}

// Mac 0x38cc40, Win 0x5bfd60
String PostProcessingEffectNone::GetName() const
{
    return String("None");
}

} // namespace LEGO
