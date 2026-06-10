// Direct synchronous frame source

class DirectFrameSource final : public FrameSource {
public:
    explicit DirectFrameSource(int crop_size)
        : capturer(crop_size, crop_size), crop_size(crop_size) {
        crop_region = CenterCropRegion(capturer.getWidth(), capturer.getHeight(), crop_size);
        std::cout << "--- Capture source: direct synchronous DXGI ---" << std::endl;
    }

    bool CaptureFrame(cv::Mat& frame, double& capture_ms) override {
        const auto start = std::chrono::steady_clock::now();
        const bool ok = capturer.CaptureFrame(frame, crop_region);
        const auto end = std::chrono::steady_clock::now();
        capture_ms = std::chrono::duration<double, std::milli>(end - start).count();
        return ok;
    }

    int getWidth() const override { return capturer.getWidth(); }
    int getHeight() const override { return capturer.getHeight(); }
    int getCropSize() const override { return crop_size; }
    cv::Rect getCropRegion() const override { return crop_region; }
    const char* ModeName() const override { return "direct"; }

private:
    ScreenCapturer capturer;
    int crop_size = 0;
    cv::Rect crop_region;
};

