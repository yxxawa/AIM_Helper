    void applyLiveConfig(const std::unordered_map<std::string, std::string>& values) {
        const Config old_cfg = cfg;
        Config next = readLiveConfigSnapshot(values, cfg);

        applyLiveRuntimeReloads(old_cfg, next);
        commitLiveConfig(old_cfg, std::move(next));
        logLiveConfigApplied();
    }
