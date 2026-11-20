// LEGO Digital Designer — restored source
// TU 128 "UploadModelDialog.cpp" (Mac 0x1e5d30-0x1e86fb; Win 0x694d75-0x695a70): the upload
// dialog (see UploadModelDialog.h). Static initialiser Mac 0x1e8680: <iostream> + a header
// static.
#include "UploadModelDialog.h"

#include "BrickConnectSceneControl.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "LDDWebCommunicator.h"
#include "RenderControlManager.h"
#include "ControlManager.h"
#include "Application.h"
#include "Platform.h"
#include "Directory.h"
#include "Path.h"
#include "Bitmap.h"
#include "Texture.h"
#include "TextLocale.h"
#include "Geometry.h"
#include "Updater.h"

#include <iostream>

namespace LEGO {

// Mac 0x1e5d30 (the copy in UploadToFactoryCommand Win 0x684f3d is inlined)
UploadModelDialog::UploadModelDialog(ControlManager* manager, BrickConnectSceneControl* sceneControl)
    : ModalControl(manager), m_sceneControl(sceneControl), m_state(kStateFinished),
      m_checkPrice(false), m_cancelling(false), m_redirecting(false)
{
}

// Mac 0x1e7d20 (D1) / 0x1e7d80 (D0), thunks 0x1e7df0-0x1e8310; Win sdd 0x694d75
UploadModelDialog::~UploadModelDialog()
{
}

// Mac 0x1e5f80, Win 0x694d97
bool UploadModelDialog::Initialize()
{
    if (!ModalControl::Initialize())
        return false;
    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/General/Popup Final.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image.Get())
        return false;

    m_icon = new RenderControl(m_manager);
    if (!m_icon->Initialize())
        return false;
    m_icon->Setup(GetContent(), 0, 7, Rect(Vector2(10.0f, 10.0f), Vector2()), 0, false);
    m_icon->SetImage(image, 0);
    Rect iconRect(Vector2(453.0f, 63.0f), Vector2(59.0f, 59.0f));
    m_icon->SetImageRect(&iconRect);

    m_title = new RenderControl(m_manager);
    if (!m_title->Initialize())
        return false;
    m_title->Setup(GetContent(), 0, 7, Rect(79.0f, 10.0f, 380.0f, 10.0f),
                   RenderControl::kInputWrapText, false);
    m_title->SetTransparent(true);
    m_title->SetFont(LDDApplication::GetInstance()->GetFont(2));
    m_title->SetText(LocalizedString(String("UPLOADMODELTITLE")));
    m_title->ResizeToFitText();

    m_text = new RenderControl(m_manager);
    if (!m_text->Initialize())
        return false;
    m_text->Setup(GetContent(), 0, 7,
                  Rect(Vector2(m_title->GetPosition().x,
                               m_title->GetPosition().y + m_title->GetSize().y + 4.0f),
                       m_title->GetSize()),
                  RenderControl::kInputWrapText, false);
    m_text->SetTransparent(true);
    m_text->SetFont(LDDApplication::GetInstance()->GetFont(6));
    m_text->SetText(LocalizedString(String("UPLOADMODEL")));

    m_cancel = new LDDButtonControl(m_manager);
    if (!m_cancel->Create(LocalizedString(String("CANCEL")), GetContent(), Vector2()))
        return false;
    m_cancel->SetAlignment(0x33, 0x33, 10.0f, 10.0f, 0);
    ControlObserver::Attach(m_cancel.Get());
    m_text->ResizeToFitText();
    Real32 height = m_text->GetPosition().y + m_text->GetSize().y + 10.0f;
    if (height <= 79.0f)
        height = 79.0f;
    GetContent()->SetSize(Vector2(390.0f, height));
    return true;
}

// Mac 0x1e6840, Win 0x695a0c
void UploadModelDialog::ShowModal()
{
    ModalControl::ShowModal();
    m_state = kStateStart;
    RefPointer<Updater> updater = MainUpdater::GetInstance();
    UpdateObserver::Attach(updater.Get());
}

// Mac 0x1e67e0
void UploadModelDialog::ShowWaiting()
{
    ModalControl::ShowModal();
    m_state = kStateFinished;
    UpdateObserver::Detach();
}

