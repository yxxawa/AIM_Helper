    void handleMouseBackendTestHotkey() {
        static bool f9_was_pressed = false;
        const bool f9_is_pressed = GetAsyncKeyState(cfg.mouse_test_key) & 0x8000;
        if (f9_is_pressed && !f9_was_pressed) {
            std::cout << "\n[INFO] F9 mouse backend test: move +20 then -20." << std::endl;
            mouse->MoveRelative(20, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            mouse->MoveRelative(-20, 0);
        }
        f9_was_pressed = f9_is_pressed;
    }

    bool isAimHotkeyDown() const {
        return isKeyDown(cfg.smooth_aim_key) || isKeyDown(cfg.smooth_aim_secondary_key);
    }

    bool isAimAllowedThisFrame() const {
        return !cfg.enable_hold_to_aim || isAimHotkeyDown();
    }

    void handleMouseInput(const TargetLock& target) {
        if (!cfg.enable_mouse_movement) {
            last_gate_state = "mouse_off";
            resetAimCarry();
            resetAimFilters();
            return;
        }

        if (!target.valid) {
            last_gate_state = "no_target";
            resetAimCarry();
            resetAimFilters();
            return;
        }

        const bool smooth_triggered = isAimAllowedThisFrame();
        int move_x = 0;
        int move_y = 0;
        if (smooth_triggered) {
            if (computeAimMove(target, true, move_x, move_y)) {
                last_gate_state = cfg.enable_hold_to_aim ? "hold_move" : "auto_move";
            }
        }
        else {
            last_gate_state = "hold_wait";
            resetTargetLock();
            resetAimCarry();
            resetAimFilters();
        }

        if (!smooth_triggered) {
            updateRawAimTelemetry(target);
        }

        if (move_x == 0 && move_y == 0) {
            return;
        }

        int output_move_x = move_x;
        int output_move_y = move_y;
        const bool bypass_visual_coalescing = cfg.enable_humanized_movement;
        if (!coalesceVisualMouseMove(move_x, move_y, bypass_visual_coalescing, output_move_x, output_move_y)) {
            last_gate_state = "visual_coalesce";
            return;
        }
        if (cfg.enable_anti_snap) {
            const double output_len = std::hypot(
                static_cast<double>(output_move_x),
                static_cast<double>(output_move_y));
            if (output_len > static_cast<double>(cfg.anti_snap_max_delta)) {
                last_gate_state = "anti_snap";
                last_move_x = 0;
                last_move_y = 0;
                resetAimCarry();
                resetAimFilters();
                return;
            }
        }
        last_move_x = output_move_x;
        last_move_y = output_move_y;
        mouse->MoveRelative(output_move_x, output_move_y, &cfg);
    }
