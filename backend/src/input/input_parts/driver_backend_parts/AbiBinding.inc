// DLL export binding for standard, Logi, LgMouse, Razer and DD ABIs

    bool BindStandardAbi() {
        driverInit = reinterpret_cast<DriverInitFn>(::GetProcAddress(hDll, "DriverInit"));
        driverMove = reinterpret_cast<DriverMoveFn>(::GetProcAddress(hDll, "DriverMove"));
        driverClick = reinterpret_cast<DriverClickFn>(::GetProcAddress(hDll, "DriverClick"));
        driverScroll = reinterpret_cast<DriverScrollFn>(::GetProcAddress(hDll, "DriverScroll"));
        driverCleanup = reinterpret_cast<DriverCleanupFn>(::GetProcAddress(hDll, "DriverCleanup"));
        if (!driverInit || !driverMove || !driverClick || !driverScroll || !driverCleanup) {
            return false;
        }
        if (!driverInit()) {
            throw std::runtime_error("DriverInit returned false.");
        }
        std::cout << "--- Driver ABI selected: StandardABI exports DriverMove/DriverClick/DriverScroll ---" << std::endl;
        api_mode = ApiMode::Standard;
        initialized = true;
        return true;
    }

    bool BindLogiDriverAbi() {
        legacyOpen = reinterpret_cast<LegacyOpenFn>(::GetProcAddress(hDll, "device_open"));
        legacyOpen2 = reinterpret_cast<LegacyOpen2Fn>(::GetProcAddress(hDll, "device_open2"));
        legacyClose = reinterpret_cast<LegacyCloseFn>(::GetProcAddress(hDll, "device_close"));
        legacyMove = reinterpret_cast<LegacyMoveFn>(::GetProcAddress(hDll, "moveR"));
        legacyMouseDown = reinterpret_cast<LegacyButtonFn>(::GetProcAddress(hDll, "mouse_down"));
        legacyMouseUp = reinterpret_cast<LegacyButtonFn>(::GetProcAddress(hDll, "mouse_up"));
        legacyGetDriverType = reinterpret_cast<LegacyGetDriverTypeFn>(::GetProcAddress(hDll, "get_driver_type"));
        if (!legacyOpen || !legacyClose || !legacyMove || !legacyMouseDown || !legacyMouseUp) {
            return false;
        }
        int open_result = 0;
        if (legacyOpen2) {
            open_result = legacyOpen2(preferred_driver_type);
            std::cout << "--- LogiDriver device_open2(" << preferred_driver_type << ") => "
                << open_result << " ---" << std::endl;
        }
        else {
            open_result = legacyOpen();
            std::cout << "--- LogiDriver device_open() => " << open_result << " ---" << std::endl;
        }
        if (!open_result) {
            throw std::runtime_error("device_open returned 0.");
        }
        if (legacyGetDriverType) {
            std::cout << "--- LogiDriver legacy driver_type=" << legacyGetDriverType() << " ---" << std::endl;
        }
        std::cout << "--- Driver ABI selected: LogiDriverLegacyABI export moveR ---" << std::endl;
        api_mode = ApiMode::LogiDriver;
        initialized = true;
        return true;
    }

    bool BindLgMouseAbi() {
        lgMouseOpen = reinterpret_cast<LgMouseOpenFn>(::GetProcAddress(hDll, "LgMouseOpen"));
        lgMouseClose = reinterpret_cast<LgMouseCloseFn>(::GetProcAddress(hDll, "LgMouseClose"));
        lgMouseMove = reinterpret_cast<LgMouseMoveFn>(::GetProcAddress(hDll, "LgMouseMoveEx"));
        if (!lgMouseMove) {
            lgMouseMove = reinterpret_cast<LgMouseMoveFn>(::GetProcAddress(hDll, "LgMouseMove"));
        }
        lgMouseLeftClick = reinterpret_cast<LgMouseClickFn>(::GetProcAddress(hDll, "LgMouseLeftClick"));
        lgMouseRightClick = reinterpret_cast<LgMouseClickFn>(::GetProcAddress(hDll, "LgMouseRightClick"));
        lgMouseMiddleClick = reinterpret_cast<LgMouseClickFn>(::GetProcAddress(hDll, "LgMouseMiddleClick"));
        lgMouseScroll = reinterpret_cast<LgMouseScrollFn>(::GetProcAddress(hDll, "LgMouseScroll"));
        lgMouseLeftDown = reinterpret_cast<LgMouseButtonFn>(::GetProcAddress(hDll, "LgMouseLeftDown"));
        lgMouseLeftUp = reinterpret_cast<LgMouseButtonFn>(::GetProcAddress(hDll, "LgMouseLeftUp"));
        lgMouseRightDown = reinterpret_cast<LgMouseButtonFn>(::GetProcAddress(hDll, "LgMouseRightDown"));
        lgMouseRightUp = reinterpret_cast<LgMouseButtonFn>(::GetProcAddress(hDll, "LgMouseRightUp"));
        lgMouseMiddleDown = reinterpret_cast<LgMouseButtonFn>(::GetProcAddress(hDll, "LgMouseMiddleDown"));
        lgMouseMiddleUp = reinterpret_cast<LgMouseButtonFn>(::GetProcAddress(hDll, "LgMouseMiddleUp"));
        lgMouseButtonDown = reinterpret_cast<LgMouseButtonWithCodeFn>(::GetProcAddress(hDll, "LgMouseButtonDown"));
        lgMouseButtonUp = reinterpret_cast<LgMouseButtonWithCodeFn>(::GetProcAddress(hDll, "LgMouseButtonUp"));
        if (!lgMouseOpen || !lgMouseClose || !lgMouseMove || !lgMouseLeftClick || !lgMouseRightClick || !lgMouseMiddleClick || !lgMouseScroll) {
            return false;
        }
        lgMouseHandle = lgMouseOpen();
        if (!lgMouseHandle) {
            throw std::runtime_error("LgMouseOpen returned null.");
        }
        std::cout << "--- LgMouse legacy handle opened ---" << std::endl;
        std::cout << "--- Driver ABI selected: LgMouseLegacyABI export LgMouseMove/LgMouseMoveEx ---" << std::endl;
        api_mode = ApiMode::LgMouse;
        initialized = true;
        return true;
    }

    bool BindRazerAbi() {
        razerInit = reinterpret_cast<RazerInitFn>(::GetProcAddress(hDll, "RazerInit"));
        razerMove = reinterpret_cast<RazerMoveFn>(::GetProcAddress(hDll, "RazerMouse_move"));
        razerClick = reinterpret_cast<RazerClickFn>(::GetProcAddress(hDll, "RazerMouse_click"));
        if (!razerInit || !razerMove || !razerClick) {
            return false;
        }
        razerInit();
        std::cout << "--- Driver ABI selected: RazerABI exports RazerMouse_move/RazerMouse_click ---" << std::endl;
        api_mode = ApiMode::Razer;
        initialized = true;
        return true;
    }

    bool BindDdAbi() {
        ddBtn = reinterpret_cast<DdBtnFn>(::GetProcAddress(hDll, "DD_btn"));
        ddMoveR = reinterpret_cast<DdMoveFn>(::GetProcAddress(hDll, "DD_movR"));
        ddMoveAbs = reinterpret_cast<DdMoveFn>(::GetProcAddress(hDll, "DD_mov"));
        ddWhl = reinterpret_cast<DdWhlFn>(::GetProcAddress(hDll, "DD_whl"));
        ddKey = reinterpret_cast<DdKeyFn>(::GetProcAddress(hDll, "DD_key"));
        ddToDc = reinterpret_cast<DdToDcFn>(::GetProcAddress(hDll, "DD_todc"));
        if (!ddBtn || !ddMoveR) {
            return false;
        }
        const int init_result = ddBtn(0);
        std::cout << "--- DD driver DD_btn(0) => " << init_result << " ---" << std::endl;
        if (init_result != 1) {
            throw std::runtime_error("DD_btn(0) did not return 1.");
        }
        std::cout << "--- Driver ABI selected: DDABI export DD_movR/DD_btn ---" << std::endl;
        if (ddKey && ddToDc) {
            std::cout << "--- DD keyboard input enabled: DD_todc + DD_key ---" << std::endl;
        }
        else {
            std::cerr << "\n[WARN] DD keyboard exports DD_todc/DD_key are missing; key events will use SendInput fallback." << std::endl;
        }
        api_mode = ApiMode::Dd;
        initialized = true;
        return true;
    }

    int DdCodeFromVk(int vk) const {
        if (!ddToDc) {
            return -1;
        }
        int dd_code = ddToDc(vk);
        if (dd_code == -1) {
            if (vk == VK_LSHIFT || vk == VK_RSHIFT) {
                dd_code = ddToDc(VK_SHIFT);
            }
            else if (vk == VK_LCONTROL || vk == VK_RCONTROL) {
                dd_code = ddToDc(VK_CONTROL);
            }
            else if (vk == VK_LMENU || vk == VK_RMENU) {
                dd_code = ddToDc(VK_MENU);
            }
        }
        return dd_code;
    }

    void SendKeyEvent(int vk, bool key_down) {
        if (vk <= 0) {
            return;
        }
        if (api_mode == ApiMode::Dd && ddKey && ddToDc) {
            const int dd_code = DdCodeFromVk(vk);
            if (dd_code != -1) {
                const int result = ddKey(dd_code, key_down ? 1 : 2);
                LogKeyResult("DD_key", vk, dd_code, key_down, result);
                return;
            }
            if (!keyboardFallbackWarned) {
                std::cerr << "\n[WARN] DD_todc does not support " << KeyNameFromVk(vk)
                    << "; key event will use SendInput fallback." << std::endl;
                keyboardFallbackWarned = true;
            }
        }
        else if (!keyboardFallbackWarned) {
            std::cerr << "\n[WARN] Active driver backend has no keyboard ABI; key events will use SendInput fallback." << std::endl;
            keyboardFallbackWarned = true;
        }

        SendKeyboardInputVk(vk, key_down, Name());
    }

