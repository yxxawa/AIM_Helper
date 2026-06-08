#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <chrono>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <memory>
#include <sstream> // For std::ostringstream
#include <thread>  // For std::this_thread
#include <cstdlib>
#include <utility>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <limits>
#include <random>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <array>

// Windows & DirectX
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")

// ONNX Runtime
#include <onnxruntime_cxx_api.h>
#pragma comment(lib, "onnxruntime.lib")

// OpenCV
#include <opencv2/opencv.hpp>
#ifdef _DEBUG
#pragma comment(lib, "opencv_world4120d.lib") // 请根据你的OpenCV版本修改
#else
#pragma comment(lib, "opencv_world4120.lib") // 请根据你的OpenCV版本修改
#endif

// Mouse input defaults to Win32 SendInput, with an opt-in user-provided DLL backend.

// Forward Declarations
struct Detection;
struct TimingDetails;
class Config;
class ScreenCapturer;
class FrameSource;
class ObjectDetector;
class MouseController;
class AimAssistant;

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
    const Detection* selected = nullptr;
    const Detection* head = nullptr;
    const Detection* body = nullptr;
    std::string part = "none";
    bool prediction_applied = false;
};

// --- 1. 配置中心 (Config) ---
class Config {
public:
    std::wstring model_path = L"cs2yolomaax.onnx";
    std::wstring live_config_path;
    bool use_end_to_end_onnx = false;
    std::string backend = "cpu";
    std::string input_backend = "sendinput";
    std::wstring driver_dll_path;
    int driver_type = 1;
    const char* trt_cache_path = ".\\engine_cache";
    int crop_size = 320;
    int max_lock_distance_pixels = 100;
    float confidence_threshold = 0.5f;
    float nms_threshold = 0.4f;
    int smooth_aim_key = VK_LBUTTON;
    int smooth_aim_secondary_key = 0;
    int mouse_test_key = VK_F9;
    double aim_smoothing = 1.0;
    double aim_gain = 0.45;
    double aim_deadzone_pixels = 1.5;
    std::string aim_filter_mode = "none";
    double pid_kp = 1.0;
    double pid_ki = 0.0;
    double pid_kd = 0.0;
    double pid_integral_limit = 120.0;
    double one_euro_min_cutoff = 1.0;
    double one_euro_beta = 0.02;
    double one_euro_d_cutoff = 1.0;
    std::string prediction_mode = "off";
    double prediction_lead_ms = 20.0;
    double prediction_smoothing = 0.12;
    double prediction_acceleration_smoothing = 0.18;
    double prediction_alpha = 0.45;
    double prediction_beta = 0.06;
    double prediction_kalman_measurement_noise = 34.0;
    double prediction_kalman_process_noise = 72.0;
    int prediction_max_pixels = 18;
    int prediction_reset_pixels = 70;
    double prediction_noise_pixels = 1.5;
    double prediction_output_smoothing = 0.20;
    double prediction_servo_gain = 0.65;
    bool enable_drone_tracking = false;
    std::string drone_track_controller = "px4";
    double drone_track_gain = 0.72;
    double drone_track_velocity_gain = 0.18;
    double drone_track_damping = 0.10;
    double drone_track_smoothing = 0.60;
    int drone_track_max_move_pixels = 90;
    double drone_track_deadzone_pixels = 0.6;
    double drone_track_position_gain = 0.90;
    double drone_track_velocity_damping = 0.28;
    double drone_track_accel_limit = 2600.0;
    double drone_track_visp_lambda = 0.85;
    double drone_track_visp_damping = 0.22;
    double target_x_ratio = 0.5;
    double target_y_ratio = 0.3;
    bool auto_target_part = true;
    std::string aim_part_priority = "distance";
    std::string enemy_camp = "all";
    std::string detection_part = "all";
    double sensitivity = 1.0;
    std::string aim_mode = "atan";
    bool enable_capture = true;
    bool enable_async_capture = true;
    bool enable_visualization = true;
    bool enable_mouse_movement = true;
    bool enable_hold_to_aim = true;
    bool bounded_movement = true;
    bool input_self_test = false;
    bool require_driver_backend = false;
    bool enable_console_stats = false;
    int max_move_pixels = 60;
    bool enable_tracking_boost = true;
    double tracking_boost_threshold_pixels = 4.0;
    double tracking_boost_gain = 2.0;
    int tracking_boost_max_move_pixels = 120;
    bool enable_humanized_movement = false;
    double human_move_max_step = 50.0;
    double human_move_jitter = 0.5;
    int human_move_delay_min_ms = 5;
    int human_move_delay_max_ms = 20;
    bool enable_auto_click = false;
    int auto_click_delay_min_ms = 80;
    int auto_click_delay_max_ms = 160;
    int auto_click_interval_min_ms = 120;
    int auto_click_interval_max_ms = 220;
    double auto_click_tolerance_pixels = 3.0;
    bool enable_auto_stop = false;
    std::string auto_stop_mode = "counter_tap";
    int auto_stop_hold_ms = 75;
    int auto_stop_settle_ms = 15;
    std::string window_name = "YOLO Real-time Detection";
};

static bool ReadArgValue(const std::string& arg, const std::string& name, std::string& value) {
    const std::string prefix = "--" + name + "=";
    if (arg.rfind(prefix, 0) != 0) {
        return false;
    }
    value = arg.substr(prefix.size());
    return true;
}

static int ReadInt(const std::string& value, int fallback) {
    try {
        return std::stoi(value);
    }
    catch (...) {
        return fallback;
    }
}

static double ReadDouble(const std::string& value, double fallback) {
    try {
        return std::stod(value);
    }
    catch (...) {
        return fallback;
    }
}

static std::wstring WidenUtf8(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int size = ::MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 1) {
        return {};
    }
    std::wstring result(static_cast<size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    result.resize(static_cast<size_t>(size - 1));
    return result;
}

static std::string NarrowAscii(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size = ::WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return {};
    }
    std::string result(static_cast<size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    result.resize(static_cast<size_t>(size - 1));
    return result;
}

static std::string LowerAscii(std::string value) {
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return value;
}

static std::string UpperAscii(std::string value) {
    for (char& ch : value) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return value;
}

static std::string KeyNameFromVk(int vk) {
    if (vk <= 0) return "NONE";
    if (vk == VK_LBUTTON) return "MouseLeft";
    if (vk == VK_RBUTTON) return "MouseRight";
    if (vk == VK_MBUTTON) return "MouseMiddle";
    if (vk == VK_XBUTTON1) return "MouseX1";
    if (vk == VK_XBUTTON2) return "MouseX2";
    if (vk >= VK_F1 && vk <= VK_F24) return "F" + std::to_string(vk - VK_F1 + 1);
    if (vk >= 'A' && vk <= 'Z') return std::string(1, static_cast<char>(vk));
    if (vk >= '0' && vk <= '9') return std::string(1, static_cast<char>(vk));
    if (vk == VK_SPACE) return "Space";
    if (vk == VK_RETURN) return "Enter";
    if (vk == VK_TAB) return "Tab";
    if (vk == VK_SHIFT) return "Shift";
    if (vk == VK_CONTROL) return "Ctrl";
    if (vk == VK_MENU) return "Alt";
    return "VK_" + std::to_string(vk);
}

static std::string TrimAscii(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static bool ReadBoolText(const std::string& value, bool fallback) {
    const std::string normalized = LowerAscii(TrimAscii(value));
    if (normalized == "1" || normalized == "true" || normalized == "on" || normalized == "yes") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "off" || normalized == "no") {
        return false;
    }
    return fallback;
}

static std::string NormalizeEnemyCamp(std::string value) {
    value = LowerAscii(TrimAscii(value));
    if (value == "ct" || value == "t") {
        return value;
    }
    return "all";
}

static bool IsValidEnemyCamp(const std::string& value) {
    return value == "all" || value == "ct" || value == "t";
}

static std::string NormalizeDetectionPart(std::string value) {
    value = LowerAscii(TrimAscii(value));
    if (value == "head" || value == "body") {
        return value;
    }
    return "all";
}

static bool IsValidDetectionPart(const std::string& value) {
    return value == "all" || value == "head" || value == "body";
}

static std::string NormalizeAutoStopMode(std::string value) {
    value = LowerAscii(TrimAscii(value));
    if (value == "ad_pair" || value == "ad" || value == "a_d") {
        return "ad_pair";
    }
    if (value == "crouch" || value == "duck") {
        return "crouch";
    }
    return "counter_tap";
}

static bool IsValidAutoStopMode(const std::string& value) {
    return value == "counter_tap" || value == "ad_pair" || value == "crouch";
}

static std::string Cs2ClassLabel(int class_id) {
    switch (class_id) {
    case 0:
        return "CT-body";
    case 1:
        return "CT-head";
    case 2:
        return "T-body";
    case 3:
        return "T-head";
    default:
        return "ID:" + std::to_string(class_id);
    }
}

static std::string EnemyCampTargetSummary(const std::string& enemy_camp) {
    const std::string camp = NormalizeEnemyCamp(enemy_camp);
    if (camp == "ct") {
        return "T-body(2)+T-head(3)";
    }
    if (camp == "t") {
        return "CT-body(0)+CT-head(1)";
    }
    return "ALL";
}

static bool DetectionMatchesEnemyCamp(int class_id, const std::string& enemy_camp) {
    const std::string camp = NormalizeEnemyCamp(enemy_camp);
    if (camp == "ct") {
        return class_id == 2 || class_id == 3;
    }
    if (camp == "t") {
        return class_id == 0 || class_id == 1;
    }
    return true;
}

static std::string DetectionPartSummary(const std::string& detection_part) {
    const std::string part = NormalizeDetectionPart(detection_part);
    if (part == "head") {
        return "head(1/3)";
    }
    if (part == "body") {
        return "body(0/2)";
    }
    return "ALL";
}

static bool IsHeadClass(int class_id) {
    return class_id == 1 || class_id == 3;
}

static bool IsBodyClass(int class_id) {
    return class_id == 0 || class_id == 2;
}

static bool DetectionMatchesPartFilter(int class_id, const std::string& detection_part) {
    const std::string part = NormalizeDetectionPart(detection_part);
    if (part == "head") {
        return IsHeadClass(class_id);
    }
    if (part == "body") {
        return IsBodyClass(class_id);
    }
    return true;
}

static int ClassCampId(int class_id) {
    if (class_id == 0 || class_id == 1) {
        return 0;
    }
    if (class_id == 2 || class_id == 3) {
        return 1;
    }
    return -1;
}

static std::string NormalizeAimPartPriority(const std::string& value) {
    const std::string mode = LowerAscii(value);
    if (mode == "head" || mode == "head_first") {
        return "head";
    }
    if (mode == "other" || mode == "body" || mode == "body_first") {
        return "other";
    }
    return "distance";
}

static bool IsValidAimPartPriority(const std::string& mode) {
    return mode == "distance" || mode == "head" || mode == "other";
}

static std::string NormalizePredictionMode(std::string value) {
    value = LowerAscii(TrimAscii(value));
    if (value == "linear" || value == "arc" || value == "hybrid" || value == "adaptive" || value == "alphabeta" || value == "kalman" || value == "servo") {
        return value;
    }
    return "off";
}

static bool IsValidPredictionMode(const std::string& mode) {
    return mode == "off" || mode == "linear" || mode == "arc" || mode == "hybrid" || mode == "adaptive" || mode == "alphabeta" || mode == "kalman" || mode == "servo";
}

static std::string NormalizeDroneTrackController(std::string value) {
    value = LowerAscii(TrimAscii(value));
    if (value == "classic" || value == "px4" || value == "visp") {
        return value;
    }
    return "px4";
}

static bool IsValidDroneTrackController(const std::string& mode) {
    return mode == "classic" || mode == "px4" || mode == "visp";
}

static std::unordered_map<std::string, std::string> ReadKeyValueFile(const std::wstring& path) {
    std::unordered_map<std::string, std::string> values;
    std::ifstream file{ std::filesystem::path(path) };
    if (!file) {
        return values;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = TrimAscii(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        std::string key = TrimAscii(line.substr(0, equals));
        std::string value = TrimAscii(line.substr(equals + 1));
        if (!key.empty()) {
            values[LowerAscii(key)] = value;
        }
    }
    return values;
}

static bool ReadLiveBool(const std::unordered_map<std::string, std::string>& values, const std::string& name, bool fallback) {
    const auto it = values.find(name);
    if (it == values.end()) {
        return fallback;
    }
    const std::string value = LowerAscii(it->second);
    if (value == "1" || value == "true" || value == "on" || value == "yes") {
        return true;
    }
    if (value == "0" || value == "false" || value == "off" || value == "no") {
        return false;
    }
    return fallback;
}

static int ReadLiveInt(const std::unordered_map<std::string, std::string>& values, const std::string& name, int fallback) {
    const auto it = values.find(name);
    return it == values.end() ? fallback : ReadInt(it->second, fallback);
}

static double ReadLiveDouble(const std::unordered_map<std::string, std::string>& values, const std::string& name, double fallback) {
    const auto it = values.find(name);
    return it == values.end() ? fallback : ReadDouble(it->second, fallback);
}

static std::string ReadLiveString(const std::unordered_map<std::string, std::string>& values, const std::string& name, const std::string& fallback) {
    const auto it = values.find(name);
    return it == values.end() ? fallback : LowerAscii(it->second);
}

static std::string ReadLiveRawString(const std::unordered_map<std::string, std::string>& values, const std::string& name, const std::string& fallback) {
    const auto it = values.find(name);
    return it == values.end() ? fallback : it->second;
}

static bool ReadArgValue(const std::wstring& arg, const std::wstring& name, std::wstring& value) {
    const std::wstring prefix = L"--" + name + L"=";
    if (arg.rfind(prefix, 0) != 0) {
        return false;
    }
    value = arg.substr(prefix.size());
    return true;
}

static Config ConfigFromArgs(int argc, wchar_t* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i] ? argv[i] : L"";
        std::wstring value;
        if (ReadArgValue(arg, L"model", value)) cfg.model_path = value;
        else if (ReadArgValue(arg, L"live-config", value)) cfg.live_config_path = value;
        else if (ReadArgValue(arg, L"backend", value)) cfg.backend = NarrowAscii(value);
        else if (ReadArgValue(arg, L"input-backend", value)) cfg.input_backend = NarrowAscii(value);
        else if (ReadArgValue(arg, L"driver-dll", value)) cfg.driver_dll_path = value;
        else if (ReadArgValue(arg, L"driver-type", value)) cfg.driver_type = std::clamp(ReadInt(NarrowAscii(value), cfg.driver_type), 0, 3);
        else if (ReadArgValue(arg, L"crop-size", value)) cfg.crop_size = std::clamp(ReadInt(NarrowAscii(value), cfg.crop_size), 160, 960);
        else if (ReadArgValue(arg, L"lock-radius", value)) cfg.max_lock_distance_pixels = std::clamp(ReadInt(NarrowAscii(value), cfg.max_lock_distance_pixels), 10, 500);
        else if (ReadArgValue(arg, L"confidence", value)) cfg.confidence_threshold = static_cast<float>(std::clamp(ReadDouble(NarrowAscii(value), cfg.confidence_threshold), 0.01, 0.99));
        else if (ReadArgValue(arg, L"smoothing", value)) cfg.aim_smoothing = std::clamp(ReadDouble(NarrowAscii(value), cfg.aim_smoothing), 0.01, 5.0);
        else if (ReadArgValue(arg, L"aim-key", value)) cfg.smooth_aim_key = std::clamp(ReadInt(NarrowAscii(value), cfg.smooth_aim_key), 0, 255);
        else if (ReadArgValue(arg, L"aim-key2", value)) cfg.smooth_aim_secondary_key = std::clamp(ReadInt(NarrowAscii(value), cfg.smooth_aim_secondary_key), 0, 255);
        else if (ReadArgValue(arg, L"aim-gain", value)) cfg.aim_gain = std::clamp(ReadDouble(NarrowAscii(value), cfg.aim_gain), 0.01, 5.0);
        else if (ReadArgValue(arg, L"deadzone", value)) cfg.aim_deadzone_pixels = std::clamp(ReadDouble(NarrowAscii(value), cfg.aim_deadzone_pixels), 0.0, 30.0);
        else if (ReadArgValue(arg, L"auto-target-part", value)) cfg.auto_target_part = ReadBoolText(NarrowAscii(value), cfg.auto_target_part);
        else if (ReadArgValue(arg, L"aim-part-priority", value)) cfg.aim_part_priority = NormalizeAimPartPriority(NarrowAscii(value));
        else if (ReadArgValue(arg, L"aim-mode", value)) {
            const std::string mode = LowerAscii(NarrowAscii(value));
            if (mode == "atan" || mode == "linear") {
                cfg.aim_mode = mode;
            }
        }
        else if (ReadArgValue(arg, L"aim-filter", value)) {
            const std::string mode = LowerAscii(NarrowAscii(value));
            if (mode == "none" || mode == "pid" || mode == "oneeuro" || mode == "pid_oneeuro") {
                cfg.aim_filter_mode = mode;
            }
        }
        else if (ReadArgValue(arg, L"pid-kp", value)) cfg.pid_kp = std::clamp(ReadDouble(NarrowAscii(value), cfg.pid_kp), 0.0, 10.0);
        else if (ReadArgValue(arg, L"pid-ki", value)) cfg.pid_ki = std::clamp(ReadDouble(NarrowAscii(value), cfg.pid_ki), 0.0, 10.0);
        else if (ReadArgValue(arg, L"pid-kd", value)) cfg.pid_kd = std::clamp(ReadDouble(NarrowAscii(value), cfg.pid_kd), 0.0, 10.0);
        else if (ReadArgValue(arg, L"pid-i-limit", value)) cfg.pid_integral_limit = std::clamp(ReadDouble(NarrowAscii(value), cfg.pid_integral_limit), 0.0, 1000.0);
        else if (ReadArgValue(arg, L"one-euro-min-cutoff", value)) cfg.one_euro_min_cutoff = std::clamp(ReadDouble(NarrowAscii(value), cfg.one_euro_min_cutoff), 0.01, 100.0);
        else if (ReadArgValue(arg, L"one-euro-beta", value)) cfg.one_euro_beta = std::clamp(ReadDouble(NarrowAscii(value), cfg.one_euro_beta), 0.0, 100.0);
        else if (ReadArgValue(arg, L"one-euro-d-cutoff", value)) cfg.one_euro_d_cutoff = std::clamp(ReadDouble(NarrowAscii(value), cfg.one_euro_d_cutoff), 0.01, 100.0);
        else if (ReadArgValue(arg, L"prediction-mode", value)) cfg.prediction_mode = NormalizePredictionMode(NarrowAscii(value));
        else if (ReadArgValue(arg, L"prediction-lead-ms", value)) cfg.prediction_lead_ms = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_lead_ms), 0.0, 250.0);
        else if (ReadArgValue(arg, L"prediction-smoothing", value)) cfg.prediction_smoothing = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_smoothing), 0.0, 1.0);
        else if (ReadArgValue(arg, L"prediction-acceleration-smoothing", value)) cfg.prediction_acceleration_smoothing = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_acceleration_smoothing), 0.0, 1.0);
        else if (ReadArgValue(arg, L"prediction-alpha", value)) cfg.prediction_alpha = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_alpha), 0.0, 1.0);
        else if (ReadArgValue(arg, L"prediction-beta", value)) cfg.prediction_beta = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_beta), 0.0, 1.0);
        else if (ReadArgValue(arg, L"prediction-kalman-measurement-noise", value)) cfg.prediction_kalman_measurement_noise = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_kalman_measurement_noise), 0.1, 500.0);
        else if (ReadArgValue(arg, L"prediction-kalman-process-noise", value)) cfg.prediction_kalman_process_noise = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_kalman_process_noise), 0.1, 5000.0);
        else if (ReadArgValue(arg, L"prediction-max-pixels", value)) cfg.prediction_max_pixels = std::clamp(ReadInt(NarrowAscii(value), cfg.prediction_max_pixels), 0, 250);
        else if (ReadArgValue(arg, L"prediction-reset-pixels", value)) cfg.prediction_reset_pixels = std::clamp(ReadInt(NarrowAscii(value), cfg.prediction_reset_pixels), 5, 500);
        else if (ReadArgValue(arg, L"prediction-noise-pixels", value)) cfg.prediction_noise_pixels = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_noise_pixels), 0.0, 20.0);
        else if (ReadArgValue(arg, L"prediction-output-smoothing", value)) cfg.prediction_output_smoothing = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_output_smoothing), 0.0, 1.0);
        else if (ReadArgValue(arg, L"prediction-servo-gain", value)) cfg.prediction_servo_gain = std::clamp(ReadDouble(NarrowAscii(value), cfg.prediction_servo_gain), 0.0, 2.0);
        else if (ReadArgValue(arg, L"drone-tracking", value)) cfg.enable_drone_tracking = ReadBoolText(NarrowAscii(value), cfg.enable_drone_tracking);
        else if (ReadArgValue(arg, L"drone-track-controller", value)) cfg.drone_track_controller = NormalizeDroneTrackController(NarrowAscii(value));
        else if (ReadArgValue(arg, L"drone-track-gain", value)) cfg.drone_track_gain = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_gain), 0.01, 5.0);
        else if (ReadArgValue(arg, L"drone-track-velocity-gain", value)) cfg.drone_track_velocity_gain = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_velocity_gain), 0.0, 3.0);
        else if (ReadArgValue(arg, L"drone-track-damping", value)) cfg.drone_track_damping = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_damping), 0.0, 3.0);
        else if (ReadArgValue(arg, L"drone-track-smoothing", value)) cfg.drone_track_smoothing = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_smoothing), 0.02, 1.0);
        else if (ReadArgValue(arg, L"drone-track-max-move", value)) cfg.drone_track_max_move_pixels = std::clamp(ReadInt(NarrowAscii(value), cfg.drone_track_max_move_pixels), 1, 800);
        else if (ReadArgValue(arg, L"drone-track-deadzone", value)) cfg.drone_track_deadzone_pixels = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_deadzone_pixels), 0.0, 30.0);
        else if (ReadArgValue(arg, L"drone-track-position-gain", value)) cfg.drone_track_position_gain = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_position_gain), 0.0, 5.0);
        else if (ReadArgValue(arg, L"drone-track-velocity-damping", value)) cfg.drone_track_velocity_damping = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_velocity_damping), 0.0, 5.0);
        else if (ReadArgValue(arg, L"drone-track-accel-limit", value)) cfg.drone_track_accel_limit = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_accel_limit), 100.0, 20000.0);
        else if (ReadArgValue(arg, L"drone-track-visp-lambda", value)) cfg.drone_track_visp_lambda = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_visp_lambda), 0.01, 5.0);
        else if (ReadArgValue(arg, L"drone-track-visp-damping", value)) cfg.drone_track_visp_damping = std::clamp(ReadDouble(NarrowAscii(value), cfg.drone_track_visp_damping), 0.0, 5.0);
        else if (ReadArgValue(arg, L"target-x", value)) cfg.target_x_ratio = std::clamp(ReadDouble(NarrowAscii(value), cfg.target_x_ratio), 0.0, 1.0);
        else if (ReadArgValue(arg, L"target-y", value)) cfg.target_y_ratio = std::clamp(ReadDouble(NarrowAscii(value), cfg.target_y_ratio), 0.0, 1.0);
        else if (ReadArgValue(arg, L"enemy-camp", value)) cfg.enemy_camp = NormalizeEnemyCamp(NarrowAscii(value));
        else if (ReadArgValue(arg, L"detection-part", value)) cfg.detection_part = NormalizeDetectionPart(NarrowAscii(value));
        else if (ReadArgValue(arg, L"max-move", value)) cfg.max_move_pixels = std::clamp(ReadInt(NarrowAscii(value), cfg.max_move_pixels), 1, 500);
        else if (ReadArgValue(arg, L"tracking-boost", value)) cfg.enable_tracking_boost = ReadBoolText(NarrowAscii(value), cfg.enable_tracking_boost);
        else if (ReadArgValue(arg, L"tracking-boost-threshold", value)) cfg.tracking_boost_threshold_pixels = std::clamp(ReadDouble(NarrowAscii(value), cfg.tracking_boost_threshold_pixels), 1.0, 200.0);
        else if (ReadArgValue(arg, L"tracking-boost-gain", value)) cfg.tracking_boost_gain = std::clamp(ReadDouble(NarrowAscii(value), cfg.tracking_boost_gain), 1.0, 5.0);
        else if (ReadArgValue(arg, L"tracking-boost-max-move", value)) cfg.tracking_boost_max_move_pixels = std::clamp(ReadInt(NarrowAscii(value), cfg.tracking_boost_max_move_pixels), 1, 500);
        else if (ReadArgValue(arg, L"human-slide", value)) cfg.enable_humanized_movement = ReadBoolText(NarrowAscii(value), cfg.enable_humanized_movement);
        else if (ReadArgValue(arg, L"human-slide-max-step", value)) cfg.human_move_max_step = std::clamp(ReadDouble(NarrowAscii(value), cfg.human_move_max_step), 1.0, 500.0);
        else if (ReadArgValue(arg, L"human-slide-jitter", value)) cfg.human_move_jitter = std::clamp(ReadDouble(NarrowAscii(value), cfg.human_move_jitter), 0.0, 20.0);
        else if (ReadArgValue(arg, L"human-slide-delay-min", value)) cfg.human_move_delay_min_ms = std::clamp(ReadInt(NarrowAscii(value), cfg.human_move_delay_min_ms), 0, 100);
        else if (ReadArgValue(arg, L"human-slide-delay-max", value)) cfg.human_move_delay_max_ms = std::clamp(ReadInt(NarrowAscii(value), cfg.human_move_delay_max_ms), 0, 100);
        else if (ReadArgValue(arg, L"auto-click", value)) cfg.enable_auto_click = ReadBoolText(NarrowAscii(value), cfg.enable_auto_click);
        else if (ReadArgValue(arg, L"auto-click-delay-min", value)) cfg.auto_click_delay_min_ms = std::clamp(ReadInt(NarrowAscii(value), cfg.auto_click_delay_min_ms), 0, 5000);
        else if (ReadArgValue(arg, L"auto-click-delay-max", value)) cfg.auto_click_delay_max_ms = std::clamp(ReadInt(NarrowAscii(value), cfg.auto_click_delay_max_ms), 0, 5000);
        else if (ReadArgValue(arg, L"auto-click-interval-min", value)) cfg.auto_click_interval_min_ms = std::clamp(ReadInt(NarrowAscii(value), cfg.auto_click_interval_min_ms), 0, 5000);
        else if (ReadArgValue(arg, L"auto-click-interval-max", value)) cfg.auto_click_interval_max_ms = std::clamp(ReadInt(NarrowAscii(value), cfg.auto_click_interval_max_ms), 0, 5000);
        else if (ReadArgValue(arg, L"auto-click-tolerance", value)) cfg.auto_click_tolerance_pixels = std::clamp(ReadDouble(NarrowAscii(value), cfg.auto_click_tolerance_pixels), 0.0, 50.0);
        else if (ReadArgValue(arg, L"auto-stop", value)) cfg.enable_auto_stop = ReadBoolText(NarrowAscii(value), cfg.enable_auto_stop);
        else if (ReadArgValue(arg, L"auto-stop-mode", value)) cfg.auto_stop_mode = NormalizeAutoStopMode(NarrowAscii(value));
        else if (ReadArgValue(arg, L"auto-stop-hold-ms", value)) cfg.auto_stop_hold_ms = std::clamp(ReadInt(NarrowAscii(value), cfg.auto_stop_hold_ms), 0, 250);
        else if (ReadArgValue(arg, L"auto-stop-settle-ms", value)) cfg.auto_stop_settle_ms = std::clamp(ReadInt(NarrowAscii(value), cfg.auto_stop_settle_ms), 0, 250);
        else if (ReadArgValue(arg, L"console-stats", value)) cfg.enable_console_stats = ReadBoolText(NarrowAscii(value), cfg.enable_console_stats);
        else if (arg == L"--end-to-end") cfg.use_end_to_end_onnx = true;
        else if (arg == L"--enable-human-slide") cfg.enable_humanized_movement = true;
        else if (arg == L"--disable-capture") cfg.enable_capture = false;
        else if (arg == L"--disable-async-capture") cfg.enable_async_capture = false;
        else if (arg == L"--enable-async-capture") cfg.enable_async_capture = true;
        else if (arg == L"--disable-mouse-move") cfg.enable_mouse_movement = false;
        else if (arg == L"--disable-hold-to-aim") cfg.enable_hold_to_aim = false;
        else if (arg == L"--disable-visualization") cfg.enable_visualization = false;
        else if (arg == L"--unbounded-movement") cfg.bounded_movement = false;
        else if (arg == L"--input-self-test") cfg.input_self_test = true;
        else if (arg == L"--require-driver") cfg.require_driver_backend = true;
    }
    if (cfg.auto_click_delay_min_ms > cfg.auto_click_delay_max_ms) {
        std::swap(cfg.auto_click_delay_min_ms, cfg.auto_click_delay_max_ms);
    }
    if (cfg.auto_click_interval_min_ms > cfg.auto_click_interval_max_ms) {
        std::swap(cfg.auto_click_interval_min_ms, cfg.auto_click_interval_max_ms);
    }
    if (cfg.detection_part == "body") {
        cfg.auto_target_part = false;
    }
    return cfg;
}

