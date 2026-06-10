    std::pair<double, double> mapAimOffsetToMouse(double dx_pixels, double dy_pixels) const {
        const double effective_gain = cfg.aim_gain * cfg.aim_smoothing;
        if (cfg.aim_mode == "linear") {
            return {
                dx_pixels * effective_gain,
                dy_pixels * effective_gain
            };
        }

        const double denominator = static_cast<double>(std::max(1, cfg.crop_size));
        return {
            std::atan2(dx_pixels, denominator) * denominator * effective_gain,
            std::atan2(dy_pixels, denominator) * denominator * effective_gain
        };
    }

    void updateTargetOffset(const TargetLock& target) {
        if (!target.valid) {
            last_target_offset_x = 0;
            last_target_offset_y = 0;
            return;
        }
        const cv::Point target_point = target.point;
        last_target_offset_x = target_point.x - active_crosshair.x;
        last_target_offset_y = target_point.y - active_crosshair.y;
    }

    void updateRawAimTelemetry(const TargetLock& target) {
        if (!target.valid) {
            last_raw_move_x = 0.0;
            last_raw_move_y = 0.0;
            return;
        }
        const cv::Point target_point = target.point;
        const double dx_pixels = static_cast<double>(target_point.x - active_crosshair.x);
        const double dy_pixels = static_cast<double>(target_point.y - active_crosshair.y);
        auto raw = mapAimOffsetToMouse(dx_pixels, dy_pixels);
        raw = applyAimFilter(raw);
        last_raw_move_x = raw.first;
        last_raw_move_y = raw.second;
    }

    bool computeAimMove(const TargetLock& target, bool use_carry, int& move_x, int& move_y) {
        move_x = 0;
        move_y = 0;
        const cv::Point target_point = target.point;
        const double dx_pixels = static_cast<double>(target_point.x - active_crosshair.x);
        const double dy_pixels = static_cast<double>(target_point.y - active_crosshair.y);
        const double distance_pixels = std::hypot(dx_pixels, dy_pixels);

        if (cfg.enable_small_lock_only && distance_pixels > static_cast<double>(cfg.small_lock_radius_pixels)) {
            last_raw_move_x = 0.0;
            last_raw_move_y = 0.0;
            last_gate_state = "small_lock_wait";
            if (use_carry) {
                resetAimCarry();
                resetAimFilters();
            }
            return false;
        }

        if (distance_pixels <= cfg.aim_deadzone_pixels) {
            last_raw_move_x = 0.0;
            last_raw_move_y = 0.0;
            last_gate_state = "deadzone";
            if (use_carry) {
                resetAimCarry();
                resetAimFilters();
            }
            return false;
        }

        auto raw = mapAimOffsetToMouse(dx_pixels, dy_pixels);
        raw = applyAimFilter(raw);
        raw = stabilizeAimOutput(raw, distance_pixels, false);
        last_raw_move_x = raw.first;
        last_raw_move_y = raw.second;
        raw = limitSmoothSlideVector(raw);

        if (use_carry) {
            move_x = takeMouseDelta(raw.first, aim_carry_x);
            move_y = takeMouseDelta(raw.second, aim_carry_y);
        }
        else {
            move_x = toMouseDelta(raw.first);
            move_y = toMouseDelta(raw.second);
        }

        if (move_x == 0 && move_y == 0) {
            last_gate_state = use_carry ? "accum" : "subpixel";
            return false;
        }
        return true;
    }
