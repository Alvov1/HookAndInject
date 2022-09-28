#include "pch.h"

#include <iostream>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>

#include "Proxies.h"

void suspendThreads() {
    DWORD thisProcess = GetProcessId(GetCurrentProcess());
    DWORD thisThread = GetThreadId(GetCurrentThread());

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE)
        throw std::runtime_error("CreateToolhelp32Snapshot failed: " + std::to_string(GetLastError()) + ".");

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(snap, &threadEntry))
        throw std::runtime_error("Thread32First failed: " + std::to_string(GetLastError()) + ".");

    do {
        if (threadEntry.th32OwnerProcessID == thisProcess && threadEntry.th32ThreadID != thisThread) {
            HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);
            if (thread == INVALID_HANDLE_VALUE || thread == nullptr) continue;
            SuspendThread(thread); CloseHandle(thread);
        }
    } while (Thread32Next(snap, &threadEntry));
}

void resumeThreads() {
    DWORD thisProcess = GetProcessId(GetCurrentProcess());
    DWORD thisThread = GetThreadId(GetCurrentThread());

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE)
        throw std::runtime_error("CreateToolhelp32Snapshot failed: " + std::to_string(GetLastError()) + ".");

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(snap, &threadEntry))
        throw std::runtime_error("Thread32First failed: " + std::to_string(GetLastError()) + ".");

    do {
        if (threadEntry.th32OwnerProcessID == thisProcess && threadEntry.th32ThreadID != thisThread) {
            HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);
            if (thread == NULL) continue;
            ResumeThread(thread); CloseHandle(thread);
        }
    } while (Thread32Next(snap, &threadEntry));
}

void swap(const std::pair<PCHAR, FARPROC>& addresses) {
    const auto [original, our] = addresses;

    /* Copy original bytes from function to buffer. */
    auto* originalBytesBuffer = new char[6];
    if (0 == ReadProcessMemory(
        GetCurrentProcess(), 
        original, 
        originalBytesBuffer, 
        6, 
        nullptr))
        throw std::runtime_error("ReadProcessMemory failed: " + std::to_string(GetLastError()) + ".");

    /* Insert JMP near instruction, followed by address. */
    auto* proxyBytesBuffer = new char[5];
    DWORD offset = (PCHAR)our - original - 5;
    proxyBytesBuffer[0] = '\xE9'; // JMP near instruction.
    memcpy(proxyBytesBuffer + 1, &offset, 4);
    if (0 == WriteProcessMemory(
        GetCurrentProcess(), 
        original, 
        proxyBytesBuffer,
        5, 
        nullptr))
        throw std::runtime_error("WriteProcessMemory failed: " + std::to_string(GetLastError()) + ".");

    /* Write original bytes to new location. */
    //originalBytesBuffer[2] -= 5;
    if (0 == WriteProcessMemory(
        GetCurrentProcess(), 
        original + 5, 
        originalBytesBuffer, 
        6, 
        nullptr))
        throw std::runtime_error("WriteProcessMemory failed: " + std::to_string(GetLastError()) + ".");

    delete[] originalBytesBuffer; delete[] proxyBytesBuffer;
}

std::pair<std::pair<PCHAR, FARPROC>, std::pair<PCHAR, FARPROC>> getAddresses() {
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32 == NULL)
        throw std::runtime_error("GetModuleHandle failed for kernel32.dll: " + std::to_string(GetLastError()) + ".");
    HMODULE hHooker = GetModuleHandleA("hooker");
    if (hHooker == NULL)
        throw std::runtime_error("GetModuleHandle failed for our dll: " + std::to_string(GetLastError()) + ".");

    auto originalAddressFuncA = GetProcAddress(hUser32, "SetWindowLongPtrA");
    if (originalAddressFuncA == NULL)
        throw std::runtime_error("Failed to receive address of first original function.");
    auto ourAddressFuncA = GetProcAddress(hHooker, "OurSetWindowLongPtrA");
    if (ourAddressFuncA == NULL)
        throw std::runtime_error("Failed to receive address of first our function.");    

    auto originalAddressFuncW = GetProcAddress(hUser32, "SetWindowLongPtrW");
    if (originalAddressFuncW == NULL)
        throw std::runtime_error("Failed to receive address of second original function.");
    auto ourAddressFuncW = GetProcAddress(hHooker, "OurSetWindowLongPtrW");
    if (ourAddressFuncW == NULL)
        throw std::runtime_error("Failed to receive address of second our function.");

    return { {(PCHAR) originalAddressFuncA, (FARPROC) ourAddressFuncA}, {(PCHAR) originalAddressFuncW, (FARPROC) ourAddressFuncW} };
}

void process() {
    OutputDebugStringW(L"Dll is started.");

    const auto& [first, second] = getAddresses();

    OutputDebugStringW(L"Function's addresses are received.");

    suspendThreads();

    swap(first);
    swap(second);

    resumeThreads();

    OutputDebugStringW(L"Functions were replaced successfully.");
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        try {
            init();
            process();
        }
        catch (const std::exception& e) {
            OutputDebugStringA(e.what());
        };
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

