// LEGO Digital Designer — restored source
// TU 006 "main.mm" (file name inferred) — Mac 0x8c70-0x91bb, Mac build only.
// The Mach-O entry (Mac 0x2030, crt1 start: exit(main(argc, argv, envp))) is not source. Also
// emitted here: BasicArray<String>::PushBack 0x8f80 (Win 0x404780) and the BasicArray<String>
// dtors 0x9070 / 0x90d0 (Win 0x42b8e0 / 0x42b9f0), static initialiser 0x9140.
// The NSAutoreleasePool sits behind __OBJC__; on the Linux-hosted Mac branch this file is the
// process entry of the plain C++ host build.
#if !defined(_WIN32)
#include "Main.h"
#include "Thread.h"
#include "Trace.h"

#include <locale.h>

#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#elif defined(LDD_LINUX)
#include "LinuxShell.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

// Mac 0x8c70
int main(int argc, char** argv)
{
#if defined(__OBJC__)
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
#elif defined(LDD_LINUX)
    LEGO::LinuxShell::Initialize(&argc, &argv);   // (takes GTK's own options)
#endif
    LEGO::BasicArray<LEGO::String> arguments;
#if defined(LDD_LINUX)
    bool optionValue = false;   // the argument is the value of the "-key" before it
#endif
    for (int i = 0; i < argc; ++i) {
#if defined(LDD_LINUX)
        // Linux: a file argument outside the "-key value" pairs (Preferences::ParseArguments)
        // opens that model: "-Open <file>", the Windows build's preference (a launcher's %f)
        const bool option = argv[i][0] == '-';
        if (i > 0 && !optionValue && !option && access(argv[i], R_OK) == 0) {
            char* absolute = realpath(argv[i], 0);
            arguments.PushBack(LEGO::String("-Open"));
            arguments.PushBack(LEGO::String(absolute ? absolute : argv[i]));
            free(absolute);
            continue;
        }
        optionValue = i > 0 && option && !optionValue;
#endif
        arguments.PushBack(LEGO::String(argv[i]));
    }
    // "-TraceName <name>": the first option of that name (options are skipped with their value)
    for (LEGO::String* it = arguments.Begin(); it != arguments.End();) {
        if (!it->StartsWith((LEGO::UniChar16)'-')) {
            ++it;
            continue;
        }
        LEGO::String* value = it + 1;
        if (value == arguments.End())
            break;
        if (it->CompareNoCase(LEGO::String("-TraceName")) == 0) {
            LEGO::SetTraceName(*value);
            break;
        }
        it += 2;
    }
    setlocale(LC_NUMERIC, "C");
    LEGO::Thread thread;
    thread.Attach();
    int result = AppMain(arguments);
    thread.Detach();
#if defined(__OBJC__)
    [pool release];
#endif
    return result;
}

#endif // !_WIN32
