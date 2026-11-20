// LEGO Digital Designer — restored source
// LDDUtilities — helper functions of the application (TU 063, Mac 0x147b30-0x15064b, 141
// functions, guessed "BrickCountAugmentation.cpp"; Win 0x635e9f-0x639696 + 0x513ab7-0x513c6e).
// The object file also holds the LDDUtilities singleton (a PreferencesObserver that caches the
// PLAYSOUND preference) and its instantiator, the BrickCountAugmentation and
// ExtendedTextTooltipAugmentation classes and inline code of FramedTextControl,
// StandardTextTooltipAugmentation, CommandButtonControl, SelectableMesh, DataStreamWithDataRef
// (MacImageIO.h) and several Observer templates.
//
// Split over five source files (one object in the original; the screenshot path links on its
// own in the render tests):
//   LDDUtilities.cpp         SaveScreenshot / EncodePNG (R7.7)
//   LDDUtilitiesModels.cpp   model files (loader errors, alerts, LoadModel), model brand, brick
//                            descriptions, palette bricks of stored bricks (validation), lazy
//                            loads, element bricks, part boxes
//   LDDUtilitiesMeshes.cpp   the hinge tool's rotation bands (SelectableMesh.h)
//   LDDUtilitiesUI.cpp       the singleton, sounds, application textures, FramedTextControl,
//                            BrickCountAugmentation, ExtendedTextTooltipAugmentation, tooltip
//                            and text control factories
//   LDDUtilitiesDropDown.cpp the drop-down item factory
// 0x14b320 (the brick description of a scene element) is in LDDUtilitiesModels.cpp, 0x14d5a0
// (CreateSelectionToolTooltip) in SelectionTool.cpp (so that LDDUtilities' users do not link
// the tools).
// NOT restored here (in their own files): Mac 0x14a980 / 0x14abe0 (BricksControl factories,
// BricksControl.cpp), 0x14ba40 + 0x14c360 (drop-down item, LDDUtilitiesDropDown.cpp),
// CommandButtonControl's inline code (0x14ecc0-0x14f530, CommandButtonControl.h).
// File / function names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "DataStream.h"
#include "Brand.h"
#include "BrickPalette.h"
#include "Preferences.h"
#include "ObjectRegistry.h"
#include "ObjectRefPointer.h"
#include "StorageScene2.h"
#include "BrickConnectSceneManager.h"

namespace LEGO {

class Part;

class BrickConnectSceneControl;
class Path;
class ModelLoader;
class Texture;
class RenderQueue;
class Bitmap;
class Render;
class SimpleSound;
class String;
class BrickDatabase;
struct StorageScene2;
class Scene;
class Control;
class ControlManager;
class RenderControl;
class ToggleControl;
class Font;
class Directory;
class PrimitiveElementBrick;
class PrimitiveElement;
class StandardTextTooltipAugmentation;
class ExtendedTextTooltipAugmentation;
class SelectableMesh;
class VertexBuffer;
class Material;
struct Color;              // (struct, as defined: -Wmismatched-tags)
class SelectionToolTooltipAugmentation;

// The application's utilities: sounds that follow the PLAYSOUND preference. Registered in the
// ObjectRegistry under 'LDDU' (created on first use by LDDUtilitiesInstantiator).
// RTTI: si, base PreferencesObserver. Mac sizeof 0x14: vptr +0 (vtable 0x70d0d8 {D1 0x14de90,
//   D0 0x14df00, OnFinalRelease 0x45080, OnAttached 0x14dd10, OnDetached 0x52090,
//   OnPreferenceChanged 0x14dc50}), subject +0x04, m_playSound +0x08, Object +0x0c (vtable
//   0x70d108). Win: vfptr +0 {OnAttached 0x636d60, OnDetached 0x40be80, OnPreferenceChanged
//   0x636d0c}, vbptr +4, subject +8, m_playSound +0x0c, Object +0x14 (vftable 0x9d37a8 {sdd
//   thunk 0x636de6 -> 0x637cc6}).
class LDDUtilities : public PreferencesObserver {
public:
    enum { kRegistryID = 0x4c444455 };   // 'LDDU'

    // Observes the Preferences (OnAttached reads PLAYSOUND). Mac 0x14ddc0, Win 0x638eb7
    LDDUtilities();
    virtual ~LDDUtilities() {}                                               // Mac 0x14de90/0x14df00, Win 0x636da3/sdd 0x637cc6

