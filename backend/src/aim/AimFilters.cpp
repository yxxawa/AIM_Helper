struct PidAxis {
    double integral = 0.0;
    double previous_error = 0.0;
    bool has_previous = false;

    void Reset() {
        integral = 0.0;
        previous_error = 0.0;
        has_previous = false;
    }

    double Update(double error, double dt, const Config& cfg) {
        dt = std::clamp(dt, 0.001, 0.1);
        integral += error * dt;
        if (cfg.pid_integral_limit > 0.0) {
            integral = std::clamp(integral, -cfg.pid_integral_limit, cfg.pid_integral_limit);
        }
        else {
            integral = 0.0;
        }

        const double derivative = has_previous ? (error - previous_error) / dt : 0.0;
        previous_error = error;
        has_previous = true;
        return cfg.pid_kp * error + cfg.pid_ki * integral + cfg.pid_kd * derivative;
    }
};

struct LowPassAxis {
    bool initialized = false;
    double value = 0.0;

    void Reset() {
        initialized = false;
        value = 0.0;
    }

    double Update(double input, double alpha) {
        alpha = std::clamp(alpha, 0.0, 1.0);
        if (!initialized) {
            value = input;
            initialized = true;
            return value;
        }
        value = alpha * input + (1.0 - alpha) * value;
        return value;
    }
};

struct OneEuroAxis {
    LowPassAxis signal;
    LowPassAxis derivative;
    double previous_raw = 0.0;
    bool has_previous_raw = false;

    void Reset() {
        signal.Reset();
        derivative.Reset();
        previous_raw = 0.0;
        has_previous_raw = false;
    }

    static double Alpha(double cutoff, double dt) {
        constexpr double pi = 3.14159265358979323846;
        cutoff = std::max(0.001, cutoff);
        dt = std::clamp(dt, 0.001, 0.1);
        const double tau = 1.0 / (2.0 * pi * cutoff);
        return 1.0 / (1.0 + tau / dt);
    }

    double Update(double input, double dt, const Config& cfg) {
        dt = std::clamp(dt, 0.001, 0.1);
        const double raw_derivative = has_previous_raw ? (input - previous_raw) / dt : 0.0;
        previous_raw = input;
        has_previous_raw = true;

        const double smoothed_derivative = derivative.Update(raw_derivative, Alpha(cfg.one_euro_d_cutoff, dt));
        const double cutoff = cfg.one_euro_min_cutoff + cfg.one_euro_beta * std::abs(smoothed_derivative);
        return signal.Update(input, Alpha(cutoff, dt));
    }
};

