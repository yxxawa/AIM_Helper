#pragma once

// Implement these exports in the user-provided, signed driver bridge DLL.
// This project loads the DLL dynamically and falls back to SendInput if any
// export is missing or DriverInit returns false.
extern "C" {
    __declspec(dllexport) bool __stdcall DriverInit();
    __declspec(dllexport) void __stdcall DriverMove(int x, int y);
    __declspec(dllexport) void __stdcall DriverClick(int button);
    __declspec(dllexport) void __stdcall DriverScroll(int delta);
    __declspec(dllexport) void __stdcall DriverCleanup();
}
