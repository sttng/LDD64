// LEGO Digital Designer — restored source
// TU 403 "InvertSelectionCommand.cpp": Mac 0x38bad0-0x38c07b, Win 0x66dab1-0x66dc2a.
#include "InvertSelectionCommand.h"
#include "BrickConnectSceneControl.h"
#include "Scene.h"

namespace LEGO {

namespace {

// std::find + swap-with-last erase: the BasicArray<RefPointer<Part>> template instance Mac
// 0x3f680 (emitted with BrickConnectSceneControl, TU 021; header template code).
void RemovePart(BasicArray<RefPointer<Part> >& parts, const RefPointer<Part>& part)
{
    for (UInt32 i = 0; i < parts.GetSize(); ++i) {
        if (parts[i].Get() != part.Get())
            continue;
        if (i + 1 != parts.GetSize())
            parts[i] = parts[parts.GetSize() - 1];
        parts.PopBack();
        return;
    }
}

} // namespace

// Mac 0x38bad0, Win 0x66db30
InvertSelectionCommand::InvertSelectionCommand(BrickConnectSceneControl* sceneControl)
    : Command(false, false), m_sceneControl(sceneControl)
{
    m_sceneControl->AddRef();
    UpdateEnable();
}

// Mac 0x38bd70 (D1) / 0x38bdd0 (D0), Win sdd 0x66dc2a
InvertSelectionCommand::~InvertSelectionCommand()
{
    if (m_sceneControl)
        m_sceneControl->Release();
}

// Mac 0x38bc00, Win 0x66dab1
void InvertSelectionCommand::Do()
{
    const BasicArray<RefPointer<Part> >& selection = m_sceneControl->GetSelection();
    BasicArray<RefPointer<Part> > parts;
    m_sceneControl->GetScene()->GetParts(parts);
    for (BasicArray<RefPointer<Part> >::ConstIterator it = selection.Begin(); it != selection.End(); ++it)
        RemovePart(parts, *it);
    m_sceneControl->SetSelection(parts);
}

// Mac 0x38bd50, Win 0x630465
void InvertSelectionCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
