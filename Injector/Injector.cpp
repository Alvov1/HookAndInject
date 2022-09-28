#include <iostream>
#include <string>
#include <filesystem>
#include <thread>

#include <Windows.h>
#include <TlHelp32.h>

unsigned getPid(const std::string& process) {
    try {
        return std::stoi(process);
    }
    catch (const std::invalid_argument& ia) {}

    DWORD processPid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Get pid by name: Creating snapshot failed.");

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(processEntry);
    if (Process32First(snap, &processEntry))
    {
        do {
            if (std::wstring(processEntry.szExeFile) == std::wstring(process.begin(), process.end())) {
                CloseHandle(snap);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snap, &processEntry));
    }
    CloseHandle(snap);
    throw std::runtime_error("Get pid by name: No such process.");
}

void getDebugPrivilege() {
    HANDLE hCurrentProcess = GetCurrentProcess(); HANDLE hToken = INVALID_HANDLE_VALUE;
    OpenProcessToken(hCurrentProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

    LPCTSTR lpszPrivilege = SE_DEBUG_NAME; LUID luid;
    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
        throw std::runtime_error("Get debugg privilege: LookupPrivilegeValue error: " + std::to_string(GetLastError()) + ".");

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
        throw std::runtime_error("Get debugg privilege: AdjustTokenPrivileges error: " + std::to_string(GetLastError()) + ".");
}

void injectHooker(const std::filesystem::path& hookerLocation, unsigned processPid) {
    getDebugPrivilege();
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, false, processPid);
    if (process == INVALID_HANDLE_VALUE || process == nullptr)
        throw std::runtime_error("Injecting: OpenProcess failed: " + std::to_string(GetLastError()) + ".");

    LPVOID hookerPathInMemory = VirtualAllocEx(process, 0, hookerLocation.string().size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (hookerPathInMemory == nullptr)
        throw std::runtime_error("Injectring: VirtualAllocEx failed: " + std::to_string(GetLastError()) + ".");

    if (TRUE != WriteProcessMemory(process, hookerPathInMemory, hookerLocation.string().c_str(), hookerLocation.string().size() + 1, nullptr))
        throw std::runtime_error("Injecting: WriteProcessMemory failed: " + std::to_string(GetLastError()) + ".");

    HANDLE hookerBootThread = CreateRemoteThread(process, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, hookerPathInMemory, 0, 0);
    if (hookerBootThread == INVALID_HANDLE_VALUE || hookerBootThread == nullptr)
        throw std::runtime_error("Injecting: CreateRemoteThread failed: " + std::to_string(GetLastError()) + ".");

    std::cout << "Dll is attached to the process." << std::endl;
}

std::pair<unsigned, std::filesystem::path> init(int argc, const char* const* argv) {
    if (argc < 2)
        throw std::runtime_error("Usage: " + std::string(argv[0]) + " (pid | name) [ hooker location ].");
    unsigned processPid = getPid(argv[1]);

    static constexpr auto defaultHookerLocation = "C:\\Users\\Alexander\\Desktop\\Folder\\Hooker.dll";
    const auto hookerLocation = (argc > 2) ? std::filesystem::path(argv[2]) : std::filesystem::path(defaultHookerLocation);
    if (!std::filesystem::exists(hookerLocation))
        throw std::runtime_error("Path to the hoooker file is incorrect.");
    return { processPid, hookerLocation };
}

int main(int argc, const char* const* argv) {
    try {
        const auto& [pid, hookerPath] = init(argc, argv);
        std::cout << "Trying to attach to the process with pid " << pid << "." << std::endl;
        std::cout << "Using dll located on " << hookerPath << "." << std::endl;
        injectHooker(hookerPath, pid);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}