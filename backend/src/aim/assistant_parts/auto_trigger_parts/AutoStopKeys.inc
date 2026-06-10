    static int OppositeMovementKey(int vk) {
        switch (vk) {
        case 'W': return 'S';
        case 'S': return 'W';
        case 'A': return 'D';
        case 'D': return 'A';
        default: return 0;
        }
    }

    static const std::array<int, 4>& MovementKeys() {
        static const std::array<int, 4> keys{ 'W', 'A', 'S', 'D' };
        return keys;
    }

    void addAutoStopKey(std::vector<int>& keys, int vk) const {
        if (vk <= 0 || isKeyDown(vk)) {
            return;
        }
        if (std::find(keys.begin(), keys.end(), vk) == keys.end()) {
            keys.push_back(vk);
        }
    }

    void updateAutoStopMovementHistory(const std::chrono::steady_clock::time_point& now) {
        const auto& movement_keys = MovementKeys();
        for (size_t i = 0; i < movement_keys.size(); ++i) {
            const bool down = isKeyDown(movement_keys[i]);
            if (auto_stop_movement_was_down[i] && !down) {
                last_auto_stop_released_key = movement_keys[i];
                last_auto_stop_release_time = now;
                last_auto_stop_release_initialized = true;
            }
            auto_stop_movement_was_down[i] = down;
        }
    }

    bool isAnyMovementKeyDown() const {
        for (int vk : MovementKeys()) {
            if (isKeyDown(vk)) {
                return true;
            }
        }
        return false;
    }

    std::vector<int> counterTapAutoStopKeys(const std::chrono::steady_clock::time_point& now) const {
        std::vector<int> keys;
        const bool w_down = isKeyDown('W');
        const bool a_down = isKeyDown('A');
        const bool s_down = isKeyDown('S');
        const bool d_down = isKeyDown('D');

        if (w_down && !s_down) addAutoStopKey(keys, 'S');
        if (s_down && !w_down) addAutoStopKey(keys, 'W');
        if (a_down && !d_down) addAutoStopKey(keys, 'D');
        if (d_down && !a_down) addAutoStopKey(keys, 'A');

        if (keys.empty() && last_auto_stop_release_initialized) {
            const auto released_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_auto_stop_release_time).count();
            if (released_ms <= 180) {
                addAutoStopKey(keys, OppositeMovementKey(last_auto_stop_released_key));
            }
        }
        return keys;
    }

    std::vector<int> autoStopKeysForClick(const std::chrono::steady_clock::time_point& now) const {
        std::vector<int> keys;
        if (!cfg.enable_auto_stop) {
            return keys;
        }
        if (cfg.auto_stop_mode == "ad_pair") {
            addAutoStopKey(keys, 'A');
            addAutoStopKey(keys, 'D');
        }
        else if (cfg.auto_stop_mode == "crouch") {
            addAutoStopKey(keys, VK_CONTROL);
        }
        else {
            keys = counterTapAutoStopKeys(now);
        }
        return keys;
    }

    std::vector<int> beginAutoStopForClick(const std::chrono::steady_clock::time_point& now) {
        if (!cfg.enable_auto_stop) {
            last_auto_stop_state = "off";
            return {};
        }

        std::vector<int> keys = autoStopKeysForClick(now);
        if (keys.empty()) {
            last_auto_stop_state = "ready";
            return keys;
        }

        const int hold_ms = std::clamp(cfg.auto_stop_hold_ms, 0, 250);
        const int settle_ms = std::clamp(cfg.auto_stop_settle_ms, 0, 250);
        mouse->PressKeys(keys);
        if (hold_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
        }
        last_auto_stop_state = cfg.auto_stop_mode;
        if (cfg.auto_stop_mode == "counter_tap") {
            mouse->ReleaseKeys(keys);
            keys.clear();
            if (settle_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(settle_ms));
            }
            last_auto_stop_state = "counter_ready";
        }
        else if (settle_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(settle_ms));
        }
        return keys;
    }

    void finishAutoStopForClick(const std::vector<int>& keys) {
        if (!keys.empty()) {
            mouse->ReleaseKeys(keys);
        }
    }
