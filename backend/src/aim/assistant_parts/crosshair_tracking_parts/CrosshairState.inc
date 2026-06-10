    void resetCrosshairTracking() {
        active_crosshair = crop_center;
        detected_crosshair = crop_center;
        crosshair_tracking_initialized = false;
        last_crosshair_found = false;
        last_crosshair_area = 0;
        last_crosshair_score = 0.0;
        last_crosshair_offset_x = 0;
        last_crosshair_offset_y = 0;
        crosshair_missed_frames = 0;
        crosshair_rejected_frames = 0;
    }
