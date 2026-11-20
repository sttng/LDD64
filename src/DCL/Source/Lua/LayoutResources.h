// LEGO Digital Designer — restored source
// Layout attribute helpers shared by the Lua controls: fonts (font="name" -> an element of the
// fonts layout, Scripts/Fonts.xml), images (image / subRect / subPosSize) and selection masks.
// TU 322 "LayoutResources.cpp" (names inferred; the TU emits Font::SystemFontArgs' weak dtors):
// Mac 0x2cd5d0-0x2cf08b, Win 0x46c390-0x46dc30.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Geometry.h"

namespace LEGO {

class XMLDOMNode;
class ControlManager;
class Texture;
class Font;
class SelectionMask;

// The texture of image file `name`: Assets/<language>/<name> when it exists, else
// Assets/<name>, through the control manager's object factory (Texture::LoadArgs2D).
// Null for an empty name or when neither loads.
Texture* LoadLayoutTexture(const String& name, ControlManager* manager);  // Mac 0x2cd5d0, Win 0x46c9d0

// font="name": LoadLayoutFont(name, …). False (font untouched) without the attribute.
bool ParseLayoutFont(const XMLDOMNode* node, const XMLDOMNode* fonts, RefPointer<Font>& font,
                     ControlManager* manager);                              // Mac 0x2cd940, Win 0x46cc20
// The font described by the child `name` of `fonts`: color="r g b a" (or rgb="R,G,B" 0-255),
// height (default 18), then fontName (+ bold, italic) -> Font::SystemFontArgs, or typeFontName
// (boldSystemFont … toolTipsFont) -> Font::SystemTypeFontArgs. Null font when the name is empty
// or unknown or no face is given. Always true.
bool LoadLayoutFont(const String& name, const XMLDOMNode* fonts, RefPointer<Font>& font,
                    ControlManager* manager);                               // Mac 0x2cda00, Win 0x46cd00

// image="file" (LoadLayoutTexture), subRect="l t r b" or subPosSize="x y,w h"; without either
// the whole image. hasImage / hasRect tell which attributes were found (hasRect also when the
// image's size was used).
void ParseLayoutImage(const XMLDOMNode* node, RefPointer<Texture>& image, Rect& rect,
                      bool& hasImage, bool& hasRect, ControlManager* manager); // Mac 0x2ce510, Win 0x46d490

// selectionMask="file.bmp" (BitmapMask from Assets) with maskRect="l t r b" or
// maskPosSize="x y,w h" (default: the whole mask) and maskSample="x y,inside" (default 0 0, yes)
// -> a BitmapSelectionMask. False without the attribute; an empty file name clears the mask.
bool ParseSelectionMask(const XMLDOMNode* node, RefPointer<SelectionMask>& mask); // Mac 0x2ce7b0, Win 0x46d6e0

} // namespace LEGO
