// LEGO Digital Designer — restored source
// LEGO::ModelLoader — base of the model file loaders (LXFML 2/3/4/5, LDraw, the format-sniffing
// ModelLoaderAbstract), with its nested error classes; LEGO::ModelSaver — base of the savers.
//
// Original layout: everything here is header-inline. Mac emits the members as weak functions in
// the first object using them (Error, PathError, ContentError, ModelLoader::~ModelLoader and
// ModelLoader::LoadFile in TU 335 ModelIOStorageScene.cpp; VersionError/ParserError in TU 338;
// PartError in 337; JointError in 339; CameraError/BIError in 341; BrickError/BoneError/
// RigidError in 342c). Mac TU 338 ("ModelLoader_VersionError.cpp") holds no code of its own
// besides those weak copies and template instances (static initialiser 0x2fdfa0 =
// `#include <iostream>`). Win /LTCG kept constructors, destructors and ToString out of line in
// one block, Win 0x4a2d60-0x4a3ea0 (+ PathError 0x4d8290-0x4d83e0, LoadFile 0x4d8400).
// Restored out of line in ModelLoader.cpp.
//
// Offsets: Mac (String 16 bytes, Path 0x18) / Win (String 4 bytes, Path 0x0c).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "BasicArray.h"
#include "Path.h"
#include "Version.h"
#include "StorageScene2.h"

namespace LEGO {

// (not in the original; H13) The largest surface id / flex control point a model file may use:
// the loaders grow arrays to the index + 1 (an index near 4G wraps the size).
const UInt32 kMaxModelFileIndex = 0x10000;

// Win vtable 0x9cdb80 {sdd 0x4a3dd0, OnFinalRelease, AddRef, Release, Load = _purecall};
// Mac vtable 0x777138 {D1 0x2f5900, D0 0x2f59a0, …, ___cxa_pure_virtual}.
class ModelLoader : public Object {
public:
    // ---- errors -------------------------------------------------------------------------
    // Win vtable 0xa24474 {sdd 0x4a3c70, …, ToString 0x4a2d60}; Mac vtable 0x7771a0
    // {D1 0x2f38b0, D0 0x2f3900, …, ToString 0x2f3970}. sizeof Mac 0x1c / Win 0x10.
    class Error : public Object {
    public:
        // Error categories; ModelLoader::GetErrorMask() ORs them. (enumerator names = the
        // strings ToString writes; contentError has no string: ToString says "unknown")
        enum Type {
            noError      = 0x000,
            pathError    = 0x001,
            versionError = 0x002,
            jointError   = 0x004,
            cameraError  = 0x008,
            brickError   = 0x010,
            partError    = 0x020,
            boneError    = 0x040,
            rigidError   = 0x080,
            biError      = 0x100,
            parserError  = 0x200,
            contentError = 0x400,   // (name inferred)
            miscError    = 0x800
        };

        explicit Error(UInt32 type);            // Win 0x4a2ed0 (this on the stack, type in ECX)
        virtual ~Error();                       // Mac 0x2f38b0, Win 0x4a2f30

        // Appends "<type name>" and, when a description is set, " Description: <text>".
        // Derived classes append " error detail <detail>" etc. (name inferred; vtable slot 4)
        virtual void ToString(String& out) const;   // Mac 0x2f3970, Win 0x4a2d60

        UInt32        GetType() const { return m_type; }                       // (name inferred)
        const String& GetDescription() const { return m_description; }        // (name inferred)
        void          SetDescription(const String& text) { m_description = text; } // (name inferred)

    protected:
        UInt32 m_type;          // +0x08 Type
        String m_description;   // +0x0c (never set by the loaders seen so far)
    };

    // The model file cannot be read. Mac vtable 0x776e94, Win vtable 0xa25784.
    // sizeof Mac 0x34 / Win 0x1c.
    class PathError : public Error {
    public:
        explicit PathError(const Path& path);   // Mac 0x2f36e0, Win 0x4d8290
        virtual ~PathError();                   // Mac 0x2f3b80, Win 0x4d8380
        virtual void ToString(String& out) const;   // Mac 0x2f3820, Win 0x4d8300 (" path <path>")

        const Path& GetPath() const { return m_path; }

    protected:
        Path m_path;   // Mac +0x1c / Win +0x10
    };

