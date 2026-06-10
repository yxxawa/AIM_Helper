    explicit MouseController(const Config& cfg) {
        if (cfg.input_backend == "driver") {
            try {
                backend = std::make_unique<DriverBackend>(cfg.driver_dll_path, cfg.driver_type);
            }
            catch (const std::exception& e) {
                std::cerr << "\n[WARN] DriverBackend unavailable: " << e.what()
                    << " Falling back to SendInputBackend." << std::endl;
            }
        }
        if (!backend) {
            backend = std::make_unique<SendInputBackend>();
        }
        movement_worker = std::thread(&MouseController::MovementWorkerLoop, this);
        std::cout << "--- Active mouse backend: " << backend->Name() << " ---" << std::endl;
    }

    ~MouseController() {
        movement_worker_stop.store(true);
        {
            std::lock_guard<std::mutex> lock(movement_mutex);
            pending_move_x = 0;
            pending_move_y = 0;
        }
        movement_cv.notify_one();
        if (movement_worker.joinable()) {
            movement_worker.join();
        }
        std::cout << "--- Mouse input resources cleaned up. ---" << std::endl;
    }

    MouseController(const MouseController&) = delete;
    MouseController& operator=(const MouseController&) = delete;

    void MoveRelative(int dx, int dy, const Config* movement_cfg = nullptr) {
        if (!backend || (dx == 0 && dy == 0)) {
            return;
        }
        if (movement_cfg && movement_cfg->enable_humanized_movement) {
            QueueHumanizedMove(dx, dy, *movement_cfg);
            return;
        }
        CancelHumanizedMove();
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->MoveMouse(dx, dy);
    }

    void Click(int button) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->Click(button);
    }

    bool MouseDown(int button) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        return backend && backend->MouseDown(button);
    }

    bool MouseUp(int button) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        return backend && backend->MouseUp(button);
    }

    void Scroll(int delta) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->Scroll(delta);
    }

    void KeyDown(int vk) {
        if (vk <= 0 || !backend) {
            return;
        }
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->KeyDown(vk);
    }

    void KeyUp(int vk) {
        if (vk <= 0 || !backend) {
            return;
        }
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->KeyUp(vk);
    }

    void PressKeys(const std::vector<int>& keys) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        for (int vk : keys) {
            if (vk > 0) {
                backend->KeyDown(vk);
            }
        }
    }

    void ReleaseKeys(const std::vector<int>& keys) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
            if (*it > 0) {
                backend->KeyUp(*it);
            }
        }
    }

    const char* BackendName() const {
        return backend ? backend->Name() : "None";
    }

    bool IsDriverBackend() const {
        return backend && backend->IsDriverBackend();
    }
