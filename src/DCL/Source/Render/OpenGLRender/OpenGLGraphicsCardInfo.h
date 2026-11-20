// LEGO Digital Designer — restored source
// LEGO::OpenGLGraphicsCardInfo — GraphicsCardInfo + the OpenGL and GLSL versions of a render.
// TU 420 "OpenGLGraphicsCardInfo.cpp": Mac 0x399660-0x39982f, Win 0x4f1ee0-0x4f2070.
// Vtable: Mac 0x757fa0 {D1 0x3997a0, D0 0x3997b0, Object slots, GetDescription 0x399690};
// Win sdd 0x4f1f40, GetDescription 0x4f1f60. Layout: GraphicsCardInfo + OpenGLRender* (Mac +0x5c,
// Win +0x2c; not reference counted). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "GraphicsCardInfo.h"

namespace LEGO {

class OpenGLRender;

class OpenGLGraphicsCardInfo : public GraphicsCardInfo {
public:
    explicit OpenGLGraphicsCardInfo(OpenGLRender* render);           // Mac 0x399660, Win 0x4f1ee0
    virtual ~OpenGLGraphicsCardInfo();                                // Mac 0x3997a0/0x3997b0, Win sdd 0x4f1f40

    // GraphicsCardInfo's report + "OpenGL version: x.y\n" + "GLSL version: x.y\n".
    virtual String GetDescription() const;                            // Mac 0x399690, Win 0x4f1f60

protected:
    OpenGLRender* m_render;   // Mac +0x5c, Win +0x2c
};

} // namespace LEGO