    // The file is empty / not LXFML. Mac vtable 0x776ed8, Win vtable 0xa24564. 0x20 / 0x14.
    class ContentError : public Error {
    public:
        enum Detail {
            empty           = 0,
            missingLXFMLTag = 1
        };
        explicit ContentError(UInt32 detail);   // Win 0x4a39d0
        virtual ~ContentError();                // Mac 0x2f3c50, Win 0x4a3aa0
        virtual void ToString(String& out) const;   // Mac 0x2f3d10, Win 0x4a3a40

        UInt32 GetDetail() const { return m_detail; }

    protected:
        UInt32 m_detail;   // Mac +0x1c / Win +0x10 Detail
    };

    // Unsupported LXFML version. Mac vtable 0x7772c0, Win vtable 0xa2448c. 0x3c / 0x30.
    class VersionError : public Error {
    public:
        VersionError(const Version& expected, const Version& actual);  // Win 0x4a2f90 (EBX, EDI)
        virtual ~VersionError();                // Mac 0x2fc760, Win 0x4a3070
        virtual void ToString(String& out) const;   // Mac 0x2fc820, Win 0x4a3020

        const Version& GetExpected() const { return m_expected; }
        const Version& GetActual() const { return m_actual; }

    protected:
        Version m_expected;   // Mac +0x1c / Win +0x10
        Version m_actual;     // Mac +0x2c / Win +0x20
    };

    // The XML parser failed. Mac vtable 0x7772f0, Win vtable 0xa2454c. 0x1c / 0x10.
    class ParserError : public Error {
    public:
        ParserError();                          // Win 0x4a3910
        virtual ~ParserError();                 // Mac 0x2fd350, Win 0x4a3980
        virtual void ToString(String& out) const;   // Mac 0x2fd410, Win 0x4a3970 (= Error's)
    };

    // Mac vtable 0x7773b4, Win vtable 0xa244a4. 0x20 / 0x14. The detail is not printed.
    class JointError : public Error {
    public:
        explicit JointError(UInt32 detail);     // Win 0x4a30c0
        virtual ~JointError();                  // Mac 0x3028e0, Win 0x4a3130
        virtual void ToString(String& out) const;   // Mac 0x3029a0, Win 0x4a3970 (= Error's)

        UInt32 GetDetail() const { return m_detail; }

    protected:
        UInt32 m_detail;   // Mac +0x1c / Win +0x10
    };

    // Mac vtable 0x777488, Win vtable 0xa244bc. 0x20 / 0x14.
    class CameraError : public Error {
    public:
        enum Detail {
            nonExistingCamera = 0,
            missingRefID      = 1,
            reusedRefID       = 2
        };
        explicit CameraError(UInt32 detail);    // Win 0x4a3180
        virtual ~CameraError();                 // Mac 0x3107e0, Win 0x4a3270
        virtual void ToString(String& out) const;   // Mac 0x3108a0, Win 0x4a31f0

        UInt32 GetDetail() const { return m_detail; }

    protected:
        UInt32 m_detail;   // Mac +0x1c / Win +0x10 Detail
    };

    // Mac vtable 0x7774ac, Win vtable 0xa244d4. 0x20 / 0x14.
    class BrickError : public Error {
    public:
        enum Detail {
            missingRefID    = 0,
            reusedRefID     = 1,
            missingDesignID = 2
        };
        explicit BrickError(UInt32 detail);     // Win 0x4a32c0
        virtual ~BrickError();                  // Mac 0x3246c0, Win 0x4a33b0
        virtual void ToString(String& out) const;   // Mac 0x324780, Win 0x4a3330

        UInt32 GetDetail() const { return m_detail; }

    protected:
        UInt32 m_detail;   // Mac +0x1c / Win +0x10 Detail
    };

    // Mac vtable 0x7771c0, Win vtable 0xa244ec. 0x24 / 0x18.
    class PartError : public Error {
    public:
        enum Detail {
            missingRefID    = 0,
            reusedRefID     = 1,
            missingDesignID = 2,
            missingMaterial = 3,
            wrongMaterial   = 4,
            wrongDecoration = 5,
            notOneMaterial  = 6,
            assembly        = 7,
            flex            = 8,
            misc            = 9
        };
        PartError(StorageScene2::Part* part, UInt32 detail);   // Mac 0x324800, Win 0x4a3400 (EDI = part)
        virtual ~PartError();                   // Mac 0x2fbca0, Win 0x4a35a0
        // " error detail <detail>" and, with a part, " design id <designID>".
        virtual void ToString(String& out) const;   // Mac 0x2fbb70, Win 0x4a3480

