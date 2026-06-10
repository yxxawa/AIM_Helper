    void commitLiveConfig(const Config& old_cfg, Config&& next) {
        const bool aim_math_changed = aimMathConfigChanged(old_cfg, next);
        const bool auto_click_changed = autoClickConfigChanged(old_cfg, next);

        cfg = std::move(next);
        setVisualizationEnabled(cfg.enable_visualization);
        if (aim_math_changed) {
            resetAimCarry();
            resetAimFilters();
            resetTargetLock();
            resetCrosshairTracking();
        }
        if (auto_click_changed) {
            resetAutoClick("config");
        }
    }

    void logLiveConfigApplied() const {
        std::cout << "\n[CONFIG] Live settings applied: backend=" << cfg.backend
            << ", input=" << cfg.input_backend
            << ", crop=" << cfg.crop_size
            << ", lock_radius=" << cfg.max_lock_distance_pixels
            << ", confidence=" << cfg.confidence_threshold
            << ", screen_capture=" << (cfg.enable_capture ? "on" : "off")
            << ", capture_mode=" << (cfg.enable_async_capture ? "latest" : "direct")
            << ", yolo_fps_limit=" << cfg.yolo_fps_limit
            << ", aim=" << cfg.aim_mode
            << ", filter=" << cfg.aim_filter_mode
            << ", crosshair_color=" << (cfg.enable_crosshair_color ? "on" : "off")
            << "(rgb=" << cfg.crosshair_color_r << "," << cfg.crosshair_color_g << "," << cfg.crosshair_color_b
            << ",tol=" << cfg.crosshair_color_tolerance
            << ",area=" << cfg.crosshair_min_area << "-" << cfg.crosshair_max_area
            << ",smooth=" << cfg.crosshair_smoothing << ")"
            << ", smooth_slide=" << (cfg.enable_instant_snap ? "instant" : "smooth")
            << "(" << cfg.smooth_slide_max_step << "px)"
            << ", human_slide=" << (cfg.enable_humanized_movement ? "on" : "off")
            << "(" << cfg.human_move_max_step << "," << cfg.human_move_jitter
            << "," << cfg.human_move_delay_min_ms << "-" << cfg.human_move_delay_max_ms << "ms)"
            << ", auto_click=" << (cfg.enable_auto_click ? "on" : "off")
            << "(mode=" << (cfg.auto_click_hold_mode ? "hold" : "single")
            << ", delay=" << cfg.auto_click_delay_min_ms << "-" << cfg.auto_click_delay_max_ms
            << "ms, hold_delay=" << cfg.auto_click_hold_delay_min_ms << "-" << cfg.auto_click_hold_delay_max_ms
            << "ms, interval=" << cfg.auto_click_interval_min_ms << "-" << cfg.auto_click_interval_max_ms
            << "ms, tol=" << cfg.auto_click_tolerance_pixels
            << "px, auto_stop=" << (cfg.enable_auto_stop ? cfg.auto_stop_mode : "off")
            << ", stop_hold=" << cfg.auto_stop_hold_ms
            << "ms, stop_settle=" << cfg.auto_stop_settle_ms << "ms)"
            << ", target_mode=" << (cfg.auto_target_part ? "auto" : "custom")
            << ", target_priority=" << cfg.target_entity_priority
            << ", part_priority=" << cfg.aim_part_priority
            << ", target=" << cfg.target_x_ratio << "," << cfg.target_y_ratio
            << ", camp=" << cfg.enemy_camp
            << ", target_classes=" << EnemyCampTargetSummary(cfg.enemy_camp)
            << ", detection_part=" << DetectionPartSummary(cfg.detection_part)
            << ", class_whitelist=" << TargetClassWhitelistSummary(cfg.target_class_ids)
            << ", class_roles=" << IntStringMapSummary(cfg.model_class_roles)
            << ", class_camps=" << IntStringMapSummary(cfg.model_class_camps)
            << ", anti_snap=" << (cfg.enable_anti_snap ? "on" : "off")
            << "(" << cfg.anti_snap_max_delta << "px)"
            << ", fallen_filter=" << (cfg.enable_fallen_target_filter ? "on" : "off")
            << ", small_lock_only=" << (cfg.enable_small_lock_only ? "on" : "off")
            << "(" << cfg.small_lock_radius_pixels << "px)"
            << ", hotkeys=" << KeyNameFromVk(cfg.smooth_aim_key) << "/" << KeyNameFromVk(cfg.smooth_aim_secondary_key)
            << ", visualization=" << (cfg.enable_visualization ? "on" : "off") << std::endl;
    }
