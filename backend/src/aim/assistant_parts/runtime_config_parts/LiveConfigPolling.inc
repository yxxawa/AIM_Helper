// Live config file polling

    void pollLiveConfig() {
        if (cfg.live_config_path.empty()) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (live_config_check_initialized) {
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_live_config_check).count();
            if (elapsed_ms < 200) {
                return;
            }
        }
        last_live_config_check = now;
        live_config_check_initialized = true;

        std::error_code ec;
        if (!std::filesystem::exists(cfg.live_config_path, ec)) {
            return;
        }
        const auto write_time = std::filesystem::last_write_time(cfg.live_config_path, ec);
        if (ec) {
            return;
        }
        if (live_config_write_initialized && write_time == last_live_config_write) {
            return;
        }
        last_live_config_write = write_time;
        live_config_write_initialized = true;

        const auto values = ReadKeyValueFile(cfg.live_config_path);
        if (values.empty()) {
            return;
        }
        applyLiveConfig(values);
    }

