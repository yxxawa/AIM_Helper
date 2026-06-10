    std::string aimSampleSettingsJson() const {
        std::ostringstream out;
        out << std::boolalpha << std::setprecision(10)
            << "\"backend\":\"" << JsonEscape(cfg.backend) << "\""
            << ",\"input_backend\":\"" << JsonEscape(cfg.input_backend) << "\""
            << ",\"driver_type\":" << cfg.driver_type
            << ",\"crop_size\":" << cfg.crop_size
            << ",\"lock_radius\":" << cfg.max_lock_distance_pixels
            << ",\"confidence\":" << cfg.confidence_threshold
            << ",\"aim_key\":" << cfg.smooth_aim_key
            << ",\"aim_key2\":" << cfg.smooth_aim_secondary_key
            << ",\"aim_mode\":\"" << JsonEscape(cfg.aim_mode) << "\""
            << ",\"aim_gain\":" << cfg.aim_gain
            << ",\"smoothing\":" << cfg.aim_smoothing
            << ",\"deadzone\":" << cfg.aim_deadzone_pixels
            << ",\"aim_filter\":\"" << JsonEscape(cfg.aim_filter_mode) << "\""
            << ",\"pid_kp\":" << cfg.pid_kp
            << ",\"pid_ki\":" << cfg.pid_ki
            << ",\"pid_kd\":" << cfg.pid_kd
            << ",\"pid_i_limit\":" << cfg.pid_integral_limit
            << ",\"one_euro_min_cutoff\":" << cfg.one_euro_min_cutoff
            << ",\"one_euro_beta\":" << cfg.one_euro_beta
            << ",\"one_euro_d_cutoff\":" << cfg.one_euro_d_cutoff
            << ",\"crosshair_color_enabled\":" << cfg.enable_crosshair_color
            << ",\"crosshair_color_r\":" << cfg.crosshair_color_r
            << ",\"crosshair_color_g\":" << cfg.crosshair_color_g
            << ",\"crosshair_color_b\":" << cfg.crosshair_color_b
            << ",\"crosshair_color_tolerance\":" << cfg.crosshair_color_tolerance
            << ",\"crosshair_min_area\":" << cfg.crosshair_min_area
            << ",\"crosshair_max_area\":" << cfg.crosshair_max_area
            << ",\"crosshair_smoothing\":" << cfg.crosshair_smoothing
            << ",\"auto_target_part\":" << cfg.auto_target_part
            << ",\"target_entity_priority\":\"" << JsonEscape(cfg.target_entity_priority) << "\""
            << ",\"aim_part_priority\":\"" << JsonEscape(cfg.aim_part_priority) << "\""
            << ",\"target_x\":" << cfg.target_x_ratio
            << ",\"target_y\":" << cfg.target_y_ratio
            << ",\"enemy_camp\":\"" << JsonEscape(cfg.enemy_camp) << "\""
            << ",\"detection_part\":\"" << JsonEscape(cfg.detection_part) << "\""
            << ",\"max_move\":" << cfg.max_move_pixels
            << ",\"instant_snap_enabled\":" << cfg.enable_instant_snap
            << ",\"smooth_slide_max_step\":" << cfg.smooth_slide_max_step
            << ",\"human_slide_enabled\":" << cfg.enable_humanized_movement
            << ",\"human_slide_max_step\":" << cfg.human_move_max_step
            << ",\"human_slide_jitter\":" << cfg.human_move_jitter
            << ",\"human_slide_delay_min\":" << cfg.human_move_delay_min_ms
            << ",\"human_slide_delay_max\":" << cfg.human_move_delay_max_ms
            << ",\"auto_click_enabled\":" << cfg.enable_auto_click
            << ",\"auto_click_hold_mode\":" << cfg.auto_click_hold_mode
            << ",\"auto_click_delay_min\":" << cfg.auto_click_delay_min_ms
            << ",\"auto_click_delay_max\":" << cfg.auto_click_delay_max_ms
            << ",\"auto_click_hold_delay_min\":" << cfg.auto_click_hold_delay_min_ms
            << ",\"auto_click_hold_delay_max\":" << cfg.auto_click_hold_delay_max_ms
            << ",\"auto_click_interval_min\":" << cfg.auto_click_interval_min_ms
            << ",\"auto_click_interval_max\":" << cfg.auto_click_interval_max_ms
            << ",\"auto_click_tolerance\":" << cfg.auto_click_tolerance_pixels
            << ",\"auto_stop_enabled\":" << cfg.enable_auto_stop
            << ",\"auto_stop_mode\":\"" << JsonEscape(cfg.auto_stop_mode) << "\""
            << ",\"auto_stop_hold_ms\":" << cfg.auto_stop_hold_ms
            << ",\"auto_stop_settle_ms\":" << cfg.auto_stop_settle_ms
            << ",\"enable_capture\":" << cfg.enable_capture
            << ",\"enable_async_capture\":" << cfg.enable_async_capture
            << ",\"enable_mouse_move\":" << cfg.enable_mouse_movement
            << ",\"enable_hold_to_aim\":" << cfg.enable_hold_to_aim
            << ",\"bounded_movement\":" << cfg.bounded_movement
            << ",\"anti_snap_enabled\":" << cfg.enable_anti_snap
            << ",\"anti_snap_max_delta\":" << cfg.anti_snap_max_delta
            << ",\"fallen_target_filter_enabled\":" << cfg.enable_fallen_target_filter
            << ",\"small_lock_only_enabled\":" << cfg.enable_small_lock_only
            << ",\"small_lock_radius\":" << cfg.small_lock_radius_pixels;
        return out.str();
    }
