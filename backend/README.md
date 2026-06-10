# 后端源码结构

后端已经从单个 `main.cpp` 拆成按功能组织的目录。当前阶段仍通过 `main.cpp` 聚合 include 成一个编译单元，以保证行为、初始化顺序和链接方式不变。

## 入口

- `main.cpp`
  - 后端聚合入口。
  - 按顺序 include `backend/src` 下各模块。
  - 后续如果要改成真正多编译单元，优先从这里和 `CMakeLists.txt` 开始。

## 核心公共代码

- `backend/src/core/Pch.h`
  - Windows、OpenCV、ONNX Runtime、STL 等公共 include。

- `backend/src/core/Types.h`
  - 公共数据结构。
  - 包括 `Detection`、`TimingDetails`、`TargetEntity`、`TargetLock`。

## 运行配置

- `backend/src/runtime/Config.cpp`
  - 运行配置聚合模块。
  - include `config_parts` 下的配置模型、文本工具、过滤规则、live config 读取和命令行解析。

### Config 子模块

- `ConfigClass.inc`
  - `Config` 配置类和默认值。

- `TextHelpers.inc`
  - 文本解析、UTF-8/宽字符转换、热键名称、JSON 转义和时间戳。

- `GameFilters.inc`
  - CS2 类别 ID 解释、阵营筛选、检测部位筛选、自动选择部位优先级。

- `LiveConfigReaders.inc`
  - live config 文件读取。
  - live config 的 bool/int/double/string 读取辅助函数。

- `CommandLineConfig.inc`
  - 命令行配置聚合入口。
  - 具体解析拆在 `command_line_parts`：
    - `BackendArgs.inc`：模型、推理后端、输入后端、驱动 DLL 参数。
    - `AimArgs.inc`：自瞄范围、热键、瞄准模式、滤波器、目标部位参数。
    - `CrosshairArgs.inc`：图色准心参数。
    - `MovementAndTriggerArgs.inc`：鼠标移动、仿人类滑动、自动扳机、急停参数。
    - `FlagArgs.inc`：无值开关参数。
    - `ConfigFromArgs.inc`：最终 `ConfigFromArgs` 入口。

- `backend/src/runtime/SelfTest.cpp`
  - 输入后端自测逻辑。

- `backend/src/runtime/AppMain.cpp`
  - `wmain` 程序入口。
  - 初始化 DPI、读取配置、启动 `AimAssistant`。

## 输入后端

- `backend/src/input/InputBackends.cpp`
  - 鼠标键盘输入聚合模块。
  - include `input_parts` 下的 SendInput、DLL 驱动适配和高层 MouseController。

### Input 子模块

- `SendInputBackend.inc`
  - `InputBackend` 抽象接口。
  - Win32 `SendInputBackend`。
  - SendInput 键盘/鼠标基础操作。

- `DriverBackend.inc`
  - 用户提供 DLL 的驱动输入后端聚合入口。
  - 具体适配拆在 `driver_backend_parts`：
    - `PublicApi.inc`：构造、析构、鼠标/键盘公开操作。
    - `AbiTypes.inc`：标准 ABI、Logi legacy、LgMouse、DD 等函数指针类型。
    - `ButtonAndKeyHelpers.inc`：按钮和键盘码转换辅助。
    - `AbiBinding.inc`：DLL 加载和导出函数绑定。
    - `LoggingAndCleanup.inc`：日志、清理和状态输出。
    - `State.inc`：DriverBackend 内部状态。

- `MouseController.inc`
  - 输入后端选择和高层输入门面。
  - 具体逻辑拆在 `mouse_controller_parts`：
    - `PublicApi.inc`：鼠标移动、点击、滚轮、键盘按键统一入口。
    - `HumanizedMovement.inc`：仿人类移动 worker。
    - `State.inc`：输入后端、worker、队列和锁。

## 屏幕捕获

- `backend/src/capture/ScreenCapture.cpp`
  - 屏幕捕获聚合模块。
  - include `capture_parts` 下的 D3D 捕获器、帧源模式和工厂。

### Capture 子模块

- `ScreenCapturer.inc`
  - D3D/DXGI 捕获器聚合入口。
  - 具体逻辑拆在 `screen_capturer_parts`：
    - `PublicApi.inc`：D3D 初始化、资源释放、裁剪帧读取。
    - `OutputSelection.inc`：主显示器/DXGI output 选择。
    - `State.inc`：D3D/DXGI 资源指针和屏幕尺寸。
    - `CenterCropRegion.inc`：屏幕中心裁剪区域计算。

- `FrameSourceBase.inc`
  - `FrameSource` 抽象接口。

- `DirectFrameSource.inc`
  - 同步直接捕获。

- `LatestFrameSource.inc`
  - 异步 latest-frame 捕获。
  - 后台捕获线程和最新帧缓存。

- `FrameSourceFactory.inc`
  - 根据配置创建直接捕获或异步捕获。

## YOLO 推理

- `backend/src/detection/ObjectDetector.cpp`
  - YOLO 检测器聚合入口。
  - 具体逻辑拆在 `object_detector_parts`：
    - `PublicApi.inc`：ONNX Runtime 初始化、TensorRT/CUDA/CPU 后端选择、预处理、推理、NMS 后处理。
    - `State.inc`：ONNX Runtime 会话、输入输出名、缓存 Mat 和检测缓存。

