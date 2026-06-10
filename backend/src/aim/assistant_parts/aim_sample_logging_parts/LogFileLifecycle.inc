    void ensureAimSampleLogOpen() {
        if (aim_sample_log_opened) {
            return;
        }
        std::filesystem::path dir = std::filesystem::current_path() / "aim_samples";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        aim_sample_log_path = dir / ("aim_samples_" + CurrentLocalTimestampForFile() + ".jsonl");
        aim_sample_log.open(aim_sample_log_path, std::ios::out | std::ios::app);
        aim_sample_log_opened = aim_sample_log.is_open();
        if (aim_sample_log_opened) {
            std::cout << "\n[INFO] Aim sample log: " << aim_sample_log_path.string() << std::endl;
        }
        else {
            std::cerr << "\n[WARN] Failed to open aim sample log: " << aim_sample_log_path.string() << std::endl;
        }
    }

    void writeAimSampleSettingsIfNeeded() {
        ensureAimSampleLogOpen();
        if (!aim_sample_log_opened) {
            return;
        }
        const std::string settings = aimSampleSettingsJson();
        if (settings == last_aim_sample_settings_json) {
            return;
        }
        last_aim_sample_settings_json = settings;
        aim_sample_log << "{\"type\":\"settings\",\"ts\":\"" << CurrentLocalTimestampIso()
            << "\",\"frame\":" << aim_sample_frame_index
            << ",\"settings\":{" << settings << "}}\n";
        ++aim_sample_lines_since_flush;
        flushAimSampleLogIfNeeded(true);
    }

    void flushAimSampleLogIfNeeded(bool force = false) {
        if (!aim_sample_log_opened) {
            return;
        }
        if (force || aim_sample_lines_since_flush >= 120) {
            aim_sample_log.flush();
            aim_sample_lines_since_flush = 0;
        }
    }
