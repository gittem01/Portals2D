#include <Game.h>

#ifdef WIN32

#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <dbghelp.h>
#include "steam_api.h"

typedef BOOL(WINAPI* SymInitialize_t)(HANDLE, PCSTR, BOOL);
typedef BOOL(WINAPI* SymFromAddr_t)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
typedef BOOL(WINAPI* StackWalk64_t)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID,
    PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64,
    PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
typedef BOOL(WINAPI* SymCleanup_t)(HANDLE);
typedef PVOID(WINAPI* SymFunctionTableAccess64_t)(HANDLE, DWORD64);
typedef DWORD64(WINAPI* SymGetModuleBase64_t)(HANDLE, DWORD64);

HMODULE hDbgHelp;

HMODULE LoadDbgHelp()
{
    HMODULE hDbgHelp = LoadLibrary(TEXT("debughelper.dll"));
    if (!hDbgHelp)
    {
        MessageBox(NULL, TEXT("Failed to load debughelper.dll"), TEXT("Error"), MB_OK | MB_ICONERROR);
        return nullptr;
    }
    return hDbgHelp;
}

// Initialize symbols for the process
void InitializeSymbols()
{
    hDbgHelp = LoadDbgHelp();

    SymInitialize_t SymInitialize = (SymInitialize_t)GetProcAddress(hDbgHelp, "SymInitialize");

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
}

void PrintCallStack(CONTEXT* context)
{
    // Resolve function addresses
    SymFromAddr_t SymFromAddr = (SymFromAddr_t)GetProcAddress(hDbgHelp, "SymFromAddr");
    StackWalk64_t StackWalk64 = (StackWalk64_t)GetProcAddress(hDbgHelp, "StackWalk64");
    SymFunctionTableAccess64_t SymFunctionTableAccess64 = (SymFunctionTableAccess64_t)GetProcAddress(hDbgHelp, "SymFunctionTableAccess64");
    SymGetModuleBase64_t SymGetModuleBase64 = (SymGetModuleBase64_t)GetProcAddress(hDbgHelp, "SymGetModuleBase64");

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    STACKFRAME64 stackFrame;
    memset(&stackFrame, 0, sizeof(STACKFRAME64));

    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context->Rip;
    stackFrame.AddrFrame.Offset = context->Rbp;
    stackFrame.AddrStack.Offset = context->Rsp;

    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Mode = AddrModeFlat;

    // Open the crash report file for writing
    std::ofstream crashReportFile("crash_report.txt", std::ios::out);

    if (!crashReportFile.is_open())
    {
        std::cerr << "Failed to open crash_report.txt for writing" << std::endl;
        free(symbol);
        return;
    }

    // Write the call stack to the file
    crashReportFile << "Call stack:\n";
    while (StackWalk64(machineType, process, thread, &stackFrame, context, NULL,
        SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
        DWORD64 address = stackFrame.AddrPC.Offset;
        if (address == 0)
        {
            break;
        }

        crashReportFile << "Address: 0x" << std::hex << address;

        if (SymFromAddr(process, address, 0, symbol))
        {
            crashReportFile << " - " << symbol->Name << std::endl;
        }
        else
        {
            crashReportFile << " - [Unknown function]\n";
        }
    }

    // Close the file
    crashReportFile.close();

    free(symbol);

    // not implemented yet
    if (false)
    {
        int result = MessageBox(NULL,
            "Game crashed\nSend crash report?",
            "Crash Report",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

        if (result == IDYES)
        {
        }
    }
}

// Custom exception handler
LONG WINAPI CrashHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    std::cerr << "Application crashed! Generating stack trace..." << std::endl;

    // Initialize symbol handler for the process
    InitializeSymbols();

    // Print the call stack
    PrintCallStack(exceptionInfo->ContextRecord);

    SymCleanup_t SymCleanup = (SymCleanup_t)GetProcAddress(hDbgHelp, "SymCleanup");
    // Clean up symbol handler
    SymCleanup(GetCurrentProcess());

    return EXCEPTION_EXECUTE_HANDLER;
}

#endif

int main(void)
{
#ifdef WIN32
    SetUnhandledExceptionFilter(CrashHandler);
#endif

    Game* game = new Game();

    game->loop();

    return 0;
}

#ifdef ENABLE_WINMAIN && WIN32
    #include "windows.h"
    int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow)
    {
        return main();
    }
#endif