    cv::Point clampPointToCrop(const cv::Point2d& point) const {
        const int max_coord = std::max(0, cfg.crop_size - 1);
        return cv::Point(
            std::clamp(static_cast<int>(std::round(point.x)), 0, max_coord),
            std::clamp(static_cast<int>(std::round(point.y)), 0, max_coord));
    }

    double nextFilterDt() {
        const auto now = std::chrono::steady_clock::now();
        if (!filter_time_initialized) {
            last_filter_time = now;
            filter_time_initialized = true;
            return 1.0 / 144.0;
        }
        const double dt = std::chrono::duration<double>(now - last_filter_time).count();
        last_filter_time = now;
        return std::clamp(dt, 0.001, 0.1);
    }

    std::pair<double, double> applyAimFilter(std::pair<double, double> raw) {
        if (cfg.aim_filter_mode == "none") {
            return raw;
        }

        const double dt = nextFilterDt();
        if (cfg.aim_filter_mode == "pid") {
            raw.first = pid_x.Update(raw.first, dt, cfg);
            raw.second = pid_y.Update(raw.second, dt, cfg);
        }
        if (cfg.aim_filter_mode == "oneeuro") {
            raw.first = euro_x.Update(raw.first, dt, cfg);
            raw.second = euro_y.Update(raw.second, dt, cfg);
        }
        return raw;
    }

    std::pair<double, double> stabilizeAimOutput(std::pair<double, double> raw, double distance_pixels, bool direct_tracking) {
        (void)direct_tracking;
        const double raw_len = std::hypot(raw.first, raw.second);
        if (raw_len < 0.001) {
            last_aim_output_raw_x = 0.0;
            last_aim_output_raw_y = 0.0;
            aim_output_initialized = false;
            return raw;
        }

        const double near_deadzone = std::max(cfg.aim_deadzone_pixels + 1.2, 3.2);
        if (distance_pixels <= near_deadzone && raw_len < 1.15) {
            raw = { 0.0, 0.0 };
            aim_carry_x = 0.0;
            aim_carry_y = 0.0;
        }

        last_aim_output_raw_x = raw.first;
        last_aim_output_raw_y = raw.second;
        aim_output_initialized = true;
        return raw;
    }