// Mac 0x1e6b40 (thunk 0x1e7b50), Win 0x69529e
void UploadModelDialog::Update()
{
    switch (m_state) {
    case kStateStart:
        m_state = kStateUpload;
        break;

    case kStateUpload: {
        LDDApplication* application = LDDApplication::GetInstance();
        if (!application->GetDocumentPath().Exists()) {
            EndModal();
            LocalizedString message(String("MISSINGUPLOADMODEL"));
            LocalizedString title(String("UPLOADMODELFAILEDTITLE"));
            LocalizedString ok(String("OK"));
            Application::GetInstance()->Alert(2, message, title, ok, String(), String());
            UpdateObserver::Detach();
            break;
        }
        // a square picture of the scene, at most 2048 and at most the window's smaller side
        Bitmap bitmap;
        {
            const Vector2& rootSize = m_manager->GetLayer(0)->GetSize();
            UInt32 width = (UInt32)(Int64)rootSize.x;
            UInt32 height = (UInt32)(Int64)rootSize.y;
            UInt32 side = width < height ? width : height;
            if (side >= 0x800)
                side = 0x800;
            bitmap.m_width = side;
            bitmap.m_height = side;
#if defined(_WIN32)
            bitmap.m_format = kBitmapFormatBGRA32;
            bitmap.m_pitch = (32 >> 3) * side;                  // Win 0x4f5180 (BitmapRowBytes)
#else
            bitmap.m_depth = 1;
            bitmap.m_format = kBitmapFormatRGBA32;
            bitmap.m_pitch = side * 4;
#endif
            bitmap.m_data = new UInt8[bitmap.m_pitch * side];
        }
        if (m_sceneControl->IsBuildingGuideMode())
            m_sceneControl->RebuildRenderData();
        m_sceneControl->RenderToBitmap(bitmap, 0, true, false);
        if (m_sceneControl->IsBuildingGuideMode())
            m_sceneControl->RebuildPartRenderData();

        // the document is saved into the "memory" directory under its own name
        RefPointer<Directory> memory = Directory::GetRoot()->GetDirectory(String("memory"), false);
        Path path(application->GetDocumentPath().GetPath(false), memory.Get());
        LocalizedString message(String("UPLOADMODELFAILED"));
        bool saved = LDDApplication::GetInstance()->SaveModel(
            m_sceneControl.Get(), path, message, LocalizedString(String("UPLOADMODELFAILEDTITLE")),
            true, this);
        if (!saved) {
            // NOTE (both builds): the dialog stays shown without updates; the picture leaks.
            // Fixed: the picture is freed (low).
            UpdateObserver::Detach();
            delete[] (UInt8*)bitmap.m_data;
            delete[] (UInt8*)bitmap.m_palette;
            break;
        }
        if (m_upload.Get())
            MainUpdater::ReleaseLater(m_upload.Get());
        m_upload = new UploadModelCaller();
        {
            AString site = LDDWebCommunicator::GetInstance()->GetBrandName(
                LDDApplication::GetInstance()->GetBrand());
            m_upload->Call(path, bitmap, site);
        }
        delete[] (UInt8*)bitmap.m_data;
        delete[] (UInt8*)bitmap.m_palette;
        m_state = kStateWaiting;
        break;
    }

    case kStateWaiting:
        if ((m_upload.Get() && m_upload->IsDone()) || (m_redirect.Get() && m_redirect->IsDone()))
            m_state = kStateFinished;
        break;

    default:
        m_state = kStateFinished;
        break;

    case kStateFinished:
        if (!m_redirecting) {
            // NOTE (both builds): after a failed upload the original leaves the dialog in this
            // state without a caller and dereferences null on the next update (Mac 0x1e6b40 /
            // Win 0x69529e, state 3). Here a failure closes the dialog like the missing-model
            // branch (EndModal, alert, Detach); this check is only a safety net.
            if (!m_upload.Get())
                break;
            if (!m_upload->HasError()) {
                String url(m_upload->GetResultURL());
                if (m_checkPrice) {
                    url << "&checkprice=true";
                    m_checkPrice = false;
                }
                m_redirecting = true;
                m_state = kStateWaiting;
                if (m_redirect.Get())
                    MainUpdater::ReleaseLater(m_redirect.Get());
                m_redirect = new GetRedirectURLCaller();
                m_redirect->Call(url);
            } else {
                EndModal();
                LocalizedString message(String("UPLOADMODELFAILED"));
                LocalizedString title(String("UPLOADMODELFAILEDTITLE"));
                LocalizedString ok(String("OK"));
                Application::GetInstance()->Alert(2, message, title, ok, String(), String());
                UpdateObserver::Detach();
            }
            MainUpdater::ReleaseLater(m_upload.Get());
            m_upload = 0;
        } else {
            EndModal();
            String url(m_redirect->GetURL());
            Platform::GetInstance()->OpenURL(url);
            MainUpdater::ReleaseLater(m_redirect.Get());
            m_redirect = 0;
            UpdateObserver::Detach();
            m_redirecting = false;
        }
        break;

    case kStateCancelled:
        if (!m_cancelling) {
            if (m_upload.Get()) {
                m_upload->Cancel();
            } else {
                if (!m_redirect.Get())
                    return;
                m_redirect->Cancel();
            }
            m_cancelling = true;
            return;
        }
        if (m_upload.Get() && m_upload->IsDone()) {
            EndModal();
            MainUpdater::ReleaseLater(m_upload.Get());
            m_upload = 0;
        } else {
            if (!m_cancelling || !m_redirect.Get() || !m_redirect->IsDone())
                return;
            EndModal();
            MainUpdater::ReleaseLater(m_redirect.Get());
            m_redirect = 0;
        }
        UpdateObserver::Detach();
        m_cancelling = false;
        break;
    }
}

// Mac 0x1e7ce0 (thunk 0x1e7d00), Win 0x694abb
void UploadModelDialog::OnMouseClicked(Control* control, const Vector2&, UInt32)
{
    if (m_cancel.Get() == control)
        m_state = kStateCancelled;
}

// Mac 0x1e7b70
void UploadModelDialog::OnFinalRelease()
{
    UpdateObserver::OnFinalRelease();
    ModalControl::OnFinalRelease();
    ControlObserver::OnFinalRelease();
}

} // namespace LEGO
