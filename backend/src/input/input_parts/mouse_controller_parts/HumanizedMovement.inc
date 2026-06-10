    void QueueHumanizedMove(int dx, int dy, const Config& movement_cfg) {
        {
            std::lock_guard<std::mutex> lock(movement_mutex);
            pending_move_x += dx;
            pending_move_y += dy;
            pending_movement_cfg = movement_cfg;
            pending_movement_generation = movement_generation.fetch_add(1, std::memory_order_relaxed) + 1;
        }
        movement_cv.notify_one();
    }

    void CancelHumanizedMove() {
        {
            std::lock_guard<std::mutex> lock(movement_mutex);
            pending_move_x = 0;
            pending_move_y = 0;
            pending_movement_generation = movement_generation.fetch_add(1, std::memory_order_relaxed) + 1;
        }
        movement_cv.notify_one();
    }

    void MovementWorkerLoop() {
        while (!movement_worker_stop.load()) {
            int dx = 0;
            int dy = 0;
            Config movement_cfg;
            uint64_t generation = 0;
            {
                std::unique_lock<std::mutex> lock(movement_mutex);
                movement_cv.wait(lock, [this]() {
                    return movement_worker_stop.load() || pending_move_x != 0 || pending_move_y != 0;
                });
                if (movement_worker_stop.load()) {
                    break;
                }
                dx = pending_move_x;
                dy = pending_move_y;
                pending_move_x = 0;
                pending_move_y = 0;
                movement_cfg = pending_movement_cfg;
                generation = pending_movement_generation;
            }
            MoveRelativeHumanizedBlocking(dx, dy, movement_cfg, generation);
        }
    }

    void MoveRelativeHumanizedBlocking(int dx, int dy, const Config& movement_cfg, uint64_t generation) {
        const double distance = std::hypot(static_cast<double>(dx), static_cast<double>(dy));
        const double max_step = std::max(1.0, movement_cfg.human_move_max_step);
        const int segments = std::clamp(static_cast<int>(std::ceil(distance / max_step)), 1, 120);
        const double jitter = std::max(0.0, movement_cfg.human_move_jitter);
        int delay_min = std::clamp(movement_cfg.human_move_delay_min_ms, 0, 100);
        int delay_max = std::clamp(movement_cfg.human_move_delay_max_ms, 0, 100);
        if (delay_min > delay_max) {
            std::swap(delay_min, delay_max);
        }

        std::uniform_real_distribution<double> jitter_dist(-jitter, jitter);
        std::uniform_int_distribution<int> delay_dist(delay_min, delay_max);
        double previous_x = 0.0;
        double previous_y = 0.0;
        int applied_x = 0;
        int applied_y = 0;

        for (int segment = 1; segment <= segments && !movement_worker_stop.load(); ++segment) {
            if (generation != movement_generation.load(std::memory_order_relaxed)) {
                break;
            }
            const double t = static_cast<double>(segment) / static_cast<double>(segments);
            const double eased_t = t * t * (3.0 - 2.0 * t);
            const double current_x = static_cast<double>(dx) * eased_t;
            const double current_y = static_cast<double>(dy) * eased_t;
            double step_x = current_x - previous_x;
            double step_y = current_y - previous_y;

            if (segment < segments && jitter > 0.0) {
                step_x += jitter_dist(rng);
                step_y += jitter_dist(rng);
            }

            int move_x = static_cast<int>(std::round(step_x));
            int move_y = static_cast<int>(std::round(step_y));
            if (segment == segments) {
                move_x = dx - applied_x;
                move_y = dy - applied_y;
            }

            if (move_x != 0 || move_y != 0) {
                std::lock_guard<std::mutex> lock(backend_mutex);
                backend->MoveMouse(move_x, move_y);
                applied_x += move_x;
                applied_y += move_y;
            }

            previous_x = current_x;
            previous_y = current_y;
            if (segment < segments && delay_max > 0) {
                std::unique_lock<std::mutex> lock(movement_mutex);
                movement_cv.wait_for(lock, std::chrono::milliseconds(delay_dist(rng)), [this, generation]() {
                    return movement_worker_stop.load() ||
                        generation != movement_generation.load(std::memory_order_relaxed);
                });
            }
        }
    }
