// LEGO Digital Designer — restored source
// LEGO::ImageLoader — the UI image atlas: reads Graphics/imagedescriptions.xml (named sub-
// rectangles of image files, dotted paths such as "IconGeneral.createbutton.up") and hands out
// an image's texture (loaded on first use, cached) and rectangle. LEGO::ImageLoaderInstantiator
// creates the shared instance (ObjectRegistry id 'IMLO').
// TU 277 "ImageLoaderInstantiator.cpp" (Mac 0x28fb80-0x29104b, Win 0x4e94b0-0x4ea3af). Users:
// start-up (Mac 0x13a1c0: SetRender + Load), RenderControl::SetImage (Mac 0x155520),
// ButtonControl (Mac 0xeb990).
// Vtables: ImageLoader Mac 0x73effc {D1 0x290540, D0 0x290610} + Object-in 0x73f01c; Win
//   Object-in 0xa25bd4 {sdd 0x4ea000, …} only. ImageLoaderInstantiator Mac 0x774d08 {D1 0x28fc10,
//   D0 0x290690, CreateObject 0x2906a0}; Win 0xa25be8 {sdd 0x4e9550, CreateObject 0x4e94b0}.
// Layout: Mac {vptr, Render* +0x04, BasicMap<String, Image> +0x08, Object +0x24} (sizeof 0x2c);
//   Win {vbptr, Render* +0x04, BasicMap +0x08, Object +0x2c} (sizeof 0x34). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectRegistry.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "LEGO/String.h"

namespace LEGO {

class Path;
class Render;
class Texture;
class XMLDOMNode;

class ImageLoader : public virtual Object {
public:
    enum { kRegistryID = 0x494d4c4f };   // 'IMLO'

    // One named image: its file (relative to the asset directory), its rectangle in that file
    // (pixels) and the texture once loaded. Node payload Mac +0x20..+0x44 (sizeof 0x24).
    struct Image {
        Image() : rect(0.0f, 0.0f, 0.0f, 0.0f) {}   // the default value of the map (Mac 0x290740)
        String              file;      // +0x00
        Rect                rect;      // Mac +0x10
        RefPointer<Texture> texture;   // Mac +0x20
    };
    typedef BasicMap<String, Image> ImageMap;

    ImageLoader();                                                      // inline (Mac 0x2906a0, Win 0x4e9f70)
    virtual ~ImageLoader();                                             // Mac 0x290540/0x290610, Win 0x4ea030/0x4ea000

    // The shared instance (created through ImageLoaderInstantiator on first use). The registry
    // keeps it alive; the reference taken here is dropped again.
    static ImageLoader* GetInstance();                                  // Mac 0x28fb80, Win 0x4e95c0

    // The render whose object factory creates the textures. NOTE: must not be null (the
    // original references the new render without a check).
    void SetRender(Render* render);                                     // Mac 0x28fc20, Win 0x4e9680
    // Reads an image description file; false when it cannot be parsed.
    bool Load(const Path& path);                                        // Mac 0x28fc70, Win 0x4e96c0
    // The texture of the image `name` (loaded on first use; not referenced) and its rectangle.
    // Null for an unknown name (`rect` untouched) or when the texture cannot be loaded.
    Texture* GetImage(const String& name, Rect& rect);                  // Mac 0x290230, Win 0x4e9810

protected:
    // One element of the description and its children. Attributes: file (inherited), subRect
    // "l t r b" or subPosSize "x y w h" (relative to the parent's rectangle; empty: the parent's
    // size). Elements with a file and a non-empty rectangle are stored under prefix.name.
    void ParseNode(XMLDOMNode* node, const String& prefix, const String& parentFile,
                   const Rect& parentRect);                             // Mac 0x28fdb0, Win 0x4e9ac0

    RefPointer<Render> m_render;   // Mac +0x04, Win +0x04
    ImageMap           m_images;   // Mac +0x08, Win +0x08
};

class ImageLoaderInstantiator : public ObjectRegistryInstantiator {
public:
    virtual ~ImageLoaderInstantiator() {}                               // Mac 0x28fc10/0x290690, Win 0x4e9550
    virtual RefPointer<Object> CreateObject() const;                    // Mac 0x2906a0, Win 0x4e94b0
};

} // namespace LEGO