// --- 2. Mouse input backends ---
static bool SendKeyboardInputVk(int vk, bool key_down, const char* context) {
    if (vk <= 0) {
        return false;
    }

    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = static_cast<WORD>(vk);
    input.ki.dwFlags = key_down ? 0 : KEYEVENTF_KEYUP;
    if (::SendInput(1, &input, sizeof(INPUT)) != 1) {
        const DWORD error = GetLastError();
        std::cerr << "\n[WARN] " << context << " SendInput key "
            << (key_down ? "down" : "up")
            << " failed for " << KeyNameFromVk(vk)
            << ". GetLastError=" << error << std::endl;
        return false;
    }
    return true;
}

class InputBackend {
public:
    virtual ~InputBackend() = default;
    virtual void MoveMouse(int x, int y) = 0;
    virtual void Click(int button) = 0;
    virtual void Scroll(int delta) = 0;
    virtual void KeyDown(int vk) = 0;
    virtual void KeyUp(int vk) = 0;
    virtual const char* Name() const = 0;
    virtual bool IsDriverBackend() const { return false; }
};

class SendInputBackend final : public InputBackend {
public:
    SendInputBackend() {
        std::cout << "--- Mouse input initialized: Win32 SendInput (no driver) ---" << std::endl;
    }

    void MoveMouse(int x, int y) override {
        if (x == 0 && y == 0) {
            return;
        }

        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dx = static_cast<LONG>(x);
        input.mi.dy = static_cast<LONG>(y);
        input.mi.dwFlags = MOUSEEVENTF_MOVE;

        if (::SendInput(1, &input, sizeof(INPUT)) != 1) {
            const DWORD error = GetLastError();
            std::cerr << "\n[WARN] SendInput mouse move failed. GetLastError=" << error << std::endl;
        }
    }

    void Click(int button) override {
        DWORD down_flag = 0;
        DWORD up_flag = 0;
        if (button == 0) {
            down_flag = MOUSEEVENTF_LEFTDOWN;
            up_flag = MOUSEEVENTF_LEFTUP;
        }
        else if (button == 1) {
            down_flag = MOUSEEVENTF_RIGHTDOWN;
            up_flag = MOUSEEVENTF_RIGHTUP;
        }
        else if (button == 2) {
            down_flag = MOUSEEVENTF_MIDDLEDOWN;
            up_flag = MOUSEEVENTF_MIDDLEUP;
        }
        else {
            std::cerr << "\n[WARN] Unsupported SendInput mouse button: " << button << std::endl;
            return;
        }

        INPUT inputs[2]{};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = down_flag;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = up_flag;
        if (::SendInput(2, inputs, sizeof(INPUT)) != 2) {
            const DWORD error = GetLastError();
            std::cerr << "\n[WARN] SendInput click failed. GetLastError=" << error << std::endl;
        }
    }

    void Scroll(int delta) override {
        if (delta == 0) {
            return;
        }

        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.mouseData = static_cast<DWORD>(delta);
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        if (::SendInput(1, &input, sizeof(INPUT)) != 1) {
            const DWORD error = GetLastError();
            std::cerr << "\n[WARN] SendInput scroll failed. GetLastError=" << error << std::endl;
        }
    }

    void KeyDown(int vk) override {
        SendKeyboardInputVk(vk, true, "SendInput");
    }

    void KeyUp(int vk) override {
        SendKeyboardInputVk(vk, false, "SendInput");
    }

    const char* Name() const override {
        return "SendInput";
    }
};

class DriverBackend final : public InputBackend {
public:
    DriverBackend(const std::wstring& dll_path, int requested_driver_type)
        : preferred_driver_type(requested_driver_type) {
        if (dll_path.empty()) {
            throw std::runtime_error("Driver DLL path is empty.");
        }

        std::wcout << L"--- Loading driver mouse backend DLL: " << dll_path << L" ---" << std::endl;
        hDll = ::LoadLibraryW(dll_path.c_str());
        if (!hDll) {
            throw std::runtime_error("LoadLibraryW failed for driver DLL.");
        }

        try {
            if (!BindStandardAbi() && !BindLogiDriverAbi() && !BindLgMouseAbi() && !BindDdAbi()) {
                throw std::runtime_error("Driver DLL exports do not match any supported input backend ABI.");
            }
        }
        catch (...) {
            CleanupLibrary();
            throw;
        }
        std::cout << "--- Mouse input initialized: " << Name() << " ---" << std::endl;
    }

    ~DriverBackend() override {
        if (api_mode == ApiMode::Standard && initialized && driverCleanup) {
            driverCleanup();
        }
        else if (api_mode == ApiMode::LogiDriver && initialized && legacyClose) {
            legacyClose();
        }
        else if (api_mode == ApiMode::LgMouse && lgMouseHandle && lgMouseClose) {
            lgMouseClose(lgMouseHandle);
            lgMouseHandle = nullptr;
        }
        CleanupLibrary();
    }

    DriverBackend(const DriverBackend&) = delete;
    DriverBackend& operator=(const DriverBackend&) = delete;

    void MoveMouse(int x, int y) override {
        if (x == 0 && y == 0) {
            return;
        }
        if (api_mode == ApiMode::Standard && driverMove) {
            driverMove(x, y);
            LogMoveCall("DriverMove", x, y);
        }
        else if (api_mode == ApiMode::LogiDriver && legacyMove) {
            const int result = legacyMove(x, y);
            LogMoveResult("moveR", x, y, result);
        }
        else if (api_mode == ApiMode::LgMouse && lgMouseMove && lgMouseHandle) {
            const int result = lgMouseMove(lgMouseHandle, x, y);
            LogMoveResult("LgMouseMove", x, y, result);
        }
        else if (api_mode == ApiMode::Dd && ddMoveR) {
            const int result = ddMoveR(x, y);
            LogMoveResult("DD_movR", x, y, result);
        }
    }

    void Click(int button) override {
        if (api_mode == ApiMode::Standard && driverClick) {
            driverClick(button);
        }
        else if (api_mode == ApiMode::LogiDriver && legacyMouseDown && legacyMouseUp) {
            const int legacy_button = button + 1;
            legacyMouseDown(legacy_button);
            ::Sleep(10);
            legacyMouseUp(legacy_button);
        }
        else if (api_mode == ApiMode::LgMouse && lgMouseHandle) {
            if (button == 0 && lgMouseLeftClick) lgMouseLeftClick(lgMouseHandle, 10);
            else if (button == 1 && lgMouseRightClick) lgMouseRightClick(lgMouseHandle, 10);
            else if (button == 2 && lgMouseMiddleClick) lgMouseMiddleClick(lgMouseHandle, 10);
        }
        else if (api_mode == ApiMode::Dd && ddBtn) {
            int down_code = 0;
            int up_code = 0;
            if (button == 0) {
                down_code = 1;
                up_code = 2;
            }
            else if (button == 1) {
                down_code = 4;
                up_code = 8;
            }
            else if (button == 2) {
                down_code = 16;
                up_code = 32;
            }
            if (down_code && up_code) {
                ddBtn(down_code);
                ::Sleep(10);
                ddBtn(up_code);
            }
        }
    }

    void Scroll(int delta) override {
        if (delta == 0) {
            return;
        }
        if (api_mode == ApiMode::Standard && driverScroll) {
            driverScroll(delta);
        }
        else if (api_mode == ApiMode::LgMouse && lgMouseScroll && lgMouseHandle) {
            lgMouseScroll(lgMouseHandle, delta);
        }
        else if (api_mode == ApiMode::Dd && ddWhl) {
            ddWhl(delta);
        }
        else if (api_mode == ApiMode::LogiDriver && !legacyScrollWarned) {
            std::cerr << "\n[WARN] LogiDriver legacy ABI has no scroll export; scroll ignored." << std::endl;
            legacyScrollWarned = true;
        }
    }

    void KeyDown(int vk) override {
        SendKeyEvent(vk, true);
    }

    void KeyUp(int vk) override {
        SendKeyEvent(vk, false);
    }

    const char* Name() const override {
        switch (api_mode) {
        case ApiMode::Standard: return "DriverBackend(StandardABI)";
        case ApiMode::LogiDriver: return "DriverBackend(LogiDriverLegacyABI)";
        case ApiMode::LgMouse: return "DriverBackend(LgMouseLegacyABI)";
        case ApiMode::Dd: return "DriverBackend(DDABI)";
        default: return "DriverBackend(Unbound)";
        }
    }

    bool IsDriverBackend() const override {
        return true;
    }

private:
    enum class ApiMode {
        None,
        Standard,
        LogiDriver,
        LgMouse,
        Dd
    };

    using DriverInitFn = bool(__stdcall*)();
    using DriverMoveFn = void(__stdcall*)(int, int);
    using DriverClickFn = void(__stdcall*)(int);
    using DriverScrollFn = void(__stdcall*)(int);
    using DriverCleanupFn = void(__stdcall*)();

    using LegacyOpenFn = int(__stdcall*)();
    using LegacyOpen2Fn = int(__stdcall*)(int);
    using LegacyCloseFn = void(__stdcall*)();
    using LegacyMoveFn = int(__stdcall*)(int, int);
    using LegacyButtonFn = int(__stdcall*)(int);
    using LegacyGetDriverTypeFn = int(__stdcall*)();

    using LgMouseHandle = void*;
    using LgMouseOpenFn = LgMouseHandle(__stdcall*)();
    using LgMouseCloseFn = void(__stdcall*)(LgMouseHandle);
    using LgMouseMoveFn = int(__stdcall*)(LgMouseHandle, int, int);
    using LgMouseClickFn = int(__stdcall*)(LgMouseHandle, int);
    using LgMouseScrollFn = int(__stdcall*)(LgMouseHandle, int);

    using DdBtnFn = int(__stdcall*)(int);
    using DdMoveFn = int(__stdcall*)(int, int);
    using DdWhlFn = int(__stdcall*)(int);
    using DdKeyFn = int(__stdcall*)(int, int);
    using DdToDcFn = int(__stdcall*)(int);

    bool BindStandardAbi() {
        driverInit = reinterpret_cast<DriverInitFn>(::GetProcAddress(hDll, "DriverInit"));
        driverMove = reinterpret_cast<DriverMoveFn>(::GetProcAddress(hDll, "DriverMove"));
        driverClick = reinterpret_cast<DriverClickFn>(::GetProcAddress(hDll, "DriverClick"));
        driverScroll = reinterpret_cast<DriverScrollFn>(::GetProcAddress(hDll, "DriverScroll"));
        driverCleanup = reinterpret_cast<DriverCleanupFn>(::GetProcAddress(hDll, "DriverCleanup"));
        if (!driverInit || !driverMove || !driverClick || !driverScroll || !driverCleanup) {
            return false;
        }
        if (!driverInit()) {
            throw std::runtime_error("DriverInit returned false.");
        }
        std::cout << "--- Driver ABI selected: StandardABI exports DriverMove/DriverClick/DriverScroll ---" << std::endl;
        api_mode = ApiMode::Standard;
        initialized = true;
        return true;
    }

    bool BindLogiDriverAbi() {
        legacyOpen = reinterpret_cast<LegacyOpenFn>(::GetProcAddress(hDll, "device_open"));
        legacyOpen2 = reinterpret_cast<LegacyOpen2Fn>(::GetProcAddress(hDll, "device_open2"));
        legacyClose = reinterpret_cast<LegacyCloseFn>(::GetProcAddress(hDll, "device_close"));
        legacyMove = reinterpret_cast<LegacyMoveFn>(::GetProcAddress(hDll, "moveR"));
        legacyMouseDown = reinterpret_cast<LegacyButtonFn>(::GetProcAddress(hDll, "mouse_down"));
        legacyMouseUp = reinterpret_cast<LegacyButtonFn>(::GetProcAddress(hDll, "mouse_up"));
        legacyGetDriverType = reinterpret_cast<LegacyGetDriverTypeFn>(::GetProcAddress(hDll, "get_driver_type"));
        if (!legacyOpen || !legacyClose || !legacyMove || !legacyMouseDown || !legacyMouseUp) {
            return false;
        }
        int open_result = 0;
        if (legacyOpen2) {
            open_result = legacyOpen2(preferred_driver_type);
            std::cout << "--- LogiDriver device_open2(" << preferred_driver_type << ") => "
                << open_result << " ---" << std::endl;
        }
        else {
            open_result = legacyOpen();
            std::cout << "--- LogiDriver device_open() => " << open_result << " ---" << std::endl;
        }
        if (!open_result) {
            throw std::runtime_error("device_open returned 0.");
        }
        if (legacyGetDriverType) {
            std::cout << "--- LogiDriver legacy driver_type=" << legacyGetDriverType() << " ---" << std::endl;
        }
        std::cout << "--- Driver ABI selected: LogiDriverLegacyABI export moveR ---" << std::endl;
        api_mode = ApiMode::LogiDriver;
        initialized = true;
        return true;
    }

    bool BindLgMouseAbi() {
        lgMouseOpen = reinterpret_cast<LgMouseOpenFn>(::GetProcAddress(hDll, "LgMouseOpen"));
        lgMouseClose = reinterpret_cast<LgMouseCloseFn>(::GetProcAddress(hDll, "LgMouseClose"));
        lgMouseMove = reinterpret_cast<LgMouseMoveFn>(::GetProcAddress(hDll, "LgMouseMoveEx"));
        if (!lgMouseMove) {
            lgMouseMove = reinterpret_cast<LgMouseMoveFn>(::GetProcAddress(hDll, "LgMouseMove"));
        }
        lgMouseLeftClick = reinterpret_cast<LgMouseClickFn>(::GetProcAddress(hDll, "LgMouseLeftClick"));
        lgMouseRightClick = reinterpret_cast<LgMouseClickFn>(::GetProcAddress(hDll, "LgMouseRightClick"));
        lgMouseMiddleClick = reinterpret_cast<LgMouseClickFn>(::GetProcAddress(hDll, "LgMouseMiddleClick"));
        lgMouseScroll = reinterpret_cast<LgMouseScrollFn>(::GetProcAddress(hDll, "LgMouseScroll"));
        if (!lgMouseOpen || !lgMouseClose || !lgMouseMove || !lgMouseLeftClick || !lgMouseRightClick || !lgMouseMiddleClick || !lgMouseScroll) {
            return false;
        }
        lgMouseHandle = lgMouseOpen();
        if (!lgMouseHandle) {
            throw std::runtime_error("LgMouseOpen returned null.");
        }
        std::cout << "--- LgMouse legacy handle opened ---" << std::endl;
        std::cout << "--- Driver ABI selected: LgMouseLegacyABI export LgMouseMove/LgMouseMoveEx ---" << std::endl;
        api_mode = ApiMode::LgMouse;
        initialized = true;
        return true;
    }

