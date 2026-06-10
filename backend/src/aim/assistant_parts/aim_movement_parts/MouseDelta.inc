    int mouseDeltaLimit(int override_limit = 0) const {
        return std::max(1, override_limit > 0 ? override_limit : cfg.max_move_pixels);
    }

    int clampMouseDelta(int value, int override_limit = 0) const {
        if (!cfg.bounded_movement) {
            return value;
        }
        const int limit = mouseDeltaLimit(override_limit);
        return std::clamp(value, -limit, limit);
    }

    int toMouseDelta(double value, int override_limit = 0) const {
        return clampMouseDelta(static_cast<int>(std::lround(value)), override_limit);
    }

    double clampMouseDeltaDouble(double value, int override_limit = 0) const {
        if (!cfg.bounded_movement) {
            return value;
        }
        const double limit = static_cast<double>(mouseDeltaLimit(override_limit));
        return std::clamp(value, -limit, limit);
    }

    int takeMouseDelta(double value, double& carry, int override_limit = 0) const {
        carry += clampMouseDeltaDouble(value, override_limit);
        int move = static_cast<int>(carry);
        if (move == 0) {
            return 0;
        }
        move = clampMouseDelta(move, override_limit);
        carry -= static_cast<double>(move);
        if (std::abs(carry) > static_cast<double>(mouseDeltaLimit(override_limit))) {
            carry = 0.0;
        }
        return move;
    }

    std::pair<double, double> limitSmoothSlideVector(std::pair<double, double> raw) const {
        if (cfg.enable_instant_snap) {
            return raw;
        }

        const double len = std::hypot(raw.first, raw.second);
        const double max_step = static_cast<double>(std::clamp(cfg.smooth_slide_max_step, 1, 300));
        if (len <= max_step || len < 0.001) {
            return raw;
        }

        const double scale = max_step / len;
        return { raw.first * scale, raw.second * scale };
    }
