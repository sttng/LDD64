// LEGO Digital Designer — restored source
// TU 239 "GroupToLibraryCommand.cpp": Mac 0x26fbd0-0x270beb, Win 0x6846af-0x684aef.
#include "GroupToLibraryCommand.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnect.h"
#include "Scene.h"
#include "StorageScene2.h"
#include "ModelIOLXFML5.h"
#include "LDDApplication.h"
#include "BrickPalette.h"
#include "Brand.h"
#include "UUID.h"
#include "Path.h"
#include "Directory.h"
#include "Application.h"
#include "Platform.h"
#include "TextLocale.h"
#include "SafeFileWrite.h"   // (M2) Do

namespace LEGO {

// Mac 0x26fbd0, Win 0x6846af
GroupToLibraryCommand::GroupToLibraryCommand(BrickConnectSceneControl* sceneControl)
    : Command(false, false)
{
    m_scene = sceneControl->GetSceneManager()->GetBrickConnect()->NewScene(0);
    BrickConnectSceneControlObserver::Attach(sceneControl);
    UpdateEnable();
}

// Mac 0x270710 (D1) / 0x270770 (D0), Win sdd 0x6847fb
GroupToLibraryCommand::~GroupToLibraryCommand()
{
}

// Mac 0x2707e0
void GroupToLibraryCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x26fd70, Win 0x68481a
void GroupToLibraryCommand::Do()
{
    BrickConnectSceneControl* control = GetSubject();
    if (!control || control->GetSelection().IsEmpty())
        return;
    // NOTE: the original copies the parts' materials as they are, the paint bucket's hover
    // preview included; fixed: the previews are undone first (M8).
    control->ClearToolPreviews();

    BasicArray<RefPointer<Brick> > bricks;
    Scene::GetBricks(control->GetSelection(), bricks);
    Scene::CopyMap copyMap;
    control->GetScene()->CopyBricks(bricks, m_scene, copyMap, true,
                                    Scene::CopyFlagGroups | Scene::CopyFlagBricks |
                                    Scene::CopyFlagParts | Scene::CopyFlagElements);   // 0x1e

    RefPointer<StorageScene2> storage = new StorageScene2();
    Scene::SaveMap saveMap;
    m_scene->Save(*storage, saveMap);
    m_scene->Clear();

    String brandName;
    ToString(LDDApplication::GetInstance()->GetBrand(), brandName);
    StorageScene2::Meta meta;
    meta.m_name = "Brand";
    StorageScene2::Meta::Attribute attribute;
    attribute.m_name = "name";
    attribute.m_value = brandName;
    meta.m_attributes.PushBack(attribute);
    storage->m_metas.PushBack(meta);

    UUID uuid;
    uuid.Generate();
    String name("LibraryGroups/");
    name << brandName << "/" << uuid << ".lxfml";
    Path path(name, Application::GetInstance()->GetUserDataDirectory());

    RefPointer<ModelSaverLXFML5> saver = new ModelSaverLXFML5;
    // NOTE: the original ignores Save's result (Win 0x68481a) and adds the model to the palette
    // all the same, so a failed write (a full disk, a folder that takes no files) shows a library
    // model whose file is missing or cut; fixed: written all or nothing into the brand's folder
    // ("<uuid>.lxfml.tmp" renamed over the file, SafeWriteTarget / CommitSafeWrite), and a
    // failure shows UNABLETOSAVEMODELTO and adds nothing (M2).
    bool saved = false;
    Directory* userData = Application::GetInstance()->GetUserDataDirectory();
    RefPointer<Directory> folder;
    if (userData)
        folder = userData->GetDirectory(String("LibraryGroups/") << brandName, true);
    if (folder) {
        String fileName;
        fileName << uuid << ".lxfml";
        const Path file(fileName, folder.Get());
        const Path target = SafeWriteTarget(file);
        const bool written = saver->Save(target, *storage);              // slot 5
        saved = CommitSafeWrite(file, target, written);
    }
    if (!saved) {
        String message(LocalizedString(String("UNABLETOSAVEMODELTO")));
        message.ReplaceAll(String("%s"), 0, String::npos,
                           Platform::GetInstance()->VFSToNativePath(path.GetPath(true)), 0, String::npos);
        LocalizedString title(String("ERRORTITLE"));
        LocalizedString ok(String("OK"));
        Application::GetInstance()->Alert(2, message, title, ok, String(), String());
        return;
    }
    LDDApplication::GetInstance()->GetBrickPalette()->AddModel(storage, path);
}

// Mac 0x270670, Win 0x66e2c6
void GroupToLibraryCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = GetSubject();
    SetEnable(control && !control->GetSelection().IsEmpty());
}

// Mac 0x270870, Win 0x66e988
void GroupToLibraryCommand::OnSceneControlAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x2706b0 (thunk 0x2706e0), Win 0x684af0
void GroupToLibraryCommand::OnEvent14(bool empty)
{
    SetEnable(!empty);
}

} // namespace LEGO