    bool BindDdAbi() {
        ddBtn = reinterpret_cast<DdBtnFn>(::GetProcAddress(hDll, "DD_btn"));
        ddMoveR = reinterpret_cast<DdMoveFn>(::GetProcAddress(hDll, "DD_movR"));
        ddMoveAbs = reinterpret_cast<DdMoveFn>(::GetProcAddress(hDll, "DD_mov"));
        ddWhl = reinterpret_cast<DdWhlFn>(::GetProcAddress(hDll, "DD_whl"));
        ddKey = reinterpret_cast<DdKeyFn>(::GetProcAddress(hDll, "DD_key"));
        ddToDc = reinterpret_cast<DdToDcFn>(::GetProcAddress(hDll, "DD_todc"));
        if (!ddBtn || !ddMoveR) {
            return false;
        }
        const int init_result = ddBtn(0);
        std::cout << "--- DD driver DD_btn(0) => " << init_result << " ---" << std::endl;
        if (init_result != 1) {
            throw std::runtime_error("DD_btn(0) did not return 1.");
        }
        std::cout << "--- Driver ABI selected: DDABI export DD_movR/DD_btn ---" << std::endl;
        if (ddKey && ddToDc) {
            std::cout << "--- DD keyboard input enabled: DD_todc + DD_key ---" << std::endl;
        }
        else {
            std::cerr << "\n[WARN] DD keyboard exports DD_todc/DD_key are missing; key events will use SendInput fallback." << std::endl;
        }
        api_mode = ApiMode::Dd;
        initialized = true;
        return true;
    }

    int DdCodeFromVk(int vk) const {
        if (!ddToDc) {
            return -1;
        }
        int dd_code = ddToDc(vk);
        if (dd_code == -1) {
            if (vk == VK_LSHIFT || vk == VK_RSHIFT) {
                dd_code = ddToDc(VK_SHIFT);
            }
            else if (vk == VK_LCONTROL || vk == VK_RCONTROL) {
                dd_code = ddToDc(VK_CONTROL);
            }
            else if (vk == VK_LMENU || vk == VK_RMENU) {
                dd_code = ddToDc(VK_MENU);
            }
        }
        return dd_code;
    }

    void SendKeyEvent(int vk, bool key_down) {
        if (vk <= 0) {
            return;
        }
        if (api_mode == ApiMode::Dd && ddKey && ddToDc) {
            const int dd_code = DdCodeFromVk(vk);
            if (dd_code != -1) {
                const int result = ddKey(dd_code, key_down ? 1 : 2);
                LogKeyResult("DD_key", vk, dd_code, key_down, result);
                return;
            }
            if (!keyboardFallbackWarned) {
                std::cerr << "\n[WARN] DD_todc does not support " << KeyNameFromVk(vk)
                    << "; key event will use SendInput fallback." << std::endl;
                keyboardFallbackWarned = true;
            }
        }
        else if (!keyboardFallbackWarned) {
            std::cerr << "\n[WARN] Active driver backend has no keyboard ABI; key events will use SendInput fallback." << std::endl;
            keyboardFallbackWarned = true;
        }

        SendKeyboardInputVk(vk, key_down, Name());
    }

    void LogMoveCall(const char* function_name, int x, int y) {
        if (moveLogBudget > 0) {
            std::cout << "\n[DRIVER] " << function_name << "(" << x << "," << y << ") => called" << std::endl;
            --moveLogBudget;
        }
    }

    void LogMoveResult(const char* function_name, int x, int y, int result) {
        if (moveLogBudget > 0 || result == 0) {
            std::cout << "\n[DRIVER] " << function_name << "(" << x << "," << y << ") => "
                << result << std::endl;
            if (moveLogBudget > 0) {
                --moveLogBudget;
            }
        }
    }

    void LogKeyResult(const char* function_name, int vk, int dd_code, bool key_down, int result) {
        if (keyLogBudget > 0 || result == 0) {
            std::cout << "\n[DRIVER] " << function_name << "("
                << KeyNameFromVk(vk) << ", dd=" << dd_code
                << ", " << (key_down ? "down" : "up") << ") => "
                << result << std::endl;
            if (keyLogBudget > 0) {
                --keyLogBudget;
            }
        }
    }

    void CleanupLibrary() {
        if (hDll) {
            ::FreeLibrary(hDll);
            hDll = nullptr;
        }
        driverInit = nullptr;
        driverMove = nullptr;
        driverClick = nullptr;
        driverScroll = nullptr;
        driverCleanup = nullptr;
        legacyOpen = nullptr;
        legacyOpen2 = nullptr;
        legacyClose = nullptr;
        legacyMove = nullptr;
        legacyMouseDown = nullptr;
        legacyMouseUp = nullptr;
        legacyGetDriverType = nullptr;
        lgMouseOpen = nullptr;
        lgMouseClose = nullptr;
        lgMouseMove = nullptr;
        lgMouseLeftClick = nullptr;
        lgMouseRightClick = nullptr;
        lgMouseMiddleClick = nullptr;
        lgMouseScroll = nullptr;
        ddBtn = nullptr;
        ddMoveR = nullptr;
        ddMoveAbs = nullptr;
        ddWhl = nullptr;
        ddKey = nullptr;
        ddToDc = nullptr;
        initialized = false;
        api_mode = ApiMode::None;
    }

    HMODULE hDll = nullptr;
    ApiMode api_mode = ApiMode::None;
    int preferred_driver_type = 0;
    DriverInitFn driverInit = nullptr;
    DriverMoveFn driverMove = nullptr;
    DriverClickFn driverClick = nullptr;
    DriverScrollFn driverScroll = nullptr;
    DriverCleanupFn driverCleanup = nullptr;
    LegacyOpenFn legacyOpen = nullptr;
    LegacyOpen2Fn legacyOpen2 = nullptr;
    LegacyCloseFn legacyClose = nullptr;
    LegacyMoveFn legacyMove = nullptr;
    LegacyButtonFn legacyMouseDown = nullptr;
    LegacyButtonFn legacyMouseUp = nullptr;
    LegacyGetDriverTypeFn legacyGetDriverType = nullptr;
    LgMouseOpenFn lgMouseOpen = nullptr;
    LgMouseCloseFn lgMouseClose = nullptr;
    LgMouseMoveFn lgMouseMove = nullptr;
    LgMouseClickFn lgMouseLeftClick = nullptr;
    LgMouseClickFn lgMouseRightClick = nullptr;
    LgMouseClickFn lgMouseMiddleClick = nullptr;
    LgMouseScrollFn lgMouseScroll = nullptr;
    LgMouseHandle lgMouseHandle = nullptr;
    DdBtnFn ddBtn = nullptr;
    DdMoveFn ddMoveR = nullptr;
    DdMoveFn ddMoveAbs = nullptr;
    DdWhlFn ddWhl = nullptr;
    DdKeyFn ddKey = nullptr;
    DdToDcFn ddToDc = nullptr;
    bool initialized = false;
    bool legacyScrollWarned = false;
    bool keyboardFallbackWarned = false;
    int moveLogBudget = 6;
    int keyLogBudget = 10;
};

class MouseController {
public:
    explicit MouseController(const Config& cfg) {
        if (cfg.input_backend == "driver") {
            try {
                backend = std::make_unique<DriverBackend>(cfg.driver_dll_path, cfg.driver_type);
            }
            catch (const std::exception& e) {
                std::cerr << "\n[WARN] DriverBackend unavailable: " << e.what()
                    << " Falling back to SendInputBackend." << std::endl;
            }
        }
        if (!backend) {
            backend = std::make_unique<SendInputBackend>();
        }
        movement_worker = std::thread(&MouseController::MovementWorkerLoop, this);
        std::cout << "--- Active mouse backend: " << backend->Name() << " ---" << std::endl;
    }

    ~MouseController() {
        movement_worker_stop.store(true);
        {
            std::lock_guard<std::mutex> lock(movement_mutex);
            pending_move_x = 0;
            pending_move_y = 0;
        }
        movement_cv.notify_one();
        if (movement_worker.joinable()) {
            movement_worker.join();
        }
        std::cout << "--- Mouse input resources cleaned up. ---" << std::endl;
    }

    MouseController(const MouseController&) = delete;
    MouseController& operator=(const MouseController&) = delete;

    void MoveRelative(int dx, int dy, const Config* movement_cfg = nullptr) {
        if (!backend || (dx == 0 && dy == 0)) {
            return;
        }
        if (movement_cfg && movement_cfg->enable_humanized_movement) {
            QueueHumanizedMove(dx, dy, *movement_cfg);
            return;
        }
        CancelHumanizedMove();
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->MoveMouse(dx, dy);
    }

    void Click(int button) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->Click(button);
    }

    void Scroll(int delta) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->Scroll(delta);
    }

    void KeyDown(int vk) {
        if (vk <= 0 || !backend) {
            return;
        }
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->KeyDown(vk);
    }

    void KeyUp(int vk) {
        if (vk <= 0 || !backend) {
            return;
        }
        std::lock_guard<std::mutex> lock(backend_mutex);
        backend->KeyUp(vk);
    }

    void PressKeys(const std::vector<int>& keys) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        for (int vk : keys) {
            if (vk > 0) {
                backend->KeyDown(vk);
            }
        }
    }

    void ReleaseKeys(const std::vector<int>& keys) {
        std::lock_guard<std::mutex> lock(backend_mutex);
        for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
            if (*it > 0) {
                backend->KeyUp(*it);
            }
        }
    }

    const char* BackendName() const {
        return backend ? backend->Name() : "None";
    }

    bool IsDriverBackend() const {
        return backend && backend->IsDriverBackend();
    }

private:
    void QueueHumanizedMove(int dx, int dy, const Config& movement_cfg) {
        {
            std::lock_guard<std::mutex> lock(movement_mutex);
            pending_move_x += dx;
            pending_move_y += dy;
            pending_movement_cfg = movement_cfg;
            pending_movement_generation = movement_generation.fetch_add(1, std::memory_order_relaxed) + 1;
        }
        movement_cv.notify_one();
    }

    void CancelHumanizedMove() {
        {
            std::lock_guard<std::mutex> lock(movement_mutex);
            pending_move_x = 0;
            pending_move_y = 0;
            pending_movement_generation = movement_generation.fetch_add(1, std::memory_order_relaxed) + 1;
        }
        movement_cv.notify_one();
    }

    void MovementWorkerLoop() {
        while (!movement_worker_stop.load()) {
            int dx = 0;
            int dy = 0;
            Config movement_cfg;
            uint64_t generation = 0;
            {
                std::unique_lock<std::mutex> lock(movement_mutex);
                movement_cv.wait(lock, [this]() {
                    return movement_worker_stop.load() || pending_move_x != 0 || pending_move_y != 0;
                });
                if (movement_worker_stop.load()) {
                    break;
                }
                dx = pending_move_x;
                dy = pending_move_y;
                pending_move_x = 0;
                pending_move_y = 0;
                movement_cfg = pending_movement_cfg;
                generation = pending_movement_generation;
            }
            MoveRelativeHumanizedBlocking(dx, dy, movement_cfg, generation);
        }
    }

    void MoveRelativeHumanizedBlocking(int dx, int dy, const Config& movement_cfg, uint64_t generation) {
        const double distance = std::hypot(static_cast<double>(dx), static_cast<double>(dy));
        const double max_step = std::max(1.0, movement_cfg.human_move_max_step);
        const int segments = std::clamp(static_cast<int>(std::ceil(distance / max_step)), 1, 120);
        const double jitter = std::max(0.0, movement_cfg.human_move_jitter);
        int delay_min = std::clamp(movement_cfg.human_move_delay_min_ms, 0, 100);
        int delay_max = std::clamp(movement_cfg.human_move_delay_max_ms, 0, 100);
        if (delay_min > delay_max) {
            std::swap(delay_min, delay_max);
        }

        std::uniform_real_distribution<double> jitter_dist(-jitter, jitter);
        std::uniform_int_distribution<int> delay_dist(delay_min, delay_max);
        double previous_x = 0.0;
        double previous_y = 0.0;
        int applied_x = 0;
        int applied_y = 0;

        for (int segment = 1; segment <= segments && !movement_worker_stop.load(); ++segment) {
            if (generation != movement_generation.load(std::memory_order_relaxed)) {
                break;
            }
            const double t = static_cast<double>(segment) / static_cast<double>(segments);
            const double eased_t = t * t * (3.0 - 2.0 * t);
            const double current_x = static_cast<double>(dx) * eased_t;
            const double current_y = static_cast<double>(dy) * eased_t;
            double step_x = current_x - previous_x;
            double step_y = current_y - previous_y;

            if (segment < segments && jitter > 0.0) {
                step_x += jitter_dist(rng);
                step_y += jitter_dist(rng);
            }

            int move_x = static_cast<int>(std::round(step_x));
            int move_y = static_cast<int>(std::round(step_y));
            if (segment == segments) {
                move_x = dx - applied_x;
                move_y = dy - applied_y;
            }

            if (move_x != 0 || move_y != 0) {
                std::lock_guard<std::mutex> lock(backend_mutex);
                backend->MoveMouse(move_x, move_y);
                applied_x += move_x;
                applied_y += move_y;
            }

            previous_x = current_x;
            previous_y = current_y;
            if (segment < segments && delay_max > 0) {
                std::unique_lock<std::mutex> lock(movement_mutex);
                movement_cv.wait_for(lock, std::chrono::milliseconds(delay_dist(rng)), [this, generation]() {
                    return movement_worker_stop.load() ||
                        generation != movement_generation.load(std::memory_order_relaxed);
                });
            }
        }
    }

    std::unique_ptr<InputBackend> backend;
    std::mt19937 rng{ std::random_device{}() };
    std::mutex backend_mutex;
    std::mutex movement_mutex;
    std::condition_variable movement_cv;
    std::thread movement_worker;
    std::atomic_bool movement_worker_stop{ false };
    std::atomic<uint64_t> movement_generation{ 0 };
    Config pending_movement_cfg;
    uint64_t pending_movement_generation = 0;
    int pending_move_x = 0;
    int pending_move_y = 0;
};

// --- 3. 屏幕捕捉器 (ScreenCapturer) - 优化版本 ---
class ScreenCapturer {
public:
    ScreenCapturer(int crop_width, int crop_height) {
        std::cout << "--- Initializing D3D for screen capture... ---" << std::endl;
        HRESULT hr;

        hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory);
        if (FAILED(hr)) throw std::runtime_error("Failed to create DXGI Factory.");

        DXGI_OUTPUT_DESC outputDesc{};
        SelectPrimaryOutput(outputDesc);
        width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
        height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

        if (FAILED(D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &pDevice, nullptr, &pContext))) {
            throw std::runtime_error("Failed to create D3D11 device.");
        }
        if (FAILED(pOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&pOutput1))) {
            throw std::runtime_error("Failed to query IDXGIOutput1.");
        }
        if (FAILED(pOutput1->DuplicateOutput(pDevice, &pDuplicator))) {
            throw std::runtime_error("Failed to create output duplication.");
        }

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = crop_width;
        desc.Height = crop_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        hr = pDevice->CreateTexture2D(&desc, NULL, &m_pStagingTexture);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create staging texture for cropping.");
        }
        std::cout << "--- Screen capture initialized successfully (" << width << "x" << height
            << "), desktop_rect=(" << outputDesc.DesktopCoordinates.left << ","
            << outputDesc.DesktopCoordinates.top << ")-("
            << outputDesc.DesktopCoordinates.right << ","
            << outputDesc.DesktopCoordinates.bottom << ") ---" << std::endl;
    }

    ~ScreenCapturer() {
        std::cout << "--- Cleaning up D3D resources... ---" << std::endl;
        SafeRelease(&m_pStagingTexture);
        SafeRelease(&pDuplicator);
        SafeRelease(&pOutput1);
        SafeRelease(&pOutput);
        SafeRelease(&pAdapter);
        SafeRelease(&pFactory);
        SafeRelease(&pContext);
        SafeRelease(&pDevice);
    }

    ScreenCapturer(const ScreenCapturer&) = delete;
    ScreenCapturer& operator=(const ScreenCapturer&) = delete;

    bool CaptureFrame(cv::Mat& frame, const cv::Rect& crop_region) {
        IDXGIResource* pDesktopResource = nullptr;
        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        HRESULT hr = pDuplicator->AcquireNextFrame(16, &frameInfo, &pDesktopResource);

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;
        if (FAILED(hr)) {
            std::cerr << "AcquireNextFrame failed. HRESULT: 0x" << std::hex << hr << std::endl;
            return false;
        }

        ID3D11Texture2D* pAcquiredDesktopImage = nullptr;
        hr = pDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pAcquiredDesktopImage);
        SafeRelease(&pDesktopResource);
        if (FAILED(hr)) {
            pDuplicator->ReleaseFrame();
            return false;
        }

        D3D11_BOX sourceRegion;
        sourceRegion.left = crop_region.x;
        sourceRegion.right = crop_region.x + crop_region.width;
        sourceRegion.top = crop_region.y;
        sourceRegion.bottom = crop_region.y + crop_region.height;
        sourceRegion.front = 0;
        sourceRegion.back = 1;

        pContext->CopySubresourceRegion(m_pStagingTexture, 0, 0, 0, 0, pAcquiredDesktopImage, 0, &sourceRegion);

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = pContext->Map(m_pStagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (FAILED(hr)) {
            SafeRelease(&pAcquiredDesktopImage);
            pDuplicator->ReleaseFrame();
            return false;
        }

        cv::Mat bgra_frame(crop_region.height, crop_region.width, CV_8UC4, mappedResource.pData, mappedResource.RowPitch);
        cv::cvtColor(bgra_frame, frame, cv::COLOR_BGRA2BGR);

        pContext->Unmap(m_pStagingTexture, 0);
        SafeRelease(&pAcquiredDesktopImage);
        pDuplicator->ReleaseFrame();
        return true;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    void SelectPrimaryOutput(DXGI_OUTPUT_DESC& selected_desc) {
        const RECT primary_rect{
            0,
            0,
            ::GetSystemMetrics(SM_CXSCREEN),
            ::GetSystemMetrics(SM_CYSCREEN)
        };
        const POINT primary_center{
            (primary_rect.right - primary_rect.left) / 2,
            (primary_rect.bottom - primary_rect.top) / 2
        };

        IDXGIAdapter1* fallback_adapter = nullptr;
        IDXGIOutput* fallback_output = nullptr;
        DXGI_OUTPUT_DESC fallback_desc{};

        for (UINT adapter_index = 0;; ++adapter_index) {
            IDXGIAdapter1* candidate_adapter = nullptr;
            HRESULT adapter_hr = pFactory->EnumAdapters1(adapter_index, &candidate_adapter);
            if (adapter_hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(adapter_hr) || !candidate_adapter) {
                continue;
            }

            bool adapter_taken = false;
            for (UINT output_index = 0;; ++output_index) {
                IDXGIOutput* candidate_output = nullptr;
                HRESULT output_hr = candidate_adapter->EnumOutputs(output_index, &candidate_output);
                if (output_hr == DXGI_ERROR_NOT_FOUND) {
                    break;
                }
                if (FAILED(output_hr) || !candidate_output) {
                    continue;
                }

                DXGI_OUTPUT_DESC desc{};
                candidate_output->GetDesc(&desc);
                const RECT& rect = desc.DesktopCoordinates;
                const bool contains_primary_center =
                    primary_center.x >= rect.left &&
                    primary_center.x < rect.right &&
                    primary_center.y >= rect.top &&
                    primary_center.y < rect.bottom;

                if (contains_primary_center) {
                    if (fallback_output) {
                        SafeRelease(&fallback_output);
                        SafeRelease(&fallback_adapter);
                    }
                    pAdapter = candidate_adapter;
                    pOutput = candidate_output;
                    selected_desc = desc;
                    adapter_taken = true;
                    std::cout << "--- Primary display selected: "
                        << (primary_rect.right - primary_rect.left) << "x"
                        << (primary_rect.bottom - primary_rect.top) << " ---" << std::endl;
                    return;
                }
                else if (!fallback_output) {
                    fallback_adapter = candidate_adapter;
                    fallback_output = candidate_output;
                    fallback_desc = desc;
                    adapter_taken = true;
                }
                else {
                    SafeRelease(&candidate_output);
                }
            }

            if (!adapter_taken) {
                SafeRelease(&candidate_adapter);
            }
        }

        if (!fallback_output || !fallback_adapter) {
            throw std::runtime_error("Failed to enumerate outputs.");
        }

        pAdapter = fallback_adapter;
        pOutput = fallback_output;
        selected_desc = fallback_desc;
        std::cout << "--- Primary display not matched; using first DXGI output. Windows primary="
            << (primary_rect.right - primary_rect.left) << "x"
            << (primary_rect.bottom - primary_rect.top) << " ---" << std::endl;
    }

    ID3D11Texture2D* m_pStagingTexture = nullptr;
    IDXGIFactory1* pFactory = nullptr;
    IDXGIAdapter1* pAdapter = nullptr;
    IDXGIOutput* pOutput = nullptr;
    IDXGIOutput1* pOutput1 = nullptr;
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;
    IDXGIOutputDuplication* pDuplicator = nullptr;
    int width = 0;
    int height = 0;
};

static cv::Rect CenterCropRegion(int screen_width, int screen_height, int crop_size) {
    return cv::Rect(
        (screen_width - crop_size) / 2,
        (screen_height - crop_size) / 2,
        crop_size,
        crop_size);
}

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

static std::unique_ptr<FrameSource> CreateFrameSource(const Config& cfg) {
    if (cfg.enable_async_capture) {
        return std::make_unique<LatestFrameSource>(cfg.crop_size);
    }
    return std::make_unique<DirectFrameSource>(cfg.crop_size);
}

