// LEGO Digital Designer — restored source
// LEGO::ModelLoaderAbstract (Mac TU 336, Win 0x5a2160 / 0x5fd8b6-0x5fd90e). See the header.
#include "ModelLoaderAbstract.h"
#include "ModelIOLXFML5.h"
#include "ModelIOLXFML4.h"
#include "ModelIOLXFML3.h"
#include "ModelIOLXFML2.h"
#include "ModelIOLDraw.h"

#include <iostream>

namespace LEGO {

namespace {

// A failed loader is final unless the text did not look like its format (wrong version or not
// parseable XML).
inline bool TryNextFormat(const ModelLoader* loader)
{
    return (loader->GetErrorMask() &
            (ModelLoader::Error::versionError | ModelLoader::Error::parserError)) != 0;
}

} // namespace

ModelLoaderAbstract::ModelLoaderAbstract(BrickDatabase* database)          // Win 0x5fd8b6
    : m_database(database)
{
}

ModelLoaderAbstract::~ModelLoaderAbstract() {}                              // Mac 0x2f73d0, Win 0x5fd8ea

bool ModelLoaderAbstract::Load(const AString& content, StorageScene2& scene, bool flag) // Mac 0x2f6e90, Win 0x5a2160
{
    m_errors.Clear();
    m_errorMask = 0;
    scene.Clear();

    RefPointer<ModelLoader> loader = new ModelLoaderLXFML5(m_database);
    if (loader->Load(content, scene, flag))
        return true;

    if (TryNextFormat(loader)) {
        loader = new ModelLoaderLXFML4;
        if (loader->Load(content, scene, flag))
            return true;

        if (TryNextFormat(loader)) {
            // from here on no error check: the last loader's result is the answer
            loader = new ModelLoaderLXFML3(m_database);
            if (loader->Load(content, scene, flag))
                return true;
            loader = new ModelLoaderLXFML2;
            if (loader->Load(content, scene, flag))
                return true;
            loader = new ModelLoaderLDraw;
            return loader->Load(content, scene, flag);
        }
    }

    // The failed loader's errors become ours.
    m_errors = loader->GetErrors();
    m_errorMask = loader->GetErrorMask();
    return false;
}

} // namespace LEGO
