    void releaseAutoClickHoldIfOwned() {
        if (auto_click_owned_mouse_down && mouse) {
            if (!mouse->MouseUp(0)) {
                last_auto_click_state = "hold_release_failed";
                return;
            }
        }
        auto_click_owned_mouse_down = false;
    }

    bool pressAutoClickHoldIfNeeded() {
        if (auto_click_owned_mouse_down) {
            last_auto_click_state = "hold";
            return true;
        }
        if (isKeyDown(VK_LBUTTON)) {
            last_auto_click_state = "manual_hold";
            return false;
        }
        if (mouse) {
            auto_click_owned_mouse_down = true;
            if (!mouse->MouseDown(0)) {
                auto_click_owned_mouse_down = false;
                last_auto_click_state = "hold_unavailable";
                return false;
            }
            last_auto_click_state = "hold";
            return true;
        }
        last_auto_click_state = "hold_unavailable";
        return false;
    }
