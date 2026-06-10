    void resetAimCarry() {
        aim_carry_x = 0.0;
        aim_carry_y = 0.0;
        last_aim_output_raw_x = 0.0;
        last_aim_output_raw_y = 0.0;
        aim_output_initialized = false;
        resetVisualMoveCoalescer();
    }

    void resetVisualMoveCoalescer() {
        visual_move_accum_x = 0.0;
        visual_move_accum_y = 0.0;
        visual_move_emit_initialized = false;
    }

    void resetAimFilters() {
        pid_x.Reset();
        pid_y.Reset();
        euro_x.Reset();
        euro_y.Reset();
        filter_time_initialized = false;
    }
