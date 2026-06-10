    void resetAutoClick(const std::string& state = "idle") {
        releaseAutoClickHoldIfOwned();
        auto_click_locked = false;
        auto_click_waiting_initial_delay = true;
        auto_click_next_time = {};
        auto_click_last_target_initialized = false;
        auto_click_last_target_seen_initialized = false;
        last_auto_click_state = cfg.enable_auto_click ? state : "off";
        if (!cfg.enable_auto_click || !cfg.enable_auto_stop) {
            last_auto_stop_state = "off";
        }
    }

    int randomAutoClickMs(int min_ms, int max_ms) {
        min_ms = std::clamp(min_ms, 0, 5000);
        max_ms = std::clamp(max_ms, 0, 5000);
        if (min_ms > max_ms) {
            std::swap(min_ms, max_ms);
        }
        std::uniform_int_distribution<int> dist(min_ms, max_ms);
        return dist(auto_click_rng);
    }

    bool isKeyDown(int vk) const {
        return vk > 0 && (::GetAsyncKeyState(vk) & 0x8000);
    }
