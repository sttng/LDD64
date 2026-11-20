// LEGO Digital Designer — restored source
// LEGO::HTMLBIGenerator — writes the building instructions of the building-guide mode as an HTML
//   page: "Building Instructions [<model>].html" plus a folder "Building Instructions
//   [<model>]-images" with one PNG per step ("Step<n>.png", 800 x 600, the scene of the mode as
//   the building guide shows it) and one per distinct brick ("Brick<i>.png", 64 x 64, a
//   BricksControl), and a copy of the template images ("Images").
// LEGO::HTMLBIGeneratorObserver — Observer<HTMLBIGenerator, HTMLBIGeneratorObserver> with three
//   pure notifications (the generator never sends them; HTMLBICommand implements them).
//
// Generate (HTMLBICommand::Do): folder chooser (Preferences "UserBIDirectory"), writability and
// overwrite checks, image folder with a copy of <assets>/Templates/Images, the five UTF-8
// templates of <assets>/Templates (BITemplate, BIStepTemplate, BIPaletteListTemplate,
// PartListTemplate, ElementListTemplate .html), guide rewound, page-wide tags filled, then the
// MainUpdater and a ProgressBarDialog ("GENERATEHTMLBI"). Each Update writes one step (step
// template with #StepNumber#, #StepImage#, #PartList#) before the page's #BISteps# marker. At the
// end the overview tags (#OverviewImage#, #ImageDir#, #ElementList#, #MaxStepNumber#,
// #BrickCount#) are filled and the page is written and opened (Platform::OpenDocument).
// Cancelling the dialog deletes the output.
//
// TU 113 "HTMLBIGenerator.cpp" (Mac 0x1c06a0-0x1c829b, 61 functions; Win 0x68dd8c-0x6910f7 +
// 0x675447-0x675489 and the helpers 0x68da7e-0x68dbd7). The observer class is emitted in TU 240
// (HTMLBICommand.cpp).
//
// RTTI (Mac vmi): UpdateObserver +0x00, Notifier<HTMLBIGeneratorObserver> +0x08,
//   PreferencesObserver +0x2c, virtual base Object +0x130 (sizeof 0x138, operator new in
//   HTMLBICommand's constructor). Win: UpdateObserver +0x00, PreferencesObserver +0x0c,
//   Notifier +0x18, Object +0xec.
// Vtables (Mac): primary 0x71f6dc {D1 0x1c6510, D0 0x1c6570, OnFinalRelease 0x1c65e0, OnAttached
//   0x1e380, OnDetached 0x1e390, Update 0x1c3140, OnPreferenceChanged 0x1c5ed0 (the Mac-only
//   primary entry of the PreferencesObserver overrider)}; Notifier-in 0x71f704 {0x1c66c0,
//   0x1c6730, 0x1c67b0}; PreferencesObserver-in 0x71f71c {0x1c67d0, 0x1c6840, 0x1c68c0, OnAttached,
//   OnDetached, OnPreferenceChanged thunk 0x1c6000}; Object-in 0x71f74c {0x1c68e0, 0x1c6950,
//   0x1c69d0, AddRef, Release}. Win: UpdateObserver-in 0x9dcd0c {.., Update 0x6908a9},
//   PreferencesObserver-in 0x9dccfc {.., OnPreferenceChanged 0x68dd8c}, Object-in 0x9dcce8
//   {sdd 0x68efbe -> 0x68f8f5}.
// The class names are RTTI; method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Notifier.h"
#include "Updater.h"
#include "Preferences.h"
#include "Geometry.h"
#include "PrimitiveElementBrick.h"

namespace LEGO {

class HTMLBIGenerator;
class BGMode;
class BrickConnectSceneControl;
class BrickConnectSceneManager;
class RenderControlManager;
class RenderQueue;
class ProgressBarDialog;
class SimpleSound;
class Directory;
class Window;
class Texture;
class ModelControl;

// Mac vtable 0x738f1c {D1 0x270e60, D0 0x270e90, OnFinalRelease 0x270ee0 (Observer's),
// OnAttached 0x270f30, OnDetached 0x270f40, 3 x pure}; Win 0x9d7070 {OnAttached, OnDetached,
// 3 x _purecall}. Layout: Observer (vptr, subject +0x04), Object +0x08 (Mac).
class HTMLBIGeneratorObserver : public Observer<HTMLBIGenerator, HTMLBIGeneratorObserver> {
public:
    HTMLBIGeneratorObserver() {}
    virtual ~HTMLBIGeneratorObserver() {}                                    // Mac 0x270e60/0x270e90, Win sdd 0x65e892

