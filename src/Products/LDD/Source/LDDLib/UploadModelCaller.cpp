// LEGO Digital Designer — restored source
// TU 102 "UploadModelCaller.cpp" (Mac 0x1b6b20-0x1b835b; Win 0x697cf1-0x69819b): the reply
// parser and the request of UploadModelCaller, with the file and picture helpers of the TU.
// Static initialiser Mac 0x1b82e0: <iostream> + a header static.
#include "UploadModelCaller.h"

#include "Path.h"
#include "File.h"
#include "Bitmap.h"
#include "DataStream.h"
#include "Base64.h"
#include "XMLDOMNode.h"
#include "HTTPCommunicator.h"
#include "Socket.h"
#include "LDDWebCommunicator.h"
#include "LDDUtilities.h"
#if !defined(_WIN32)
#include "MacImageIO.h"
#endif

#include <string.h>
#include <iostream>

namespace LEGO {

namespace {

// The whole file `path` into `bytes`; false when it cannot be opened or read completely.
// Mac 0x1b72f0, Win 0x697e8b (names inferred)
bool ReadModelFile(const Path& path, BasicArray<UInt8>& bytes)
{
    RefPointer<File> file = path.OpenFile(OpenMode(true, false, true, true), false);
    if (!file.Get())
        return false;
    bytes.Resize((UInt32)file->GetSize(), 0);
    UInt64 read = 0;
    file->ReadAt(0, file->GetSize(), bytes.Begin(), &read);
    return read == file->GetSize();
}

// The PNG file of `bitmap` into `bytes` (empty when encoding fails); true.
// Mac 0x1b7440, Win 0x697f3a
bool EncodePicture(const Bitmap& bitmap, BasicArray<UInt8>& bytes)
{
#if defined(_WIN32)
    RefPointer<DataStreamInterface> png = EncodePNG(bitmap);
#else
    RefPointer<DataStreamInterface> png = EncodePNGWithImageIO(bitmap);
#endif
    if (!png.IsNull()) {
        bytes.Resize(png->GetSize(), 0);
        memcpy(bytes.Begin(), png->GetData(), png->GetSize());
    }
    return true;
}

// Every `from` of `text` (one character) replaced by `to`. Mac 0x1b7580 (Win: the shared
// BasicString<char>::ReplaceAll copy, 0x697fad -> 0x43a790)
void ReplaceAll(AString& text, const char* from, const char* to)
{
    text.ReplaceAll(from, (UInt32)strlen(from), to, (UInt32)strlen(to));
}

} // namespace

// Mac 0x1b7710 (D1) / 0x1b7770 (D0), Win sdd 0x694a7d
UploadModelCaller::~UploadModelCaller()
{
}

// Mac 0x1b6b20, Win 0x697cf1
void UploadModelCaller::OnResponse(UInt32 size)
{
    RefPointer<XMLDOM> dom = new XMLDOM();
    if (!ParseReply(size, dom.Get())) {
        SetFailed();
        return;
    }
    bool parsed = GetChildText(dom.Get(), String("resultURL"), m_resultURL)
                  && GetChildText(dom.Get(), String("errorDescription"), m_errorDescription)
                  && GetChildText(dom.Get(), String("error"), m_error);
    if (!parsed)
        SetFailed();
    else
        SetSucceeded();
}

// Mac 0x1b6ce0, Win 0x697fd6
void UploadModelCaller::Call(const Path& model, const Bitmap& picture, const AString& targetSite)
{
    AString data;
    BasicArray<UInt8> modelBytes;
    if (!ReadModelFile(model, modelBytes)) {
        SetFailed();
        return;
    }
    AString modelText;
    if (!Base64Encode(modelBytes, modelText)) {
        SetFailed();
        return;
    }
    data.Append("model=");
    data.Append(modelText);

    BasicArray<UInt8> pictureBytes;
    if (!EncodePicture(picture, pictureBytes)) {
        SetFailed();
        return;
    }
    AString pictureText;
    if (!Base64Encode(pictureBytes, pictureText)) {
        SetFailed();
        return;
    }
    data.Append("&modelImage=");
    data.Append(pictureText);
    data.Append("&targetSite=");
    data.Append(targetSite);
    ReplaceAll(data, "+", "%2B");
    ReplaceAll(data, "/", "%2F");
    Request(LDDWebCommunicator::GetInstance()->GetUploadModelURL(), data);
}

// Mac 0x1b7540 (Win: ICF with GetRedirectURLCaller::Cancel)
void UploadModelCaller::Cancel()
{
    HTTPCommunicator* communicator = m_communicator.Get();
    if (communicator->GetSocket()) {
        communicator->GetSocket()->Close();
        communicator->ResetSocket();
    }
}

} // namespace LEGO