// --- 4. 目标检测器 (ObjectDetector) ---
class ObjectDetector {
public:
    ObjectDetector(const Config& cfg)
        : env(ORT_LOGGING_LEVEL_WARNING, "Realtime_YOLO_Detector"), session(nullptr) {

        std::cout << "--- Initializing ONNX Runtime and YOLO model... ---" << std::endl;
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        if (cfg.backend == "tensorrt") {
            OrtTensorRTProviderOptions trt_options{};
            trt_options.device_id = 0;
            trt_options.trt_max_partition_iterations = 1000;
            trt_options.trt_min_subgraph_size = 1;
            trt_options.trt_fp16_enable = 1;
            trt_options.trt_engine_cache_enable = 1;
            trt_options.trt_engine_cache_path = cfg.trt_cache_path;
            session_options.AppendExecutionProvider_TensorRT(trt_options);

            OrtCUDAProviderOptions cuda_options{};
            cuda_options.device_id = 0;
            session_options.AppendExecutionProvider_CUDA(cuda_options);
            std::cout << "--- Backend: TensorRT with CUDA fallback ---" << std::endl;
        }
        else if (cfg.backend == "cuda") {
            OrtCUDAProviderOptions cuda_options{};
            cuda_options.device_id = 0;
            session_options.AppendExecutionProvider_CUDA(cuda_options);
            std::cout << "--- Backend: CUDA ---" << std::endl;
        }
        else {
            std::cout << "--- Backend: CPU ---" << std::endl;
        }

        session = Ort::Session(env, cfg.model_path.c_str(), session_options);
        Ort::AllocatorWithDefaultOptions allocator;
        auto input_name_ptr = session.GetInputNameAllocated(0, allocator);
        auto output_name_ptr = session.GetOutputNameAllocated(0, allocator);
        input_name = input_name_ptr.get();
        output_name = output_name_ptr.get();

        input_shape = session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
        if (input_shape.size() != 4 || input_shape[2] <= 0 || input_shape[3] <= 0) {
            throw std::runtime_error("Unsupported YOLO input shape.");
        }
        input_height = input_shape[2];
        input_width = input_shape[3];
        canvas.create(cv::Size(static_cast<int>(input_width), static_cast<int>(input_height)), CV_8UC3);
        boxes.reserve(1024);
        confidences.reserve(1024);
        class_ids.reserve(1024);
        nms_indices.reserve(256);
        std::cout << "--- Model loaded successfully! ---" << std::endl;
    }

    ObjectDetector(const ObjectDetector&) = delete;
    ObjectDetector& operator=(const ObjectDetector&) = delete;

    void Detect(const cv::Mat& image, std::vector<Detection>& detections, const Config& cfg, TimingDetails& timings) {
        detections.clear();
        if (detections.capacity() < 128) {
            detections.reserve(128);
        }
        auto stage_start = std::chrono::high_resolution_clock::now();

        float ratio_h = static_cast<float>(input_height) / image.rows;
        float ratio_w = static_cast<float>(input_width) / image.cols;
        float ratio = std::min(ratio_h, ratio_w);
        int new_w = static_cast<int>(image.cols * ratio);
        int new_h = static_cast<int>(image.rows * ratio);

        cv::resize(image, resized_img, cv::Size(new_w, new_h));

        canvas.setTo(cv::Scalar(114, 114, 114));
        int paste_x = (static_cast<int>(input_width) - new_w) / 2;
        int paste_y = (static_cast<int>(input_height) - new_h) / 2;
        resized_img.copyTo(canvas(cv::Rect(paste_x, paste_y, new_w, new_h)));

        cv::dnn::blobFromImage(canvas, blob, 1.0 / 255.0, cv::Size(input_width, input_height), cv::Scalar(), true, false);

        auto stage_end_preprocess = std::chrono::high_resolution_clock::now();
        timings.preprocess_ms = std::chrono::duration_cast<std::chrono::microseconds>(stage_end_preprocess - stage_start).count() / 1000.0;

        const char* input_names[] = { input_name.c_str() };
        const char* output_names[] = { output_name.c_str() };
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, blob.ptr<float>(), blob.total(), input_shape.data(), input_shape.size());

        auto output_tensors = session.Run(run_options, input_names, &input_tensor, 1, output_names, 1);
        auto stage_end_inference = std::chrono::high_resolution_clock::now();
        timings.inference_ms = std::chrono::duration_cast<std::chrono::microseconds>(stage_end_inference - stage_end_preprocess).count() / 1000.0;

        if (cfg.use_end_to_end_onnx) {
            const float* output_data = output_tensors.front().GetTensorData<float>();
            const auto& output_shape = output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
            const int num_detections = static_cast<int>(output_shape[1]);

            for (int i = 0; i < num_detections; ++i) {
                const float confidence = output_data[i * 6 + 4];
                if (confidence >= cfg.confidence_threshold) {
                    const float x1 = output_data[i * 6 + 0];
                    const float y1 = output_data[i * 6 + 1];
                    const float x2 = output_data[i * 6 + 2];
                    const float y2 = output_data[i * 6 + 3];
                    const int class_id = static_cast<int>(output_data[i * 6 + 5]);
                    int left = static_cast<int>((x1 - paste_x) / ratio);
                    int top = static_cast<int>((y1 - paste_y) / ratio);
                    int width = static_cast<int>((x2 - x1) / ratio);
                    int height = static_cast<int>((y2 - y1) / ratio);
                    detections.emplace_back(Detection{ cv::Rect(left, top, width, height), confidence, class_id });
                }
            }
        }
        else {
            const float* output_data = output_tensors.front().GetTensorData<float>();
            const auto& output_shape = output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
            cv::Mat raw_output(static_cast<int>(output_shape[1]), static_cast<int>(output_shape[2]), CV_32F, (void*)output_data);
            cv::transpose(raw_output, output_transposed);
            const cv::Mat& output_mat = output_transposed;

            boxes.clear();
            confidences.clear();
            class_ids.clear();
            if (boxes.capacity() < static_cast<size_t>(output_mat.rows)) {
                boxes.reserve(output_mat.rows);
                confidences.reserve(output_mat.rows);
                class_ids.reserve(output_mat.rows);
            }
            for (int i = 0; i < output_mat.rows; ++i) {
                cv::Mat classes_scores = output_mat.row(i).colRange(4, output_mat.cols);
                cv::Point class_id_point;
                double max_score;
                cv::minMaxLoc(classes_scores, 0, &max_score, 0, &class_id_point);
                if (max_score > cfg.confidence_threshold) {
                    confidences.push_back(static_cast<float>(max_score));
                    class_ids.push_back(class_id_point.x);
                    float cx = output_mat.at<float>(i, 0);
                    float cy = output_mat.at<float>(i, 1);
                    float w = output_mat.at<float>(i, 2);
                    float h = output_mat.at<float>(i, 3);
                    int left = static_cast<int>((cx - 0.5f * w - paste_x) / ratio);
                    int top = static_cast<int>((cy - 0.5f * h - paste_y) / ratio);
                    int width = static_cast<int>(w / ratio);
                    int height = static_cast<int>(h / ratio);
                    boxes.emplace_back(left, top, width, height);
                }
            }
            nms_indices.clear();
            cv::dnn::NMSBoxes(boxes, confidences, cfg.confidence_threshold, cfg.nms_threshold, nms_indices);
            for (int idx : nms_indices) {
                detections.emplace_back(Detection{ boxes[idx], confidences[idx], class_ids[idx] });
            }
        }
        auto stage_end_postprocess = std::chrono::high_resolution_clock::now();
        timings.postprocess_ms = std::chrono::duration_cast<std::chrono::microseconds>(stage_end_postprocess - stage_end_inference).count() / 1000.0;
    }

private:
    Ort::Env env;
    Ort::Session session;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::RunOptions run_options{ nullptr };
    std::string input_name;
    std::string output_name;
    std::vector<int64_t> input_shape;
    int64_t input_height = 0;
    int64_t input_width = 0;
    cv::Mat resized_img;
    cv::Mat canvas;
    cv::Mat blob;
    cv::Mat output_transposed;
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> class_ids;
    std::vector<int> nms_indices;
};

struct PidAxis {
    double integral = 0.0;
    double previous_error = 0.0;
    bool has_previous = false;

    void Reset() {
        integral = 0.0;
        previous_error = 0.0;
        has_previous = false;
    }

    double Update(double error, double dt, const Config& cfg) {
        dt = std::clamp(dt, 0.001, 0.1);
        integral += error * dt;
        if (cfg.pid_integral_limit > 0.0) {
            integral = std::clamp(integral, -cfg.pid_integral_limit, cfg.pid_integral_limit);
        }
        else {
            integral = 0.0;
        }

        const double derivative = has_previous ? (error - previous_error) / dt : 0.0;
        previous_error = error;
        has_previous = true;
        return cfg.pid_kp * error + cfg.pid_ki * integral + cfg.pid_kd * derivative;
    }
};

struct LowPassAxis {
    bool initialized = false;
    double value = 0.0;

    void Reset() {
        initialized = false;
        value = 0.0;
    }

    double Update(double input, double alpha) {
        alpha = std::clamp(alpha, 0.0, 1.0);
        if (!initialized) {
            value = input;
            initialized = true;
            return value;
        }
        value = alpha * input + (1.0 - alpha) * value;
        return value;
    }
};

struct OneEuroAxis {
    LowPassAxis signal;
    LowPassAxis derivative;
    double previous_raw = 0.0;
    bool has_previous_raw = false;

    void Reset() {
        signal.Reset();
        derivative.Reset();
        previous_raw = 0.0;
        has_previous_raw = false;
    }

    static double Alpha(double cutoff, double dt) {
        constexpr double pi = 3.14159265358979323846;
        cutoff = std::max(0.001, cutoff);
        dt = std::clamp(dt, 0.001, 0.1);
        const double tau = 1.0 / (2.0 * pi * cutoff);
        return 1.0 / (1.0 + tau / dt);
    }

    double Update(double input, double dt, const Config& cfg) {
        dt = std::clamp(dt, 0.001, 0.1);
        const double raw_derivative = has_previous_raw ? (input - previous_raw) / dt : 0.0;
        previous_raw = input;
        has_previous_raw = true;

        const double smoothed_derivative = derivative.Update(raw_derivative, Alpha(cfg.one_euro_d_cutoff, dt));
        const double cutoff = cfg.one_euro_min_cutoff + cfg.one_euro_beta * std::abs(smoothed_derivative);
        return signal.Update(input, Alpha(cutoff, dt));
    }
};

struct PredictionTracker {
    struct KalmanAxis {
        double position = 0.0;
        double velocity = 0.0;
        double p00 = 25.0;
        double p01 = 0.0;
        double p10 = 0.0;
        double p11 = 25.0;

        void Reset() {
            position = 0.0;
            velocity = 0.0;
            p00 = 25.0;
            p01 = 0.0;
            p10 = 0.0;
            p11 = 25.0;
        }

        void Initialize(double measurement) {
            Reset();
            position = measurement;
        }

        void Update(double measurement, double dt, double process_noise, double measurement_noise) {
            dt = std::clamp(dt, 0.001, 0.1);
            process_noise = std::max(0.001, process_noise);
            measurement_noise = std::max(0.001, measurement_noise);

            position += velocity * dt;

            const double dt2 = dt * dt;
            const double dt3 = dt2 * dt;
            const double dt4 = dt2 * dt2;
            const double q00 = 0.25 * dt4 * process_noise;
            const double q01 = 0.5 * dt3 * process_noise;
            const double q11 = dt2 * process_noise;

            const double predicted_p00 = p00 + dt * (p10 + p01) + dt2 * p11 + q00;
            const double predicted_p01 = p01 + dt * p11 + q01;
            const double predicted_p10 = p10 + dt * p11 + q01;
            const double predicted_p11 = p11 + q11;
            p00 = predicted_p00;
            p01 = predicted_p01;
            p10 = predicted_p10;
            p11 = predicted_p11;

            const double residual = measurement - position;
            const double s = p00 + measurement_noise;
            const double k0 = p00 / s;
            const double k1 = p10 / s;

            position += k0 * residual;
            velocity += k1 * residual;

            const double old_p00 = p00;
            const double old_p01 = p01;
            const double old_p10 = p10;
            const double old_p11 = p11;
            p00 = (1.0 - k0) * old_p00;
            p01 = (1.0 - k0) * old_p01;
            p10 = old_p10 - k1 * old_p00;
            p11 = old_p11 - k1 * old_p01;
        }
    };

    struct MotionSample {
        cv::Point2d point{ 0.0, 0.0 };
        std::chrono::steady_clock::time_point time{};
    };

    bool initialized = false;
    cv::Point2d position{ 0.0, 0.0 };
    cv::Point2d velocity{ 0.0, 0.0 };
    cv::Point2d acceleration{ 0.0, 0.0 };
    cv::Point2d lead{ 0.0, 0.0 };
    cv::Point2d last_measurement{ 0.0, 0.0 };
    std::chrono::steady_clock::time_point last_time{};
    bool lead_initialized = false;
    KalmanAxis kalman_x;
    KalmanAxis kalman_y;
    std::deque<MotionSample> samples;

    void Reset() {
        initialized = false;
        position = cv::Point2d(0.0, 0.0);
        velocity = cv::Point2d(0.0, 0.0);
        acceleration = cv::Point2d(0.0, 0.0);
        lead = cv::Point2d(0.0, 0.0);
        last_measurement = cv::Point2d(0.0, 0.0);
        last_time = {};
        lead_initialized = false;
        kalman_x.Reset();
        kalman_y.Reset();
        samples.clear();
    }

    void Initialize(const cv::Point2d& measurement, std::chrono::steady_clock::time_point now) {
        initialized = true;
        position = measurement;
        velocity = cv::Point2d(0.0, 0.0);
        acceleration = cv::Point2d(0.0, 0.0);
        lead = cv::Point2d(0.0, 0.0);
        last_measurement = measurement;
        last_time = now;
        lead_initialized = false;
        kalman_x.Initialize(measurement.x);
        kalman_y.Initialize(measurement.y);
        samples.clear();
        samples.push_back(MotionSample{ measurement, now });
    }

    void AddSample(const cv::Point2d& measurement, std::chrono::steady_clock::time_point now) {
        samples.push_back(MotionSample{ measurement, now });
        while (samples.size() > 12) {
            samples.pop_front();
        }
        while (samples.size() > 3) {
            const double age = std::chrono::duration<double>(now - samples.front().time).count();
            if (age <= 0.35) {
                break;
            }
            samples.pop_front();
        }
    }

    static bool Solve3x3(double matrix[3][4], double out[3]) {
        for (int pivot = 0; pivot < 3; ++pivot) {
            int best = pivot;
            for (int row = pivot + 1; row < 3; ++row) {
                if (std::abs(matrix[row][pivot]) > std::abs(matrix[best][pivot])) {
                    best = row;
                }
            }
            if (std::abs(matrix[best][pivot]) < 1e-9) {
                return false;
            }
            if (best != pivot) {
                for (int col = pivot; col < 4; ++col) {
                    std::swap(matrix[pivot][col], matrix[best][col]);
                }
            }
            const double divisor = matrix[pivot][pivot];
            for (int col = pivot; col < 4; ++col) {
                matrix[pivot][col] /= divisor;
            }
            for (int row = 0; row < 3; ++row) {
                if (row == pivot) {
                    continue;
                }
                const double factor = matrix[row][pivot];
                for (int col = pivot; col < 4; ++col) {
                    matrix[row][col] -= factor * matrix[pivot][col];
                }
            }
        }
        out[0] = matrix[0][3];
        out[1] = matrix[1][3];
        out[2] = matrix[2][3];
        return true;
    }

    bool EstimateAdaptive(cv::Point2d& estimated_position, cv::Point2d& estimated_velocity, cv::Point2d& estimated_acceleration) const {
        if (samples.empty()) {
            return false;
        }
        estimated_position = samples.back().point;
        if (samples.size() < 2) {
            estimated_velocity = cv::Point2d(0.0, 0.0);
            estimated_acceleration = cv::Point2d(0.0, 0.0);
            return false;
        }
        if (samples.size() == 2) {
            const MotionSample& a = samples[samples.size() - 2];
            const MotionSample& b = samples.back();
            const double dt = std::max(0.001, std::chrono::duration<double>(b.time - a.time).count());
            estimated_velocity = cv::Point2d((b.point.x - a.point.x) / dt, (b.point.y - a.point.y) / dt);
            estimated_acceleration = cv::Point2d(0.0, 0.0);
            return true;
        }

        const auto newest_time = samples.back().time;
        double normal_x[3][4]{};
        double normal_y[3][4]{};
        for (const MotionSample& sample : samples) {
            const double t = std::chrono::duration<double>(sample.time - newest_time).count();
            const double age = std::abs(t);
            const double weight = 1.0 / (1.0 + age * 18.0);
            const double terms[3] = { 1.0, t, t * t };
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    normal_x[row][col] += weight * terms[row] * terms[col];
                    normal_y[row][col] += weight * terms[row] * terms[col];
                }
                normal_x[row][3] += weight * sample.point.x * terms[row];
                normal_y[row][3] += weight * sample.point.y * terms[row];
            }
        }

        double coeff_x[3]{};
        double coeff_y[3]{};
        if (!Solve3x3(normal_x, coeff_x) || !Solve3x3(normal_y, coeff_y)) {
            return false;
        }
        estimated_position = samples.back().point;
        estimated_velocity = cv::Point2d(coeff_x[1], coeff_y[1]);
        estimated_acceleration = cv::Point2d(2.0 * coeff_x[2], 2.0 * coeff_y[2]);
        return true;
    }
};

struct DroneTrackingState {
    bool initialized = false;
    cv::Point2d last_measurement{ 0.0, 0.0 };
    cv::Point2d velocity{ 0.0, 0.0 };
    cv::Point2d last_error{ 0.0, 0.0 };
    cv::Point2d command{ 0.0, 0.0 };
    std::chrono::steady_clock::time_point last_time{};

    void Reset() {
        initialized = false;
        last_measurement = cv::Point2d(0.0, 0.0);
        velocity = cv::Point2d(0.0, 0.0);
        last_error = cv::Point2d(0.0, 0.0);
        command = cv::Point2d(0.0, 0.0);
        last_time = {};
    }

    void Initialize(const cv::Point2d& measurement, const cv::Point2d& error, std::chrono::steady_clock::time_point now) {
        initialized = true;
        last_measurement = measurement;
        velocity = cv::Point2d(0.0, 0.0);
        last_error = error;
        command = cv::Point2d(0.0, 0.0);
        last_time = now;
    }
};

// --- 5. 核心协调器 (AimAssistant) ---
class AimAssistant {
public:
    explicit AimAssistant(Config runtime_config)
        : cfg(std::move(runtime_config)),
        frame_source(CreateFrameSource(cfg)),
        detector(std::make_unique<ObjectDetector>(cfg)),
        mouse(std::make_unique<MouseController>(cfg)),
        is_visualizing(cfg.enable_visualization)
    {
        if (frame_source->getWidth() < cfg.crop_size || frame_source->getHeight() < cfg.crop_size) {
            throw std::runtime_error("Screen resolution is smaller than configured crop_size.");
        }
        updateCropGeometry(cfg.crop_size);
        if (is_visualizing) {
            cv::namedWindow(cfg.window_name, cv::WINDOW_AUTOSIZE);
        }
        if (!cfg.live_config_path.empty()) {
            std::wcout << L"--- Live config enabled: " << cfg.live_config_path << L" ---" << std::endl;
        }
        std::cout << "--- Detection will run on a centered " << cfg.crop_size << "x" << cfg.crop_size
            << " region using " << frame_source->ModeName() << " capture. ---" << std::endl;
        std::cout << "--- Switches: screen_capture=" << (cfg.enable_capture ? "on" : "off")
            << ", capture_mode=" << (cfg.enable_async_capture ? "latest" : "direct")
            << ", mouse_move=" << (cfg.enable_mouse_movement ? "on" : "off")
            << ", hold_to_aim=" << (cfg.enable_hold_to_aim ? "on" : "off")
            << ", visualization=" << (cfg.enable_visualization ? "on" : "off")
            << ", bounded=" << (cfg.bounded_movement ? "on" : "off") << " ---" << std::endl;
        std::cout << "--- Aim mapping: mode=" << cfg.aim_mode
            << ", gain=" << cfg.aim_gain
            << ", smoothing=" << cfg.aim_smoothing
            << ", deadzone=" << cfg.aim_deadzone_pixels
            << ", target_mode=" << (cfg.auto_target_part ? "auto" : "custom")
            << ", part_priority=" << cfg.aim_part_priority
            << ", custom_target=" << cfg.target_x_ratio << "," << cfg.target_y_ratio << " ---" << std::endl;
        std::cout << "--- Tracking boost: " << (cfg.enable_tracking_boost ? "on" : "off")
            << ", threshold=" << cfg.tracking_boost_threshold_pixels
            << ", gain=" << cfg.tracking_boost_gain
            << ", max_move=" << cfg.tracking_boost_max_move_pixels << " ---" << std::endl;
        std::cout << "--- Team filter: camp=" << cfg.enemy_camp
            << ", target_classes=" << EnemyCampTargetSummary(cfg.enemy_camp)
            << ", detection_part=" << DetectionPartSummary(cfg.detection_part) << " ---" << std::endl;
        std::cout << "--- Aim filter: mode=" << cfg.aim_filter_mode
            << ", PID(kp=" << cfg.pid_kp << ", ki=" << cfg.pid_ki << ", kd=" << cfg.pid_kd
            << "), OneEuro(min_cutoff=" << cfg.one_euro_min_cutoff
            << ", beta=" << cfg.one_euro_beta
            << ", d_cutoff=" << cfg.one_euro_d_cutoff << ") ---" << std::endl;
        std::cout << "--- Prediction: mode=" << cfg.prediction_mode
            << ", lead=" << cfg.prediction_lead_ms << "ms"
            << ", smoothing=" << cfg.prediction_smoothing
            << ", arc=" << cfg.prediction_acceleration_smoothing
            << ", alpha=" << cfg.prediction_alpha
            << ", beta=" << cfg.prediction_beta
            << ", kalman_r=" << cfg.prediction_kalman_measurement_noise
            << ", kalman_q=" << cfg.prediction_kalman_process_noise
            << ", max=" << cfg.prediction_max_pixels
            << ", reset=" << cfg.prediction_reset_pixels
            << ", noise=" << cfg.prediction_noise_pixels
            << ", output_smoothing=" << cfg.prediction_output_smoothing
            << ", servo_gain=" << cfg.prediction_servo_gain << " ---" << std::endl;
        std::cout << "--- Drone tracking: " << (cfg.enable_drone_tracking ? "on" : "off")
            << ", controller=" << cfg.drone_track_controller
            << ", gain=" << cfg.drone_track_gain
            << ", velocity=" << cfg.drone_track_velocity_gain
            << ", damping=" << cfg.drone_track_damping
            << ", smoothing=" << cfg.drone_track_smoothing
            << ", max_move=" << cfg.drone_track_max_move_pixels
            << ", deadzone=" << cfg.drone_track_deadzone_pixels
            << ", pos_gain=" << cfg.drone_track_position_gain
            << ", vel_damping=" << cfg.drone_track_velocity_damping
            << ", accel_limit=" << cfg.drone_track_accel_limit
            << ", visp_lambda=" << cfg.drone_track_visp_lambda
            << ", visp_damping=" << cfg.drone_track_visp_damping << " ---" << std::endl;
        std::cout << "--- Aim hotkeys: primary=" << KeyNameFromVk(cfg.smooth_aim_key)
            << ", secondary=" << KeyNameFromVk(cfg.smooth_aim_secondary_key) << " ---" << std::endl;
        std::cout << "--- Mouse slide: humanized=" << (cfg.enable_humanized_movement ? "on" : "off")
            << ", max_step=" << cfg.human_move_max_step
            << ", jitter=" << cfg.human_move_jitter
            << ", delay=" << cfg.human_move_delay_min_ms << "-" << cfg.human_move_delay_max_ms << "ms ---" << std::endl;
        std::cout << "--- Auto click: " << (cfg.enable_auto_click ? "on" : "off")
            << ", delay=" << cfg.auto_click_delay_min_ms << "-" << cfg.auto_click_delay_max_ms << "ms"
            << ", interval=" << cfg.auto_click_interval_min_ms << "-" << cfg.auto_click_interval_max_ms << "ms"
            << ", tolerance=" << cfg.auto_click_tolerance_pixels << "px"
            << ", auto_stop=" << (cfg.enable_auto_stop ? cfg.auto_stop_mode : "off")
            << ", stop_hold=" << cfg.auto_stop_hold_ms << "ms"
            << ", stop_settle=" << cfg.auto_stop_settle_ms << "ms ---" << std::endl;
        print_instructions();
    }

