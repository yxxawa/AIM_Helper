    void recordAimSampleFrame(const TargetLock& target, bool aim_triggered) {
        if (!aim_triggered || !target.valid) {
            return;
        }
        writeAimSampleSettingsIfNeeded();
        if (!aim_sample_log_opened) {
            return;
        }

        const cv::Point2d measured = target.selected
            ? BoxCenterPrecise(target.selected->box)
            : cv::Point2d(static_cast<double>(target.measured_point.x), static_cast<double>(target.measured_point.y));
        const cv::Point2d aim_point(static_cast<double>(target.point.x), static_cast<double>(target.point.y));
        const cv::Point2d crosshair(static_cast<double>(active_crosshair.x), static_cast<double>(active_crosshair.y));
        const cv::Point2d screen_center(static_cast<double>(crop_center.x), static_cast<double>(crop_center.y));
        const double measured_error_x = measured.x - crosshair.x;
        const double measured_error_y = measured.y - crosshair.y;
        const double aim_error_x = aim_point.x - crosshair.x;
        const double aim_error_y = aim_point.y - crosshair.y;
        const int class_id = target.selected ? target.selected->class_id : -1;
        const float confidence = target.selected ? target.selected->confidence : 0.0f;
        const cv::Rect selected_box = target.selected ? target.selected->box : target.box;

        aim_sample_log << std::boolalpha << std::fixed << std::setprecision(3)
            << "{\"type\":\"sample\",\"ts\":\"" << CurrentLocalTimestampIso() << "\""
            << ",\"frame\":" << aim_sample_frame_index++
            << ",\"part\":\"" << JsonEscape(target.part) << "\""
            << ",\"class_id\":" << class_id
            << ",\"confidence\":" << confidence
            << ",\"crosshair\":{\"x\":" << crosshair.x << ",\"y\":" << crosshair.y << "}"
            << ",\"screen_center\":{\"x\":" << screen_center.x << ",\"y\":" << screen_center.y << "}"
            << ",\"crosshair_color\":{\"enabled\":" << cfg.enable_crosshair_color
            << ",\"found\":" << last_crosshair_found
            << ",\"x\":" << detected_crosshair.x
            << ",\"y\":" << detected_crosshair.y
            << ",\"offset_x\":" << last_crosshair_offset_x
            << ",\"offset_y\":" << last_crosshair_offset_y
            << ",\"area\":" << last_crosshair_area
            << ",\"score\":" << last_crosshair_score << "}"
            << ",\"target\":{\"x\":" << measured.x << ",\"y\":" << measured.y << "}"
            << ",\"aim_point\":{\"x\":" << aim_point.x << ",\"y\":" << aim_point.y << "}"
            << ",\"target_box\":{\"x\":" << selected_box.x << ",\"y\":" << selected_box.y
            << ",\"w\":" << selected_box.width << ",\"h\":" << selected_box.height << "}"
            << ",\"entity_box\":{\"x\":" << target.box.x << ",\"y\":" << target.box.y
            << ",\"w\":" << target.box.width << ",\"h\":" << target.box.height << "}"
            << ",\"measured_error\":{\"x\":" << measured_error_x << ",\"y\":" << measured_error_y
            << ",\"dist\":" << std::hypot(measured_error_x, measured_error_y) << "}"
            << ",\"aim_error\":{\"x\":" << aim_error_x << ",\"y\":" << aim_error_y
            << ",\"dist\":" << std::hypot(aim_error_x, aim_error_y) << "}"
            << ",\"raw_move\":{\"x\":" << last_raw_move_x << ",\"y\":" << last_raw_move_y << "}"
            << ",\"move\":{\"x\":" << last_move_x << ",\"y\":" << last_move_y << "}"
            << ",\"gate\":\"" << JsonEscape(last_gate_state) << "\""
            << ",\"aim_point_source\":\"" << JsonEscape(last_aim_point_source) << "\""
            << ",\"detections\":" << last_detection_count
            << ",\"targetable_entities\":" << last_targetable_detection_count
            << ",\"timing_ms\":{\"total\":" << timings.total_loop_ms
            << ",\"capture\":" << timings.capture_ms
            << ",\"preprocess\":" << timings.preprocess_ms
            << ",\"inference\":" << timings.inference_ms
            << ",\"postprocess\":" << timings.postprocess_ms
            << ",\"targeting\":" << timings.targeting_ms
            << ",\"input\":" << timings.input_ms << "}}\n";
        ++aim_sample_lines_since_flush;
        flushAimSampleLogIfNeeded(true);
    }
