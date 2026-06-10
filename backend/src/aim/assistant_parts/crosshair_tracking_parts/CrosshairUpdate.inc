    void updateCrosshairTracking() {
        if (!cfg.enable_crosshair_color || captured_frame.empty()) {
            resetCrosshairTracking();
            return;
        }

        cv::Point2d detected_point;
        int detected_area = 0;
        double detected_score = 0.0;
        if (!findCrosshairColorBlock(detected_point, detected_area, detected_score)) {
            resetCrosshairTracking();
            return;
        }

        const cv::Point2d current(
            static_cast<double>(active_crosshair.x),
            static_cast<double>(active_crosshair.y));
        const double jump = crosshair_tracking_initialized
            ? std::hypot(detected_point.x - current.x, detected_point.y - current.y)
            : 0.0;
        const double jump_limit = std::clamp(static_cast<double>(cfg.crop_size) * 0.10, 14.0, 42.0);
        if (crosshair_tracking_initialized && jump > jump_limit) {
            ++crosshair_rejected_frames;
            if (crosshair_rejected_frames <= 2) {
                resetCrosshairTracking();
                return;
            }
            const double scale = jump_limit / std::max(jump, 0.001);
            detected_point = cv::Point2d(
                current.x + (detected_point.x - current.x) * scale,
                current.y + (detected_point.y - current.y) * scale);
        }
        else {
            crosshair_rejected_frames = 0;
        }
        crosshair_missed_frames = 0;

        {
            const double center_offset = std::hypot(detected_point.x - crop_center.x, detected_point.y - crop_center.y);
            const double max_center_offset = std::clamp(static_cast<double>(cfg.crop_size) * 0.42, 80.0, 260.0);
            if (center_offset > max_center_offset && crosshair_tracking_initialized) {
                resetCrosshairTracking();
                return;
            }
        }

        {
            const double accepted_jump = std::hypot(detected_point.x - current.x, detected_point.y - current.y);
            if (crosshair_tracking_initialized && accepted_jump > jump_limit) {
                const double scale = jump_limit / std::max(accepted_jump, 0.001);
                detected_point = cv::Point2d(
                    current.x + (detected_point.x - current.x) * scale,
                    current.y + (detected_point.y - current.y) * scale);
            }
        }

        detected_crosshair = cv::Point(
            std::clamp(static_cast<int>(std::lround(detected_point.x)), 0, std::max(0, cfg.crop_size - 1)),
            std::clamp(static_cast<int>(std::lround(detected_point.y)), 0, std::max(0, cfg.crop_size - 1)));

        const bool aiming_now = cfg.enable_mouse_movement && isAimAllowedThisFrame();
        const double alpha = (crosshair_tracking_initialized && !aiming_now)
            ? std::clamp(1.0 - cfg.crosshair_smoothing, 0.02, 1.0)
            : 1.0;
        const cv::Point2d smoothed(
            current.x * (1.0 - alpha) + detected_point.x * alpha,
            current.y * (1.0 - alpha) + detected_point.y * alpha);
        active_crosshair = cv::Point(
            std::clamp(static_cast<int>(std::lround(smoothed.x)), 0, std::max(0, cfg.crop_size - 1)),
            std::clamp(static_cast<int>(std::lround(smoothed.y)), 0, std::max(0, cfg.crop_size - 1)));
        crosshair_tracking_initialized = true;
        last_crosshair_found = true;
        last_crosshair_area = detected_area;
        last_crosshair_score = detected_score;
        last_crosshair_offset_x = active_crosshair.x - crop_center.x;
        last_crosshair_offset_y = active_crosshair.y - crop_center.y;
    }
