// Console telemetry and visualization window drawing

    void updateAndPrintStats() {
        const double smoothing_factor = 0.05;
        if (is_first_frame) {
            smoothed_total = timings.total_loop_ms;
            smoothed_capture = timings.capture_ms;
            smoothed_pre = timings.preprocess_ms;
            smoothed_inf = timings.inference_ms;
            smoothed_post = timings.postprocess_ms;
            smoothed_targeting = timings.targeting_ms;
            smoothed_input = timings.input_ms;
            smoothed_visualization = timings.visualization_ms;
            is_first_frame = false;
        }
        else {
            smoothed_total = smoothing_factor * timings.total_loop_ms + (1.0 - smoothing_factor) * smoothed_total;
            smoothed_capture = smoothing_factor * timings.capture_ms + (1.0 - smoothing_factor) * smoothed_capture;
            smoothed_pre = smoothing_factor * timings.preprocess_ms + (1.0 - smoothing_factor) * smoothed_pre;
            smoothed_inf = smoothing_factor * timings.inference_ms + (1.0 - smoothing_factor) * smoothed_inf;
            smoothed_post = smoothing_factor * timings.postprocess_ms + (1.0 - smoothing_factor) * smoothed_post;
            smoothed_targeting = smoothing_factor * timings.targeting_ms + (1.0 - smoothing_factor) * smoothed_targeting;
            smoothed_input = smoothing_factor * timings.input_ms + (1.0 - smoothing_factor) * smoothed_input;
            smoothed_visualization = smoothing_factor * timings.visualization_ms + (1.0 - smoothing_factor) * smoothed_visualization;
        }
        ++frame_count_for_console;
        if (!cfg.enable_console_stats || smoothed_total <= 0) {
            return;
        }
        const auto stats_now = std::chrono::steady_clock::now();
        if (stats_print_initialized) {
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                stats_now - last_stats_print_time).count();
            if (elapsed_ms < 1000) {
                return;
            }
        }
        stats_print_initialized = true;
        last_stats_print_time = stats_now;
        {
            double smoothed_fps = 1000.0 / smoothed_total;
            std::cout << std::fixed << std::setprecision(1)
                << "\r[LIVE] FPS: " << std::setw(5) << smoothed_fps
                << " | Total Delay: " << std::setw(5) << smoothed_total << "ms"
                << " | Stages: Cap " << smoothed_capture
                << ", Pre " << smoothed_pre
                << ", Inf " << smoothed_inf
                << ", Post " << smoothed_post
                << ", Target " << smoothed_targeting
                << ", Input " << smoothed_input
                << ", Viz " << smoothed_visualization << "ms"
                << " | Dets: " << last_detection_count
                << " | Match: " << last_targetable_detection_count
                << " | Target: " << (last_target_available ? "yes" : "no")
                << " | Part: " << last_target_part
                << " | AimPoint: " << last_aim_point_source
                << " | Offset: " << last_target_offset_x << "," << last_target_offset_y
                << " | Gate: " << last_gate_state
                << " | Raw: " << last_raw_move_x << "," << last_raw_move_y
                << " | Move: " << last_move_x << "," << last_move_y
                << " | AutoClick: " << last_auto_click_state
                << " | AutoStop: " << last_auto_stop_state
                << " | Slide: " << (cfg.enable_humanized_movement ? "human" : "direct")
                << "        " << std::flush;
        }
    }

    void handleVisualization(const TargetLock& best_target) {
        cv::circle(captured_frame, active_crosshair, static_cast<int>(cfg.max_lock_distance_pixels), cv::Scalar(255, 255, 0), 1);
        cv::drawMarker(captured_frame, crop_center, cv::Scalar(120, 120, 120), cv::MARKER_CROSS, 12, 1);
        const cv::Scalar crosshair_color = cfg.enable_crosshair_color && last_crosshair_found
            ? cv::Scalar(0, 255, 255)
            : cv::Scalar(180, 180, 180);
        cv::drawMarker(captured_frame, active_crosshair, crosshair_color, cv::MARKER_TILTED_CROSS, 16, 2);
        if (cfg.enable_crosshair_color) {
            cv::circle(captured_frame, detected_crosshair, 3, cv::Scalar(255, 255, 255), 1);
            cv::line(captured_frame, crop_center, active_crosshair, cv::Scalar(90, 90, 90), 1);
        }
        for (const auto& det : detections) {
            const bool targetable = DetectionMatchesTargetFilters(det.class_id, cfg);
            if (!targetable) {
                continue;
            }
            const bool selected_part = best_target.valid && (&det == best_target.selected);
            const bool same_entity = best_target.valid && (&det == best_target.head || &det == best_target.body);
            cv::Scalar color = selected_part
                ? cv::Scalar(0, 0, 255)
                : (same_entity ? cv::Scalar(0, 165, 255) : cv::Scalar(0, 255, 0));
            cv::rectangle(captured_frame, det.box, color, 2);
            std::string label = ClassDisplayName(det.class_id, cfg) + " " + cv::format("%.2f", det.confidence);
            cv::putText(captured_frame, label, cv::Point(det.box.x, det.box.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
        if (smoothed_total > 0) {
            double smoothed_fps = 1000.0 / smoothed_total;
            std::ostringstream stats_stream;
            stats_stream << std::fixed << std::setprecision(1) << "FPS: " << smoothed_fps;
            cv::putText(captured_frame, stats_stream.str(), cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 255), 2);
        }
        cv::imshow(cfg.window_name, captured_frame);
    }

    void toggleVisualization() {
        is_visualizing = !is_visualizing;
        if (is_visualizing) {
            cv::namedWindow(cfg.window_name, cv::WINDOW_AUTOSIZE);
            std::cout << "\n[INFO] Visualization toggled ON." << std::endl;
        }
        else {
            cv::destroyAllWindows();
            std::cout << "\n[INFO] Visualization toggled OFF. Console stats will continue." << std::endl;
        }
    }

