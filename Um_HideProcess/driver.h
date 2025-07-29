#pragma once

#include <Windows.h>
#include <TlHelp32.h>

#define DEVICE_NAME L"\\\\.\\HideProcess_LINK"

#define IOCTL_HIDE_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_SHOW_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
namespace hideprocess_km {
    HANDLE driverhandle;
    static inline BOOL Initialize() {
        driverhandle = CreateFileW(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
        if (driverhandle == INVALID_HANDLE_VALUE)
            return false;
        return true;
    }
    static inline BOOL HideProcess(ULONG pid) {
        DWORD bytes;
        return DeviceIoControl(driverhandle, IOCTL_HIDE_PROCESS, &pid, sizeof(pid), NULL, 0, &bytes, NULL);
    }

    static inline BOOL ShowProcess(ULONG pid) {
        DWORD bytes;
        return DeviceIoControl(driverhandle, IOCTL_SHOW_PROCESS, &pid, sizeof(pid), NULL, 0, &bytes, NULL);
    }

    static inline INT32 GetProcessId(LPCTSTR pn) {
        PROCESSENTRY32 pt;
        HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        pt.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hsnap, &pt)) {
            do {
                if (!lstrcmpi(pt.szExeFile, pn)) {
                    CloseHandle(hsnap);
                    return pt.th32ProcessID;
                }
            } while (Process32Next(hsnap, &pt));
        }
        CloseHandle(hsnap);
        return { NULL };
    }
}