    ~AimAssistant() {
        if (is_visualizing) cv::destroyAllWindows();
    }

    void Run() {
        while (true) {
            auto loop_start_time = std::chrono::steady_clock::now();
            timings = TimingDetails{};
            pollLiveConfig();
            handleMouseBackendTestHotkey();
            if (!cfg.enable_capture || !frame_source) {
                detections.clear();
                last_detection_count = 0;
                last_targetable_detection_count = 0;
                last_target_available = false;
                last_gate_state = "capture_off";
                resetPrediction();
                resetTargetLock();
                resetAutoClick("capture_off");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (!frame_source->CaptureFrame(captured_frame, timings.capture_ms)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (detector) {
                detector->Detect(captured_frame, detections, cfg, timings);
            }
            else {
                detections.clear();
                timings.preprocess_ms = 0.0;
                timings.inference_ms = 0.0;
                timings.postprocess_ms = 0.0;
            }

            auto targeting_start_time = std::chrono::steady_clock::now();
            const bool aim_allowed_for_lock = isAimAllowedThisFrame();
            TargetLock best_target = findBestTarget(aim_allowed_for_lock);
            best_target = applyPrediction(best_target);
            last_detection_count = static_cast<int>(detections.size());
            last_target_available = best_target.valid;
            last_target_part = best_target.valid ? best_target.part : "none";
            last_move_x = 0;
            last_move_y = 0;
            last_raw_move_x = 0.0;
            last_raw_move_y = 0.0;
            last_gate_state = "idle";
            last_auto_click_state = cfg.enable_auto_click ? "idle" : "off";
            updateTargetOffset(best_target);
            auto targeting_end_time = std::chrono::steady_clock::now();
            timings.targeting_ms = std::chrono::duration<double, std::milli>(targeting_end_time - targeting_start_time).count();

            auto input_start_time = std::chrono::steady_clock::now();
            handleMouseInput(best_target);
            handleAutoClick(best_target);
            auto input_end_time = std::chrono::steady_clock::now();
            timings.input_ms = std::chrono::duration<double, std::milli>(input_end_time - input_start_time).count();
            auto core_loop_end_time = std::chrono::steady_clock::now();
            timings.total_loop_ms = std::chrono::duration<double, std::milli>(core_loop_end_time - loop_start_time).count();

            bool should_exit = false;
            if (is_visualizing) {
                auto visualization_start_time = std::chrono::steady_clock::now();
                handleVisualization(best_target);
                char key = static_cast<char>(cv::waitKey(1));
                if (key == 27) { // ESC
                    std::cout << "\nESC pressed. Exiting..." << std::endl;
                    should_exit = true;
                }
                if (key == 'v' || key == 'V') {
                    toggleVisualization();
                }
                auto visualization_end_time = std::chrono::steady_clock::now();
                timings.visualization_ms = std::chrono::duration<double, std::milli>(visualization_end_time - visualization_start_time).count();
            }

            updateAndPrintStats();
            if (should_exit) {
                break;
            }
        }
    }

private:
    static bool IsValidAimMode(const std::string& mode) {
        return mode == "atan" || mode == "linear";
    }

    static bool IsValidFilterMode(const std::string& mode) {
        return mode == "none" || mode == "pid" || mode == "oneeuro" || mode == "pid_oneeuro";
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

    void applyLiveConfig(const std::unordered_map<std::string, std::string>& values) {
        const Config old_cfg = cfg;
        Config next = cfg;

        const std::string model_path = ReadLiveRawString(values, "model_path", NarrowAscii(next.model_path));
        if (!model_path.empty()) {
            next.model_path = WidenUtf8(model_path);
        }

        const std::string inference_backend = ReadLiveString(values, "backend", next.backend);
        if (IsValidInferenceBackend(inference_backend)) {
            next.backend = inference_backend;
        }

        const std::string input_backend = ReadLiveString(values, "input_backend", next.input_backend);
        if (IsValidInputBackend(input_backend)) {
            next.input_backend = input_backend;
        }
        const std::string driver_dll_path = ReadLiveRawString(values, "driver_dll_path", NarrowAscii(next.driver_dll_path));
        next.driver_dll_path = WidenUtf8(driver_dll_path);
        next.driver_type = std::clamp(ReadLiveInt(values, "driver_type", next.driver_type), 0, 3);

        next.crop_size = std::clamp(ReadLiveInt(values, "crop_size", next.crop_size), 160, 960);
        next.max_lock_distance_pixels = std::clamp(ReadLiveInt(values, "lock_radius", next.max_lock_distance_pixels), 10, 500);
        next.confidence_threshold = static_cast<float>(std::clamp(ReadLiveDouble(values, "confidence", next.confidence_threshold), 0.01, 0.99));
        next.aim_smoothing = std::clamp(ReadLiveDouble(values, "smoothing", next.aim_smoothing), 0.01, 5.0);
        next.smooth_aim_key = std::clamp(ReadLiveInt(values, "aim_key", next.smooth_aim_key), 0, 255);
        next.smooth_aim_secondary_key = std::clamp(ReadLiveInt(values, "aim_key2", next.smooth_aim_secondary_key), 0, 255);
        next.aim_gain = std::clamp(ReadLiveDouble(values, "aim_gain", next.aim_gain), 0.01, 5.0);
        next.aim_deadzone_pixels = std::clamp(ReadLiveDouble(values, "deadzone", next.aim_deadzone_pixels), 0.0, 30.0);
        next.target_x_ratio = std::clamp(ReadLiveDouble(values, "target_x", next.target_x_ratio), 0.0, 1.0);
        next.target_y_ratio = std::clamp(ReadLiveDouble(values, "target_y", next.target_y_ratio), 0.0, 1.0);
        next.auto_target_part = ReadLiveBool(values, "auto_target_part", next.auto_target_part);
        next.aim_part_priority = NormalizeAimPartPriority(ReadLiveString(values, "aim_part_priority", next.aim_part_priority));
        const std::string enemy_camp = NormalizeEnemyCamp(ReadLiveString(values, "enemy_camp", next.enemy_camp));
        if (IsValidEnemyCamp(enemy_camp)) {
            next.enemy_camp = enemy_camp;
        }
        const std::string detection_part = NormalizeDetectionPart(ReadLiveString(values, "detection_part", next.detection_part));
        if (IsValidDetectionPart(detection_part)) {
            next.detection_part = detection_part;
        }
        if (next.detection_part == "body") {
            next.auto_target_part = false;
        }
        next.max_move_pixels = std::clamp(ReadLiveInt(values, "max_move", next.max_move_pixels), 1, 500);
        next.enable_tracking_boost = ReadLiveBool(values, "tracking_boost_enabled", next.enable_tracking_boost);
        next.tracking_boost_threshold_pixels = std::clamp(ReadLiveDouble(values, "tracking_boost_threshold", next.tracking_boost_threshold_pixels), 1.0, 200.0);
        next.tracking_boost_gain = std::clamp(ReadLiveDouble(values, "tracking_boost_gain", next.tracking_boost_gain), 1.0, 5.0);
        next.tracking_boost_max_move_pixels = std::clamp(ReadLiveInt(values, "tracking_boost_max_move", next.tracking_boost_max_move_pixels), 1, 500);
        next.enable_humanized_movement = ReadLiveBool(values, "human_slide_enabled", next.enable_humanized_movement);
        next.human_move_max_step = std::clamp(ReadLiveDouble(values, "human_slide_max_step", next.human_move_max_step), 1.0, 500.0);
        next.human_move_jitter = std::clamp(ReadLiveDouble(values, "human_slide_jitter", next.human_move_jitter), 0.0, 20.0);
        next.human_move_delay_min_ms = std::clamp(ReadLiveInt(values, "human_slide_delay_min", next.human_move_delay_min_ms), 0, 100);
        next.human_move_delay_max_ms = std::clamp(ReadLiveInt(values, "human_slide_delay_max", next.human_move_delay_max_ms), 0, 100);
        if (next.human_move_delay_min_ms > next.human_move_delay_max_ms) {
            std::swap(next.human_move_delay_min_ms, next.human_move_delay_max_ms);
        }
        next.enable_auto_click = ReadLiveBool(values, "auto_click_enabled", next.enable_auto_click);
        next.auto_click_delay_min_ms = std::clamp(ReadLiveInt(values, "auto_click_delay_min", next.auto_click_delay_min_ms), 0, 5000);
        next.auto_click_delay_max_ms = std::clamp(ReadLiveInt(values, "auto_click_delay_max", next.auto_click_delay_max_ms), 0, 5000);
        if (next.auto_click_delay_min_ms > next.auto_click_delay_max_ms) {
            std::swap(next.auto_click_delay_min_ms, next.auto_click_delay_max_ms);
        }
        next.auto_click_interval_min_ms = std::clamp(ReadLiveInt(values, "auto_click_interval_min", next.auto_click_interval_min_ms), 0, 5000);
        next.auto_click_interval_max_ms = std::clamp(ReadLiveInt(values, "auto_click_interval_max", next.auto_click_interval_max_ms), 0, 5000);
        if (next.auto_click_interval_min_ms > next.auto_click_interval_max_ms) {
            std::swap(next.auto_click_interval_min_ms, next.auto_click_interval_max_ms);
        }
        next.auto_click_tolerance_pixels = std::clamp(ReadLiveDouble(values, "auto_click_tolerance", next.auto_click_tolerance_pixels), 0.0, 50.0);
        next.enable_auto_stop = ReadLiveBool(values, "auto_stop_enabled", next.enable_auto_stop);
        const std::string auto_stop_mode = NormalizeAutoStopMode(ReadLiveString(values, "auto_stop_mode", next.auto_stop_mode));
        if (IsValidAutoStopMode(auto_stop_mode)) {
            next.auto_stop_mode = auto_stop_mode;
        }
        next.auto_stop_hold_ms = std::clamp(ReadLiveInt(values, "auto_stop_hold_ms", next.auto_stop_hold_ms), 0, 250);
        next.auto_stop_settle_ms = std::clamp(ReadLiveInt(values, "auto_stop_settle_ms", next.auto_stop_settle_ms), 0, 250);
        next.enable_console_stats = ReadLiveBool(values, "console_stats_enabled", next.enable_console_stats);
        next.enable_capture = ReadLiveBool(values, "enable_capture", next.enable_capture);
        next.enable_async_capture = ReadLiveBool(values, "async_capture_enabled", next.enable_async_capture);
        next.enable_mouse_movement = ReadLiveBool(values, "enable_mouse_move", next.enable_mouse_movement);
        next.enable_hold_to_aim = ReadLiveBool(values, "enable_hold_to_aim", next.enable_hold_to_aim);
        next.bounded_movement = ReadLiveBool(values, "bounded_movement", next.bounded_movement);

        const std::string aim_mode = ReadLiveString(values, "aim_mode", next.aim_mode);
        if (IsValidAimMode(aim_mode)) {
            next.aim_mode = aim_mode;
        }
        const std::string filter_mode = ReadLiveString(values, "aim_filter", next.aim_filter_mode);
        if (IsValidFilterMode(filter_mode)) {
            next.aim_filter_mode = filter_mode;
        }
        next.pid_kp = std::clamp(ReadLiveDouble(values, "pid_kp", next.pid_kp), 0.0, 10.0);
        next.pid_ki = std::clamp(ReadLiveDouble(values, "pid_ki", next.pid_ki), 0.0, 10.0);
        next.pid_kd = std::clamp(ReadLiveDouble(values, "pid_kd", next.pid_kd), 0.0, 10.0);
        next.pid_integral_limit = std::clamp(ReadLiveDouble(values, "pid_i_limit", next.pid_integral_limit), 0.0, 1000.0);
        next.one_euro_min_cutoff = std::clamp(ReadLiveDouble(values, "one_euro_min_cutoff", next.one_euro_min_cutoff), 0.01, 100.0);
        next.one_euro_beta = std::clamp(ReadLiveDouble(values, "one_euro_beta", next.one_euro_beta), 0.0, 100.0);
        next.one_euro_d_cutoff = std::clamp(ReadLiveDouble(values, "one_euro_d_cutoff", next.one_euro_d_cutoff), 0.01, 100.0);
        next.prediction_mode = NormalizePredictionMode(ReadLiveString(values, "prediction_mode", next.prediction_mode));
        next.prediction_lead_ms = std::clamp(ReadLiveDouble(values, "prediction_lead_ms", next.prediction_lead_ms), 0.0, 250.0);
        next.prediction_smoothing = std::clamp(ReadLiveDouble(values, "prediction_smoothing", next.prediction_smoothing), 0.0, 1.0);
        next.prediction_acceleration_smoothing = std::clamp(ReadLiveDouble(values, "prediction_acceleration_smoothing", next.prediction_acceleration_smoothing), 0.0, 1.0);
        next.prediction_alpha = std::clamp(ReadLiveDouble(values, "prediction_alpha", next.prediction_alpha), 0.0, 1.0);
        next.prediction_beta = std::clamp(ReadLiveDouble(values, "prediction_beta", next.prediction_beta), 0.0, 1.0);
        next.prediction_kalman_measurement_noise = std::clamp(ReadLiveDouble(values, "prediction_kalman_measurement_noise", next.prediction_kalman_measurement_noise), 0.1, 500.0);
        next.prediction_kalman_process_noise = std::clamp(ReadLiveDouble(values, "prediction_kalman_process_noise", next.prediction_kalman_process_noise), 0.1, 5000.0);
        next.prediction_max_pixels = std::clamp(ReadLiveInt(values, "prediction_max_pixels", next.prediction_max_pixels), 0, 250);
        next.prediction_reset_pixels = std::clamp(ReadLiveInt(values, "prediction_reset_pixels", next.prediction_reset_pixels), 5, 500);
        next.prediction_noise_pixels = std::clamp(ReadLiveDouble(values, "prediction_noise_pixels", next.prediction_noise_pixels), 0.0, 20.0);
        next.prediction_output_smoothing = std::clamp(ReadLiveDouble(values, "prediction_output_smoothing", next.prediction_output_smoothing), 0.0, 1.0);
        next.prediction_servo_gain = std::clamp(ReadLiveDouble(values, "prediction_servo_gain", next.prediction_servo_gain), 0.0, 2.0);
        next.enable_drone_tracking = ReadLiveBool(values, "drone_tracking_enabled", next.enable_drone_tracking);
        next.drone_track_controller = NormalizeDroneTrackController(ReadLiveString(values, "drone_track_controller", next.drone_track_controller));
        next.drone_track_gain = std::clamp(ReadLiveDouble(values, "drone_track_gain", next.drone_track_gain), 0.01, 5.0);
        next.drone_track_velocity_gain = std::clamp(ReadLiveDouble(values, "drone_track_velocity_gain", next.drone_track_velocity_gain), 0.0, 3.0);
        next.drone_track_damping = std::clamp(ReadLiveDouble(values, "drone_track_damping", next.drone_track_damping), 0.0, 3.0);
        next.drone_track_smoothing = std::clamp(ReadLiveDouble(values, "drone_track_smoothing", next.drone_track_smoothing), 0.02, 1.0);
        next.drone_track_max_move_pixels = std::clamp(ReadLiveInt(values, "drone_track_max_move", next.drone_track_max_move_pixels), 1, 800);
        next.drone_track_deadzone_pixels = std::clamp(ReadLiveDouble(values, "drone_track_deadzone", next.drone_track_deadzone_pixels), 0.0, 30.0);
        next.drone_track_position_gain = std::clamp(ReadLiveDouble(values, "drone_track_position_gain", next.drone_track_position_gain), 0.0, 5.0);
        next.drone_track_velocity_damping = std::clamp(ReadLiveDouble(values, "drone_track_velocity_damping", next.drone_track_velocity_damping), 0.0, 5.0);
        next.drone_track_accel_limit = std::clamp(ReadLiveDouble(values, "drone_track_accel_limit", next.drone_track_accel_limit), 100.0, 20000.0);
        next.drone_track_visp_lambda = std::clamp(ReadLiveDouble(values, "drone_track_visp_lambda", next.drone_track_visp_lambda), 0.01, 5.0);
        next.drone_track_visp_damping = std::clamp(ReadLiveDouble(values, "drone_track_visp_damping", next.drone_track_visp_damping), 0.0, 5.0);
        next.enable_visualization = ReadLiveBool(values, "enable_visualization", next.enable_visualization);

        const bool detector_changed =
            old_cfg.model_path != next.model_path ||
            old_cfg.backend != next.backend ||
            old_cfg.use_end_to_end_onnx != next.use_end_to_end_onnx;
        if (detector_changed && !reloadDetector(next)) {
            next.model_path = old_cfg.model_path;
            next.backend = old_cfg.backend;
            next.use_end_to_end_onnx = old_cfg.use_end_to_end_onnx;
        }

        const bool capture_changed =
            old_cfg.crop_size != next.crop_size ||
            old_cfg.enable_async_capture != next.enable_async_capture;
        if (capture_changed && !reinitializeCapture(next)) {
            next.crop_size = old_cfg.crop_size;
            next.enable_async_capture = old_cfg.enable_async_capture;
        }

        const bool mouse_backend_changed =
            old_cfg.input_backend != next.input_backend ||
            old_cfg.driver_dll_path != next.driver_dll_path ||
            old_cfg.driver_type != next.driver_type;
        if (mouse_backend_changed) {
            reloadMouseBackend(next);
        }

        const bool aim_math_changed =
            old_cfg.crop_size != next.crop_size ||
            old_cfg.max_move_pixels != next.max_move_pixels ||
            old_cfg.bounded_movement != next.bounded_movement ||
            old_cfg.aim_mode != next.aim_mode ||
            old_cfg.aim_filter_mode != next.aim_filter_mode ||
            old_cfg.aim_gain != next.aim_gain ||
            old_cfg.aim_smoothing != next.aim_smoothing ||
            old_cfg.aim_deadzone_pixels != next.aim_deadzone_pixels ||
            old_cfg.target_x_ratio != next.target_x_ratio ||
            old_cfg.target_y_ratio != next.target_y_ratio ||
            old_cfg.auto_target_part != next.auto_target_part ||
            old_cfg.aim_part_priority != next.aim_part_priority ||
            old_cfg.enemy_camp != next.enemy_camp ||
            old_cfg.detection_part != next.detection_part ||
            old_cfg.enable_tracking_boost != next.enable_tracking_boost ||
            old_cfg.tracking_boost_threshold_pixels != next.tracking_boost_threshold_pixels ||
            old_cfg.tracking_boost_gain != next.tracking_boost_gain ||
            old_cfg.tracking_boost_max_move_pixels != next.tracking_boost_max_move_pixels ||
            old_cfg.pid_kp != next.pid_kp ||
            old_cfg.pid_ki != next.pid_ki ||
            old_cfg.pid_kd != next.pid_kd ||
            old_cfg.pid_integral_limit != next.pid_integral_limit ||
            old_cfg.one_euro_min_cutoff != next.one_euro_min_cutoff ||
            old_cfg.one_euro_beta != next.one_euro_beta ||
            old_cfg.one_euro_d_cutoff != next.one_euro_d_cutoff ||
            old_cfg.prediction_mode != next.prediction_mode ||
            old_cfg.prediction_lead_ms != next.prediction_lead_ms ||
            old_cfg.prediction_smoothing != next.prediction_smoothing ||
            old_cfg.prediction_acceleration_smoothing != next.prediction_acceleration_smoothing ||
            old_cfg.prediction_alpha != next.prediction_alpha ||
            old_cfg.prediction_beta != next.prediction_beta ||
            old_cfg.prediction_kalman_measurement_noise != next.prediction_kalman_measurement_noise ||
            old_cfg.prediction_kalman_process_noise != next.prediction_kalman_process_noise ||
            old_cfg.prediction_max_pixels != next.prediction_max_pixels ||
            old_cfg.prediction_reset_pixels != next.prediction_reset_pixels ||
            old_cfg.prediction_noise_pixels != next.prediction_noise_pixels ||
            old_cfg.prediction_output_smoothing != next.prediction_output_smoothing ||
            old_cfg.prediction_servo_gain != next.prediction_servo_gain ||
            old_cfg.enable_drone_tracking != next.enable_drone_tracking ||
            old_cfg.drone_track_controller != next.drone_track_controller ||
            old_cfg.drone_track_gain != next.drone_track_gain ||
            old_cfg.drone_track_velocity_gain != next.drone_track_velocity_gain ||
            old_cfg.drone_track_damping != next.drone_track_damping ||
            old_cfg.drone_track_smoothing != next.drone_track_smoothing ||
            old_cfg.drone_track_max_move_pixels != next.drone_track_max_move_pixels ||
            old_cfg.drone_track_deadzone_pixels != next.drone_track_deadzone_pixels ||
            old_cfg.drone_track_position_gain != next.drone_track_position_gain ||
            old_cfg.drone_track_velocity_damping != next.drone_track_velocity_damping ||
            old_cfg.drone_track_accel_limit != next.drone_track_accel_limit ||
            old_cfg.drone_track_visp_lambda != next.drone_track_visp_lambda ||
            old_cfg.drone_track_visp_damping != next.drone_track_visp_damping ||
            old_cfg.enable_humanized_movement != next.enable_humanized_movement ||
            old_cfg.human_move_max_step != next.human_move_max_step ||
            old_cfg.human_move_jitter != next.human_move_jitter ||
            old_cfg.human_move_delay_min_ms != next.human_move_delay_min_ms ||
            old_cfg.human_move_delay_max_ms != next.human_move_delay_max_ms;

        const bool auto_click_changed =
            old_cfg.enable_auto_click != next.enable_auto_click ||
            old_cfg.auto_click_delay_min_ms != next.auto_click_delay_min_ms ||
            old_cfg.auto_click_delay_max_ms != next.auto_click_delay_max_ms ||
            old_cfg.auto_click_interval_min_ms != next.auto_click_interval_min_ms ||
            old_cfg.auto_click_interval_max_ms != next.auto_click_interval_max_ms ||
            old_cfg.auto_click_tolerance_pixels != next.auto_click_tolerance_pixels ||
            old_cfg.enable_auto_stop != next.enable_auto_stop ||
            old_cfg.auto_stop_mode != next.auto_stop_mode ||
            old_cfg.auto_stop_hold_ms != next.auto_stop_hold_ms ||
            old_cfg.auto_stop_settle_ms != next.auto_stop_settle_ms;

        cfg = std::move(next);
        setVisualizationEnabled(cfg.enable_visualization);
        if (aim_math_changed) {
            resetAimCarry();
            resetAimFilters();
            resetPrediction();
            resetTargetLock();
        }
        if (auto_click_changed) {
            resetAutoClick("config");
        }

        std::cout << "\n[CONFIG] Live settings applied: backend=" << cfg.backend
            << ", input=" << cfg.input_backend
            << ", crop=" << cfg.crop_size
            << ", lock_radius=" << cfg.max_lock_distance_pixels
            << ", confidence=" << cfg.confidence_threshold
            << ", screen_capture=" << (cfg.enable_capture ? "on" : "off")
            << ", capture_mode=" << (cfg.enable_async_capture ? "latest" : "direct")
            << ", aim=" << cfg.aim_mode
            << ", filter=" << cfg.aim_filter_mode
            << ", prediction=" << cfg.prediction_mode
            << "(" << cfg.prediction_lead_ms << "ms," << cfg.prediction_max_pixels
            << "px,arc=" << cfg.prediction_acceleration_smoothing
            << ",R=" << cfg.prediction_kalman_measurement_noise
            << ",Q=" << cfg.prediction_kalman_process_noise
            << ",servo=" << cfg.prediction_servo_gain << ")"
            << ", drone_tracking=" << (cfg.enable_drone_tracking ? "on" : "off")
            << "(controller=" << cfg.drone_track_controller
            << ",gain=" << cfg.drone_track_gain
            << ",vel=" << cfg.drone_track_velocity_gain
            << ",damping=" << cfg.drone_track_damping
            << ",smooth=" << cfg.drone_track_smoothing
            << ",max=" << cfg.drone_track_max_move_pixels
            << ",deadzone=" << cfg.drone_track_deadzone_pixels
            << ",pos=" << cfg.drone_track_position_gain
            << ",vel_damp=" << cfg.drone_track_velocity_damping
            << ",accel=" << cfg.drone_track_accel_limit
            << ",visp_lambda=" << cfg.drone_track_visp_lambda
            << ",visp_damp=" << cfg.drone_track_visp_damping << ")"
            << ", human_slide=" << (cfg.enable_humanized_movement ? "on" : "off")
            << "(" << cfg.human_move_max_step << "," << cfg.human_move_jitter
            << "," << cfg.human_move_delay_min_ms << "-" << cfg.human_move_delay_max_ms << "ms)"
            << ", auto_click=" << (cfg.enable_auto_click ? "on" : "off")
            << "(delay=" << cfg.auto_click_delay_min_ms << "-" << cfg.auto_click_delay_max_ms
            << "ms, interval=" << cfg.auto_click_interval_min_ms << "-" << cfg.auto_click_interval_max_ms
            << "ms, tol=" << cfg.auto_click_tolerance_pixels
            << "px, auto_stop=" << (cfg.enable_auto_stop ? cfg.auto_stop_mode : "off")
            << ", stop_hold=" << cfg.auto_stop_hold_ms
            << "ms, stop_settle=" << cfg.auto_stop_settle_ms << "ms)"
            << ", target_mode=" << (cfg.auto_target_part ? "auto" : "custom")
            << ", part_priority=" << cfg.aim_part_priority
            << ", target=" << cfg.target_x_ratio << "," << cfg.target_y_ratio
            << ", tracking_boost=" << (cfg.enable_tracking_boost ? "on" : "off")
            << "(" << cfg.tracking_boost_threshold_pixels << "px,"
            << cfg.tracking_boost_gain << "x,max=" << cfg.tracking_boost_max_move_pixels << ")"
            << ", camp=" << cfg.enemy_camp
            << ", target_classes=" << EnemyCampTargetSummary(cfg.enemy_camp)
            << ", detection_part=" << DetectionPartSummary(cfg.detection_part)
            << ", hotkeys=" << KeyNameFromVk(cfg.smooth_aim_key) << "/" << KeyNameFromVk(cfg.smooth_aim_secondary_key)
            << ", visualization=" << (cfg.enable_visualization ? "on" : "off") << std::endl;
    }

    void print_instructions() {
        std::cout << "\n--- Controls ---" << std::endl;
        if (cfg.enable_hold_to_aim) {
            std::cout << "    [Hold " << KeyNameFromVk(cfg.smooth_aim_key);
            if (cfg.smooth_aim_secondary_key > 0) {
                std::cout << " or " << KeyNameFromVk(cfg.smooth_aim_secondary_key);
            }
            std::cout << "] to smoothly lock onto a target." << std::endl;
        }
        else {
            std::cout << "    [Auto Move] Hold-to-aim is OFF; target lock moves without a mouse button." << std::endl;
        }
        std::cout << "    [Press F9] to test active mouse backend (+20px then -20px)." << std::endl;
        std::cout << "    [Press V in visualization window] to toggle visualization ON/OFF." << std::endl;
        std::cout << "    [Press ESC in visualization window] to quit." << std::endl;
    }

    static cv::Point2d BoxCenterPrecise(const cv::Rect& box) {
        return cv::Point2d(
            static_cast<double>(box.x) + static_cast<double>(box.width) * 0.5,
            static_cast<double>(box.y) + static_cast<double>(box.height) * 0.5);
    }

    static cv::Point BoxCenter(const cv::Rect& box) {
        const cv::Point2d center = BoxCenterPrecise(box);
        return cv::Point(
            static_cast<int>(std::lround(center.x)),
            static_cast<int>(std::lround(center.y)));
    }

    static std::string PartNameForDetection(const Detection* det) {
        if (!det) {
            return "none";
        }
        return IsHeadClass(det->class_id) ? "head" : "other";
    }

    bool headMatchesBody(const Detection& head, const Detection& body) const {
        if (ClassCampId(head.class_id) != ClassCampId(body.class_id)) {
            return false;
        }

        const cv::Point head_center = BoxCenter(head.box);
        const int x_margin = std::max(6, body.box.width * 35 / 100);
        const int y_above = std::max(8, body.box.height * 65 / 100);
        const int y_below = std::max(8, body.box.height * 70 / 100);
        const bool x_ok = head_center.x >= body.box.x - x_margin
            && head_center.x <= body.box.x + body.box.width + x_margin;
        const bool y_ok = head_center.y >= body.box.y - y_above
            && head_center.y <= body.box.y + y_below;
        return x_ok && y_ok;
    }

    std::vector<TargetEntity> buildTargetEntities() {
        std::vector<TargetEntity> entities;
        std::vector<const Detection*> heads;
        for (const auto& det : detections) {
            if (!DetectionMatchesEnemyCamp(det.class_id, cfg.enemy_camp)
                || !DetectionMatchesPartFilter(det.class_id, cfg.detection_part)) {
                continue;
            }
            if (IsHeadClass(det.class_id)) {
                heads.push_back(&det);
                continue;
            }

            TargetEntity entity;
            entity.body = &det;
            entity.bounds = det.box;
            entity.camp_id = ClassCampId(det.class_id);
            entities.push_back(entity);
        }

        for (const Detection* head : heads) {
            int best_index = -1;
            double best_score = std::numeric_limits<double>::max();
            for (size_t index = 0; index < entities.size(); ++index) {
                TargetEntity& entity = entities[index];
                if (!entity.body || entity.head || entity.camp_id != ClassCampId(head->class_id)) {
                    continue;
                }
                if (!headMatchesBody(*head, *entity.body)) {
                    continue;
                }
                const cv::Point head_center = BoxCenter(head->box);
                const cv::Point body_top(entity.body->box.x + entity.body->box.width / 2, entity.body->box.y);
                const double score = std::hypot(head_center.x - body_top.x, head_center.y - body_top.y);
                if (score < best_score) {
                    best_score = score;
                    best_index = static_cast<int>(index);
                }
            }

            if (best_index >= 0) {
                TargetEntity& entity = entities[static_cast<size_t>(best_index)];
                entity.head = head;
                entity.bounds = entity.bounds | head->box;
                continue;
            }

            TargetEntity entity;
            entity.head = head;
            entity.bounds = head->box;
            entity.camp_id = ClassCampId(head->class_id);
            entities.push_back(entity);
        }

        return entities;
    }

    TargetLock makeSelectedPartLock(const TargetEntity& entity, const Detection* selected) const {
        TargetLock lock;
        lock.valid = true;
        lock.head = entity.head;
        lock.body = entity.body;
        lock.box = entity.bounds;

        if (!selected) {
            lock.valid = false;
            return lock;
        }

        lock.selected = selected;
        lock.box = selected->box;
        lock.point = BoxCenter(selected->box);
        lock.measured_point = lock.point;
        lock.part = PartNameForDetection(selected);
        return lock;
    }

    TargetLock makeTargetLock(const TargetEntity& entity) const {
        TargetLock lock;
        lock.valid = true;
        lock.head = entity.head;
        lock.body = entity.body;
        lock.box = entity.bounds;

        if (!cfg.auto_target_part) {
            lock.point = cv::Point(
                entity.bounds.x + static_cast<int>(entity.bounds.width * cfg.target_x_ratio),
                entity.bounds.y + static_cast<int>(entity.bounds.height * cfg.target_y_ratio));
            lock.measured_point = lock.point;
            lock.selected = entity.head ? entity.head : entity.body;
            lock.part = "custom";
            return lock;
        }

        const Detection* selected = nullptr;
        if (cfg.aim_part_priority == "head") {
            selected = entity.head ? entity.head : entity.body;
        }
        else if (cfg.aim_part_priority == "other") {
            selected = entity.body ? entity.body : entity.head;
        }
        else {
            double best_dist = std::numeric_limits<double>::max();
            for (const Detection* det : { entity.head, entity.body }) {
                if (!det) {
                    continue;
                }
                const cv::Point point = BoxCenter(det->box);
                const double dist = std::hypot(point.x - crop_center.x, point.y - crop_center.y);
                if (dist < best_dist) {
                    best_dist = dist;
                    selected = det;
                }
            }
        }

        return makeSelectedPartLock(entity, selected);
    }

    const Detection* preferredLockedPart(const TargetEntity& entity) const {
        if (cfg.auto_target_part && cfg.aim_part_priority == "head") {
            return entity.head ? entity.head : entity.body;
        }
        if (cfg.auto_target_part && cfg.aim_part_priority == "other") {
            return entity.body ? entity.body : entity.head;
        }
        if (target_lock_part == "head" && entity.head) {
            return entity.head;
        }
        if (target_lock_part == "other" && entity.body) {
            return entity.body;
        }
        for (const Detection* det : { entity.head, entity.body }) {
            if (det && det->class_id == target_lock_class_id) {
                return det;
            }
        }
        return nullptr;
    }

    bool targetLockConfigCompatible(const TargetLock& candidate) const {
        if (!target_lock_initialized || !candidate.selected) {
            return false;
        }
        if (target_lock_camp_id >= 0 && ClassCampId(candidate.selected->class_id) != target_lock_camp_id) {
            return false;
        }
        const bool part_switch_allowed = cfg.auto_target_part
            && (cfg.aim_part_priority == "head" || cfg.aim_part_priority == "other");
        if (!part_switch_allowed && !target_lock_part.empty() && target_lock_part != "custom" && candidate.part != target_lock_part) {
            return false;
        }
        return true;
    }

    TargetLock findExistingLockedTarget(const std::vector<TargetEntity>& entities) const {
        TargetLock best;
        double best_score = std::numeric_limits<double>::max();
        const double threshold = std::max(70.0, static_cast<double>(cfg.max_lock_distance_pixels) * 1.25);
        for (const TargetEntity& entity : entities) {
            const Detection* selected = preferredLockedPart(entity);
            if (!selected) {
                continue;
            }
            TargetLock candidate = cfg.auto_target_part
                ? makeSelectedPartLock(entity, selected)
                : makeTargetLock(entity);
            if (!candidate.valid || !targetLockConfigCompatible(candidate)) {
                continue;
            }
            const cv::Point center = BoxCenter(selected->box);
            const double dist = std::hypot(
                static_cast<double>(center.x - target_lock_point.x),
                static_cast<double>(center.y - target_lock_point.y));
            const double size_change = std::abs(selected->box.area() - target_lock_box.area())
                / static_cast<double>(std::max(1, target_lock_box.area()));
            const double score = dist + size_change * 18.0;
            if (dist <= threshold && score < best_score) {
                best_score = score;
                best = candidate;
            }
        }
        return best;
    }

    void resetTargetLock() {
        target_lock_initialized = false;
        target_lock_missed_frames = 0;
        target_lock_point = cv::Point();
        target_lock_box = cv::Rect();
        target_lock_part.clear();
        target_lock_class_id = -1;
        target_lock_camp_id = -1;
    }

    void rememberTargetLock(const TargetLock& target) {
        if (!target.valid || !target.selected) {
            resetTargetLock();
            return;
        }
        target_lock_initialized = true;
        target_lock_missed_frames = 0;
        target_lock_point = BoxCenter(target.selected->box);
        target_lock_box = target.selected->box;
        target_lock_part = target.part;
        target_lock_class_id = target.selected->class_id;
        target_lock_camp_id = ClassCampId(target.selected->class_id);
    }

    TargetLock findBestTarget(bool keep_existing_lock) {
        const std::vector<TargetEntity> entities = buildTargetEntities();
        last_targetable_detection_count = static_cast<int>(entities.size());

        if (!keep_existing_lock) {
            resetTargetLock();
        }
        else if (target_lock_initialized) {
            TargetLock locked = findExistingLockedTarget(entities);
            if (locked.valid) {
                rememberTargetLock(locked);
                return locked;
            }
            ++target_lock_missed_frames;
            if (target_lock_missed_frames <= 3) {
                return TargetLock{};
            }
            resetTargetLock();
        }

        TargetLock best_target;
        double min_dist_to_center = cfg.max_lock_distance_pixels;
        for (const TargetEntity& entity : entities) {
            TargetLock candidate = makeTargetLock(entity);
            if (!candidate.valid) {
                continue;
            }
            const double dist = std::hypot(candidate.point.x - crop_center.x, candidate.point.y - crop_center.y);
            if (dist < min_dist_to_center) {
                min_dist_to_center = dist;
                best_target = candidate;
            }
        }
        if (keep_existing_lock && best_target.valid) {
            rememberTargetLock(best_target);
        }
        return best_target;
    }

    int mouseDeltaLimit(int override_limit = 0) const {
        return std::max(1, override_limit > 0 ? override_limit : cfg.max_move_pixels);
    }

    int clampMouseDelta(int value, int override_limit = 0) const {
        if (!cfg.bounded_movement) {
            return value;
        }
        const int limit = mouseDeltaLimit(override_limit);
        return std::clamp(value, -limit, limit);
    }

    int toMouseDelta(double value, int override_limit = 0) const {
        return clampMouseDelta(static_cast<int>(std::lround(value)), override_limit);
    }

    double clampMouseDeltaDouble(double value, int override_limit = 0) const {
        if (!cfg.bounded_movement) {
            return value;
        }
        const double limit = static_cast<double>(mouseDeltaLimit(override_limit));
        return std::clamp(value, -limit, limit);
    }

    int takeMouseDelta(double value, double& carry, int override_limit = 0) const {
        carry += clampMouseDeltaDouble(value, override_limit);
        int move = static_cast<int>(carry);
        if (move == 0) {
            return 0;
        }
        move = clampMouseDelta(move, override_limit);
        carry -= static_cast<double>(move);
        if (std::abs(carry) > static_cast<double>(mouseDeltaLimit(override_limit))) {
            carry = 0.0;
        }
        return move;
    }

    void resetAimCarry() {
        aim_carry_x = 0.0;
        aim_carry_y = 0.0;
    }

    void resetAimFilters() {
        pid_x.Reset();
        pid_y.Reset();
        euro_x.Reset();
        euro_y.Reset();
        filter_time_initialized = false;
    }

    void resetPrediction() {
        prediction_tracker.Reset();
        drone_tracking.Reset();
        last_prediction_dx = 0;
        last_prediction_dy = 0;
    }

    void resetAutoClick(const std::string& state = "idle") {
        auto_click_locked = false;
        auto_click_waiting_initial_delay = true;
        auto_click_next_time = {};
        auto_click_last_target_initialized = false;
        auto_click_last_target_seen_initialized = false;
        last_auto_click_state = cfg.enable_auto_click ? state : "off";
        if (!cfg.enable_auto_click || !cfg.enable_auto_stop) {
            last_auto_stop_state = "off";
        }
    }

    int randomAutoClickMs(int min_ms, int max_ms) {
        min_ms = std::clamp(min_ms, 0, 5000);
        max_ms = std::clamp(max_ms, 0, 5000);
        if (min_ms > max_ms) {
            std::swap(min_ms, max_ms);
        }
        std::uniform_int_distribution<int> dist(min_ms, max_ms);
        return dist(auto_click_rng);
    }

    bool isTargetAlignedForAutoClick(const TargetLock& target) const {
        if (!target.valid) {
            return false;
        }
        const double dx = static_cast<double>(target.point.x - crop_center.x);
        const double dy = static_cast<double>(target.point.y - crop_center.y);
        return std::hypot(dx, dy) <= std::max(0.0, cfg.auto_click_tolerance_pixels);
    }

    bool updateAutoClickTargetContinuity(const TargetLock& target) {
        if (!target.valid) {
            return false;
        }
        if (!auto_click_last_target_initialized) {
            auto_click_last_target_point = target.point;
            auto_click_last_target_initialized = true;
            return false;
        }

        const double jump = std::hypot(
            static_cast<double>(target.point.x - auto_click_last_target_point.x),
            static_cast<double>(target.point.y - auto_click_last_target_point.y));
        auto_click_last_target_point = target.point;
        const double switch_threshold = std::max(45.0, static_cast<double>(cfg.max_lock_distance_pixels) * 0.75);
        return jump > switch_threshold;
    }

    cv::Point clampPointToCrop(const cv::Point2d& point) const {
        const int max_coord = std::max(0, cfg.crop_size - 1);
        return cv::Point(
            std::clamp(static_cast<int>(std::round(point.x)), 0, max_coord),
            std::clamp(static_cast<int>(std::round(point.y)), 0, max_coord));
    }

    TargetLock applyPrediction(TargetLock target) {
        last_prediction_dx = 0;
        last_prediction_dy = 0;
        last_aim_point_source = "measured";
        if (!target.valid) {
            last_aim_point_source = "none";
            resetPrediction();
            return target;
        }

        target.measured_point = target.point;
        if (cfg.enable_drone_tracking) {
            prediction_tracker.Reset();
            last_aim_point_source = "drone";
            return target;
        }
        if (cfg.prediction_mode == "off" || cfg.prediction_lead_ms <= 0.0 || cfg.prediction_max_pixels <= 0) {
            if (cfg.prediction_mode == "off") {
                prediction_tracker.Reset();
            }
            return target;
        }

        const auto now = std::chrono::steady_clock::now();
        const cv::Point2d measurement(static_cast<double>(target.point.x), static_cast<double>(target.point.y));
        if (!prediction_tracker.initialized) {
            prediction_tracker.Initialize(measurement, now);
            return target;
        }

        double dt = std::chrono::duration<double>(now - prediction_tracker.last_time).count();
        dt = std::clamp(dt, 0.001, 0.1);
        const double jump = std::hypot(
            measurement.x - prediction_tracker.last_measurement.x,
            measurement.y - prediction_tracker.last_measurement.y);
        if (jump > static_cast<double>(cfg.prediction_reset_pixels)) {
            prediction_tracker.Initialize(measurement, now);
            return target;
        }
        prediction_tracker.AddSample(measurement, now);

        const bool noise_only = jump <= cfg.prediction_noise_pixels;
        if (noise_only && cfg.prediction_mode != "kalman" && cfg.prediction_mode != "adaptive") {
            prediction_tracker.velocity = cv::Point2d(
                prediction_tracker.velocity.x * 0.85,
                prediction_tracker.velocity.y * 0.85);
            prediction_tracker.acceleration = cv::Point2d(
                prediction_tracker.acceleration.x * 0.70,
                prediction_tracker.acceleration.y * 0.70);
            prediction_tracker.position = measurement;
        }
        else {
            if (cfg.prediction_mode == "linear") {
                const cv::Point2d raw_velocity(
                    (measurement.x - prediction_tracker.last_measurement.x) / dt,
                    (measurement.y - prediction_tracker.last_measurement.y) / dt);
                const double alpha = std::clamp(cfg.prediction_smoothing, 0.0, 1.0);
                prediction_tracker.velocity = cv::Point2d(
                    alpha * raw_velocity.x + (1.0 - alpha) * prediction_tracker.velocity.x,
                    alpha * raw_velocity.y + (1.0 - alpha) * prediction_tracker.velocity.y);
                prediction_tracker.position = measurement;
            }
            else if (cfg.prediction_mode == "adaptive" || cfg.prediction_mode == "servo") {
                cv::Point2d fitted_position;
                cv::Point2d fitted_velocity;
                cv::Point2d fitted_acceleration;
                if (prediction_tracker.EstimateAdaptive(fitted_position, fitted_velocity, fitted_acceleration)) {
                    prediction_tracker.position = fitted_position;
                    prediction_tracker.velocity = fitted_velocity;
                    prediction_tracker.acceleration = fitted_acceleration;
                }
                else {
                    prediction_tracker.position = measurement;
                    prediction_tracker.velocity = cv::Point2d(0.0, 0.0);
                    prediction_tracker.acceleration = cv::Point2d(0.0, 0.0);
                }
            }
            else if (cfg.prediction_mode == "arc" || cfg.prediction_mode == "hybrid") {
                const cv::Point2d raw_velocity(
                    (measurement.x - prediction_tracker.last_measurement.x) / dt,
                    (measurement.y - prediction_tracker.last_measurement.y) / dt);
                const cv::Point2d raw_acceleration(
                    (raw_velocity.x - prediction_tracker.velocity.x) / dt,
                    (raw_velocity.y - prediction_tracker.velocity.y) / dt);
                const double velocity_alpha = std::clamp(cfg.prediction_smoothing, 0.0, 1.0);
                const double acceleration_alpha = std::clamp(cfg.prediction_acceleration_smoothing, 0.0, 1.0);
                prediction_tracker.velocity = cv::Point2d(
                    velocity_alpha * raw_velocity.x + (1.0 - velocity_alpha) * prediction_tracker.velocity.x,
                    velocity_alpha * raw_velocity.y + (1.0 - velocity_alpha) * prediction_tracker.velocity.y);
                prediction_tracker.acceleration = cv::Point2d(
                    acceleration_alpha * raw_acceleration.x + (1.0 - acceleration_alpha) * prediction_tracker.acceleration.x,
                    acceleration_alpha * raw_acceleration.y + (1.0 - acceleration_alpha) * prediction_tracker.acceleration.y);
                prediction_tracker.position = measurement;
            }
            else if (cfg.prediction_mode == "alphabeta") {
                const cv::Point2d predicted_position(
                    prediction_tracker.position.x + prediction_tracker.velocity.x * dt,
                    prediction_tracker.position.y + prediction_tracker.velocity.y * dt);
                const cv::Point2d residual(
                    measurement.x - predicted_position.x,
                    measurement.y - predicted_position.y);
                const double alpha = std::clamp(cfg.prediction_alpha, 0.0, 1.0);
                const double beta = std::clamp(cfg.prediction_beta, 0.0, 1.0);
                prediction_tracker.position = cv::Point2d(
                    predicted_position.x + alpha * residual.x,
                    predicted_position.y + alpha * residual.y);
                prediction_tracker.velocity = cv::Point2d(
                    prediction_tracker.velocity.x + (beta * residual.x) / dt,
                    prediction_tracker.velocity.y + (beta * residual.y) / dt);
            }
            else if (cfg.prediction_mode == "kalman") {
                const double measurement_noise = std::clamp(cfg.prediction_kalman_measurement_noise, 0.1, 500.0);
                const double process_noise = std::clamp(cfg.prediction_kalman_process_noise, 0.1, 5000.0);
                prediction_tracker.kalman_x.Update(measurement.x, dt, process_noise, measurement_noise);
                prediction_tracker.kalman_y.Update(measurement.y, dt, process_noise, measurement_noise);
                prediction_tracker.position = cv::Point2d(
                    prediction_tracker.kalman_x.position,
                    prediction_tracker.kalman_y.position);
                prediction_tracker.velocity = cv::Point2d(
                    prediction_tracker.kalman_x.velocity,
                    prediction_tracker.kalman_y.velocity);
            }
            else {
                resetPrediction();
                return target;
            }

            prediction_tracker.last_measurement = measurement;
            prediction_tracker.last_time = now;
        }

        double effective_lead_ms = cfg.prediction_lead_ms;
        if (cfg.prediction_mode == "servo") {
            const double observed_loop_ms = smoothed_total > 0.0 ? smoothed_total : timings.total_loop_ms;
            if (std::isfinite(observed_loop_ms) && observed_loop_ms > 0.0) {
                effective_lead_ms += std::clamp(observed_loop_ms, 0.0, 80.0);
            }
        }
        const double lead_seconds = effective_lead_ms / 1000.0;
        cv::Point2d future_position(
            prediction_tracker.position.x + prediction_tracker.velocity.x * lead_seconds,
            prediction_tracker.position.y + prediction_tracker.velocity.y * lead_seconds);
        if (cfg.prediction_mode == "arc") {
            const double accel_scale = 0.5 * lead_seconds * lead_seconds;
            future_position.x += prediction_tracker.acceleration.x * accel_scale;
            future_position.y += prediction_tracker.acceleration.y * accel_scale;
        }
        else if (cfg.prediction_mode == "adaptive" || cfg.prediction_mode == "servo") {
            const double accel_scale = 0.5 * lead_seconds * lead_seconds;
            future_position.x += prediction_tracker.acceleration.x * accel_scale;
            future_position.y += prediction_tracker.acceleration.y * accel_scale;
        }
        else if (cfg.prediction_mode == "hybrid") {
            const double accel_scale = 0.5 * lead_seconds * lead_seconds;
            future_position.x += prediction_tracker.acceleration.x * accel_scale * 0.35;
            future_position.y += prediction_tracker.acceleration.y * accel_scale;
        }
        cv::Point2d lead(
            future_position.x - measurement.x,
            future_position.y - measurement.y);
        const double lead_length = std::hypot(lead.x, lead.y);
        int max_prediction_pixels = cfg.prediction_max_pixels;
        if (cfg.prediction_mode == "servo") {
            max_prediction_pixels = std::max(max_prediction_pixels, std::clamp(cfg.max_move_pixels, 30, 120));
        }
        if (lead_length > static_cast<double>(max_prediction_pixels) && lead_length > 0.001) {
            const double scale = static_cast<double>(max_prediction_pixels) / lead_length;
            lead.x *= scale;
            lead.y *= scale;
        }

        double output_alpha = std::clamp(cfg.prediction_output_smoothing, 0.0, 1.0);
        if (cfg.prediction_mode == "servo") {
            output_alpha = std::max(output_alpha, 0.65);
        }
        if (prediction_tracker.lead_initialized) {
            lead = cv::Point2d(
                output_alpha * lead.x + (1.0 - output_alpha) * prediction_tracker.lead.x,
                output_alpha * lead.y + (1.0 - output_alpha) * prediction_tracker.lead.y);
        }
        prediction_tracker.lead = lead;
        prediction_tracker.lead_initialized = true;
        if (std::hypot(lead.x, lead.y) < 0.25) {
            return target;
        }

        target.point = clampPointToCrop(cv::Point2d(measurement.x + lead.x, measurement.y + lead.y));
        target.prediction_applied = true;
        last_aim_point_source = cfg.prediction_mode == "servo" ? "servo" : "predicted";
        last_prediction_dx = target.point.x - target.measured_point.x;
        last_prediction_dy = target.point.y - target.measured_point.y;
        return target;
    }

    double nextFilterDt() {
        const auto now = std::chrono::steady_clock::now();
        if (!filter_time_initialized) {
            last_filter_time = now;
            filter_time_initialized = true;
            return 1.0 / 144.0;
        }
        const double dt = std::chrono::duration<double>(now - last_filter_time).count();
        last_filter_time = now;
        return std::clamp(dt, 0.001, 0.1);
    }

    std::pair<double, double> applyAimFilter(std::pair<double, double> raw) {
        if (cfg.aim_filter_mode == "none") {
            return raw;
        }

        const double dt = nextFilterDt();
        if (cfg.aim_filter_mode == "pid" || cfg.aim_filter_mode == "pid_oneeuro") {
            raw.first = pid_x.Update(raw.first, dt, cfg);
            raw.second = pid_y.Update(raw.second, dt, cfg);
        }
        if (cfg.aim_filter_mode == "oneeuro" || cfg.aim_filter_mode == "pid_oneeuro") {
            raw.first = euro_x.Update(raw.first, dt, cfg);
            raw.second = euro_y.Update(raw.second, dt, cfg);
        }
        return raw;
    }

    std::pair<double, double> mapAimOffsetToMouse(double dx_pixels, double dy_pixels) const {
        const double effective_gain = cfg.aim_gain * cfg.aim_smoothing;
        if (cfg.aim_mode == "linear") {
            return {
                dx_pixels * effective_gain,
                dy_pixels * effective_gain
            };
        }

        const double denominator = static_cast<double>(std::max(1, cfg.crop_size));
        return {
            std::atan2(dx_pixels, denominator) * denominator * effective_gain,
            std::atan2(dy_pixels, denominator) * denominator * effective_gain
        };
    }

    std::pair<double, double> servoFeedForwardMove() const {
        if (cfg.prediction_mode != "servo" || !prediction_tracker.lead_initialized) {
            return { 0.0, 0.0 };
        }

        const double gain = std::clamp(cfg.prediction_servo_gain, 0.0, 2.0);
        if (gain <= 0.0) {
            return { 0.0, 0.0 };
        }

        const cv::Point2d lead = prediction_tracker.lead;
        if (std::hypot(lead.x, lead.y) < 0.25) {
            return { 0.0, 0.0 };
        }

        auto ff = mapAimOffsetToMouse(lead.x, lead.y);
        ff.first *= gain;
        ff.second *= gain;
        return ff;
    }

    bool computeDroneTrackingMove(const TargetLock& target, bool use_carry, int& move_x, int& move_y) {
        move_x = 0;
        move_y = 0;
        last_tracking_boost_active = false;
        last_servo_direct_move = false;
        if (!cfg.enable_drone_tracking || !target.valid) {
            drone_tracking.Reset();
            return false;
        }

        const auto now = std::chrono::steady_clock::now();
        const cv::Point2d measurement = target.selected
            ? BoxCenterPrecise(target.selected->box)
            : cv::Point2d(static_cast<double>(target.point.x), static_cast<double>(target.point.y));
        const cv::Point2d error(
            measurement.x - static_cast<double>(crop_center.x),
            measurement.y - static_cast<double>(crop_center.y));
        const double error_len = std::hypot(error.x, error.y);
        const bool was_initialized = drone_tracking.initialized;
        if (!was_initialized) {
            drone_tracking.Initialize(measurement, error, now);
        }

        double dt = was_initialized
            ? std::chrono::duration<double>(now - drone_tracking.last_time).count()
            : (1.0 / 144.0);
        dt = std::clamp(dt, 0.001, 0.1);

        const double jump = std::hypot(
            measurement.x - drone_tracking.last_measurement.x,
            measurement.y - drone_tracking.last_measurement.y);
        if (was_initialized && jump > static_cast<double>(std::max(cfg.prediction_reset_pixels, cfg.drone_track_max_move_pixels * 2))) {
            drone_tracking.Initialize(measurement, error, now);
            last_raw_move_x = 0.0;
            last_raw_move_y = 0.0;
            last_gate_state = "drone_reset";
            return false;
        }

        const cv::Point2d raw_velocity = was_initialized
            ? cv::Point2d(
                (measurement.x - drone_tracking.last_measurement.x) / dt,
                (measurement.y - drone_tracking.last_measurement.y) / dt)
            : cv::Point2d(0.0, 0.0);
        const double smoothing = std::clamp(cfg.drone_track_smoothing, 0.02, 1.0);
        drone_tracking.velocity = cv::Point2d(
            smoothing * raw_velocity.x + (1.0 - smoothing) * drone_tracking.velocity.x,
            smoothing * raw_velocity.y + (1.0 - smoothing) * drone_tracking.velocity.y);

        const cv::Point2d derivative = was_initialized
            ? cv::Point2d(
                (error.x - drone_tracking.last_error.x) / dt,
                (error.y - drone_tracking.last_error.y) / dt)
            : cv::Point2d(0.0, 0.0);

        const double observed_dt = std::clamp(smoothed_total > 0.0 ? smoothed_total / 1000.0 : dt, 0.001, 0.05);
        const double response_horizon = std::clamp(observed_dt + (1.0 / 180.0), 0.008, 0.035);
        cv::Point2d control_velocity(
            cfg.drone_track_velocity_gain * drone_tracking.velocity.x * response_horizon,
            cfg.drone_track_velocity_gain * drone_tracking.velocity.y * response_horizon);
        cv::Point2d control_damping(
            cfg.drone_track_damping * derivative.x * response_horizon,
            cfg.drone_track_damping * derivative.y * response_horizon);

        const std::string controller = NormalizeDroneTrackController(cfg.drone_track_controller);
        cv::Point2d lead(
            control_velocity.x + control_damping.x,
            control_velocity.y + control_damping.y);
        const double lead_len = std::hypot(lead.x, lead.y);
        const double max_lead = static_cast<double>(std::max(1, cfg.prediction_max_pixels));
        if (lead_len > max_lead && lead_len > 0.001) {
            const double scale = max_lead / lead_len;
            lead.x *= scale;
            lead.y *= scale;
        }

        cv::Point2d desired;
        if (controller == "px4") {
            const double pos_gain = std::clamp(cfg.drone_track_position_gain, 0.0, 5.0);
            const double vel_damping = std::clamp(cfg.drone_track_velocity_damping, 0.0, 5.0);
            cv::Point2d desired_velocity(
                pos_gain * (error.x + lead.x) / std::max(response_horizon, 0.001),
                pos_gain * (error.y + lead.y) / std::max(response_horizon, 0.001));
            const double velocity_limit = std::max(1.0, static_cast<double>(cfg.drone_track_max_move_pixels) / std::max(response_horizon, 0.001));
            const double desired_velocity_len = std::hypot(desired_velocity.x, desired_velocity.y);
            if (desired_velocity_len > velocity_limit && desired_velocity_len > 0.001) {
                const double scale = velocity_limit / desired_velocity_len;
                desired_velocity.x *= scale;
                desired_velocity.y *= scale;
            }

            cv::Point2d control_velocity_px(
                desired_velocity.x - vel_damping * drone_tracking.velocity.x,
                desired_velocity.y - vel_damping * drone_tracking.velocity.y);
            const double accel_limit = std::max(100.0, cfg.drone_track_accel_limit) * response_horizon * response_horizon;
            const double current_command_len = std::hypot(drone_tracking.command.x, drone_tracking.command.y);
            const cv::Point2d target_command(
                control_velocity_px.x * response_horizon * cfg.drone_track_gain,
                control_velocity_px.y * response_horizon * cfg.drone_track_gain);
            cv::Point2d delta_command(
                target_command.x - drone_tracking.command.x,
                target_command.y - drone_tracking.command.y);
            const double delta_len = std::hypot(delta_command.x, delta_command.y);
            const double max_delta = std::max(1.0, accel_limit + current_command_len * 0.35);
            if (delta_len > max_delta && delta_len > 0.001) {
                const double scale = max_delta / delta_len;
                delta_command.x *= scale;
                delta_command.y *= scale;
            }

            desired = cv::Point2d(
                drone_tracking.command.x + delta_command.x,
                drone_tracking.command.y + delta_command.y);
        }
        else if (controller == "visp") {
            const double lambda = std::clamp(cfg.drone_track_visp_lambda, 0.01, 5.0);
            const double damping = std::clamp(cfg.drone_track_visp_damping, 0.0, 5.0);
            const double normalized_radius = std::max(1.0, static_cast<double>(cfg.crop_size) * 0.5);
            const double interaction_scale = 1.0 + std::min(1.0, error_len / normalized_radius) * damping;
            desired = cv::Point2d(
                ((error.x + lead.x) * lambda - damping * drone_tracking.velocity.x * response_horizon) * cfg.drone_track_gain / interaction_scale,
                ((error.y + lead.y) * lambda - damping * drone_tracking.velocity.y * response_horizon) * cfg.drone_track_gain / interaction_scale);
        }
        else {
            desired = cv::Point2d(
                (error.x + lead.x) * cfg.drone_track_gain,
                (error.y + lead.y) * cfg.drone_track_gain);
        }

        const double desired_len = std::hypot(desired.x, desired.y);
        const double max_error_offset = std::max(1.0, error_len + max_lead);
        if (desired_len > max_error_offset && desired_len > 0.001) {
            const double scale = max_error_offset / desired_len;
            desired.x *= scale;
            desired.y *= scale;
        }

        const double target_speed = std::hypot(drone_tracking.velocity.x, drone_tracking.velocity.y);
        const double speed_ramp = std::clamp(target_speed / 1200.0, 0.0, 1.0);
        const double command_alpha = std::clamp(
            cfg.drone_track_smoothing + (1.0 - cfg.drone_track_smoothing) * speed_ramp,
            0.08,
            1.0);
        const double command_dot = drone_tracking.command.x * desired.x + drone_tracking.command.y * desired.y;
        if (command_dot < 0.0) {
            drone_tracking.command = desired;
            resetAimCarry();
            resetAimFilters();
        }
        else {
            drone_tracking.command = cv::Point2d(
                command_alpha * desired.x + (1.0 - command_alpha) * drone_tracking.command.x,
                command_alpha * desired.y + (1.0 - command_alpha) * drone_tracking.command.y);
        }

        const double limit = static_cast<double>(std::max(1, cfg.crop_size));
        const double command_len = std::hypot(drone_tracking.command.x, drone_tracking.command.y);
        if (command_len > limit && command_len > 0.001) {
            const double scale = limit / command_len;
            drone_tracking.command.x *= scale;
            drone_tracking.command.y *= scale;
        }

        std::pair<double, double> raw = mapAimOffsetToMouse(drone_tracking.command.x, drone_tracking.command.y);
        raw = applyAimFilter(raw);
        last_raw_move_x = raw.first;
        last_raw_move_y = raw.second;
        last_aim_point_source = "drone";
        last_prediction_dx = static_cast<int>(std::lround(lead.x));
        last_prediction_dy = static_cast<int>(std::lround(lead.y));

        if (error_len <= std::max(0.0, cfg.drone_track_deadzone_pixels) && std::hypot(raw.first, raw.second) < 0.5) {
            drone_tracking.last_measurement = measurement;
            drone_tracking.last_error = error;
            drone_tracking.last_time = now;
            last_gate_state = "drone_deadzone";
            if (use_carry) {
                resetAimCarry();
            }
            return false;
        }

        if (use_carry) {
            move_x = takeMouseDelta(raw.first, aim_carry_x, cfg.drone_track_max_move_pixels);
            move_y = takeMouseDelta(raw.second, aim_carry_y, cfg.drone_track_max_move_pixels);
        }
        else {
            move_x = toMouseDelta(raw.first, cfg.drone_track_max_move_pixels);
            move_y = toMouseDelta(raw.second, cfg.drone_track_max_move_pixels);
        }

        drone_tracking.last_measurement = measurement;
        drone_tracking.last_error = error;
        drone_tracking.last_time = now;
        last_servo_direct_move = true;

        if (move_x == 0 && move_y == 0) {
            last_gate_state = use_carry ? "drone_accum" : "drone_subpixel";
            return false;
        }
        return true;
    }

    bool shouldUseTrackingBoost(double distance_pixels) const {
        return cfg.enable_tracking_boost &&
            distance_pixels >= std::max(1.0, cfg.tracking_boost_threshold_pixels);
    }

    double trackingBoostScale(double distance_pixels) const {
        if (!shouldUseTrackingBoost(distance_pixels)) {
            return 1.0;
        }
        const double threshold = std::max(1.0, cfg.tracking_boost_threshold_pixels);
        const double ramp = std::clamp((distance_pixels - threshold) / threshold, 0.0, 1.0);
        return 1.0 + (std::max(1.0, cfg.tracking_boost_gain) - 1.0) * ramp;
    }

    bool isRealtimeServoTarget(const TargetLock& target) const {
        return target.valid &&
            target.prediction_applied &&
            cfg.prediction_mode == "servo";
    }

    double servoResponseScale() const {
        const double gain = std::clamp(cfg.prediction_servo_gain, 0.0, 2.0);
        return 1.0 + gain;
    }

    int trackingBoostMaxMove(double distance_pixels) const {
        if (!shouldUseTrackingBoost(distance_pixels)) {
            return 0;
        }
        return std::max(cfg.max_move_pixels, cfg.tracking_boost_max_move_pixels);
    }

    void updateTargetOffset(const TargetLock& target) {
        if (!target.valid) {
            last_target_offset_x = 0;
            last_target_offset_y = 0;
            return;
        }
        const cv::Point target_point = target.point;
        last_target_offset_x = target_point.x - crop_center.x;
        last_target_offset_y = target_point.y - crop_center.y;
    }

    void updateRawAimTelemetry(const TargetLock& target) {
        if (!target.valid) {
            last_raw_move_x = 0.0;
            last_raw_move_y = 0.0;
            return;
        }
        const cv::Point target_point = target.point;
        const double dx_pixels = static_cast<double>(target_point.x - crop_center.x);
        const double dy_pixels = static_cast<double>(target_point.y - crop_center.y);
        auto raw = mapAimOffsetToMouse(dx_pixels, dy_pixels);
        raw = applyAimFilter(raw);
        last_raw_move_x = raw.first;
        last_raw_move_y = raw.second;
    }

    bool computeAimMove(const TargetLock& target, bool use_carry, int& move_x, int& move_y) {
        move_x = 0;
        move_y = 0;
        if (cfg.enable_drone_tracking) {
            return computeDroneTrackingMove(target, use_carry, move_x, move_y);
        }
        const cv::Point target_point = target.point;
        const double dx_pixels = static_cast<double>(target_point.x - crop_center.x);
        const double dy_pixels = static_cast<double>(target_point.y - crop_center.y);
        const double distance_pixels = std::hypot(dx_pixels, dy_pixels);
        last_tracking_boost_active = false;
        const bool realtime_servo = isRealtimeServoTarget(target);
        const auto servo_ff = realtime_servo ? std::pair<double, double>{ 0.0, 0.0 } : servoFeedForwardMove();
        const bool has_servo_feedforward = std::hypot(servo_ff.first, servo_ff.second) >= 0.01;
        if (distance_pixels <= cfg.aim_deadzone_pixels && !has_servo_feedforward) {
            last_raw_move_x = 0.0;
            last_raw_move_y = 0.0;
            last_gate_state = "deadzone";
            if (use_carry) {
                resetAimCarry();
                resetAimFilters();
            }
            return false;
        }

        auto raw = mapAimOffsetToMouse(dx_pixels, dy_pixels);
        if (realtime_servo) {
            const double response = servoResponseScale();
            raw.first *= response;
            raw.second *= response;
        }
        last_tracking_boost_active = !realtime_servo && shouldUseTrackingBoost(distance_pixels);
        if (last_tracking_boost_active) {
            const double boost = trackingBoostScale(distance_pixels);
            raw.first *= boost;
            raw.second *= boost;
        }
        raw.first += servo_ff.first;
        raw.second += servo_ff.second;
        if (realtime_servo) {
            resetAimFilters();
        }
        else {
            raw = applyAimFilter(raw);
        }
        last_raw_move_x = raw.first;
        last_raw_move_y = raw.second;
        const int effective_limit = last_tracking_boost_active ? trackingBoostMaxMove(distance_pixels) : 0;

        if (use_carry) {
            move_x = takeMouseDelta(raw.first, aim_carry_x, effective_limit);
            move_y = takeMouseDelta(raw.second, aim_carry_y, effective_limit);
        }
        else {
            move_x = toMouseDelta(raw.first, effective_limit);
            move_y = toMouseDelta(raw.second, effective_limit);
        }

        if (move_x == 0 && move_y == 0) {
            last_gate_state = use_carry ? "accum" : "subpixel";
            return false;
        }
        return true;
    }

    void handleMouseBackendTestHotkey() {
        static bool f9_was_pressed = false;
        const bool f9_is_pressed = GetAsyncKeyState(cfg.mouse_test_key) & 0x8000;
        if (f9_is_pressed && !f9_was_pressed) {
            std::cout << "\n[INFO] F9 mouse backend test: move +20 then -20." << std::endl;
            mouse->MoveRelative(20, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            mouse->MoveRelative(-20, 0);
        }
        f9_was_pressed = f9_is_pressed;
    }

    bool isKeyDown(int vk) const {
        return vk > 0 && (::GetAsyncKeyState(vk) & 0x8000);
    }

    static int OppositeMovementKey(int vk) {
        switch (vk) {
        case 'W': return 'S';
        case 'S': return 'W';
        case 'A': return 'D';
        case 'D': return 'A';
        default: return 0;
        }
    }

    static const std::array<int, 4>& MovementKeys() {
        static const std::array<int, 4> keys{ 'W', 'A', 'S', 'D' };
        return keys;
    }

    void addAutoStopKey(std::vector<int>& keys, int vk) const {
        if (vk <= 0 || isKeyDown(vk)) {
            return;
        }
        if (std::find(keys.begin(), keys.end(), vk) == keys.end()) {
            keys.push_back(vk);
        }
    }

    void updateAutoStopMovementHistory(const std::chrono::steady_clock::time_point& now) {
        const auto& movement_keys = MovementKeys();
        for (size_t i = 0; i < movement_keys.size(); ++i) {
            const bool down = isKeyDown(movement_keys[i]);
            if (auto_stop_movement_was_down[i] && !down) {
                last_auto_stop_released_key = movement_keys[i];
                last_auto_stop_release_time = now;
                last_auto_stop_release_initialized = true;
            }
            auto_stop_movement_was_down[i] = down;
        }
    }

    bool isAnyMovementKeyDown() const {
        for (int vk : MovementKeys()) {
            if (isKeyDown(vk)) {
                return true;
            }
        }
        return false;
    }

    std::vector<int> counterTapAutoStopKeys(const std::chrono::steady_clock::time_point& now) const {
        std::vector<int> keys;
        const bool w_down = isKeyDown('W');
        const bool a_down = isKeyDown('A');
        const bool s_down = isKeyDown('S');
        const bool d_down = isKeyDown('D');

        if (w_down && !s_down) addAutoStopKey(keys, 'S');
        if (s_down && !w_down) addAutoStopKey(keys, 'W');
        if (a_down && !d_down) addAutoStopKey(keys, 'D');
        if (d_down && !a_down) addAutoStopKey(keys, 'A');

        if (keys.empty() && last_auto_stop_release_initialized) {
            const auto released_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_auto_stop_release_time).count();
            if (released_ms <= 180) {
                addAutoStopKey(keys, OppositeMovementKey(last_auto_stop_released_key));
            }
        }
        return keys;
    }

    std::vector<int> autoStopKeysForClick(const std::chrono::steady_clock::time_point& now) const {
        std::vector<int> keys;
        if (!cfg.enable_auto_stop) {
            return keys;
        }
        if (cfg.auto_stop_mode == "ad_pair") {
            addAutoStopKey(keys, 'A');
            addAutoStopKey(keys, 'D');
        }
        else if (cfg.auto_stop_mode == "crouch") {
            addAutoStopKey(keys, VK_CONTROL);
        }
        else {
            keys = counterTapAutoStopKeys(now);
        }
        return keys;
    }

    std::vector<int> beginAutoStopForClick(const std::chrono::steady_clock::time_point& now) {
        if (!cfg.enable_auto_stop) {
            last_auto_stop_state = "off";
            return {};
        }

        std::vector<int> keys = autoStopKeysForClick(now);
        if (keys.empty()) {
            last_auto_stop_state = "ready";
            return keys;
        }

        const int hold_ms = std::clamp(cfg.auto_stop_hold_ms, 0, 250);
        const int settle_ms = std::clamp(cfg.auto_stop_settle_ms, 0, 250);
        mouse->PressKeys(keys);
        if (hold_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
        }
        last_auto_stop_state = cfg.auto_stop_mode;
        if (cfg.auto_stop_mode == "counter_tap") {
            mouse->ReleaseKeys(keys);
            keys.clear();
            if (settle_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(settle_ms));
            }
            last_auto_stop_state = "counter_ready";
        }
        else if (settle_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(settle_ms));
        }
        return keys;
    }

    void finishAutoStopForClick(const std::vector<int>& keys) {
        if (!keys.empty()) {
            mouse->ReleaseKeys(keys);
        }
    }

    bool isAimHotkeyDown() const {
        return isKeyDown(cfg.smooth_aim_key) || isKeyDown(cfg.smooth_aim_secondary_key);
    }

    bool isAimAllowedThisFrame() const {
        return !cfg.enable_hold_to_aim || isAimHotkeyDown();
    }

    void handleMouseInput(const TargetLock& target) {
        if (!cfg.enable_mouse_movement) {
            last_gate_state = "mouse_off";
            last_tracking_boost_active = false;
            last_servo_direct_move = false;
            resetAimCarry();
            resetAimFilters();
            return;
        }

        if (!target.valid) {
            last_gate_state = "no_target";
            last_tracking_boost_active = false;
            last_servo_direct_move = false;
            resetAimCarry();
            resetAimFilters();
            return;
        }

        const bool smooth_triggered = isAimAllowedThisFrame();
        if (smooth_triggered) {
            int move_x = 0;
            int move_y = 0;
            last_servo_direct_move = false;
            if (computeAimMove(target, true, move_x, move_y)) {
                last_move_x = move_x;
                last_move_y = move_y;
                last_gate_state = cfg.enable_hold_to_aim ? "hold_move" : "auto_move";
                last_servo_direct_move = cfg.enable_drone_tracking || isRealtimeServoTarget(target);
                const Config* movement_cfg = (last_tracking_boost_active || last_servo_direct_move) ? nullptr : &cfg;
                mouse->MoveRelative(move_x, move_y, movement_cfg);
            }
        }
        else {
            last_gate_state = "hold_wait";
            last_tracking_boost_active = false;
            last_servo_direct_move = false;
            updateRawAimTelemetry(target);
            resetTargetLock();
            resetAimCarry();
            resetAimFilters();
        }
    }

    void handleAutoClick(const TargetLock& target) {
        constexpr int target_lost_grace_ms = 120;
        const auto now = std::chrono::steady_clock::now();
        updateAutoStopMovementHistory(now);

        if (!cfg.enable_auto_click) {
            resetAutoClick("off");
            return;
        }

        if (!cfg.enable_mouse_movement) {
            resetAutoClick("mouse_off");
            return;
        }

        if (!target.valid) {
            if (auto_click_locked && auto_click_last_target_seen_initialized) {
                const auto lost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - auto_click_last_target_seen_time).count();
                if (lost_ms <= target_lost_grace_ms) {
                    last_auto_click_state = "lost_grace";
                    return;
                }
            }
            resetAutoClick("no_target");
            return;
        }

        if (!isAimAllowedThisFrame()) {
            resetAutoClick("hold_wait");
            return;
        }

        auto_click_last_target_seen_time = now;
        auto_click_last_target_seen_initialized = true;
        if (updateAutoClickTargetContinuity(target)) {
            resetAutoClick("target_switch");
            auto_click_last_target_point = target.point;
            auto_click_last_target_initialized = true;
            auto_click_last_target_seen_time = now;
            auto_click_last_target_seen_initialized = true;
        }

        if (!auto_click_locked) {
            const int delay_ms = randomAutoClickMs(cfg.auto_click_delay_min_ms, cfg.auto_click_delay_max_ms);
            auto_click_locked = true;
            auto_click_waiting_initial_delay = true;
            auto_click_next_time = now + std::chrono::milliseconds(delay_ms);
            if (delay_ms > 0) {
                last_auto_click_state = "delay";
                return;
            }
        }

        const bool aligned = isTargetAlignedForAutoClick(target);
        if (now < auto_click_next_time) {
            last_auto_click_state = auto_click_waiting_initial_delay
                ? (aligned ? "delay" : "delay_track")
                : (aligned ? "cooldown" : "cooldown_track");
            return;
        }

        if (!aligned) {
            last_auto_click_state = auto_click_waiting_initial_delay ? "ready_wait" : "cooldown_wait";
            return;
        }

        if (cfg.enable_auto_stop && cfg.auto_stop_mode == "counter_tap" && isAnyMovementKeyDown()) {
            last_auto_stop_state = "wait_release";
            last_auto_click_state = "stop_wait";
            return;
        }

        std::vector<int> auto_stop_keys = beginAutoStopForClick(now);
        mouse->Click(0);
        finishAutoStopForClick(auto_stop_keys);
        const int interval_ms = randomAutoClickMs(cfg.auto_click_interval_min_ms, cfg.auto_click_interval_max_ms);
        auto_click_waiting_initial_delay = false;
        auto_click_next_time = now + std::chrono::milliseconds(interval_ms);
        last_auto_click_state = "click";
    }