    // (names inferred; nothing calls them in either build)
    virtual void OnGenerationStarted() = 0;                                  // Mac 5, Win 2
    virtual void OnGenerationFinished() = 0;                                 // Mac 6, Win 3
    virtual void OnGenerationEvent() = 0;                                    // Mac 7, Win 4
};

class HTMLBIGenerator : public UpdateObserver,
                        public Notifier<HTMLBIGeneratorObserver>,
                        public PreferencesObserver {
public:
    typedef BasicMap<RefPointer<PrimitiveElementBrick>, UInt32, PrimitiveElementBrick::BrickLess> BrickCountMap;

    // Sizes of the pictures (Mac immediates; Win globals 0xab8b00-0xab8b14 with the same
    // values). (names inferred)
    enum {
        kOverviewImageWidth  = 800, kOverviewImageHeight = 600,   // #OverviewImageSize#
        kStepImageWidth      = 800, kStepImageHeight     = 600,   // step pictures, #StepImageSize#
        kPageWidth           = 1024,                              // #StepImageWidth2# (step width + 224)
        kBrickImageWidth     = 64,  kBrickImageHeight    = 64     // brick pictures, #BrickImageSize#
    };

    // Keeps the arguments (the scene control referenced), observes the Preferences, loads the
    // pause sound and creates its render queue (manager's render, 2000 entries).
    // Mac 0x1c0740, Win 0x68ed28
    HTMLBIGenerator(BrickConnectSceneControl* sceneControl, RenderControlManager* manager,
                    BrickConnectSceneManager* sceneManager, BGMode* mode);
    // Members only. Mac 0x1c6510 (D1, base body 0x1c71a0 with the VTT) / 0x1c6570 (D0),
    // Win sdd 0x68f8f5 (body 0x68eff3)
    virtual ~HTMLBIGenerator();

    // Starts a generation (see the header comment); `window` parents the folder chooser.
    // False when it does not start (already running, cancelled, an error alert, or the progress
    // dialog failed to initialise — NOTE: then the generator stays attached to the updater and
    // runs without a dialog: Update calls the null dialog, both builds). Mac 0x1c0d80,
    // Win 0x68f917 (thiscall)
    bool Generate(Window* window);

    // A generation is running (attached to the updater). Mac inlined, Win 0x68da9d
    bool IsRunning() const { return UpdateObserver::GetSubject() != 0; }

    // ---- UpdateObserver: one step per main-loop pass. Mac 0x1c3140, Win 0x6908a9
    virtual void Update();

protected:
    // Leaves the Preferences, the observers and the updater. Mac 0x1c65e0 (thunks 0x1c6730 /
    // 0x1c68c0 / 0x1c69d0)
    virtual void OnFinalRelease();
    // PLAYSOUND: reloads the pause sound. Mac 0x1c5ed0 (thunk 0x1c6000), Win 0x68dd8c
    virtual void OnPreferenceChanged(const String& key);

    // ---- helpers (Mac: internal functions; Win: thiscall members) ----
    // "<assets>/Templates/<name>" decoded as UTF-8; empty when it cannot be read.
    // Mac 0x1c2dc0, Win 0x68defa
    String LoadTemplate(const String& name);
    // One palette-list item per bag of the scene control's brick palette ("#PaletteCount#" = 1,
    // "#PaletteName#" = the bag name). Mac 0x1c2f20, Win 0x68f35d
    String BuildPaletteList();
    // The page of the current step (m_step): the step template with #StepNumber#, #PartList#,
    // #StepImage#; empty when the step picture could not be saved. Mac 0x1c4610, Win 0x68f18b
    String BuildStep();
    // One part-list item per brick of the step (m_stepBricks, BrickLess order); adds the counts
    // to m_partCount. Mac 0x1c51e0, Win 0x68eb5d
    String BuildPartList();
    // One element-list item per distinct brick of the model (m_allBricks). Mac 0x1c48d0,
    // Win 0x68f646
    String BuildElementList();
    // "Brick<i>.png" of `brick` (i = its index among the distinct bricks pictured so far,
    // PrimitiveElementBrick::operator==); a new brick is pictured first (64 x 64 BricksControl,
    // material set 2). Empty when that picture could not be saved. Mac 0x1c53f0, Win 0x68e975
    String GetBrickImage(PrimitiveElementBrick* brick);
    // "<localized design name> - <localized material>, <localized material>…" (the distinct
    // non-zero materials of the brick's own material arrays, ascending). Mac 0x1c56e0,
    // Win 0x68f466
    String GetBrickName(const PrimitiveElementBrick* brick);
    // Renders the mode's scene (BGMode::QueueParts into m_renderQueue) into
    // <image folder>/`name` (LDDUtilities SaveScreenshot, camera fitted). Mac 0x1c5080,
    // Win 0x68dbd7
    bool SaveStepImage(const String& name, const Vector2& size);
    // Renders `control` into a `size` BGRA bitmap and writes it as PNG to <image folder>/`name`.
    // Mac 0x1c5ae0 (Win inlined in GetBrickImage)
    bool SaveImage(const String& name, const Vector2& size, ModelControl* control);
    // Writes m_page (UTF-8) to m_directory/m_fileName and opens it. Mac 0x1c4db0, Win 0x68dfcd
    bool WritePage();
    // Deletes the page and the image folder when they exist. NOTE: not recursively (both
    // builds): an image folder with contents (after Generate: at least Images/) stays.
    // Mac inlined (Generate, Update), Win 0x68db94
    void DeleteOutput();
    // When running: closes the dialog, drops it, leaves the updater. Mac inlined, Win 0x68e81d
    void Stop();
    // target.ReplaceAll(tag, value) over the whole strings. Mac inlined, Win 0x68da7e (cdecl)
    static void ReplaceTag(String& target, const String& tag, const String& value);
    // Advances the guide one step (BGMode::NextStep) and counts the element bricks of its
    // primitive BIBricks into m_stepBricks (cleared first) and m_allBricks (cleared too when
    // `first`; otherwise m_step is incremented after NextStep). The bricks get the identity
    // transform. (restoration helper: the two copies in Update, Mac 0x1c32cc / 0x1c3c7c)
    void TakeStepBricks(bool first);

    // ---- data (Mac / Win offsets) ----
    RefPointer<BrickConnectSceneControl>       m_sceneControl;   // +0x34 / +0x3c
    BGMode*                                    m_mode;           // +0x38 / +0x40
    RenderControlManager*                      m_manager;        // +0x3c / +0x44
    BrickConnectSceneManager*                  m_sceneManager;   // +0x40 / +0x48
    // The distinct bricks pictured (GetBrickImage). RTTI BasicArray<RefPointer<
    // PrimitiveElementBrick const>>, vtable Mac 0x76e3e0 {0x1c06d0, 0x1c7680}, Win 0x68e271
    BasicArray<RefPointer<const PrimitiveElementBrick> > m_brickImages; // +0x44 / +0x4c
    RefPointer<RenderQueue>                    m_renderQueue;    // +0x54 / +0x5c
    UInt32                                     m_stepCount;      // +0x58 / +0x60
    String                                     m_page;           // +0x5c / +0x64 BITemplate.html, filled
    String                                     m_stepTemplate;   // +0x6c / +0x68
    String                                     m_paletteTemplate;// +0x7c / +0x6c
    String                                     m_partTemplate;   // +0x8c / +0x70
    String                                     m_elementTemplate;// +0x9c / +0x74
    RefPointer<Directory>                      m_directory;      // +0xac / +0x78 target folder
    RefPointer<Directory>                      m_imageDirectory; // +0xb0 / +0x7c
    String                                     m_modelName;      // +0xb4 / +0x80 or "No Name"
    String                                     m_imageDirectoryName; // +0xc4 / +0x84
    String                                     m_fileName;       // +0xd4 / +0x88
    // Sum of the part-list counts (BuildPartList; NOTE: never read, both builds).
    UInt32                                     m_partCount;      // +0xe4 / +0x8c
    RefPointer<ProgressBarDialog>              m_progressDialog; // +0xe8 / +0x90
    RefPointer<SimpleSound>                    m_pauseSound;     // +0xec / +0x94
    // Render target of the step pictures (NOTE: never set; null = the back buffer).
    Texture*                                   m_renderTarget;   // +0xf0 / +0x98
    BrickCountMap                              m_stepBricks;     // +0xf4 / +0x9c bricks of the step
    BrickCountMap                              m_allBricks;      // +0x110 bricks of the model
    // The step being written: 0 idle, 1 after Rewind (the first Update takes the first step),
    // incremented with every NextStep.
    UInt32                                     m_step;           // +0x12c / +0xe4
                                                                 // Object +0x130 / +0xec
};

} // namespace LEGO
