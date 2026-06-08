# AIM_Helper

这是一个 Windows 桌面端离线 YOLO 检测实验项目。项目包含 WebView2 控制面板、C++ 屏幕捕获/推理后端，以及可切换的鼠标输入后端。

本仓库只整理源码，不包含模型、GPU 运行库 DLL、OpenCV 二进制文件、ONNX Runtime 包、驱动 DLL、TensorRT engine 缓存或任何本机运行产物。

## 项目边界

- 面向单机、离线、可控测试环境。
- 不提供反作弊绕过、隐藏行为、漏洞驱动、常驻服务或规避检测代码。
- 默认输入后端是 Win32 `SendInput`。
- 驱动 后端只加载用户自行提供的 DLL。本仓库不包含、不分发、不下载任何驱动文件。
- YOLO `.onnx` 模型需要用户自行提供。

## 目录结构

```text
.
├─ main.cpp                         # C++ 后端：截图、ONNX 推理、目标选择、鼠标控制
├─ InputSimulator.hpp               # 历史/参考输入头文件
├─ CMakeLists.txt                   # C++ 后端构建脚本
├─ frontend/
│  ├─ index.html                    # WebView 前端页面
│  ├─ app.js                        # UI 状态、依赖检测桥接、实时配置
│  └─ styles.css                    # UI 样式和布局
├─ host/
│  ├─ AIM_Helper.Host.csproj        # WinForms + WebView2 宿主
│  ├─ MainForm.cs                   # 启停后端、依赖检测、下载逻辑
│  ├─ Program.cs
│  └─ app.manifest                  # 请求管理员权限
├─ driver/
│  └─ DriverBridgeABI.h             # 可选驱动桥接 ABI 参考
├─ DEPENDENCIES.md                  # 依赖下载说明
├─ DRIVER_EXTENSION.md              # 驱动后端扩展说明
└─ WEBVIEW_FRONTEND.md              # 前端和宿主桥接说明
```

## 系统要求

### 基础环境

- Windows 10/11 x64。
- NVIDIA GPU 不是必需项。CPU 后端不需要 CUDA/TensorRT，但推理速度会更慢。
- `host/app.manifest` 会请求管理员权限，主要用于可选 DD/驱动输入后端。普通 `SendInput` 通常不需要管理员权限。

### 构建工具

- [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/) 或 Visual Studio 2022。
  - 安装 **Desktop development with C++**。
  - 勾选 MSVC、Windows 10/11 SDK、CMake 工具；Ninja 可选。
