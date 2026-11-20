// LEGO Digital Designer — restored source
// LEGO::ModelLoaderProgressBarThread — the background thread that builds a Scene from a
//   StorageScene2 (Scene::Load with itself as the Scene::LoadCallback) while a progress bar
//   runs: palette bricks are matched per loaded brick (OnBrick), flexible parts collected, the
//   building instructions converted, and the bricks / materials / decorations to load lazily
//   gathered. BrickConnectSceneControl::OnLoadStarted (Mac 0x33e90) creates it (new 0x154,
//   inline constructor Mac 0x53190 / Win 0x62073b) and starts it; OnLoadFinished (Mac 0x346c0)
//   reads the result and the lazy load data, then deletes it (Thread D0).
//
// Home TU 124 "ModelLoaderProgressBarThread.cpp" (Mac 0x1da440-0x1db0fb, Win 0x657f6f-0x65834a
// + 0x62085d-0x620901).
// RTTI: ModelLoaderProgressBarThread : ProgressBarThread (: Thread), Scene::LoadCallback (+0x78).
// Vtables: Mac 0x722c48 {D1 0x1dada0, D0 0x1dadb0, OnThreadStart 0x1da500, OnThreadEnd 0x1da520,
//   Run 0x1da630, OnAttach, OnDetach, OnBrick 0x1daba0, OnProgress 0x1dad60}, LoadCallback-in
//   0x722c74 {thunks 0x1dadf0, 0x1dae10, 0x1dad40, 0x1dad80}; Win 0x9cfa44 {sdd 0x620901,
//   OnThreadStart 0x657f6f, OnThreadEnd 0x657f93, Run 0x657ff2, …}, LoadCallback-in 0x9cfa34
//   {sdd thunk 0x620855, OnBrick 0x65834b, OnProgress 0x657ef6}.
// Mac layout below (sizeof 0x154); Win: the BasicMaps of the storage map are 0x24 bytes, so the
//   arrays after them move (collided +0x110, missing +0x120).
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Scene.h"
#include "StorageScene2.h"
#include "ProgressBarThread.h"
#include "BrickConnectSceneManager.h"
#include "BrickPalette.h"

#include <map>
#include <vector>

namespace LEGO {

class Part;
class BrickPalette;
class BrickDatabase;
class BI;

class ModelLoaderProgressBarThread : public ProgressBarThread, public Scene::LoadCallback {
public:
    // Reads the "SortByIdOnly" preference. Mac 0x53190 (inline constructor emitted in TU 021),
    // Win 0x62073b.
    // NOTE: the scene manager, scene, parts and BI pointers are not initialised (both builds);
    // Start sets them. m_sortByIdOnly stays uninitialised without the preference. Fixed: 0 /
    // false (low).
    ModelLoaderProgressBarThread();
    // Members, then Thread. Mac 0x1dada0 / 0x1dadb0 (body 0x1dae50), Win 0x62085d / sdd 0x620901
    virtual ~ModelLoaderProgressBarThread();

    // Loads `storage` into `scene` on the thread: `parts` receives the flexible parts, `bi` the
    // building instructions (may be null); the database is the scene manager's. Mac 0x1da440
    void Start(BrickConnectSceneManager* sceneManager, Scene* scene, StorageScene2* storage,
               BrickPalette* palette, BasicArray<RefPointer<Part> >* parts, BI* bi);

    // ---- result (read by BrickConnectSceneControl / ModelLoaderHandler) ----
    bool   Succeeded() const { return GetResult(); }                        // +0x58
    UInt32 GetMissingBrickCount() const { return m_missingCount; }          // +0x9c
    UInt32 GetUnplaceableBrickCount() const { return m_unplaceableCount; }  // +0xa0
    // The bricks / materials / decorations to load lazily. +0x118
    const BrickConnectSceneManager::LazyLoadData& GetLazyLoadData() const { return m_lazyLoadData; }
    bool   IsSortByIdOnly() const { return m_sortByIdOnly; }

protected:
    // ---- Thread ----
    // The physics of the scene manager's BrickConnect is told a load starts (slot 3) / ends
    // (slot 4). Mac 0x1da500 / 0x1da520, Win 0x657f6f / 0x657f93
    virtual void OnThreadStart();
    // Also drops every input and the collided / missing bricks (the counts stay).
    virtual void OnThreadEnd();
    // Scene::Load; flexible parts; building instructions; lazy loads; the counts; with the
    // DEVELOPERMODE preference a statistics trace. A cancelled load clears the BI, the scene
    // and the parts and fails. Mac 0x1da630, Win 0x657ff2
    virtual bool Run();

    // ---- Scene::LoadCallback ----
    // The palette brick of `storage` (CreatePaletteBrick) is looked up in the palette (current
    // set, then set 1) and given to the brick; unknown ones are recorded as missing and their
    // parts skipped. Mac 0x1daba0 (thunk 0x1dad40), Win 0x65834b
    virtual bool OnBrick(const StorageScene2::Brick* storage, Brick* brick);
    // Keeps the percentage; false once cancelled. Mac 0x1dad60 (thunk 0x1dad80), Win 0x657ef6
    virtual bool OnProgress(UInt32 percent);

    // ---- data (Mac offsets) ----
    BrickConnectSceneManager*            m_sceneManager;      // +0x7c
    Scene*                               m_scene;             // +0x80
    RefPointer<StorageScene2>            m_storage;           // +0x84
    RefPointer<BrickPalette>             m_palette;           // +0x88
    BasicArray<RefPointer<Part> >*       m_flexibleParts;     // +0x8c
    BI*                                  m_bi;                // +0x90
    BrickDatabase*                       m_database;          // +0x94 the scene manager's
    bool                                 m_sortByIdOnly;      // +0x98 preference "SortByIdOnly"
    UInt32                               m_missingCount;      // +0x9c (accumulates)
    UInt32                               m_unplaceableCount;  // +0xa0
    Scene::StorageMap                    m_storageMap;        // +0xa4
    BasicArray<RefPointer<StorageScene2::Brick> > m_collided; // +0xf8 placed nowhere (Scene::Load)
    BasicArray<RefPointer<StorageScene2::Brick> > m_missing;  // +0x108 no palette brick
    BrickConnectSceneManager::LazyLoadData m_lazyLoadData;    // +0x118
    // LDD64: OnBrick's palette brick per brick description (design ID, then per part: design
    // ID, bone count, materials, decorations), so that equal bricks are looked up once.
    std::map<std::vector<UInt32>, const BrickPalette::Brick*> m_paletteBricks;
};

} // namespace LEGO
