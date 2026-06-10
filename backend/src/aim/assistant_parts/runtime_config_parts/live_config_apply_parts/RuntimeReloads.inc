    void applyLiveRuntimeReloads(const Config& old_cfg, Config& next) {
        if (detectorConfigChanged(old_cfg, next) && !reloadDetector(next)) {
            next.model_path = old_cfg.model_path;
            next.backend = old_cfg.backend;
            next.trt_cache_path = old_cfg.trt_cache_path;
            next.use_end_to_end_onnx = old_cfg.use_end_to_end_onnx;
        }

        if (captureConfigChanged(old_cfg, next) && !reinitializeCapture(next)) {
            next.crop_size = old_cfg.crop_size;
            next.enable_async_capture = old_cfg.enable_async_capture;
        }

        if (mouseBackendConfigChanged(old_cfg, next)) {
            reloadMouseBackend(next);
        }
    }
