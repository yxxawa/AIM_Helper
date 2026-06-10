// Driver call logging and DLL cleanup

    void LogMoveCall(const char* function_name, int x, int y) {
        if (moveLogBudget > 0) {
            std::cout << "\n[DRIVER] " << function_name << "(" << x << "," << y << ") => called" << std::endl;
            --moveLogBudget;
        }
    }

    void LogMoveResult(const char* function_name, int x, int y, int result) {
        if (moveLogBudget > 0 || result == 0) {
            std::cout << "\n[DRIVER] " << function_name << "(" << x << "," << y << ") => "
                << result << std::endl;
            if (moveLogBudget > 0) {
                --moveLogBudget;
            }
        }
    }

    void LogKeyResult(const char* function_name, int vk, int dd_code, bool key_down, int result) {
        if (keyLogBudget > 0 || result == 0) {
            std::cout << "\n[DRIVER] " << function_name << "("
                << KeyNameFromVk(vk) << ", dd=" << dd_code
                << ", " << (key_down ? "down" : "up") << ") => "
                << result << std::endl;
            if (keyLogBudget > 0) {
                --keyLogBudget;
            }
        }
    }

    void CleanupLibrary() {
        if (hDll) {
            ::FreeLibrary(hDll);
            hDll = nullptr;
        }
        driverInit = nullptr;
        driverMove = nullptr;
        driverClick = nullptr;
        driverScroll = nullptr;
        driverCleanup = nullptr;
        legacyOpen = nullptr;
        legacyOpen2 = nullptr;
        legacyClose = nullptr;
        legacyMove = nullptr;
        legacyMouseDown = nullptr;
        legacyMouseUp = nullptr;
        legacyGetDriverType = nullptr;
        lgMouseOpen = nullptr;
        lgMouseClose = nullptr;
        lgMouseMove = nullptr;
        lgMouseLeftClick = nullptr;
        lgMouseRightClick = nullptr;
        lgMouseMiddleClick = nullptr;
        lgMouseScroll = nullptr;
        lgMouseLeftDown = nullptr;
        lgMouseLeftUp = nullptr;
        lgMouseRightDown = nullptr;
        lgMouseRightUp = nullptr;
        lgMouseMiddleDown = nullptr;
        lgMouseMiddleUp = nullptr;
        lgMouseButtonDown = nullptr;
        lgMouseButtonUp = nullptr;
        lgMouseHandle = nullptr;
        razerInit = nullptr;
        razerMove = nullptr;
        razerClick = nullptr;
        ddBtn = nullptr;
        ddMoveR = nullptr;
        ddMoveAbs = nullptr;
        ddWhl = nullptr;
        ddKey = nullptr;
        ddToDc = nullptr;
        initialized = false;
        api_mode = ApiMode::None;
    }

