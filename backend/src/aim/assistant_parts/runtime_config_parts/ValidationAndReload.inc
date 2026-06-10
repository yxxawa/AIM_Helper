// Config validation and runtime resource reload helpers

    static bool IsValidAimMode(const std::string& mode) {
        return mode == "atan" || mode == "linear";
    }

    static bool IsValidFilterMode(const std::string& mode) {
        return mode == "none" || mode == "pid" || mode == "oneeuro";
    }

    static bool IsValidInferenceBackend(const std::string& backend) {
        return backend == "tensorrt" || backend == "cuda" || backend == "cpu";
    }

    static bool IsValidInputBackend(const std::string& backend) {
        return backend == "sendinput" || backend == "driver";
    }

    void updateCropGeometry(int crop_size) {
        if (!frame_source) {
            return;
        }
        crop_region = frame_source->getCropRegion();
        crop_center = frame_source->getCropCenter();
        active_crosshair = crop_center;
        detected_crosshair = crop_center;
        last_crosshair_offset_x = 0;
        last_crosshair_offset_y = 0;
        crosshair_tracking_initialized = false;
        last_crosshair_found = false;
    }

    bool reinitializeCapture(const Config& next_cfg) {
        const int new_crop_size = next_cfg.crop_size;
        const int old_crop_size = cfg.crop_size;
        if (frame_source && (frame_source->getWidth() < new_crop_size || frame_source->getHeight() < new_crop_size)) {
            std::cerr << "\n[CONFIG] Ignored crop_size=" << new_crop_size
                << " because it is larger than the screen." << std::endl;
            return false;
        }

        std::cout << "\n[CONFIG] Reinitializing screen capture for crop_size=" << new_crop_size
            << ", mode=" << (next_cfg.enable_async_capture ? "latest" : "direct") << "..." << std::endl;
        frame_source.reset();
        try {
            frame_source = CreateFrameSource(next_cfg);
            updateCropGeometry(new_crop_size);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "\n[CONFIG] Capture reinitialization failed: " << e.what()
                << " Attempting to restore crop_size=" << old_crop_size << "." << std::endl;
        }

        try {
            frame_source = CreateFrameSource(cfg);
            updateCropGeometry(old_crop_size);
        }
        catch (const std::exception& e) {
            std::cerr << "\n[CONFIG] Failed to restore screen capture: " << e.what() << std::endl;
        }
        return false;
    }

    bool reloadDetector(const Config& next_cfg) {
        std::cout << "\n[CONFIG] Reloading detector: backend=" << next_cfg.backend << "..." << std::endl;
        try {
            auto next_detector = std::make_unique<ObjectDetector>(next_cfg);
            detector = std::move(next_detector);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "\n[CONFIG] Detector reload failed: " << e.what()
                << " Keeping the previous detector." << std::endl;
            return false;
        }
    }

    void reloadMouseBackend(const Config& next_cfg) {
        std::cout << "\n[CONFIG] Reinitializing mouse backend..." << std::endl;
        auto next_mouse = std::make_unique<MouseController>(next_cfg);
        mouse = std::move(next_mouse);
    }

    void setVisualizationEnabled(bool enabled) {
        if (enabled == is_visualizing) {
            return;
        }
        if (enabled) {
            cv::namedWindow(cfg.window_name, cv::WINDOW_AUTOSIZE);
            is_visualizing = true;
        }
        else {
            try {
                cv::destroyWindow(cfg.window_name);
            }
            catch (...) {
                cv::destroyAllWindows();
            }
            is_visualizing = false;
        }
    }

