    void Run() {
        while (true) {
            auto loop_start_time = std::chrono::steady_clock::now();
            timings = TimingDetails{};
            pollLiveConfig();
            handleMouseBackendTestHotkey();
            if (!cfg.enable_capture || !frame_source) {
                detections.clear();
                last_detection_count = 0;
                last_targetable_detection_count = 0;
                last_target_available = false;
                last_gate_state = "capture_off";
                resetTargetLock();
                resetAutoClick("capture_off");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (!frame_source->CaptureFrame(captured_frame, timings.capture_ms)) {
                resetAutoClick("capture_fail");
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (detector) {
                detector->Detect(captured_frame, detections, cfg, timings);
            }
            else {
                detections.clear();
                timings.preprocess_ms = 0.0;
                timings.inference_ms = 0.0;
                timings.postprocess_ms = 0.0;
            }
            updateCrosshairTracking();

            auto targeting_start_time = std::chrono::steady_clock::now();
            const bool aim_allowed_for_lock = isAimAllowedThisFrame();
            TargetLock best_target = findBestTarget(aim_allowed_for_lock);
            last_detection_count = static_cast<int>(detections.size());
            last_target_available = best_target.valid;
            last_target_part = best_target.valid ? best_target.part : "none";
            last_move_x = 0;
            last_move_y = 0;
            last_raw_move_x = 0.0;
            last_raw_move_y = 0.0;
            last_gate_state = "idle";
            last_auto_click_state = cfg.enable_auto_click ? "idle" : "off";
            updateTargetOffset(best_target);
            auto targeting_end_time = std::chrono::steady_clock::now();
            timings.targeting_ms = std::chrono::duration<double, std::milli>(targeting_end_time - targeting_start_time).count();

            auto input_start_time = std::chrono::steady_clock::now();
            handleMouseInput(best_target);
            handleAutoClick(best_target);
            auto input_end_time = std::chrono::steady_clock::now();
            timings.input_ms = std::chrono::duration<double, std::milli>(input_end_time - input_start_time).count();
            auto core_loop_end_time = std::chrono::steady_clock::now();
            timings.total_loop_ms = std::chrono::duration<double, std::milli>(core_loop_end_time - loop_start_time).count();
            recordAimSampleFrame(best_target, aim_allowed_for_lock);

            bool should_exit = false;
            if (is_visualizing) {
                auto visualization_start_time = std::chrono::steady_clock::now();
                handleVisualization(best_target);
                char key = static_cast<char>(cv::waitKey(1));
                if (key == 27) {
                    std::cout << "\nESC pressed. Exiting..." << std::endl;
                    should_exit = true;
                }
                if (key == 'v' || key == 'V') {
                    toggleVisualization();
                }
                auto visualization_end_time = std::chrono::steady_clock::now();
                timings.visualization_ms = std::chrono::duration<double, std::milli>(visualization_end_time - visualization_start_time).count();
            }

            if (should_exit) {
                break;
            }

            if (cfg.yolo_fps_limit > 0) {
                const double target_frame_ms = 1000.0 / static_cast<double>(cfg.yolo_fps_limit);
                const auto before_limit_time = std::chrono::steady_clock::now();
                const double elapsed_ms = std::chrono::duration<double, std::milli>(before_limit_time - loop_start_time).count();
                if (elapsed_ms < target_frame_ms) {
                    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(target_frame_ms - elapsed_ms));
                }
            }
            updateAndPrintStats();
        }
    }
