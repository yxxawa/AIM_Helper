// Supported driver ABI modes and function pointer types

    enum class ApiMode {
        None,
        Standard,
        LogiDriver,
        LgMouse,
        Razer,
        Dd
    };

    using DriverInitFn = bool(__stdcall*)();
    using DriverMoveFn = void(__stdcall*)(int, int);
    using DriverClickFn = void(__stdcall*)(int);
    using DriverScrollFn = void(__stdcall*)(int);
    using DriverCleanupFn = void(__stdcall*)();

    using LegacyOpenFn = int(__stdcall*)();
    using LegacyOpen2Fn = int(__stdcall*)(int);
    using LegacyCloseFn = void(__stdcall*)();
    using LegacyMoveFn = int(__stdcall*)(int, int);
    using LegacyButtonFn = int(__stdcall*)(int);
    using LegacyGetDriverTypeFn = int(__stdcall*)();

    using LgMouseHandle = void*;
    using LgMouseOpenFn = LgMouseHandle(__stdcall*)();
    using LgMouseCloseFn = void(__stdcall*)(LgMouseHandle);
    using LgMouseMoveFn = int(__stdcall*)(LgMouseHandle, int, int);
    using LgMouseClickFn = int(__stdcall*)(LgMouseHandle, int);
    using LgMouseScrollFn = int(__stdcall*)(LgMouseHandle, int);
    using LgMouseButtonFn = int(__stdcall*)(LgMouseHandle);
    using LgMouseButtonWithCodeFn = int(__stdcall*)(LgMouseHandle, int);

    using RazerInitFn = void(__stdcall*)();
    using RazerMoveFn = void(__stdcall*)(int, int, bool);
    using RazerClickFn = void(__stdcall*)(int);

    using DdBtnFn = int(__stdcall*)(int);
    using DdMoveFn = int(__stdcall*)(int, int);
    using DdWhlFn = int(__stdcall*)(int);
    using DdKeyFn = int(__stdcall*)(int, int);
    using DdToDcFn = int(__stdcall*)(int);

