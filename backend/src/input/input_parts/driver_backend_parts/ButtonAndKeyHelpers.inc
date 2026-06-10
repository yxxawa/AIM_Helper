// Mouse button and DD keyboard helper methods

    static bool LegacyButtonFromButton(int button, int& code) {
        if (button < 0 || button > 2) {
            return false;
        }
        code = button + 1;
        return true;
    }

    static bool DdButtonCode(int button, bool down, int& code) {
        if (button == 0) {
            code = down ? 1 : 2;
        }
        else if (button == 1) {
            code = down ? 4 : 8;
        }
        else if (button == 2) {
            code = down ? 16 : 32;
        }
        else {
            return false;
        }
        return true;
    }

    static int RazerButtonDownCode(int button) {
        if (button == 1) {
            return 0x0004;
        }
        if (button == 2) {
            return 0x0010;
        }
        return 0x0001;
    }

    static int RazerButtonUpCode(int button) {
        if (button == 1) {
            return 0x0008;
        }
        if (button == 2) {
            return 0x0020;
        }
        return 0x0002;
    }

    static int LgMouseButtonCode(int button) {
        if (button == 1) {
            return 2;
        }
        if (button == 2) {
            return 4;
        }
        return 1;
    }

    bool SendMouseButton(int button, bool down) {
        if (api_mode == ApiMode::LogiDriver && legacyMouseDown && legacyMouseUp) {
            int legacy_button = 0;
            if (!LegacyButtonFromButton(button, legacy_button)) {
                return false;
            }
            down ? legacyMouseDown(legacy_button) : legacyMouseUp(legacy_button);
            return true;
        }
        if (api_mode == ApiMode::Dd && ddBtn) {
            int dd_code = 0;
            if (!DdButtonCode(button, down, dd_code)) {
                return false;
            }
            ddBtn(dd_code);
            return true;
        }
        if (api_mode == ApiMode::LgMouse && lgMouseHandle) {
            if (button == 0 && lgMouseLeftDown && lgMouseLeftUp) {
                return (down ? lgMouseLeftDown(lgMouseHandle) : lgMouseLeftUp(lgMouseHandle)) != 0;
            }
            if (button == 1 && lgMouseRightDown && lgMouseRightUp) {
                return (down ? lgMouseRightDown(lgMouseHandle) : lgMouseRightUp(lgMouseHandle)) != 0;
            }
            if (button == 2 && lgMouseMiddleDown && lgMouseMiddleUp) {
                return (down ? lgMouseMiddleDown(lgMouseHandle) : lgMouseMiddleUp(lgMouseHandle)) != 0;
            }
            if (lgMouseButtonDown && lgMouseButtonUp) {
                return (down
                    ? lgMouseButtonDown(lgMouseHandle, LgMouseButtonCode(button))
                    : lgMouseButtonUp(lgMouseHandle, LgMouseButtonCode(button))) != 0;
            }
        }
        if (api_mode == ApiMode::Razer && razerClick) {
            razerClick(down ? RazerButtonDownCode(button) : RazerButtonUpCode(button));
            return true;
        }

        if (!mouseHoldUnsupportedWarned) {
            std::cerr << "\n[WARN] " << Name()
                << " does not expose mouse down/up; auto trigger hold mode is unavailable for this backend."
                << std::endl;
            mouseHoldUnsupportedWarned = true;
        }
        return false;
    }