    void updateAndPrintStats() {
        const double smoothing_factor = 0.05;
        if (is_first_frame) {
            smoothed_total = timings.total_loop_ms;
            smoothed_capture = timings.capture_ms;
            smoothed_pre = timings.preprocess_ms;
            smoothed_inf = timings.inference_ms;
            smoothed_post = timings.postprocess_ms;
            smoothed_targeting = timings.targeting_ms;
            smoothed_input = timings.input_ms;
            smoothed_visualization = timings.visualization_ms;
            is_first_frame = false;
        }
        else {
            smoothed_total = smoothing_factor * timings.total_loop_ms + (1.0 - smoothing_factor) * smoothed_total;
            smoothed_capture = smoothing_factor * timings.capture_ms + (1.0 - smoothing_factor) * smoothed_capture;
            smoothed_pre = smoothing_factor * timings.preprocess_ms + (1.0 - smoothing_factor) * smoothed_pre;
            smoothed_inf = smoothing_factor * timings.inference_ms + (1.0 - smoothing_factor) * smoothed_inf;
            smoothed_post = smoothing_factor * timings.postprocess_ms + (1.0 - smoothing_factor) * smoothed_post;
            smoothed_targeting = smoothing_factor * timings.targeting_ms + (1.0 - smoothing_factor) * smoothed_targeting;
            smoothed_input = smoothing_factor * timings.input_ms + (1.0 - smoothing_factor) * smoothed_input;
            smoothed_visualization = smoothing_factor * timings.visualization_ms + (1.0 - smoothing_factor) * smoothed_visualization;
        }
        ++frame_count_for_console;
        if (!cfg.enable_console_stats || smoothed_total <= 0) {
            return;
        }
        const auto stats_now = std::chrono::steady_clock::now();
        if (stats_print_initialized) {
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                stats_now - last_stats_print_time).count();
            if (elapsed_ms < 1000) {
                return;
            }
        }
        stats_print_initialized = true;
        last_stats_print_time = stats_now;
        {
            double smoothed_fps = 1000.0 / smoothed_total;
            std::cout << std::fixed << std::setprecision(1)
                << "\r[LIVE] FPS: " << std::setw(5) << smoothed_fps
                << " | Total Delay: " << std::setw(5) << smoothed_total << "ms"
                << " | Stages: Cap " << smoothed_capture
                << ", Pre " << smoothed_pre
                << ", Inf " << smoothed_inf
                << ", Post " << smoothed_post
                << ", Target " << smoothed_targeting
                << ", Input " << smoothed_input
                << ", Viz " << smoothed_visualization << "ms"
                << " | Dets: " << last_detection_count
                << " | Match: " << last_targetable_detection_count
                << " | Target: " << (last_target_available ? "yes" : "no")
                << " | Part: " << last_target_part
                << " | AimPoint: " << last_aim_point_source
                << " | Offset: " << last_target_offset_x << "," << last_target_offset_y
                << " | Lead: " << last_prediction_dx << "," << last_prediction_dy
                << " | Gate: " << last_gate_state
                << " | Raw: " << last_raw_move_x << "," << last_raw_move_y
                << " | Move: " << last_move_x << "," << last_move_y
                << " | AutoClick: " << last_auto_click_state
                << " | AutoStop: " << last_auto_stop_state
                << " | Track: " << (cfg.enable_drone_tracking ? "drone" : (last_tracking_boost_active ? "boost" : "normal"))
                << " | Slide: " << (cfg.enable_humanized_movement && !last_tracking_boost_active && !last_servo_direct_move ? "human" : "direct")
                << "        " << std::flush;
        }
    }

    void handleVisualization(const TargetLock& best_target) {
        cv::circle(captured_frame, crop_center, static_cast<int>(cfg.max_lock_distance_pixels), cv::Scalar(255, 255, 0), 1);
        for (const auto& det : detections) {
            const bool targetable = DetectionMatchesEnemyCamp(det.class_id, cfg.enemy_camp)
                && DetectionMatchesPartFilter(det.class_id, cfg.detection_part);
            if (!targetable) {
                continue;
            }
            const bool selected_part = best_target.valid && (&det == best_target.selected);
            const bool same_entity = best_target.valid && (&det == best_target.head || &det == best_target.body);
            cv::Scalar color = selected_part
                ? cv::Scalar(0, 0, 255)
                : (same_entity ? cv::Scalar(0, 165, 255) : cv::Scalar(0, 255, 0));
            cv::rectangle(captured_frame, det.box, color, 2);
            std::string label = Cs2ClassLabel(det.class_id) + " " + cv::format("%.2f", det.confidence);
            cv::putText(captured_frame, label, cv::Point(det.box.x, det.box.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
        if (best_target.valid && best_target.prediction_applied) {
            const cv::Scalar lead_color(255, 0, 255);
            cv::line(captured_frame, best_target.measured_point, best_target.point, lead_color, 1);
            cv::circle(captured_frame, best_target.measured_point, 3, lead_color, 1);
            cv::circle(captured_frame, best_target.point, 4, lead_color, 2);
        }
        if (smoothed_total > 0) {
            double smoothed_fps = 1000.0 / smoothed_total;
            std::ostringstream stats_stream;
            stats_stream << std::fixed << std::setprecision(1) << "FPS: " << smoothed_fps;
            cv::putText(captured_frame, stats_stream.str(), cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 255), 2);
        }
        cv::imshow(cfg.window_name, captured_frame);
    }

    void toggleVisualization() {
        is_visualizing = !is_visualizing;
        if (is_visualizing) {
            cv::namedWindow(cfg.window_name, cv::WINDOW_AUTOSIZE);
            std::cout << "\n[INFO] Visualization toggled ON." << std::endl;
        }
        else {
            cv::destroyAllWindows();
            std::cout << "\n[INFO] Visualization toggled OFF. Console stats will continue." << std::endl;
        }
    }

    Config cfg;
    std::unique_ptr<FrameSource> frame_source;
    std::unique_ptr<ObjectDetector> detector;
    std::unique_ptr<MouseController> mouse;
    cv::Rect crop_region;
    cv::Point crop_center;
    cv::Mat captured_frame;
    std::vector<Detection> detections;
    TimingDetails timings;
    bool is_visualizing;
    double smoothed_total = 0.0;
    double smoothed_capture = 0.0;
    double smoothed_pre = 0.0;
    double smoothed_inf = 0.0;
    double smoothed_post = 0.0;
    double smoothed_targeting = 0.0;
    double smoothed_input = 0.0;
    double smoothed_visualization = 0.0;
    bool is_first_frame = true;
    int frame_count_for_console = 0;
    int last_detection_count = 0;
    int last_targetable_detection_count = 0;
    bool last_target_available = false;
    std::string last_target_part = "none";
    int last_target_offset_x = 0;
    int last_target_offset_y = 0;
    std::string last_aim_point_source = "none";
    int last_move_x = 0;
    int last_move_y = 0;
    double last_raw_move_x = 0.0;
    double last_raw_move_y = 0.0;
    int last_prediction_dx = 0;
    int last_prediction_dy = 0;
    bool last_tracking_boost_active = false;
    bool last_servo_direct_move = false;
    bool target_lock_initialized = false;
    int target_lock_missed_frames = 0;
    cv::Point target_lock_point{};
    cv::Rect target_lock_box{};
    std::string target_lock_part;
    int target_lock_class_id = -1;
    int target_lock_camp_id = -1;
    double aim_carry_x = 0.0;
    double aim_carry_y = 0.0;
    PidAxis pid_x;
    PidAxis pid_y;
    OneEuroAxis euro_x;
    OneEuroAxis euro_y;
    PredictionTracker prediction_tracker;
    DroneTrackingState drone_tracking;
    std::chrono::steady_clock::time_point last_filter_time{};
    bool filter_time_initialized = false;
    std::chrono::steady_clock::time_point last_live_config_check{};
    bool live_config_check_initialized = false;
    std::filesystem::file_time_type last_live_config_write{};
    bool live_config_write_initialized = false;
    std::chrono::steady_clock::time_point last_stats_print_time{};
    bool stats_print_initialized = false;
    std::string last_gate_state = "idle";
    std::string last_auto_click_state = "off";
    std::string last_auto_stop_state = "off";
    bool auto_click_locked = false;
    bool auto_click_waiting_initial_delay = true;
    std::chrono::steady_clock::time_point auto_click_next_time{};
    bool auto_click_last_target_initialized = false;
    cv::Point auto_click_last_target_point{};
    bool auto_click_last_target_seen_initialized = false;
    std::chrono::steady_clock::time_point auto_click_last_target_seen_time{};
    std::mt19937 auto_click_rng{ std::random_device{}() };
    std::array<bool, 4> auto_stop_movement_was_down{};
    int last_auto_stop_released_key = 0;
    bool last_auto_stop_release_initialized = false;
    std::chrono::steady_clock::time_point last_auto_stop_release_time{};
};

static int RunInputBackendSelfTest(const Config& cfg) {
    constexpr int test_delta = 120;
    std::cout << "--- Input backend self-test requested ---" << std::endl;
    std::cout << "--- Aim hotkeys: primary=" << KeyNameFromVk(cfg.smooth_aim_key)
        << ", secondary=" << KeyNameFromVk(cfg.smooth_aim_secondary_key) << " ---" << std::endl;
    MouseController mouse(cfg);
    if (cfg.require_driver_backend && !mouse.IsDriverBackend()) {
        std::cerr << "[SELFTEST] Driver backend was required, but active backend is "
            << mouse.BackendName() << "." << std::endl;
        return 2;
    }

    POINT before{};
    POINT after_forward{};
    POINT after_return{};
    if (::GetCursorPos(&before)) {
        std::cout << "--- Cursor before: " << before.x << "," << before.y << " ---" << std::endl;
    }
    else {
        std::cerr << "[SELFTEST] GetCursorPos before failed. GetLastError=" << ::GetLastError() << std::endl;
    }

    std::cout << "--- Self-test move sequence: +" << test_delta << ",0 then -" << test_delta << ",0 ---" << std::endl;
    mouse.MoveRelative(test_delta, 0, &cfg);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    if (::GetCursorPos(&after_forward)) {
        std::cout << "--- Cursor after +" << test_delta << ": "
            << after_forward.x << "," << after_forward.y
            << " | observed delta: " << (after_forward.x - before.x)
            << "," << (after_forward.y - before.y) << " ---" << std::endl;
    }
    else {
        std::cerr << "[SELFTEST] GetCursorPos after forward move failed. GetLastError=" << ::GetLastError() << std::endl;
    }

    mouse.MoveRelative(-test_delta, 0, &cfg);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (::GetCursorPos(&after_return)) {
        std::cout << "--- Cursor after return: "
            << after_return.x << "," << after_return.y
            << " | observed delta from start: " << (after_return.x - before.x)
            << "," << (after_return.y - before.y) << " ---" << std::endl;
    }
    else {
        std::cerr << "[SELFTEST] GetCursorPos after return move failed. GetLastError=" << ::GetLastError() << std::endl;
    }

    if (after_forward.x == before.x && after_forward.y == before.y) {
        std::cerr << "[SELFTEST] Move function returned/logged, but Windows cursor position did not change." << std::endl;
    }
    std::cout << "--- Input backend self-test finished ---" << std::endl;
    return 0;
}

// --- 主函数 ---
int wmain(int argc, wchar_t* argv[]) {
    try {
        if (!::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
        }
        Config cfg = ConfigFromArgs(argc, argv);
        if (cfg.input_self_test) {
            return RunInputBackendSelfTest(cfg);
        }
        AimAssistant assistant(std::move(cfg));
        assistant.Run();
    }
    catch (const std::exception& e) {
        std::cerr << "\n\n[FATAL ERROR] An unrecoverable error occurred: " << e.what() << std::endl;
        std::cerr << "Please check the following:\n"
            << "1. The ONNX model file is in the correct directory.\n"
            << "2. Required runtime DLLs (onnxruntime.dll, opencv_world*.dll) are present or on PATH.\n"
            << "3. You have the correct NVIDIA drivers and CUDA/cuDNN installed for TensorRT support.\n"
            << "4. SendInput is the default input backend; driver mode requires a valid user-provided DLL path.\n\n"
            << "Press Enter to exit." << std::endl;
        std::cin.get();
        return -1;
    }
    std::cout << "\nProgram finished successfully." << std::endl;
    return 0;
}
