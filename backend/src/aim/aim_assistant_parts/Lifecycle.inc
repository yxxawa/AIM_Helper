    explicit AimAssistant(Config runtime_config)
        : cfg(std::move(runtime_config)),
        frame_source(CreateFrameSource(cfg)),
        detector(std::make_unique<ObjectDetector>(cfg)),
        mouse(std::make_unique<MouseController>(cfg)),
        is_visualizing(cfg.enable_visualization)
    {
        if (frame_source->getWidth() < cfg.crop_size || frame_source->getHeight() < cfg.crop_size) {
            throw std::runtime_error("Screen resolution is smaller than configured crop_size.");
        }
        updateCropGeometry(cfg.crop_size);
        if (is_visualizing) {
            cv::namedWindow(cfg.window_name, cv::WINDOW_AUTOSIZE);
        }
        if (!cfg.live_config_path.empty()) {
            std::wcout << L"--- Live config enabled: " << cfg.live_config_path << L" ---" << std::endl;
        }
        std::cout << "--- Detection will run on a centered " << cfg.crop_size << "x" << cfg.crop_size
            << " region using " << frame_source->ModeName() << " capture. ---" << std::endl;
        std::cout << "--- YOLO FPS limit: " << (cfg.yolo_fps_limit > 0 ? std::to_string(cfg.yolo_fps_limit) : std::string("unlimited")) << " ---" << std::endl;
        std::cout << "--- Switches: screen_capture=" << (cfg.enable_capture ? "on" : "off")
            << ", capture_mode=" << (cfg.enable_async_capture ? "latest" : "direct")
            << ", mouse_move=" << (cfg.enable_mouse_movement ? "on" : "off")
            << ", hold_to_aim=" << (cfg.enable_hold_to_aim ? "on" : "off")
            << ", visualization=" << (cfg.enable_visualization ? "on" : "off")
            << ", bounded=" << (cfg.bounded_movement ? "on" : "off")
            << ", anti_snap=" << (cfg.enable_anti_snap ? "on" : "off")
            << ", small_lock_only=" << (cfg.enable_small_lock_only ? "on" : "off") << " ---" << std::endl;
        std::cout << "--- Aim mapping: mode=" << cfg.aim_mode
            << ", gain=" << cfg.aim_gain
            << ", smoothing=" << cfg.aim_smoothing
            << ", deadzone=" << cfg.aim_deadzone_pixels
            << ", target_mode=" << (cfg.auto_target_part ? "auto" : "custom")
            << ", part_priority=" << cfg.aim_part_priority
            << ", custom_target=" << cfg.target_x_ratio << "," << cfg.target_y_ratio << " ---" << std::endl;
        std::cout << "--- Team filter: camp=" << cfg.enemy_camp
            << ", target_classes=" << EnemyCampTargetSummary(cfg.enemy_camp)
            << ", detection_part=" << DetectionPartSummary(cfg.detection_part)
            << ", class_whitelist=" << TargetClassWhitelistSummary(cfg.target_class_ids)
            << ", class_roles=" << IntStringMapSummary(cfg.model_class_roles)
            << ", class_camps=" << IntStringMapSummary(cfg.model_class_camps) << " ---" << std::endl;
        std::cout << "--- Aim filter: mode=" << cfg.aim_filter_mode
            << ", PID(kp=" << cfg.pid_kp << ", ki=" << cfg.pid_ki << ", kd=" << cfg.pid_kd
            << "), OneEuro(min_cutoff=" << cfg.one_euro_min_cutoff
            << ", beta=" << cfg.one_euro_beta
            << ", d_cutoff=" << cfg.one_euro_d_cutoff << ") ---" << std::endl;
        std::cout << "--- Crosshair color: " << (cfg.enable_crosshair_color ? "on" : "off")
            << ", rgb=(" << cfg.crosshair_color_r << "," << cfg.crosshair_color_g << "," << cfg.crosshair_color_b << ")"
            << ", tolerance=" << cfg.crosshair_color_tolerance
            << ", area=" << cfg.crosshair_min_area << "-" << cfg.crosshair_max_area
            << ", smoothing=" << cfg.crosshair_smoothing << " ---" << std::endl;
        std::cout << "--- Aim hotkeys: primary=" << KeyNameFromVk(cfg.smooth_aim_key)
            << ", secondary=" << KeyNameFromVk(cfg.smooth_aim_secondary_key) << " ---" << std::endl;
        std::cout << "--- Smooth slide: mode=" << (cfg.enable_instant_snap ? "instant" : "smooth")
            << ", max_step=" << cfg.smooth_slide_max_step << "px ---" << std::endl;
        std::cout << "--- Mouse slide: humanized=" << (cfg.enable_humanized_movement ? "on" : "off")
            << ", max_step=" << cfg.human_move_max_step
            << ", jitter=" << cfg.human_move_jitter
            << ", delay=" << cfg.human_move_delay_min_ms << "-" << cfg.human_move_delay_max_ms << "ms ---" << std::endl;
        std::cout << "--- Auto click: " << (cfg.enable_auto_click ? "on" : "off")
            << ", mode=" << (cfg.auto_click_hold_mode ? "hold" : "single")
            << ", delay=" << cfg.auto_click_delay_min_ms << "-" << cfg.auto_click_delay_max_ms << "ms"
            << ", hold_delay=" << cfg.auto_click_hold_delay_min_ms << "-" << cfg.auto_click_hold_delay_max_ms << "ms"
            << ", interval=" << cfg.auto_click_interval_min_ms << "-" << cfg.auto_click_interval_max_ms << "ms"
            << ", tolerance=" << cfg.auto_click_tolerance_pixels << "px"
            << ", auto_stop=" << (cfg.enable_auto_stop ? cfg.auto_stop_mode : "off")
            << ", stop_hold=" << cfg.auto_stop_hold_ms << "ms"
            << ", stop_settle=" << cfg.auto_stop_settle_ms << "ms ---" << std::endl;
        print_instructions();
    }

    ~AimAssistant() {
        releaseAutoClickHoldIfOwned();
        flushAimSampleLogIfNeeded(true);
        if (is_visualizing) cv::destroyAllWindows();
    }