        StorageScene2::Part* GetPart() const { return m_part; }
        UInt32               GetDetail() const { return m_detail; }

    protected:
        RefPointer<StorageScene2::Part> m_part;     // Mac +0x1c / Win +0x10
        UInt32                          m_detail;   // Mac +0x20 / Win +0x14 Detail
    };

    // Mac vtable 0x7774f0, Win vtable 0xa24504. 0x20 / 0x14.
    class BoneError : public Error {
    public:
        enum Detail {
            missingRefID = 0,
            reusedRefID  = 1
        };
        explicit BoneError(UInt32 detail);      // Win 0x4a3610
        virtual ~BoneError();                   // Mac 0x3248c0, Win 0x4a36e0
        virtual void ToString(String& out) const;   // Mac 0x324980, Win 0x4a3680

        UInt32 GetDetail() const { return m_detail; }

    protected:
        UInt32 m_detail;   // Mac +0x1c / Win +0x10 Detail
    };

    // Mac vtable 0x777520, Win vtable 0xa2451c. 0x20 / 0x14.
    class RigidError : public Error {
    public:
        enum Detail {
            missingRefID = 0,
            reusedRefID  = 1
        };
        explicit RigidError(UInt32 detail);     // Win 0x4a3730
        virtual ~RigidError();                  // Mac 0x3249f0, Win 0x4a37a0
        virtual void ToString(String& out) const;   // Mac 0x324ab0, Win 0x4a3680 (= BoneError's)

        UInt32 GetDetail() const { return m_detail; }

    protected:
        UInt32 m_detail;   // Mac +0x1c / Win +0x10 Detail
    };

    // Mac vtable 0x7773f4, Win vtable 0xa24534. 0x20 / 0x14.
    class BIError : public Error {
    public:
        enum Detail {
            nonExistingCamera = 0,
            nonExistingPart   = 1
        };
        explicit BIError(UInt32 detail);        // Win 0x4a37f0
        virtual ~BIError();                     // Mac 0x313830, Win 0x4a38c0
        virtual void ToString(String& out) const;   // Mac 0x3138f0, Win 0x4a3860

        UInt32 GetDetail() const { return m_detail; }

    protected:
        UInt32 m_detail;   // Mac +0x1c / Win +0x10 Detail
    };

    // ---- the loader ---------------------------------------------------------------------
    // NOTE: m_errorMask is not initialised by the constructor (both builds); LoadFile/Load
    // reset it before use.
    ModelLoader();                              // Win 0x4a3b80 (thiscall); inlined on Mac
    virtual ~ModelLoader();                     // Mac 0x2f5900 (D0 0x2f59a0), Win 0x4a3bf0 (sdd 0x4a3dd0)

    // Reads `path` completely and hands the text to Load(). Failures: PathError (unreadable),
    // ContentError(empty). The last argument is passed through unchanged; every caller
    // passes true (Mac 0x14a680, 0x179bb0). (name inferred)
    bool LoadFile(const Path& path, StorageScene2& scene, bool flag);   // Mac 0x2ef9b0, Win 0x4d8400

    // Parses `content` into `scene`, collecting errors. (name inferred; vtable slot 4)
    virtual bool Load(const AString& content, StorageScene2& scene, bool flag) = 0;

    const BasicArray<RefPointer<Error> >& GetErrors() const { return m_errors; }   // (name inferred)
    UInt32 GetErrorMask() const { return m_errorMask; }                            // (name inferred)

protected:
    // Keeps `error` and ORs its type into the mask. Win 0x4a3af0 (this on the stack, error in
    // EBX); inlined on Mac. (name inferred)
    void AddError(Error* error);

    BasicArray<RefPointer<Error> > m_errors;      // +0x08
    UInt32                         m_errorMask;   // +0x18 OR of the Error::Type of m_errors
};

// Win vtable 0x9cd514 {sdd 0x4a3ea0, …, Save = _purecall}; no Mac vtable (never instantiated,
// every constructor inlined). sizeof 0x08.
class ModelSaver : public Object {
public:
    ModelSaver();                               // Win 0x4a3df0 (thiscall)
    virtual ~ModelSaver();                      // Win 0x4a3e50

    // Writes `scene` to `path`. (name and signature inferred from ModelSaverLXFML5 Mac 0x321740
    // and ModelSaverStorageScene Mac 0x2f1d00: (path, scene); vtable slot 4)
    virtual bool Save(const Path& path, const StorageScene2& scene) = 0;
};

} // namespace LEGO