- [.NET 9 SDK](https://dotnet.microsoft.com/download/dotnet/9.0)。
- [Microsoft Edge WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/)。
- [CMake 3.20+](https://cmake.org/download/)。

## 运行依赖

所有推理后端都需要：

- [OpenCV Windows 包](https://opencv.org/releases/)，需要其中的 `opencv_world*.dll`。
- [ONNX Runtime Releases](https://github.com/microsoft/onnxruntime/releases)。
  - CPU 包：`onnxruntime-win-x64-*.zip`。
  - GPU 包：`onnxruntime-win-x64-gpu-*.zip`。

CUDA 后端额外需要：

- [NVIDIA Driver](https://www.nvidia.com/Download/index.aspx)。
- [CUDA Toolkit Archive](https://developer.nvidia.com/cuda-toolkit-archive)。
- [cuDNN Archive](https://developer.nvidia.com/cudnn-archive)。
- ONNX Runtime GPU 包内的 `onnxruntime_providers_cuda.dll`。

TensorRT 后端额外需要：

- CUDA 后端所需的全部依赖。
- [TensorRT Download](https://developer.nvidia.com/tensorrt/download)。
- ONNX Runtime GPU 包内的 `onnxruntime_providers_tensorrt.dll`。
- TensorRT 10 相关 DLL，例如 `nvinfer_10.dll`、`nvonnxparser_10.dll`。

版本必须匹配。例如 CUDA 12.x、cuDNN 9.x、TensorRT 10.x、ONNX Runtime GPU 版本和 NVIDIA 显卡驱动需要互相兼容。

## 推荐文件放置方式

宿主程序的依赖检测会搜索常用目录。推荐目录如下：

```text
项目根目录/
├─ models/
│  └─ cs2yolomaax.onnx              # 用户自行提供，本仓库不包含
├─ deps/
│  ├─ onnxruntime/
│  ├─ opencv/
│  ├─ cuda/bin/
│  └─ tensorrt/bin/
├─ drivers/
│  └─ dd60300.dll                   # 可选用户 DLL，本仓库不包含
└─ host/bin/Release/net9.0-windows/
   └─ backend/
      └─ AIM_Helper_Backend.exe
```

也可以把运行时 DLL 直接放在后端程序旁边：

```text
host/bin/Release/net9.0-windows/backend/
```

前端里的“模型路径”默认留空。留空时宿主会自动搜索：

- `models/cs2yolomaax.onnx`
- `backend/models/cs2yolomaax.onnx`
- 后端可执行文件所在目录

如果模型文件名不同，可以在界面里选择模型，或填写 `.onnx` 文件的绝对路径。

## 构建步骤

请打开 “Developer PowerShell for VS 2022”。

### 1. 构建 WebView 宿主

```powershell
dotnet restore .\host\AIM_Helper.Host.csproj
dotnet build .\host\AIM_Helper.Host.csproj -c Release
```

输出位置：

```text
host\bin\Release\net9.0-windows\AIM_Helper.Host.exe
```

### 2. 构建 C++ 后端

需要显式传入 OpenCV 和 ONNX Runtime 路径。示例：

```powershell
$env:ONNXRUNTIME_DIR = "D:\deps\onnxruntime-win-x64-gpu-1.24.3"

cmake -S . -B build -G Ninja `
  -DOpenCV_DIR="D:\deps\opencv\build" `
  -DONNXRUNTIME_DIR="$env:ONNXRUNTIME_DIR" `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

如果使用 Visual Studio 生成器：

```powershell
cmake -S . -B build `
  -DOpenCV_DIR="D:\deps\opencv\build" `
  -DONNXRUNTIME_DIR="D:\deps\onnxruntime-win-x64-gpu-1.24.3"

cmake --build build --config Release
```

CMake 的后构建步骤会把后端程序复制到：

```text
host\bin\Release\net9.0-windows\backend\
host\bin\Release\net9.0-windows\publish\backend\
```

如果复制失败，可以手动创建目录并复制 `AIM_Helper_Backend.exe`。

## 首次运行

1. 构建 Host 和 C++ 后端。
2. 把 `.onnx` 模型放到 `models/`，或在界面里选择模型路径。
3. 把所需运行 DLL 放到 `deps/` 或 `host/bin/Release/net9.0-windows/backend/`。
4. 启动：

```powershell
Start-Process .\host\bin\Release\net9.0-windows\AIM_Helper.Host.exe
```

5. 点击右上角依赖状态按钮。
   - 依赖完整时显示环境状态。
   - 缺少依赖时会按一项一个卡片显示。
   - 部分依赖支持程序内直接下载，并显示进度和断点续传。
   - NVIDIA Driver、CUDA、cuDNN、TensorRT 通常需要手动下载，因为涉及硬件版本选择、NVIDIA 账号、许可协议或动态下载链接。

## 推理后端说明

### CPU

最简单的模式。

需要：

- `onnxruntime.dll`
- `opencv_world*.dll`
- 模型 `.onnx`

在界面选择 `CPU`。

### CUDA

需要：

- ONNX Runtime GPU 包。
- `onnxruntime.dll`
- `onnxruntime_providers_shared.dll`
- `onnxruntime_providers_cuda.dll`
- CUDA 12 运行时 DLL、cuBLAS、cuDNN 等相关 DLL。
- OpenCV 运行时 DLL。

在界面选择 `CUDA`。

### TensorRT

需要：

- CUDA 后端的全部依赖。
- `onnxruntime_providers_tensorrt.dll`
- TensorRT 10 运行时 DLL。

在界面选择 `TensorRT`。

TensorRT 第一次启动可能很慢，因为它会生成 `engine_cache`。该缓存与机器、GPU、驱动、模型结构强相关，不应提交到仓库。

## 输入后端说明

### SendInput

默认输入后端，不需要额外 DLL。

### Driver

加载用户提供的 DLL，并调用 `DRIVER_EXTENSION.md` 中定义的导出函数。项目不包含驱动、签名二进制、厂商 DLL 或绕过逻辑。

### DD 驱动

可选 DD 兼容 DLL 模式。DLL 不包含在仓库中。需要时放到 `drivers/`，或在界面里选择路径。

## 配置文件

WebView UI 会写入：

- `runtime-config.json`
- `runtime-config.live`

这些文件是运行状态，已被 `.gitignore` 忽略。大部分设置会通过 `runtime-config.live` 实时生效，不需要重启后端。

关键默认行为：

- 模型路径默认留空。
- 模型路径留空表示自动搜索。
- 日志默认关闭。
- 首页固定高度，左中右三列独立滚动。

## 程序内依赖下载

Host 内置依赖检测和下载辅助逻辑：

- ONNX Runtime：支持从 GitHub Releases 解析并下载包。
- OpenCV：支持打开/辅助下载 Windows 包。
- NVIDIA Driver/CUDA/cuDNN/TensorRT：打开官方页面，通常需要用户手动选择版本。

详细说明见 `DEPENDENCIES.md`。

## 常见问题

### `OrtSessionOptionsAppendExecutionProvider_TensorRT: Failed to load shared library`

TensorRT Provider DLL 或其依赖缺失。处理方式：

- 切换到 CPU 后端；或
- 把 TensorRT、CUDA、cuDNN、ONNX Runtime GPU 所需 DLL 放到程序搜索目录。

### 缺少 `opencv_world*.dll`

安装 OpenCV，并把包含 `opencv_world*.dll` 的目录放到：

- `deps/opencv/build/x64/vc16/bin`
- 后端可执行文件目录
- 系统 PATH

### 找不到模型

放置默认模型：

```text
models\cs2yolomaax.onnx
```

或在界面输入 `.onnx` 绝对路径。

### TensorRT 首次启动很慢

这是正常情况。TensorRT 正在生成 `engine_cache`。如果没有明显卡死，等待初始化完成。

### WebView2 启动失败

安装 Microsoft Edge WebView2 Runtime。

## 发布到 GitHub 前检查

不要提交：

- `models/`
- `deps/`
- `backend/`
- `drivers/`
- `engine_cache/`
- `downloads/`
- `host/bin/`
- `host/obj/`
- NVIDIA/OpenCV/ONNX Runtime/TensorRT/CUDA/cuDNN DLL
- 驱动 DLL 或签名二进制
- 本机绝对路径
- 运行配置文件

推荐验证：

```powershell
node --check .\frontend\app.js
dotnet build .\host\AIM_Helper.Host.csproj -c Release
cmake -S . -B build -DOpenCV_DIR="D:\deps\opencv\build" -DONNXRUNTIME_DIR="D:\deps\onnxruntime-win-x64-gpu-1.24.3"
cmake --build build --config Release
```

## 许可证

当前整理目录未包含 `LICENSE`。如果你要把它作为开源项目发布，请先添加明确的许可证文件，否则别人没有合法复用、修改、分发代码的权限。
