// LEGO Digital Designer — restored source
// TU 322 "LayoutResources.cpp" (name inferred): Mac 0x2cd5d0-0x2cf08b, Win 0x46c390-0x46dc30.
#include "LayoutResources.h"
#include "XMLDOMNode.h"
#include "ControlManager.h"
#include "ObjectFactory.h"
#include "Platform.h"
#include "Directory.h"
#include "Path.h"
#include "TextLocale.h"
#include "Texture.h"
#include "Font.h"
#include "Color.h"
#include "StringStream.h"
#include "BitmapMask.h"
#include "BitmapSelectionMask.h"

namespace LEGO {

namespace {

// Factory result as T (both builds: dynamic_cast after a null check; Win 0x46dbb0 / 0x46dc30).
template <class T>
T* CastCreated(Object* object)
{
    return object ? dynamic_cast<T*>(object) : 0;
}

// "x y,w h" -> {min = (x, y), max = (x + w, y + h)} (subPosSize / maskPosSize, inlined in both).
void ParsePositionSize(const String& text, Rect& rect)
{
    UInt32 position = 0;
    FromString(rect.m_min, text, position);
    ++position;
    Vector2 size;
    FromString(size, text, position);
    rect.m_max = rect.m_min + size;
}

} // namespace

Texture* LoadLayoutTexture(const String& name, ControlManager* manager)    // Mac 0x2cd5d0, Win 0x46c9d0
{
    if (name.IsEmpty())
        return 0;
    Texture::LoadArgs2D args;
    Path assets;
    {
        RefPointer<Directory> directory =
            Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
        assets.SetElement(directory);
    }
    args.path.Assign(assets);
    bool localized;
    {
        RefPointer<TextLocale> locale = TextLocale::GetInstance();
        localized = args.path.Append(locale->GetLanguage());
    }
    if (localized && args.path.Append(name)) {                            // Assets/<language>/<name>
        Texture* texture = CastCreated<Texture>(
            manager->GetObjectFactory()->CreateObject(typeid(Texture), args));
        if (texture)
            return texture;
    }
    args.path.Assign(assets);                                              // Assets/<name>
    if (!args.path.Append(name))
        return 0;
    return CastCreated<Texture>(manager->GetObjectFactory()->CreateObject(typeid(Texture), args));
}

bool ParseLayoutFont(const XMLDOMNode* node, const XMLDOMNode* fonts, RefPointer<Font>& font,
                     ControlManager* manager)                               // Mac 0x2cd940, Win 0x46cc20
{
    String name;
    bool found = node->GetAttribute(String("font"), name);
    if (found)
        LoadLayoutFont(name, fonts, font, manager);
    return found;
}

bool LoadLayoutFont(const String& name, const XMLDOMNode* fonts, RefPointer<Font>& font,
                    ControlManager* manager)                                // Mac 0x2cda00, Win 0x46cd00
{
    const XMLDOMNode* node = name.IsEmpty() ? 0 : fonts->FindChild(name);
    if (!node) {
        font = 0;
        return true;
    }

    Color color(1.0f, 1.0f, 1.0f, 1.0f);
    String text;
    if (node->GetAttribute(String("color"), text)) {
        UInt32 position = 0;
        FromString(color, text, position);
    } else if (node->GetAttribute(String("rgb"), text)) {
        UInt32 red = 0xff, green = 0xff, blue = 0xff;
        UInt32 position = 0;
        FromString(red, text, position);
        ++position;
        FromString(green, text, position);
        ++position;
        FromString(blue, text, position);
        color.Set((UInt8)(red < 0xff ? red : 0xff), (UInt8)(green < 0xff ? green : 0xff),
                  (UInt8)(blue < 0xff ? blue : 0xff), 0xff);
    }

    UInt32 height = 18;
    String heightText;
    if (node->GetAttribute(String("height"), heightText))
        FromString(height, heightText);

    String faceName;
    if (node->GetAttribute(String("fontName"), faceName)) {
        Font::SystemFontArgs args;
        args.name = faceName;
        args.size = height;
        args.color = color;
        String bold;
        if (node->GetAttribute(String("bold"), bold))
            FromString(args.bold, bold);
        String italic;
        if (node->GetAttribute(String("italic"), italic))
            FromString(args.italic, italic);
        font = CastCreated<Font>(manager->GetObjectFactory()->CreateObject(typeid(Font), args));
        return true;
    }
    if (!node->GetAttribute(String("typeFontName"), faceName))
        return true;

    Font::SystemTypeFontArgs args;
    args.size = height;
    args.color = color;
    if (faceName == String("boldSystemFont"))
        args.type = Font::kBoldSystemFont;
    else if (faceName == String("controlContentFont"))
        args.type = Font::kControlContentFont;
    else if (faceName == String("labelFont"))
        args.type = Font::kLabelFont;
    else if (faceName == String("menuFont"))
        args.type = Font::kMenuFont;
    else if (faceName == String("menuBarFont"))
        args.type = Font::kMenuBarFont;
    else if (faceName == String("messageFont"))
        args.type = Font::kMessageFont;
    else if (faceName == String("paletteFont"))
        args.type = Font::kPaletteFont;
    else if (faceName == String("systemFont"))
        args.type = Font::kSystemFont;
    else if (faceName == String("titleBarFont"))
        args.type = Font::kTitleBarFont;
    else if (faceName == String("toolTipsFont"))
        args.type = Font::kToolTipsFont;
    else {
        font = 0;
        return true;
    }
    font = CastCreated<Font>(manager->GetObjectFactory()->CreateObject(typeid(Font), args));
    return true;
}

void ParseLayoutImage(const XMLDOMNode* node, RefPointer<Texture>& image, Rect& rect,
                      bool& hasImage, bool& hasRect, ControlManager* manager) // Mac 0x2ce510, Win 0x46d490
{
    String file;
    hasImage = node->GetAttribute(String("image"), file);
    if (!hasImage)
        image = 0;
    else
        image = LoadLayoutTexture(file, manager);

    String text;
    hasRect = node->GetAttribute(String("subRect"), text);
    if (hasRect) {
        UInt32 position = 0;
        FromString(rect, text, position);
        return;
    }
    hasRect = node->GetAttribute(String("subPosSize"), text);
    if (hasRect) {
        ParsePositionSize(text, rect);
    } else if (image) {
        Rect whole(image->GetSize());
        rect.Set(whole);
        hasRect = true;
    }
}

bool ParseSelectionMask(const XMLDOMNode* node, RefPointer<SelectionMask>& mask) // Mac 0x2ce7b0, Win 0x46d6e0
{
    String file;
    if (!node->GetAttribute(String("selectionMask"), file))
        return false;
    mask = 0;
    if (file.IsEmpty())
        return true;

    Path path;
    {
        RefPointer<Directory> directory =
            Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
        path.SetElement(directory);
    }
    path.Append(file);
    RefPointer<BitmapMask> bitmap;
    {
        RefPointer<ObjectFactory> factory = ObjectFactory::GetInstance();
        BitmapMask::LoadArgs args(path);
        bitmap = CastCreated<BitmapMask>(factory->CreateObject(typeid(BitmapMask), args));
    }
    if (!bitmap) {
        // NOTE: built and discarded (both builds).
        String message("Unable to load selection mask: ");
        message << path.GetPath(true);
        return true;
    }

    Rect rect(0.0f, 0.0f, 0.0f, 0.0f);
    Vector2 sample;
    bool inside = true;
    String text;
    if (node->GetAttribute(String("maskRect"), text)) {
        UInt32 position = 0;
        FromString(rect, text, position);
    } else if (node->GetAttribute(String("maskPosSize"), text)) {
        ParsePositionSize(text, rect);
    } else {
        Rect whole(bitmap->GetSize());
        rect.Set(whole);
    }
    if (node->GetAttribute(String("maskSample"), text)) {
        UInt32 position = 0;
        FromString(sample, text, position);
        ++position;
        FromString(inside, text, position);
    }
    BitmapSelectionMask* selection = new BitmapSelectionMask;
    mask = selection;
    selection->SetMask(bitmap, &rect);
    selection->Pick(sample, inside);
    return true;
}

} // namespace LEGO
