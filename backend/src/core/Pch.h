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
#include <ctime>

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

