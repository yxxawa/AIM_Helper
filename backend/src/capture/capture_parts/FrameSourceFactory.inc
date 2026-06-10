// Frame source factory

static std::unique_ptr<FrameSource> CreateFrameSource(const Config& cfg) {
    if (cfg.enable_async_capture) {
        return std::make_unique<LatestFrameSource>(cfg.crop_size);
    }
    return std::make_unique<DirectFrameSource>(cfg.crop_size);
}

