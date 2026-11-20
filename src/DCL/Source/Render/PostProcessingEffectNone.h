// LEGO Digital Designer — restored source
// LEGO::PostProcessingEffectNone — the "None" entry of PostProcessor's debug list: draws nothing.
// TU 405 (Mac 0x38cba0-0x38ccff; Win 0x5bfcd0-0x5bfe0f). Vtables Mac 0x756570, Win 0xa2d0e8.
// sizeof 0x40.
#pragma once
#include "PostProcessingEffect.h"

namespace LEGO {

class PostProcessingEffectNone : public PostProcessingEffect {
public:
    // Mac 0x38cba0, Win 0x5bfcd0
    PostProcessingEffectNone(Render* render, Directory* shaders, bool roundEdges,
                             Directory* roundEdgeShaders);
    virtual ~PostProcessingEffectNone();                                    // Mac 0x38cc70 / 0x38cc80, Win 0x5bfdc0 / sdd 0x5bfd30

    virtual bool Initialize();                                              // Mac 0x38cbe0, Win 0x4f5b70 (true)
    virtual void Resize(const Vector2& size, const Vector2& imageSize);     // Mac 0x38cbf0, Win 0x424ec0 (empty)
    virtual void Deactivate();                                              // Mac 0x38cc00, Win 0x459790 (empty)
    virtual void Begin(const Color& clearColor);                            // Mac 0x38cc10, Win 0x40be80 (empty)
    virtual void Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                       const Light* light0, const Light* light1, const Light* light2,
                       const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                       Real32 nearPlane, Real32 farPlane);                  // Mac 0x38cc20, Win 0x5bfd50 (empty)
    virtual bool LoadShaders();                                             // Mac 0x38cc30, Win 0x4f5b70 (true)
    virtual String GetName() const;                                         // Mac 0x38cc40, Win 0x5bfd60 "None"
};

} // namespace LEGO
