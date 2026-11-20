// LEGO Digital Designer — restored source
// ModelLoader / ModelLoader::*Error / ModelSaver (see ModelLoader.h; header-inline in the
// original, restored out of line). Plan item 2.1, Mac TU 338 + the weak copies listed in the
// header, Win 0x4a2d60-0x4a3ea0, 0x4d8290-0x4d8560.
#include "ModelLoader.h"
#include "ReadFile.h"
#include "StringStream.h"

#include <iostream>

namespace LEGO {

// ============================================================================================
// ModelLoader::Error
// ============================================================================================

ModelLoader::Error::Error(UInt32 type) : m_type(type) {}                    // Win 0x4a2ed0

ModelLoader::Error::~Error() {}                                             // Mac 0x2f38b0, Win 0x4a2f30

void ModelLoader::Error::ToString(String& out) const                         // Mac 0x2f3970, Win 0x4a2d60
{
    const char* name;
    switch (m_type) {
    case noError:      name = "noError"; break;
    case pathError:    name = "pathError"; break;
    case versionError: name = "versionError"; break;
    case jointError:   name = "jointError"; break;
    case cameraError:  name = "cameraError"; break;
    case brickError:   name = "brickError"; break;
    case partError:    name = "partError"; break;
    case boneError:    name = "boneError"; break;
    case rigidError:   name = "rigidError"; break;
    case biError:      name = "biError"; break;
    case parserError:  name = "parserError"; break;
    case miscError:    name = "miscError"; break;
    default:           name = "unknown"; break;   // NOTE: also contentError (0x400)
    }
    out << name;
    if (!m_description.IsEmpty())
        out << " Description: " << m_description;
}

// ============================================================================================
// ModelLoader::PathError
// ============================================================================================

// NOTE: Mac default-constructs the path and assigns it (Path::Assign), Win copies (0x412680).
ModelLoader::PathError::PathError(const Path& path) : Error(pathError)     // Mac 0x2f36e0, Win 0x4d8290
{
    m_path = path;
}

ModelLoader::PathError::~PathError() {}                                     // Mac 0x2f3b80, Win 0x4d8380

void ModelLoader::PathError::ToString(String& out) const                     // Mac 0x2f3820, Win 0x4d8300
{
    Error::ToString(out);
    out << " path " << m_path.GetPath(true);
}

// ============================================================================================
// ModelLoader::ContentError
// ============================================================================================

ModelLoader::ContentError::ContentError(UInt32 detail)                       // Win 0x4a39d0
    : Error(contentError), m_detail(detail)
{
}

ModelLoader::ContentError::~ContentError() {}                               // Mac 0x2f3c50, Win 0x4a3aa0

void ModelLoader::ContentError::ToString(String& out) const                  // Mac 0x2f3d10, Win 0x4a3a40
{
    Error::ToString(out);
    out << " error detail ";
    const char* detail;
    if (m_detail == missingLXFMLTag)
        detail = "missingLXFMLTag";
    else if (m_detail == empty)
        detail = "empty";
    else
        detail = "unknown";
    out << detail;
}

// ============================================================================================
// ModelLoader::VersionError
// ============================================================================================

ModelLoader::VersionError::VersionError(const Version& expected, const Version& actual) // Win 0x4a2f90
    : Error(versionError), m_expected(expected), m_actual(actual)
{
}

ModelLoader::VersionError::~VersionError() {}                               // Mac 0x2fc760, Win 0x4a3070

void ModelLoader::VersionError::ToString(String& out) const                  // Mac 0x2fc820, Win 0x4a3020
{
    Error::ToString(out);
    out << " expected Version ";
    LEGO::ToString(m_expected, out);
    out << " actual Version ";
    LEGO::ToString(m_actual, out);
}

// ============================================================================================
// ModelLoader::ParserError / JointError
// ============================================================================================

ModelLoader::ParserError::ParserError() : Error(parserError) {}             // Win 0x4a3910

ModelLoader::ParserError::~ParserError() {}                                 // Mac 0x2fd350, Win 0x4a3980

void ModelLoader::ParserError::ToString(String& out) const                   // Mac 0x2fd410, Win 0x4a3970
{
    Error::ToString(out);
}

ModelLoader::JointError::JointError(UInt32 detail)                           // Win 0x4a30c0
    : Error(jointError), m_detail(detail)
{
}

ModelLoader::JointError::~JointError() {}                                   // Mac 0x3028e0, Win 0x4a3130

void ModelLoader::JointError::ToString(String& out) const                    // Mac 0x3029a0, Win 0x4a3970
{
    Error::ToString(out);
}

// ============================================================================================
// ModelLoader::CameraError / BrickError / PartError / BoneError / RigidError / BIError
// ============================================================================================

ModelLoader::CameraError::CameraError(UInt32 detail)                         // Win 0x4a3180
    : Error(cameraError), m_detail(detail)
{
}

ModelLoader::CameraError::~CameraError() {}                                 // Mac 0x3107e0, Win 0x4a3270

void ModelLoader::CameraError::ToString(String& out) const                   // Mac 0x3108a0, Win 0x4a31f0
{
    Error::ToString(out);
    out << " error detail ";
    const char* detail;
    if (m_detail == reusedRefID)
        detail = "reusedRefID";
    else if (m_detail == missingRefID)
        detail = "missingRefID";
    else if (m_detail == nonExistingCamera)
        detail = "nonExistingCamera";
    else
        detail = "unknown";
    out << detail;
}

ModelLoader::BrickError::BrickError(UInt32 detail)                           // Win 0x4a32c0
    : Error(brickError), m_detail(detail)
{
}

ModelLoader::BrickError::~BrickError() {}                                   // Mac 0x3246c0, Win 0x4a33b0

void ModelLoader::BrickError::ToString(String& out) const                    // Mac 0x324780, Win 0x4a3330
{
    Error::ToString(out);
    out << " error detail ";
    const char* detail;
    if (m_detail == missingDesignID)
        detail = "missingDesignID";
    else if (m_detail == reusedRefID)
        detail = "reusedRefID";
    else if (m_detail == missingRefID)
        detail = "missingRefID";
    else
        detail = "unknown";
    out << detail;
}

ModelLoader::PartError::PartError(StorageScene2::Part* part, UInt32 detail)  // Mac 0x324800, Win 0x4a3400
    : Error(partError), m_part(part), m_detail(detail)
{
}

ModelLoader::PartError::~PartError() {}                                     // Mac 0x2fbca0, Win 0x4a35a0

void ModelLoader::PartError::ToString(String& out) const                     // Mac 0x2fbb70, Win 0x4a3480
{
    Error::ToString(out);
    out << " error detail ";
    const char* detail;
    switch (m_detail) {
    case missingRefID:    detail = "missingRefID"; break;
    case reusedRefID:     detail = "reusedRefID"; break;
    case missingDesignID: detail = "missingDesignID"; break;
    case missingMaterial: detail = "missingMaterial"; break;
    case wrongMaterial:   detail = "wrongMaterial"; break;
    case wrongDecoration: detail = "wrongDecoration"; break;
    case notOneMaterial:  detail = "notOneMaterial"; break;
    case assembly:        detail = "assembly"; break;
    case flex:            detail = "flex"; break;
    case misc:            detail = "misc"; break;
    default:              detail = "unknown"; break;
    }
    out << detail;
    if (m_part)
        out << " design id " << m_part->m_designID;
}

ModelLoader::BoneError::BoneError(UInt32 detail)                             // Win 0x4a3610
    : Error(boneError), m_detail(detail)
{
}

ModelLoader::BoneError::~BoneError() {}                                     // Mac 0x3248c0, Win 0x4a36e0

void ModelLoader::BoneError::ToString(String& out) const                     // Mac 0x324980, Win 0x4a3680
{
    Error::ToString(out);
    out << " error detail ";
    const char* detail;
    if (m_detail == reusedRefID)
        detail = "reusedRefID";
    else if (m_detail == missingRefID)
        detail = "missingRefID";
    else
        detail = "unknown";
    out << detail;
}

ModelLoader::RigidError::RigidError(UInt32 detail)                           // Win 0x4a3730
    : Error(rigidError), m_detail(detail)
{
}

ModelLoader::RigidError::~RigidError() {}                                   // Mac 0x3249f0, Win 0x4a37a0

void ModelLoader::RigidError::ToString(String& out) const                    // Mac 0x324ab0, Win 0x4a3680
{
    Error::ToString(out);
    out << " error detail ";
    const char* detail;
    if (m_detail == reusedRefID)
        detail = "reusedRefID";
    else if (m_detail == missingRefID)
        detail = "missingRefID";
    else
        detail = "unknown";
    out << detail;
}

ModelLoader::BIError::BIError(UInt32 detail)                                 // Win 0x4a37f0
    : Error(biError), m_detail(detail)
{
}

ModelLoader::BIError::~BIError() {}                                         // Mac 0x313830, Win 0x4a38c0

void ModelLoader::BIError::ToString(String& out) const                       // Mac 0x3138f0, Win 0x4a3860
{
    Error::ToString(out);
    out << " error detail ";
    const char* detail;
    if (m_detail == nonExistingPart)
        detail = "nonExistingPart";
    else if (m_detail == nonExistingCamera)
        detail = "nonExistingCamera";
    else
        detail = "unknown";
    out << detail;
}

// ============================================================================================
// ModelLoader
// ============================================================================================

// NOTE: m_errorMask stays uninitialised (Win 0x4a3b80 and every inlined Mac copy); fixed: 0
// (low).
ModelLoader::ModelLoader() : m_errorMask(0) {}                              // Win 0x4a3b80

ModelLoader::~ModelLoader() {}                                              // Mac 0x2f5900, Win 0x4a3bf0

void ModelLoader::AddError(Error* error)                                    // Win 0x4a3af0
{
    m_errors.PushBack(error);
    m_errorMask |= error->GetType();
}

bool ModelLoader::LoadFile(const Path& path, StorageScene2& scene, bool flag) // Mac 0x2ef9b0, Win 0x4d8400
{
    m_errors.Clear();
    m_errorMask = 0;
    AString content;
    if (!ReadFile(path, content)) {
        AddError(new PathError(path));
    } else {
        if (content.GetLength() != 0)
            return Load(content, scene, flag);
        AddError(new ContentError(ContentError::empty));
    }
    return false;
}

// ============================================================================================
// ModelSaver
// ============================================================================================

ModelSaver::ModelSaver() {}                                                 // Win 0x4a3df0

ModelSaver::~ModelSaver() {}                                                // Win 0x4a3e50 (sdd 0x4a3ea0)

} // namespace LEGO