    // The registered instance (ObjectRegistry 'LDDU'; the registry keeps it, the reference
    // taken here is dropped again). Mac 0x147b30 (inlined on Win)
    static LDDUtilities* GetInstance();
    // "Sound/<fileName>" of the application's directory as a loaded SimpleSound; null when
    // sounds are off (PLAYSOUND) or the file does not load. Mac 0x14c5b0, Win 0x63769e
    RefPointer<SimpleSound> CreateSound(const String& fileName);
    bool IsPlaySound() const { return m_playSound; }

    // The brand of a model: the "name" attribute of its <Meta><Brand …/> element
    // (FromString(Brand)); false without one. Static (Mac 0x148400 is called after
    // GetInstance(), whose result it ignores), Win 0x636f08 (name inferred)
    // NOTE: only the first "name" attribute of the first Brand element that has one counts.
    static bool GetModelBrand(const StorageScene2* model, Brand& brand);
    // The searchable description of a palette brick: "<localized name>\r\n",
    // "Part: <id> (<name>) " / "Assembly: …", optional
    // "\r\nMaingroup: <id> (<group>)", then per part (indented by 2 for assemblies, with a
    // "Part: <id>" line) "Material: <id> - <localized material>" and "Deco: <index>, <id>"
    // lines. DescriptionBrickFilter lower-cases and matches it. Mac 0x14adf0, Win 0x637264
    // (cdecl(result, database, brick)) (name inferred)
    static String GetBrickDescription(const BrickDatabase* database, const BrickPalette::Brick* brick);
    // The same for a stored brick (no main group; its itemNos as the name). Developer trace of
    // ModelLoaderProgressBarThread::Run. Mac 0x14b430, Win 0x6373be (name inferred)
    static String GetBrickDescription(const BrickDatabase* database, const StorageScene2::Brick* brick);
    // The same for the brick of a scene element: header of the element's brick (design ID, its
    // name), then the element's part (one part) or every part of the brick with its design ID.
    // SelectionToolTooltipAugmentation's text. Mac 0x14b320 (database in ECX;
    // Win inlined into 0x667256) (name inferred)
    static String GetBrickDescription(const BrickDatabase* database, const PrimitiveElement* element);

protected:
    // Reads PLAYSOUND. Mac 0x14dd10, Win 0x636d60
    virtual void OnAttached(Preferences* preferences);
    // PLAYSOUND (any case) re-read. Mac 0x14dc50, Win 0x636d0c
    virtual void OnPreferenceChanged(const String& key);

