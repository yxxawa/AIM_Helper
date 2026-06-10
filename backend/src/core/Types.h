// --- 辅助宏 ---
template<class T>
void SafeRelease(T** ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

// --- 数据结构定义 ---

struct Detection {
    cv::Rect box;
    float confidence;
    int class_id;
};

struct TimingDetails {
    double capture_ms = 0;
    double preprocess_ms = 0;
    double inference_ms = 0;
    double postprocess_ms = 0;
    double targeting_ms = 0;
    double input_ms = 0;
    double visualization_ms = 0;
    double total_loop_ms = 0;
};

struct TargetEntity {
    const Detection* head = nullptr;
    const Detection* body = nullptr;
    cv::Rect bounds;
    int camp_id = -1;
};

struct TargetLock {
    bool valid = false;
    cv::Point point;
    cv::Point measured_point;
    cv::Rect box;
    cv::Rect entity_box;
    const Detection* selected = nullptr;
    const Detection* head = nullptr;
    const Detection* body = nullptr;
    std::string part = "none";
};

