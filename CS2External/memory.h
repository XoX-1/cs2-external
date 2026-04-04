#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>

class Memory {
public:
    uint32_t pid = 0;
    HANDLE handle = nullptr;
    uintptr_t client = 0;
    uintptr_t engine = 0;

    bool Init(const wchar_t* processName) {
        pid = GetProcessId(processName);
        if (!pid) return false;

        handle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
        if (!handle) return false;

        client = GetModuleBase(L"client.dll");
        engine = GetModuleBase(L"engine2.dll");

        return client != 0 && engine != 0;
    }

    template <typename T>
    T Read(uintptr_t address) const {
        T buffer{};
        if (address)
            ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(address), &buffer, sizeof(T), nullptr);
        return buffer;
    }

    bool ReadRaw(uintptr_t address, void* buffer, size_t size) const {
        if (!address || !buffer) return false;
        return ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr) != 0;
    }

    template <typename T>
    bool Write(uintptr_t address, T value) const {
        if (!address) return false;
        return WriteProcessMemory(handle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr) != 0;
    }

    void Cleanup() {
        if (handle) {
            CloseHandle(handle);
            handle = nullptr;
        }
        pid = 0;
        client = 0;
        engine = 0;
    }

    ~Memory() {
        Cleanup();
    }

private:
    uint32_t GetProcessId(const wchar_t* processName) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        uint32_t result = 0;
        if (Process32FirstW(snap, &entry)) {
            do {
                if (_wcsicmp(entry.szExeFile, processName) == 0) {
                    result = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &entry));
        }

        CloseHandle(snap);
        return result;
    }

    uintptr_t GetModuleBase(const wchar_t* moduleName) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) return 0;

        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        uintptr_t result = 0;
        if (Module32FirstW(snap, &entry)) {
            do {
                if (_wcsicmp(entry.szModule, moduleName) == 0) {
                    result = reinterpret_cast<uintptr_t>(entry.modBaseAddr);
                    break;
                }
            } while (Module32NextW(snap, &entry));
        }

        CloseHandle(snap);
        return result;
    }
};

inline Memory mem;
