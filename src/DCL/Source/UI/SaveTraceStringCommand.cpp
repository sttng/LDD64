// LEGO Digital Designer — restored source
// TU 385 "SaveTraceStringCommand.cpp": Mac 0x373380-0x373a8f (+ the Command weak instances
// 0x372e20-0x373380, restored inline in Command.h), Win 0x423c20 / 0x5c9a87 / 0x5c9d68.
// Not code: thunks, Mac 0x373a50 = <iostream>.
#include "SaveTraceStringCommand.h"
#include "Platform.h"
#include "Path.h"
#include "File.h"
#include "Trace.h"

namespace LEGO {

namespace {

// Platform::ShowMessage(message, "Write log error!", window). (inlined)
void ShowWriteLogError(const char* message, Window* window)
{
    String title("Write log error!");
    Platform::ShowMessage(String(message), &title, window);
}

} // namespace

// Mac 0x373840 (D1), 0x3738b0 (D0), Win 0x5c9a87 (sdd 0x5c9d68)
SaveTraceStringCommand::~SaveTraceStringCommand()
{
}

// Mac 0x373380, Win 0x423c20
void SaveTraceStringCommand::Do()
{
    Path path(String(), 0);
    bool chosen;
    {
        Platform* platform = Platform::GetInstance();
        String name;
        Platform::FileTypeArray types;
        // NOTE: uninitialised in the original (no types are offered); fixed: 0 (low)
        UInt32 typeIndex = 0;
        chosen = platform->GetSavePath(path, 0, name, types, &typeIndex, m_window, String(""));
    }
    if (!chosen)
        return;
    RefPointer<File> file = path.OpenFile(OpenMode(false, true), true);
    if (!file) {
        ShowWriteLogError("Could not open file for write.", m_window);
        return;
    }
    const String& history = GetTraceHistory();
    UniChar16 bom = 0xfeff;
    if (!file->Write(2, &bom, 0)) {
        ShowWriteLogError("Could not write header to file.", m_window);
        return;
    }
    if (!file->Write((UInt64)history.GetLength() * 2, history.GetBuffer(), 0)) {
        ShowWriteLogError("Could not write to file.", m_window);
        return;
    }
    file->SetSize(file->GetPosition());
}

} // namespace LEGO
