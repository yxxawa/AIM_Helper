// FrameSource interface and crop geometry helper

class FrameSource {
public:
    virtual ~FrameSource() = default;
    virtual bool CaptureFrame(cv::Mat& frame, double& capture_ms) = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual int getCropSize() const = 0;
    virtual cv::Rect getCropRegion() const = 0;
    virtual const char* ModeName() const = 0;

    cv::Point getCropCenter() const {
        const int crop_size = getCropSize();
        return cv::Point(crop_size / 2, crop_size / 2);
    }
};