## 自瞄核心

- `backend/src/aim/AimFilters.cpp`
  - PID。
  - OneEuro。
  - 低通滤波基础组件。

- `backend/src/aim/AimAssistant.cpp`
  - `AimAssistant` 类壳。
  - 具体结构拆在 `aim_assistant_parts` 和 `assistant_parts`。

### AimAssistant 类壳

- `aim_assistant_parts/Lifecycle.inc`
  - 构造、析构、启动时配置摘要。

- `aim_assistant_parts/RunLoop.inc`
  - 主循环。
  - 捕获、推理、目标选择、移动、自动扳机、可视化和采样日志调用顺序。

- `aim_assistant_parts/State.inc`
  - `AimAssistant` 成员变量。

### AimAssistant 功能片段

- `assistant_parts/RuntimeConfig.inc`
  - live config 热更新聚合入口。
  - 子模块：
    - `runtime_config_parts/ValidationAndReload.inc`：配置校验、检测器/捕获/鼠标重载。
    - `runtime_config_parts/LiveConfigPolling.inc`：live config 文件轮询。
    - `runtime_config_parts/LiveConfigApply.inc`：应用 live config 聚合入口。
    - `runtime_config_parts/StartupInstructions.inc`：启动说明打印。

- `runtime_config_parts/live_config_apply_parts`
  - `ReadLiveConfigSnapshot.inc`：从 live config 读取下一份配置。
  - `ChangeDetection.inc`：判断 detector/capture/mouse/aim/autoclick 是否变化。
  - `RuntimeReloads.inc`：运行时重载检测器、捕获、输入后端。
  - `CommitAndLogging.inc`：提交配置、重置状态、打印配置摘要。
  - `ApplyLiveConfigEntry.inc`：`applyLiveConfig` 入口。

- `assistant_parts/CrosshairTracking.inc`
  - 图色准心定位聚合入口。
  - 子模块：
    - `crosshair_tracking_parts/Geometry.inc`：框中心、IoU 等几何工具。
    - `crosshair_tracking_parts/CrosshairState.inc`：准心状态重置。
    - `crosshair_tracking_parts/CrosshairUpdate.inc`：准心跟踪和平滑更新。
    - `crosshair_tracking_parts/ColorBlockDetection.inc`：图色连通域检测。

- `assistant_parts/TargetSelection.inc`
  - YOLO 目标选择聚合入口。
  - 子模块：
    - `target_selection_parts/EntityGrouping.inc`：head/body 合并成同一个目标实体。
    - `target_selection_parts/PartLocking.inc`：自动/自定义部位锁定。
    - `target_selection_parts/LockContinuity.inc`：已有目标锁定连续性、切换评分。
    - `target_selection_parts/BestTargetSelection.inc`：新目标选择入口。

- `assistant_parts/AimMovement.inc`
  - 鼠标移动聚合入口。
  - 子模块：
    - `aim_movement_parts/MouseDelta.inc`：移动量限制、子像素累积。
    - `aim_movement_parts/AimStateReset.inc`：移动状态、滤波状态重置。
    - `aim_movement_parts/AimFiltering.inc`：PID/OneEuro 应用和输出稳定。
    - `aim_movement_parts/VisualMoveCoalescing.inc`：小移动合并。
    - `aim_movement_parts/AimMappingAndTelemetry.inc`：目标偏移到鼠标移动映射、遥测。
    - `aim_movement_parts/AimInputHandling.inc`：热键判断和移动输出。

- `assistant_parts/AimSampleLogging.inc`
  - 自瞄采样 JSONL 日志聚合入口。
  - 子模块：
    - `aim_sample_logging_parts/SettingsSnapshot.inc`：配置快照 JSON。
    - `aim_sample_logging_parts/LogFileLifecycle.inc`：日志文件打开、设置变更写入、flush。
    - `aim_sample_logging_parts/SampleFrameWriter.inc`：单帧 sample 写入。

- `assistant_parts/AutoTrigger.inc`
  - 自动扳机聚合入口。
  - 子模块：
    - `auto_trigger_parts/AutoTriggerState.inc`：扳机状态重置和随机延迟。
    - `auto_trigger_parts/TargetAlignment.inc`：目标对齐和目标连续性判断。
    - `auto_trigger_parts/AutoStopKeys.inc`：自动急停按键选择和按键释放。
    - `auto_trigger_parts/HoldMode.inc`：长按模式鼠标按下/释放。
    - `auto_trigger_parts/AutoTriggerFlow.inc`：自动扳机主流程。

- `assistant_parts/TelemetryVisualization.inc`
  - 控制台统计。
  - YOLO 可视化窗口绘制。

## 后续重构建议

当前 `.inc` 片段是为了低风险拆分。后续如果要继续提高工程化程度，可以按下面顺序迁移：

1. 把 `Config.cpp` 拆成 `Config.h/.cpp`。
2. 把 `InputBackends.cpp` 拆成 `InputBackend.h`、`SendInputBackend.cpp`、`DriverBackend.cpp`、`MouseController.cpp`。
3. 把 `AimAssistant` 的成员变量移动到 `AimAssistant.h`。
4. 将 `assistant_parts/*.inc` 改成真正的 `.cpp` 成员函数实现。
5. 更新 `CMakeLists.txt`，让每个 `.cpp` 独立编译。
