// LEGO Digital Designer — restored source
// ModelLoaderProgressBarThread — TU 124 "ModelLoaderProgressBarThread.cpp" (Mac 0x1da440-0x1db0fb,
// Win 0x657f6f-0x65834a + 0x62085d-0x620901). See ModelLoaderProgressBarThread.h.
#include "ModelLoaderProgressBarThread.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnect.h"
#include "BrickPhysics.h"
#include "BrickPalette.h"
#include "BrickDatabase.h"
#include "BI.h"
#include "LDDPart.h"
#include "LDDUtilities.h"
#include "Preferences.h"
#include "Trace.h"
#include "Brick.h"
#include "Part.h"

namespace LEGO {

// Mac 0x53190 (inline, emitted in TU 021), Win 0x62073b
ModelLoaderProgressBarThread::ModelLoaderProgressBarThread()
    : m_storage(0), m_palette(0), m_database(0), m_missingCount(0), m_unplaceableCount(0)
{
    m_sceneManager = 0;
    m_scene = 0;
    m_flexibleParts = 0;
    m_bi = 0;
    m_sortByIdOnly = false;
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    preferences->GetValue(String("SortByIdOnly"), m_sortByIdOnly);
}

// Mac 0x1dada0 / 0x1dadb0 (body 0x1dae50), Win 0x62085d / sdd 0x620901
ModelLoaderProgressBarThread::~ModelLoaderProgressBarThread()
{
}

// Mac 0x1da440
void ModelLoaderProgressBarThread::Start(BrickConnectSceneManager* sceneManager, Scene* scene,
                                         StorageScene2* storage, BrickPalette* palette,
                                         BasicArray<RefPointer<Part> >* parts, BI* bi)
{
    m_sceneManager = sceneManager;
    m_scene = scene;
    m_storage = storage;
    m_palette = palette;
    m_flexibleParts = parts;
    m_bi = bi;
    m_database = sceneManager->GetDatabase();
    m_progress = 0;
    Thread::Start(0);
}

// Mac 0x1da500, Win 0x657f6f
void ModelLoaderProgressBarThread::OnThreadStart()
{
    m_sceneManager->GetBrickConnect()->GetPhysics()->Unknown3();
}

// Mac 0x1da520, Win 0x657f93
void ModelLoaderProgressBarThread::OnThreadEnd()
{
    m_sceneManager->GetBrickConnect()->GetPhysics()->Unknown4();
    m_sceneManager = 0;
    m_scene = 0;
    m_storage = 0;
    m_palette = 0;
    m_flexibleParts = 0;
    m_bi = 0;
    m_database = 0;
    m_collided.Clear();
    m_missing.Clear();
}

// Mac 0x1da630, Win 0x657ff2
bool ModelLoaderProgressBarThread::Run()
{
    if (m_scene->Load(*m_storage, m_storageMap, m_collided, m_missing, m_database, this, 0)) {
        // the flexible parts (more than one element) are handed to the control
        const BasicArray<RefPointer<Brick> >& bricks = m_scene->GetBricks();
        for (UInt32 i = 0; i < bricks.GetSize(); ++i) {
            const BasicArray<RefPointer<Part> >& parts = bricks[i]->GetParts();
            for (UInt32 j = 0; j < parts.GetSize(); ++j) {
                if (parts[j]->GetElementCount() > 1)
                    m_flexibleParts->PushBack(RefPointer<Part>(parts[j]));
            }
        }
        if (m_bi != 0 && !m_storage->m_buildingInstructions.IsEmpty()) {
            const StorageScene2::BI& instructions = m_storage->m_buildingInstructions[0];
            m_bi->SetName(instructions.m_name);
            RefPointer<BIStorage> steps = new BIStorage;
            steps->Load(instructions, m_storageMap, m_scene);
            m_bi->SetStorage(steps);
        }
        AddLazyLoads(*m_scene, m_lazyLoadData);
    }
    m_unplaceableCount = m_collided.GetSize();
    m_missingCount += m_missing.GetSize();

    // NOTE: uninitialised without the preference (both builds); fixed: false (low)
    bool developerMode = false;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        preferences->GetValue(String("DEVELOPERMODE"), developerMode);
    }
    if (developerMode) {
        Trace(String("------------------------Statistics for load of model------------------------"), true);
        for (UInt32 i = 0; i < m_collided.GetSize(); ++i) {
            String text("Cannot place brick: ");
            Trace(text << m_collided[i]->m_designID, true);
        }
        for (UInt32 i = 0; i < m_missing.GetSize(); ++i) {
            String text("Cannot find brick: ");
            String description = LDDUtilities::GetBrickDescription(m_database, m_missing[i]);
            Trace(text << description, true);
        }
        Trace(String("----------------------------------------------------------------------------"), true);
    }

    if (!m_cancelled)
        return true;
    if (m_bi != 0)
        m_bi->Reset();
    m_scene->Clear();
    m_flexibleParts->Clear();
    return false;
}

// Mac 0x1daba0 (thunk 0x1dad40), Win 0x65834b
// LDD64: the palette brick depends only on the brick's design ID and its parts' design IDs, bone
// counts, materials and decorations (CreatePaletteBrick, ValidatePaletteBrick, MakeSortKey), so it
// is looked up once per distinct description; a model of a million bricks has a few thousand.
bool ModelLoaderProgressBarThread::OnBrick(const StorageScene2::Brick* storage, Brick* brick)
{
    std::vector<UInt32> key;
    key.push_back(storage->m_designID);
    for (UInt32 i = 0; i < storage->m_parts.GetSize(); ++i) {
        const StorageScene2::Part* part = storage->m_parts[i];
        key.push_back(part->m_designID);
        key.push_back(part->m_bones.GetSize());
        key.push_back(part->m_materials.GetSize());
        key.insert(key.end(), part->m_materials.Begin(), part->m_materials.End());
        key.push_back(part->m_decorations.GetSize());
        key.insert(key.end(), part->m_decorations.Begin(), part->m_decorations.End());
    }
    std::map<std::vector<UInt32>, const BrickPalette::Brick*>::iterator cached = m_paletteBricks.find(key);
    const BrickPalette::Brick* found = 0;
    if (cached != m_paletteBricks.end()) {
        found = cached->second;
    } else {
        RefPointer<BrickPalette::Brick> paletteBrick = CreatePaletteBrick(*storage, m_database);
        if (paletteBrick) {
            found = m_palette->FindBrick(*paletteBrick, *m_database);
            if (found == 0)
                found = m_palette->FindBrick(*paletteBrick, *m_database, kBrickSetLDDExtended);
        }
        m_paletteBricks[key] = found;
    }
    if (found != 0) {
        static_cast<LDDBrick*>(brick)->SetPaletteBrick(found);
        return true;
    }
    m_missing.PushBack(RefPointer<StorageScene2::Brick>(const_cast<StorageScene2::Brick*>(storage)));
    return false;
}

// Mac 0x1dad60 (thunk 0x1dad80), Win 0x657ef6
bool ModelLoaderProgressBarThread::OnProgress(UInt32 percent)
{
    m_progress = percent;
    return !m_cancelled;
}

} // namespace LEGO
