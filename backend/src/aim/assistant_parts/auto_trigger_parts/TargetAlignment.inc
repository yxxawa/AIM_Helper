    bool isTargetAlignedForAutoClick(const TargetLock& target) const {
        if (!target.valid) {
            return false;
        }
        const double dx = static_cast<double>(target.point.x - active_crosshair.x);
        const double dy = static_cast<double>(target.point.y - active_crosshair.y);
        return std::hypot(dx, dy) <= std::max(0.0, cfg.auto_click_tolerance_pixels);
    }

    bool updateAutoClickTargetContinuity(const TargetLock& target) {
        if (!target.valid) {
            return false;
        }
        if (!auto_click_last_target_initialized) {
            auto_click_last_target_point = target.point;
            auto_click_last_target_initialized = true;
            return false;
        }

        const double jump = std::hypot(
            static_cast<double>(target.point.x - auto_click_last_target_point.x),
            static_cast<double>(target.point.y - auto_click_last_target_point.y));
        auto_click_last_target_point = target.point;
        const double switch_threshold = std::max(45.0, static_cast<double>(cfg.max_lock_distance_pixels) * 0.75);
        return jump > switch_threshold;
    }
