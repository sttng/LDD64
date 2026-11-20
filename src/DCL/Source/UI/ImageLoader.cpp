// LEGO Digital Designer — restored source
// ImageLoader / ImageLoaderInstantiator — TU 277 "ImageLoaderInstantiator.cpp"
// (Mac 0x28fb80-0x29104b, Win 0x4e94b0-0x4ea3af).
#include "ImageLoader.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "Platform.h"
#include "Path.h"
#include "Directory.h"
#include "Render.h"
#include "Texture.h"

#include <typeinfo>

namespace LEGO {

// Mac 0x2906a0 (inline constructor), Win 0x4e9f70
ImageLoader::ImageLoader()
    : m_render(), m_images()
{
}

// Mac 0x290540 / 0x290610, Win 0x4ea030 / 0x4ea000
ImageLoader::~ImageLoader()
{
}

// Mac 0x2906a0, Win 0x4e94b0
RefPointer<Object> ImageLoaderInstantiator::CreateObject() const
{
    return RefPointer<Object>(new ImageLoader);
}

// Mac 0x28fb80, Win 0x4e95c0
ImageLoader* ImageLoader::GetInstance()
{
    ImageLoaderInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryID, instantiator);
    if (!object)
        return 0;
    return dynamic_cast<ImageLoader*>(object.Get());
}

// Mac 0x28fc20, Win 0x4e9680
void ImageLoader::SetRender(Render* render)
{
    m_render = render;
}

// Mac 0x28fc70, Win 0x4e96c0
bool ImageLoader::Load(const Path& path)
{
    RefPointer<XMLDOM> document = new XMLDOM;
    if (!document->Parse(path))
        return false;
    const XMLDOMNode::NodeArray& children = document->GetChildren();
    for (XMLDOMNode::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it)
        ParseNode(*it, String(), String(), Rect(0.0f, 0.0f, 0.0f, 0.0f));
    return true;
}

// Mac 0x28fdb0, Win 0x4e9ac0
void ImageLoader::ParseNode(XMLDOMNode* node, const String& prefix, const String& parentFile,
                            const Rect& parentRect)
{
    String file;
    Rect rect(0.0f, 0.0f, 0.0f, 0.0f);
    RefPointer<Texture> texture;
    file = parentFile;
    node->GetAttribute(String("file"), file);

    bool parsed;
    {
        String value;
        parsed = false;
        if (node->GetAttribute(String("subRect"), value)) {
            UInt32 position = 0;
            parsed = FromString(rect, value, position);
        }
    }
    if (!parsed) {
        String value;
        if (node->GetAttribute(String("subPosSize"), value)) {
            Vector2 position;
            Vector2 size;
            UInt32 index = 0;
            if (FromString(position, value, index)) {
                ++index;
                if (FromString(size, value, index))
                    rect.Set(position, size);
            }
        }
    }
    rect.Offset(parentRect.GetMin());
    if (!rect.IsValid())
        rect.SetSize(parentRect.GetSize());

    String name;
    if (prefix.IsEmpty())
        name = node->GetName();
    else
        name << prefix << "." << node->GetName();

    if (!file.IsEmpty() && rect.IsValid()) {
        Image& image = m_images[name];
        image.file = file;
        image.rect.Set(rect);
        image.texture = texture.Get();
    }

    const XMLDOMNode::NodeArray& children = node->GetChildren();
    for (XMLDOMNode::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it)
        ParseNode(*it, name, file, rect);
}

// Mac 0x290230, Win 0x4e9810
Texture* ImageLoader::GetImage(const String& name, Rect& rect)
{
    ImageMap::Iterator it = m_images.Find(name);
    if (it == m_images.End())
        return 0;
    Image& image = it->second;
    rect.Set(image.rect);
    if (!image.texture) {
        Texture::LoadArgs2D args;
        args.path.SetElement(Platform::GetInstance()->GetSystemDirectory(9, true));
        if (!args.path.Append(image.file))
            return 0;
        args.create.alphaMode = Texture::kAlphaImage;   // 2
        Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Texture), args);
        image.texture = object ? dynamic_cast<Texture*>(object) : 0;
        if (!image.texture)
            return 0;
        rect.Set(image.rect);
    }
    return image.texture;
}

} // namespace LEGO
