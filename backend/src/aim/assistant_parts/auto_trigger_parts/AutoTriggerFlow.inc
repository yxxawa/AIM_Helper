    void handleAutoClick(const TargetLock& target) {
        constexpr int target_lost_grace_ms = 120;
        const auto now = std::chrono::steady_clock::now();
        updateAutoStopMovementHistory(now);

        if (!cfg.enable_auto_click) {
            resetAutoClick("off");
            return;
        }

        if (!cfg.enable_mouse_movement) {
            resetAutoClick("mouse_off");
            return;
        }

        if (!target.valid) {
            if (cfg.auto_click_hold_mode) {
                resetAutoClick("no_target");
                return;
            }
            if (auto_click_locked && auto_click_last_target_seen_initialized) {
                const auto lost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - auto_click_last_target_seen_time).count();
                if (lost_ms <= target_lost_grace_ms) {
                    last_auto_click_state = "lost_grace";
                    return;
                }
            }
            resetAutoClick("no_target");
            return;
        }

        if (!isAimAllowedThisFrame()) {
            resetAutoClick("hold_wait");
            return;
        }

        auto_click_last_target_seen_time = now;
        auto_click_last_target_seen_initialized = true;
        if (updateAutoClickTargetContinuity(target)) {
            resetAutoClick("target_switch");
            auto_click_last_target_point = target.point;
            auto_click_last_target_initialized = true;
            auto_click_last_target_seen_time = now;
            auto_click_last_target_seen_initialized = true;
            if (cfg.auto_click_hold_mode) {
                return;
            }
        }

        if (cfg.auto_click_hold_mode) {
            const bool aligned = isTargetAlignedForAutoClick(target);
            if (!aligned) {
                resetAutoClick("range_wait");
                return;
            }

            if (!auto_click_locked) {
                const int delay_ms = randomAutoClickMs(cfg.auto_click_hold_delay_min_ms, cfg.auto_click_hold_delay_max_ms);
                auto_click_locked = true;
                auto_click_waiting_initial_delay = true;
                auto_click_next_time = now + std::chrono::milliseconds(delay_ms);
                if (delay_ms > 0) {
                    last_auto_click_state = "delay";
                    return;
                }
            }

            if (now < auto_click_next_time) {
                releaseAutoClickHoldIfOwned();
                last_auto_click_state = "delay";
                return;
            }

            if (cfg.enable_auto_stop && cfg.auto_stop_mode == "counter_tap" && isAnyMovementKeyDown()) {
                releaseAutoClickHoldIfOwned();
                last_auto_stop_state = "wait_release";
                last_auto_click_state = "stop_wait";
                return;
            }

            std::vector<int> auto_stop_keys = beginAutoStopForClick(now);
            pressAutoClickHoldIfNeeded();
            finishAutoStopForClick(auto_stop_keys);
            auto_click_waiting_initial_delay = false;
            auto_click_next_time = now;
            return;
        }

        if (!auto_click_locked) {
            const int delay_ms = randomAutoClickMs(cfg.auto_click_delay_min_ms, cfg.auto_click_delay_max_ms);
            auto_click_locked = true;
            auto_click_waiting_initial_delay = true;
            auto_click_next_time = now + std::chrono::milliseconds(delay_ms);
            if (delay_ms > 0) {
                last_auto_click_state = "delay";
                return;
            }
        }

        const bool aligned = isTargetAlignedForAutoClick(target);
        if (now < auto_click_next_time) {
            last_auto_click_state = auto_click_waiting_initial_delay
                ? (aligned ? "delay" : "delay_track")
                : (aligned ? "cooldown" : "cooldown_track");
            return;
        }

        if (!aligned) {
            last_auto_click_state = auto_click_waiting_initial_delay ? "ready_wait" : "cooldown_wait";
            return;
        }

        if (cfg.enable_auto_stop && cfg.auto_stop_mode == "counter_tap" && isAnyMovementKeyDown()) {
            last_auto_stop_state = "wait_release";
            last_auto_click_state = "stop_wait";
            return;
        }

        std::vector<int> auto_stop_keys = beginAutoStopForClick(now);
        if (isKeyDown(VK_LBUTTON)) {
            finishAutoStopForClick(auto_stop_keys);
            const int interval_ms = randomAutoClickMs(cfg.auto_click_interval_min_ms, cfg.auto_click_interval_max_ms);
            auto_click_waiting_initial_delay = false;
            auto_click_next_time = now + std::chrono::milliseconds(interval_ms);
            last_auto_click_state = "manual_hold";
            return;
        }

        if (!mouse->MouseDown(0) || !mouse->MouseUp(0)) {
            mouse->Click(0);
        }
        finishAutoStopForClick(auto_stop_keys);
        const int interval_ms = randomAutoClickMs(cfg.auto_click_interval_min_ms, cfg.auto_click_interval_max_ms);
        auto_click_waiting_initial_delay = false;
        auto_click_next_time = now + std::chrono::milliseconds(interval_ms);
        last_auto_click_state = "click";
    }
