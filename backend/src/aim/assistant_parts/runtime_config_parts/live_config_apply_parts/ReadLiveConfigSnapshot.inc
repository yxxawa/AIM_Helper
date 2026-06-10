    Config readLiveConfigSnapshot(const std::unordered_map<std::string, std::string>& values, const Config& base_cfg) const {
        Config next = base_cfg;

        const std::string model_path = ReadLiveRawString(values, "model_path", NarrowAscii(next.model_path));
        if (!model_path.empty()) {
            next.model_path = WidenUtf8(model_path);
        }

        const std::string inference_backend = ReadLiveString(values, "backend", next.backend);
        if (IsValidInferenceBackend(inference_backend)) {
            next.backend = inference_backend;
        }
        const std::string trt_cache_path = ReadLiveRawString(values, "trt_cache_path", next.trt_cache_path);
        if (!trt_cache_path.empty()) {
            next.trt_cache_path = trt_cache_path;
        }

        const std::string input_backend = ReadLiveString(values, "input_backend", next.input_backend);
        if (IsValidInputBackend(input_backend)) {
            next.input_backend = input_backend;
        }
        const std::string driver_dll_path = ReadLiveRawString(values, "driver_dll_path", NarrowAscii(next.driver_dll_path));
        next.driver_dll_path = WidenUtf8(driver_dll_path);
        next.driver_type = std::clamp(ReadLiveInt(values, "driver_type", next.driver_type), 0, 3);

        next.crop_size = std::clamp(ReadLiveInt(values, "crop_size", next.crop_size), 160, 960);
        next.max_lock_distance_pixels = std::clamp(ReadLiveInt(values, "lock_radius", next.max_lock_distance_pixels), 10, 500);
        next.confidence_threshold = static_cast<float>(std::clamp(ReadLiveDouble(values, "confidence", next.confidence_threshold), 0.01, 0.99));
        next.aim_smoothing = std::clamp(ReadLiveDouble(values, "smoothing", next.aim_smoothing), 0.01, 5.0);
        next.smooth_aim_key = std::clamp(ReadLiveInt(values, "aim_key", next.smooth_aim_key), 0, 255);
        next.smooth_aim_secondary_key = std::clamp(ReadLiveInt(values, "aim_key2", next.smooth_aim_secondary_key), 0, 255);
        next.aim_gain = std::clamp(ReadLiveDouble(values, "aim_gain", next.aim_gain), 0.01, 5.0);
        next.aim_deadzone_pixels = std::clamp(ReadLiveDouble(values, "deadzone", next.aim_deadzone_pixels), 0.0, 30.0);
        next.target_x_ratio = std::clamp(ReadLiveDouble(values, "target_x", next.target_x_ratio), 0.0, 1.0);
        next.target_y_ratio = std::clamp(ReadLiveDouble(values, "target_y", next.target_y_ratio), 0.0, 1.0);
        next.auto_target_part = ReadLiveBool(values, "auto_target_part", next.auto_target_part);
        next.target_entity_priority = NormalizeTargetEntityPriority(ReadLiveString(values, "target_entity_priority", next.target_entity_priority));
        next.aim_part_priority = NormalizeAimPartPriority(ReadLiveString(values, "aim_part_priority", next.aim_part_priority));

        const std::string enemy_camp = NormalizeEnemyCamp(ReadLiveRawString(values, "enemy_camp", next.enemy_camp));
        if (IsValidEnemyCamp(enemy_camp)) {
            next.enemy_camp = enemy_camp;
        }
        const std::string detection_part = NormalizeDetectionPart(ReadLiveString(values, "detection_part", next.detection_part));
        if (IsValidDetectionPart(detection_part)) {
            next.detection_part = detection_part;
        }
        if (next.detection_part == "body") {
            next.auto_target_part = false;
        }
        next.target_class_ids = ReadIntList(ReadLiveRawString(values, "target_class_ids", TargetClassWhitelistSummary(next.target_class_ids)));
        next.model_class_names = ReadIntStringMap(ReadLiveRawString(values, "model_class_names", IntStringMapSummary(next.model_class_names)));
        next.model_class_roles = ReadIntStringMap(ReadLiveRawString(values, "model_class_roles", IntStringMapSummary(next.model_class_roles)));
        next.model_class_camps = ReadIntStringMap(ReadLiveRawString(values, "model_class_camps", IntStringMapSummary(next.model_class_camps)));

        next.max_move_pixels = std::clamp(ReadLiveInt(values, "max_move", next.max_move_pixels), 1, 500);
        next.enable_instant_snap = ReadLiveBool(values, "instant_snap_enabled", next.enable_instant_snap);
        next.smooth_slide_max_step = std::clamp(ReadLiveInt(values, "smooth_slide_max_step", next.smooth_slide_max_step), 1, 300);
        next.enable_humanized_movement = ReadLiveBool(values, "human_slide_enabled", next.enable_humanized_movement);
        next.human_move_max_step = std::clamp(ReadLiveDouble(values, "human_slide_max_step", next.human_move_max_step), 1.0, 500.0);
        next.human_move_jitter = std::clamp(ReadLiveDouble(values, "human_slide_jitter", next.human_move_jitter), 0.0, 20.0);
        next.human_move_delay_min_ms = std::clamp(ReadLiveInt(values, "human_slide_delay_min", next.human_move_delay_min_ms), 0, 100);
        next.human_move_delay_max_ms = std::clamp(ReadLiveInt(values, "human_slide_delay_max", next.human_move_delay_max_ms), 0, 100);
        if (next.human_move_delay_min_ms > next.human_move_delay_max_ms) {
            std::swap(next.human_move_delay_min_ms, next.human_move_delay_max_ms);
        }

        next.enable_auto_click = ReadLiveBool(values, "auto_click_enabled", next.enable_auto_click);
        next.auto_click_hold_mode = ReadLiveBool(values, "auto_click_hold_mode", next.auto_click_hold_mode);
        next.auto_click_delay_min_ms = std::clamp(ReadLiveInt(values, "auto_click_delay_min", next.auto_click_delay_min_ms), 0, 5000);
        next.auto_click_delay_max_ms = std::clamp(ReadLiveInt(values, "auto_click_delay_max", next.auto_click_delay_max_ms), 0, 5000);
        if (next.auto_click_delay_min_ms > next.auto_click_delay_max_ms) {
            std::swap(next.auto_click_delay_min_ms, next.auto_click_delay_max_ms);
        }
        next.auto_click_hold_delay_min_ms = std::clamp(ReadLiveInt(values, "auto_click_hold_delay_min", next.auto_click_hold_delay_min_ms), 0, 5000);
        next.auto_click_hold_delay_max_ms = std::clamp(ReadLiveInt(values, "auto_click_hold_delay_max", next.auto_click_hold_delay_max_ms), 0, 5000);
        if (next.auto_click_hold_delay_min_ms > next.auto_click_hold_delay_max_ms) {
            std::swap(next.auto_click_hold_delay_min_ms, next.auto_click_hold_delay_max_ms);
        }
        next.auto_click_interval_min_ms = std::clamp(ReadLiveInt(values, "auto_click_interval_min", next.auto_click_interval_min_ms), 0, 5000);
        next.auto_click_interval_max_ms = std::clamp(ReadLiveInt(values, "auto_click_interval_max", next.auto_click_interval_max_ms), 0, 5000);
        if (next.auto_click_interval_min_ms > next.auto_click_interval_max_ms) {
            std::swap(next.auto_click_interval_min_ms, next.auto_click_interval_max_ms);
        }
        next.auto_click_tolerance_pixels = std::clamp(ReadLiveDouble(values, "auto_click_tolerance", next.auto_click_tolerance_pixels), 0.0, 50.0);
        next.enable_auto_stop = ReadLiveBool(values, "auto_stop_enabled", next.enable_auto_stop);
        const std::string auto_stop_mode = NormalizeAutoStopMode(ReadLiveString(values, "auto_stop_mode", next.auto_stop_mode));
        if (IsValidAutoStopMode(auto_stop_mode)) {
            next.auto_stop_mode = auto_stop_mode;
        }
        next.auto_stop_hold_ms = std::clamp(ReadLiveInt(values, "auto_stop_hold_ms", next.auto_stop_hold_ms), 0, 250);
        next.auto_stop_settle_ms = std::clamp(ReadLiveInt(values, "auto_stop_settle_ms", next.auto_stop_settle_ms), 0, 250);

        next.enable_console_stats = ReadLiveBool(values, "console_stats_enabled", next.enable_console_stats);
        next.enable_capture = ReadLiveBool(values, "enable_capture", next.enable_capture);
        next.enable_async_capture = ReadLiveBool(values, "async_capture_enabled", next.enable_async_capture);
        next.enable_mouse_movement = ReadLiveBool(values, "enable_mouse_move", next.enable_mouse_movement);
        next.enable_hold_to_aim = ReadLiveBool(values, "enable_hold_to_aim", next.enable_hold_to_aim);
        next.bounded_movement = ReadLiveBool(values, "bounded_movement", next.bounded_movement);
        next.enable_anti_snap = ReadLiveBool(values, "anti_snap_enabled", next.enable_anti_snap);
        next.anti_snap_max_delta = std::clamp(ReadLiveInt(values, "anti_snap_max_delta", next.anti_snap_max_delta), 1, 500);
        next.enable_fallen_target_filter = ReadLiveBool(values, "fallen_target_filter_enabled", next.enable_fallen_target_filter);
        next.enable_small_lock_only = ReadLiveBool(values, "small_lock_only_enabled", next.enable_small_lock_only);
        next.small_lock_radius_pixels = std::clamp(ReadLiveInt(values, "small_lock_radius", next.small_lock_radius_pixels), 1, 300);

        const std::string aim_mode = ReadLiveString(values, "aim_mode", next.aim_mode);
        if (IsValidAimMode(aim_mode)) {
            next.aim_mode = aim_mode;
        }
        const std::string filter_mode = ReadLiveString(values, "aim_filter", next.aim_filter_mode);
        if (IsValidFilterMode(filter_mode)) {
            next.aim_filter_mode = filter_mode;
        }
        next.pid_kp = std::clamp(ReadLiveDouble(values, "pid_kp", next.pid_kp), 0.0, 10.0);
        next.pid_ki = std::clamp(ReadLiveDouble(values, "pid_ki", next.pid_ki), 0.0, 10.0);
        next.pid_kd = std::clamp(ReadLiveDouble(values, "pid_kd", next.pid_kd), 0.0, 10.0);
        next.pid_integral_limit = std::clamp(ReadLiveDouble(values, "pid_i_limit", next.pid_integral_limit), 0.0, 1000.0);
        next.one_euro_min_cutoff = std::clamp(ReadLiveDouble(values, "one_euro_min_cutoff", next.one_euro_min_cutoff), 0.01, 100.0);
        next.one_euro_beta = std::clamp(ReadLiveDouble(values, "one_euro_beta", next.one_euro_beta), 0.0, 100.0);
        next.one_euro_d_cutoff = std::clamp(ReadLiveDouble(values, "one_euro_d_cutoff", next.one_euro_d_cutoff), 0.01, 100.0);

        next.enable_crosshair_color = ReadLiveBool(values, "crosshair_color_enabled", next.enable_crosshair_color);
        next.crosshair_color_r = std::clamp(ReadLiveInt(values, "crosshair_color_r", next.crosshair_color_r), 0, 255);
        next.crosshair_color_g = std::clamp(ReadLiveInt(values, "crosshair_color_g", next.crosshair_color_g), 0, 255);
        next.crosshair_color_b = std::clamp(ReadLiveInt(values, "crosshair_color_b", next.crosshair_color_b), 0, 255);
        next.crosshair_color_tolerance = std::clamp(ReadLiveInt(values, "crosshair_color_tolerance", next.crosshair_color_tolerance), 0, 255);
        next.crosshair_min_area = std::clamp(ReadLiveInt(values, "crosshair_min_area", next.crosshair_min_area), 1, 10000);
        next.crosshair_max_area = std::clamp(ReadLiveInt(values, "crosshair_max_area", next.crosshair_max_area), 1, 200000);
        if (next.crosshair_min_area > next.crosshair_max_area) {
            std::swap(next.crosshair_min_area, next.crosshair_max_area);
        }
        next.crosshair_smoothing = std::clamp(ReadLiveDouble(values, "crosshair_smoothing", next.crosshair_smoothing), 0.0, 1.0);
        next.enable_visualization = ReadLiveBool(values, "enable_visualization", next.enable_visualization);
        next.yolo_fps_limit = std::clamp(ReadLiveInt(values, "yolo_fps_limit", next.yolo_fps_limit), 0, 300);

        if (next.aim_filter_mode == "pid_oneeuro") {
            next.aim_filter_mode = "pid";
        }

        return next;
    }