    bool m_playSound;   // Mac +0x08 / Win +0x0c false until read
};

// Creates the LDDUtilities. Mac vtable 0x76676c {D1 0x147bc0, D0 0x14dfb0, CreateObject
// 0x14dfc0}, Win 0x9d3678 {sdd 0x635f3e, CreateObject 0x639302}.
class LDDUtilitiesInstantiator : public ObjectRegistryInstantiator {
public:
    virtual ~LDDUtilitiesInstantiator() {}                                   // Mac 0x147bc0/0x14dfb0, Win 0x635f19
    virtual RefPointer<Object> CreateObject() const;                         // Mac 0x14dfc0, Win 0x639302
};

// "BrickCount": the name of the BrickCountAugmentation (static String, Mac 0x77ae6c, Win
// 0xaef29c; static initialiser Mac 0x1505a0).
extern const String kBrickCountAugmentationName;

// ---- small factories (names inferred) ----
// A StandardTextTooltipAugmentation of `control` (attached when not null). Mac 0x14ab90,
// Win 0x636a8a
StandardTextTooltipAugmentation* CreateStandardTextTooltip(Control* control, const String& text);
// An ExtendedTextTooltipAugmentation of `control`. Mac 0x14d140 (Lua binding TU 297)
ExtendedTextTooltipAugmentation* CreateExtendedTextTooltip(Control* control, const String& text,
                                                           const String& extendedText);
// A transparent RenderControl below `parent` with `text` in `font`, sized to the text (width
// unlimited); null when Initialize or the resize fails. Unreferenced.
// NOTE: a control that fails is leaked (never referenced), both builds; fixed: released (low).
// Mac 0x14b910, Win 0x6375cf
RenderControl* CreateTextControl(Control* parent, ControlManager* manager, Font* font,
                                 const String& text);
// A check box below `parent` at `position`: a ToggleControl (into `toggle`) whose image target
// is a RenderControl (`box`; the eight state images of
// "Graphics/General/Popup Final.png", 13 x 13: unchecked (0, 182), pressed (26, 182), checked
// (13, 182), checked pressed (39, 182)) and whose text target is a transparent label (`label`,
// font 3) `labelOffset` to the right with `text`; the toggle is fitted to its children. False
// (all three reset) when a control fails or the texture is missing. Used by LDDAlertBox and
// the dialogs of 7.2. Mac 0x14c810, Win 0x637884 (name inferred)
bool CreateCheckBox(ControlManager* manager, Control* parent, const Vector2& position,
                    const String& text, Real32 labelOffset, RefPointer<ToggleControl>& toggle,
                    RefPointer<RenderControl>& box, RefPointer<RenderControl>& label);
// An item of a DropDownControl (LDDUtilitiesDropDown.cpp): a StretchableButtonControl below
// `parent` (120 x 20) with the nine 3 x 3 parts at (360, 205)
// .. (400, 215) of "Graphics/EditScreen/EditMode_Final.png" and the four state images 44 x 14
// (up / disabled (260, 200), over / down (360, 205)) with `text` in font 3, text offset
// (5, 5). Unreferenced (the parent holds it); null when Create fails or the texture is
// missing. Users: PreferenceControlDropDown (Windows), LDDPostProcessor (9.6).
// Mac 0x14ba40 (StretchableButtonControl's inline constructor copy 0x14c360), Win 0x63874e
// (0x63846b) (name inferred)
class StretchableButtonControl;
StretchableButtonControl* CreateDropDownItem(const String& text, Control* parent,
                                             ControlManager* manager);
// The texture `path` of `directory` (Texture::LoadArgs2D, alpha mode 2; NOTE: a failing
// Path::Append is ignored) created by `render`'s object factory; unreferenced, null on
// failure. Mac 0x14b7d0, Win 0x637578 (name inferred)
Texture* LoadTexture(Directory* directory, const String& path, Render* render);

// A new SelectionToolTooltipAugmentation of `sceneManager` (not
// referenced; defined in SelectionTool.cpp). Mac 0x14d5a0 (name inferred)
SelectionToolTooltipAugmentation* CreateSelectionToolTooltip(BrickConnectSceneManager* sceneManager);

// ---- palette bricks of stored bricks (names inferred) ----
// The palette brick a stored brick stands for: the database brick of its design (name = its
// itemNos) with its parts' design IDs, materials, decorations and bone transformations; null
// when the design is unknown or ValidatePaletteBrick fails. Mac 0x148830
RefPointer<BrickPalette::Brick> CreatePaletteBrick(const StorageScene2::Brick& brick, BrickDatabase* database);
// The palette brick matches its database brick: the same number of parts, every part design
// as often as in the database brick, per part as many bone transformations as its primitive
// has bones, max(1, decoration face count) materials and at most that many decorations.
// Mac 0x148b30 (std::map<UInt32, ValidateBrickAccumulator>)
bool ValidatePaletteBrick(const BrickPalette::Brick& brick, BrickDatabase* database);
// The palette brick of every brick of `model` into `bricks` (appended); with `removeInvalid`
// the bricks without one are removed from the model. True when every brick has one.
// Mac 0x148e70 (push_back helper 0x1490d0)
bool CreatePaletteBricks(StorageScene2& model, BrickDatabase* database,
                         BasicArray<RefPointer<const BrickPalette::Brick> >& bricks, bool removeInvalid);
// Element-brick snapshots (PrimitiveElementBrick of each LDDBrick) of the bricks of `parts`.
// Mac 0x148530 (push_back helper 0x1486d0)
void CreateElementBricks(const BasicArray<RefPointer<Part> >& parts,
                         BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks);
// The design of every brick of `scene` and the materials / decorations of its parts are added
// to `data` (once each). Mac 0x149210, Win 0x636faf
void AddLazyLoads(const Scene& scene, BrickConnectSceneManager::LazyLoadData& data);

// The texture `path` (relative to the application's directory 9) loaded with `render`
// (LoadTexture, Mac 0x14b7d0); unreferenced, null on failure. Mac 0x147cd0, Win 0x637835
// (name inferred)
Texture* LoadApplicationTexture(const String& path, Render* render);

// Renders the scene of `control` into a `size` bitmap (a zero area means the control size)
// and writes it as PNG to `path`: with `queue`, that queue through RenderScene, else the scene
// (RenderToBitmap); into `target` when given, else the back buffer (`noPostProcessing`: no
// post effects; `fitCamera`: the camera is fitted to the scene). False when the rendering
// failed; true when the PNG could not be encoded (nothing written), else the write result
// (fixed: false then, and the write is all or nothing; M2, see LDDUtilities.cpp).
// The bitmap: 32-bit BGRA (0x40000220), pitch width * 4. Mac 0x149470, Win 0x637120 (cdecl,
// `size` by value)
bool SaveScreenshot(BrickConnectSceneControl* control, const Path& path, Vector2 size,
                    Texture* target, bool fitCamera, RenderQueue* queue, bool noPostProcessing);

// ---- model loading helpers LDDApplication calls (names and parameter names inferred) ----
// (GetModelBrand Mac 0x148400: LDDUtilities::GetModelBrand above.)
// Builds "Loader error:\n" + "    <error>\n" per error (the text is not used, both builds);
// false when the loader reported a path, version, parser or misc error (mask 0xa03); with a
// BI error the building instructions are dropped. Mac 0x1496c0 (Win inlined)
bool CheckLoaderErrors(const ModelLoader& loader, StorageScene2& scene);
// "Loader error:\n" and "    <Error::ToString>\n" per error appended to `text`. Win 0x636ead
// (thiscall(loader, text)); inlined on the Mac. (name inferred)
void AppendLoaderErrors(const ModelLoader& loader, String& text);
// One alert per error kind of the loader (title "UNABLETOLOADFILE", button "OK"; messages
// PATH_, VERSION_, PARSER_, MISC_, BI_, PART_, JOINT_, CAMERA_LOADFILEERROR in that order).
// `path` and `scene` are passed by the callers but not used. Mac 0x1497a0, Win 0x635f5a
void ShowModelLoadError(const ModelLoader& loader, const Path& path, StorageScene2& scene);
// Reads a model file (an .lxf: its IMAGE100.LXFML) into `scene` with the LXFML 5 loader of
// `database`; on failure the alerts (with `showErrors`) and CheckLoaderErrors decide; the
// database validates the scene. Mac 0x14a680, Win 0x639696
bool LoadModel(const Path& path, StorageScene2& scene, BrickDatabase* database, bool showErrors);

#if defined(_WIN32)
// PNG file of `bitmap` through PNGBitmapCodec into a DataStream of GetEncodedSize bytes (shrunk
// by the codec); null when encoding fails. Win 0x637015 (cdecl, result pointer first)
RefPointer<DataStreamInterface> EncodePNG(const Bitmap& bitmap);
#endif

// The transform that maps the unit box onto `part`'s box: GetPartBox, then GetBoxTransform.
// Used by the selection boxes of BrickConnectSceneControl (Mac 0x37780). Mac 0x14b600,
// Win 0x6374f5 (name inferred)
void GetPartBoxTransform(const Part* part, Matrix3D& transform, bool flag);
// The box of `part` and its transform: one element: the element's transform and the
// primitive's bounding box; several (flexible): identity and the union of the bone boxes
// moved by the elements' transforms. Mac 0x14b530, Win 0x637457 (name inferred)
void GetPartBox(const Part* part, Matrix3D& transform, BoundingBox& box);
// `transform` *= translation to the box minimum (rotated), optionally grown by 0.025 on every
// side (`grow`), then scaled by the box size (+0.05 with `grow`). Mac 0x14b650, Win 0x6369c5
// (name inferred)
void GetBoxTransform(const BoundingBox& box, Matrix3D& transform, bool grow);
// The same for one element: its transform and its bone's box. Mac 0x14b780, Win 0x637542
// (name inferred)
void GetElementBoxTransform(const PrimitiveElement* element, Matrix3D& transform, bool grow);

// ---- the rotation bands of the hinge tool; LDDUtilitiesMeshes.cpp ----
// A band around the y axis: `segments` + 1 vertex pairs (cos a, -radius / 2, sin a) /
// (cos a, radius / 2, sin a) at a = 2 i * 2 pi / segments (NOTE: original quirk: the band winds
// around twice), as a triangle strip (type 4) with texture coordinates (0 | 0.5, 4 i /
// segments) for the front side (`front`) and (0.5 | 1, …) for the back. `color` and `unused`
// are not read (both builds). Unreferenced; null when the render cannot create the buffer.
// Mac 0x14d5f0, Win 0x637f14 (name inferred)
VertexBuffer* CreateRingVertexBuffer(Render* render, const Color& color, bool front, Real32 radius,
                                     Real32 unused, UInt32 segments);
// A SelectableMesh with `material` on CreateRingVertexBuffer. Mac 0x14db80, Win 0x638220
// (cdecl; name inferred)
SelectableMesh* CreateSelectableMesh(Render* render, const Color& color, Material* material,
                                     bool front, Real32 radius, Real32 unused, UInt32 segments);

// The LDDSkybox sound seam (LDDSkybox.h): LDDUtilities::GetInstance()->
// CreateSound(fileName), inlined by the original (Mac 0x147b30 + 0x14c5b0). Restoration glue.
RefPointer<SimpleSound> CreateLDDSound(const String& fileName);

} // namespace LEGO
