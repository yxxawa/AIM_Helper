// Latest-frame asynchronous capture source

class LatestFrameSource final : public FrameSource {
public:
    explicit LatestFrameSource(int crop_size)
        : capturer(std::make_unique<ScreenCapturer>(crop_size, crop_size)), crop_size(crop_size) {
        crop_region = CenterCropRegion(capturer->getWidth(), capturer->getHeight(), crop_size);
        worker = std::thread(&LatestFrameSource::CaptureLoop, this);
        std::cout << "--- Capture source: latest-frame async DXGI ---" << std::endl;
    }

    ~LatestFrameSource() override {
        stop.store(true);
        if (worker.joinable()) {
            worker.join();
        }
    }

    LatestFrameSource(const LatestFrameSource&) = delete;
    LatestFrameSource& operator=(const LatestFrameSource&) = delete;

    bool CaptureFrame(cv::Mat& frame, double& capture_ms) override {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (!has_frame || latest_sequence == consumed_sequence) {
            capture_ms = latest_capture_ms;
            return false;
        }

        latest_frame.copyTo(frame);
        capture_ms = latest_capture_ms;
        consumed_sequence = latest_sequence;
        return true;
    }

    int getWidth() const override { return capturer ? capturer->getWidth() : 0; }
    int getHeight() const override { return capturer ? capturer->getHeight() : 0; }
    int getCropSize() const override { return crop_size; }
    cv::Rect getCropRegion() const override { return crop_region; }
    const char* ModeName() const override { return "latest"; }

private:
    void CaptureLoop() {
        cv::Mat frame;
        while (!stop.load()) {
            const auto start = std::chrono::steady_clock::now();
            const bool ok = capturer && capturer->CaptureFrame(frame, crop_region);
            const auto end = std::chrono::steady_clock::now();
            const double capture_ms = std::chrono::duration<double, std::milli>(end - start).count();

            if (ok) {
                std::lock_guard<std::mutex> lock(frame_mutex);
                frame.copyTo(latest_frame);
                latest_capture_ms = capture_ms;
                ++latest_sequence;
                has_frame = true;
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(frame_mutex);
                latest_capture_ms = capture_ms;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::unique_ptr<ScreenCapturer> capturer;
    int crop_size = 0;
    cv::Rect crop_region;
    std::thread worker;
    std::atomic_bool stop{ false };
    std::mutex frame_mutex;
    cv::Mat latest_frame;
    double latest_capture_ms = 0.0;
    uint64_t latest_sequence = 0;
    uint64_t consumed_sequence = 0;
    bool has_frame = false;
};

