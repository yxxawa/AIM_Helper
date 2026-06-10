// DriverBackend public lifecycle and input operations

    DriverBackend(const std::wstring& dll_path, int requested_driver_type)
        : preferred_driver_type(requested_driver_type) {
        if (dll_path.empty()) {
            throw std::runtime_error("Driver DLL path is empty.");
        }

        std::wcout << L"--- Loading driver mouse backend DLL: " << dll_path << L" ---" << std::endl;
        hDll = ::LoadLibraryW(dll_path.c_str());
        if (!hDll) {
            throw std::runtime_error("LoadLibraryW failed for driver DLL.");
        }

        try {
            if (!BindStandardAbi() && !BindLogiDriverAbi() && !BindLgMouseAbi() && !BindRazerAbi() && !BindDdAbi()) {
                throw std::runtime_error("Driver DLL exports do not match any supported input backend ABI.");
            }
        }
        catch (...) {
            CleanupLibrary();
            throw;
        }
        std::cout << "--- Mouse input initialized: " << Name() << " ---" << std::endl;
    }

    ~DriverBackend() override {
        if (api_mode == ApiMode::Standard && initialized && driverCleanup) {
            driverCleanup();
        }
        else if (api_mode == ApiMode::LogiDriver && initialized && legacyClose) {
            legacyClose();
        }
        else if (api_mode == ApiMode::LgMouse && lgMouseHandle && lgMouseClose) {
            lgMouseClose(lgMouseHandle);
            lgMouseHandle = nullptr;
        }
        CleanupLibrary();
    }

    DriverBackend(const DriverBackend&) = delete;
    DriverBackend& operator=(const DriverBackend&) = delete;

    void MoveMouse(int x, int y) override {
        if (x == 0 && y == 0) {
            return;
        }
        if (api_mode == ApiMode::Standard && driverMove) {
            driverMove(x, y);
            LogMoveCall("DriverMove", x, y);
        }
        else if (api_mode == ApiMode::LogiDriver && legacyMove) {
            const int result = legacyMove(x, y);
            LogMoveResult("moveR", x, y, result);
        }
        else if (api_mode == ApiMode::LgMouse && lgMouseMove && lgMouseHandle) {
            const int result = lgMouseMove(lgMouseHandle, x, y);
            LogMoveResult("LgMouseMove", x, y, result);
        }
        else if (api_mode == ApiMode::Dd && ddMoveR) {
            const int result = ddMoveR(x, y);
            LogMoveResult("DD_movR", x, y, result);
        }
        else if (api_mode == ApiMode::Razer && razerMove) {
            razerMove(x, y, false);
            LogMoveCall("RazerMouse_move", x, y);
        }
    }

    void Click(int button) override {
        if (api_mode == ApiMode::Standard && driverClick) {
            driverClick(button);
        }
        else if (api_mode == ApiMode::LogiDriver && legacyMouseDown && legacyMouseUp) {
            const int legacy_button = button + 1;
            legacyMouseDown(legacy_button);
            ::Sleep(10);
            legacyMouseUp(legacy_button);
        }
        else if (api_mode == ApiMode::LgMouse && lgMouseHandle) {
            if (button == 0 && lgMouseLeftClick) lgMouseLeftClick(lgMouseHandle, 10);
            else if (button == 1 && lgMouseRightClick) lgMouseRightClick(lgMouseHandle, 10);
            else if (button == 2 && lgMouseMiddleClick) lgMouseMiddleClick(lgMouseHandle, 10);
        }
        else if (api_mode == ApiMode::Dd && ddBtn) {
            int down_code = 0;
            int up_code = 0;
            if (button == 0) {
                down_code = 1;
                up_code = 2;
            }
            else if (button == 1) {
                down_code = 4;
                up_code = 8;
            }
            else if (button == 2) {
                down_code = 16;
                up_code = 32;
            }
            if (down_code && up_code) {
                ddBtn(down_code);
                ::Sleep(10);
                ddBtn(up_code);
            }
        }
        else if (api_mode == ApiMode::Razer && razerClick) {
            razerClick(RazerButtonDownCode(button));
            ::Sleep(10);
            razerClick(RazerButtonUpCode(button));
        }
    }

    bool MouseDown(int button) override {
        return SendMouseButton(button, true);
    }

    bool MouseUp(int button) override {
        return SendMouseButton(button, false);
    }

    void Scroll(int delta) override {
        if (delta == 0) {
            return;
        }
        if (api_mode == ApiMode::Standard && driverScroll) {
            driverScroll(delta);
        }
        else if (api_mode == ApiMode::LgMouse && lgMouseScroll && lgMouseHandle) {
            lgMouseScroll(lgMouseHandle, delta);
        }
        else if (api_mode == ApiMode::Dd && ddWhl) {
            ddWhl(delta);
        }
        else if (api_mode == ApiMode::LogiDriver && !legacyScrollWarned) {
            std::cerr << "\n[WARN] LogiDriver legacy ABI has no scroll export; scroll ignored." << std::endl;
            legacyScrollWarned = true;
        }
    }

    void KeyDown(int vk) override {
        SendKeyEvent(vk, true);
    }

    void KeyUp(int vk) override {
        SendKeyEvent(vk, false);
    }

    const char* Name() const override {
        switch (api_mode) {
        case ApiMode::Standard: return "DriverBackend(StandardABI)";
        case ApiMode::LogiDriver: return "DriverBackend(LogiDriverLegacyABI)";
        case ApiMode::LgMouse: return "DriverBackend(LgMouseLegacyABI)";
        case ApiMode::Razer: return "DriverBackend(RazerABI)";
        case ApiMode::Dd: return "DriverBackend(DDABI)";
        default: return "DriverBackend(Unbound)";
        }
    }

    bool IsDriverBackend() const override {
        return true;
    }

