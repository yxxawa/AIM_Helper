    bool detectorConfigChanged(const Config& old_cfg, const Config& next) const {
        return old_cfg.model_path != next.model_path ||
            old_cfg.backend != next.backend ||
            old_cfg.trt_cache_path != next.trt_cache_path ||
            old_cfg.use_end_to_end_onnx != next.use_end_to_end_onnx;
    }

    bool captureConfigChanged(const Config& old_cfg, const Config& next) const {
        return old_cfg.crop_size != next.crop_size ||
            old_cfg.enable_async_capture != next.enable_async_capture;
    }

    bool mouseBackendConfigChanged(const Config& old_cfg, const Config& next) const {
        return old_cfg.input_backend != next.input_backend ||
            old_cfg.driver_dll_path != next.driver_dll_path ||
            old_cfg.driver_type != next.driver_type;
    }

    bool aimMathConfigChanged(const Config& old_cfg, const Config& next) const {
        return old_cfg.crop_size != next.crop_size ||
            old_cfg.max_move_pixels != next.max_move_pixels ||
            old_cfg.enable_instant_snap != next.enable_instant_snap ||
            old_cfg.smooth_slide_max_step != next.smooth_slide_max_step ||
            old_cfg.bounded_movement != next.bounded_movement ||
            old_cfg.enable_anti_snap != next.enable_anti_snap ||
            old_cfg.anti_snap_max_delta != next.anti_snap_max_delta ||
            old_cfg.enable_fallen_target_filter != next.enable_fallen_target_filter ||
            old_cfg.enable_small_lock_only != next.enable_small_lock_only ||
            old_cfg.small_lock_radius_pixels != next.small_lock_radius_pixels ||
            old_cfg.aim_mode != next.aim_mode ||
            old_cfg.aim_filter_mode != next.aim_filter_mode ||
            old_cfg.aim_gain != next.aim_gain ||
            old_cfg.aim_smoothing != next.aim_smoothing ||
            old_cfg.aim_deadzone_pixels != next.aim_deadzone_pixels ||
            old_cfg.target_x_ratio != next.target_x_ratio ||
            old_cfg.target_y_ratio != next.target_y_ratio ||
            old_cfg.auto_target_part != next.auto_target_part ||
            old_cfg.target_entity_priority != next.target_entity_priority ||
            old_cfg.aim_part_priority != next.aim_part_priority ||
            old_cfg.enemy_camp != next.enemy_camp ||
            old_cfg.detection_part != next.detection_part ||
            old_cfg.target_class_ids != next.target_class_ids ||
            old_cfg.model_class_names != next.model_class_names ||
            old_cfg.model_class_roles != next.model_class_roles ||
            old_cfg.model_class_camps != next.model_class_camps ||
            old_cfg.pid_kp != next.pid_kp ||
            old_cfg.pid_ki != next.pid_ki ||
            old_cfg.pid_kd != next.pid_kd ||
            old_cfg.pid_integral_limit != next.pid_integral_limit ||
            old_cfg.one_euro_min_cutoff != next.one_euro_min_cutoff ||
            old_cfg.one_euro_beta != next.one_euro_beta ||
            old_cfg.one_euro_d_cutoff != next.one_euro_d_cutoff ||
            old_cfg.enable_crosshair_color != next.enable_crosshair_color ||
            old_cfg.crosshair_color_r != next.crosshair_color_r ||
            old_cfg.crosshair_color_g != next.crosshair_color_g ||
            old_cfg.crosshair_color_b != next.crosshair_color_b ||
            old_cfg.crosshair_color_tolerance != next.crosshair_color_tolerance ||
            old_cfg.crosshair_min_area != next.crosshair_min_area ||
            old_cfg.crosshair_max_area != next.crosshair_max_area ||
            old_cfg.crosshair_smoothing != next.crosshair_smoothing ||
            old_cfg.enable_humanized_movement != next.enable_humanized_movement ||
            old_cfg.human_move_max_step != next.human_move_max_step ||
            old_cfg.human_move_jitter != next.human_move_jitter ||
            old_cfg.human_move_delay_min_ms != next.human_move_delay_min_ms ||
            old_cfg.human_move_delay_max_ms != next.human_move_delay_max_ms;
    }

    bool autoClickConfigChanged(const Config& old_cfg, const Config& next) const {
        return old_cfg.enable_auto_click != next.enable_auto_click ||
            old_cfg.auto_click_hold_mode != next.auto_click_hold_mode ||
            old_cfg.auto_click_delay_min_ms != next.auto_click_delay_min_ms ||
            old_cfg.auto_click_delay_max_ms != next.auto_click_delay_max_ms ||
            old_cfg.auto_click_hold_delay_min_ms != next.auto_click_hold_delay_min_ms ||
            old_cfg.auto_click_hold_delay_max_ms != next.auto_click_hold_delay_max_ms ||
            old_cfg.auto_click_interval_min_ms != next.auto_click_interval_min_ms ||
            old_cfg.auto_click_interval_max_ms != next.auto_click_interval_max_ms ||
            old_cfg.auto_click_tolerance_pixels != next.auto_click_tolerance_pixels ||
            old_cfg.enable_auto_stop != next.enable_auto_stop ||
            old_cfg.auto_stop_mode != next.auto_stop_mode ||
            old_cfg.auto_stop_hold_ms != next.auto_stop_hold_ms ||
            old_cfg.auto_stop_settle_ms != next.auto_stop_settle_ms;
    }
