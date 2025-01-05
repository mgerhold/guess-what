#include "windows.hpp"

#ifdef _WIN32

// clang-format off
// Order of includes is important here.
#include <windows.h>
#include <tlhelp32.h>
// clang-format on
#include <iostream>
#include <string>

[[nodiscard]] static std::string getParentProcessName() {
    DWORD pid = GetCurrentProcessId();
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return "";
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == pid) {
                DWORD parentPid = pe32.th32ParentProcessID;

                // Now find the parent process name
                Process32First(hSnapshot, &pe32);
                do {
                    if (pe32.th32ProcessID == parentPid) {
                        CloseHandle(hSnapshot);
                        return std::string(pe32.szExeFile);
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return "";
}

[[nodiscard]] bool is_running_in_cmd() {
    return getParentProcessName() == "cmd.exe";
}

static void enable_ansi() {
    auto const stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout_handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error{ "Unable to get stdout handle." };
    }
    auto dwMode = DWORD{ 0 };
    GetConsoleMode(stdout_handle, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(stdout_handle, dwMode);
}

static void set_cmd_to_utf8() {
    if (SetConsoleOutputCP(CP_UTF8) == 0) {
        throw std::runtime_error{ "Unable to set console output to UTF8." };
    }

    if (SetConsoleCP(CP_UTF8) == 0) {
        throw std::runtime_error{ "Unable to set console input to UTF8." };
    }
}

void setup_terminal() {
    std::setlocale(LC_ALL, ".65001");
    enable_ansi();
    set_cmd_to_utf8();
}

#endif
