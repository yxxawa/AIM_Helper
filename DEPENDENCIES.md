# 依赖下载说明

程序右上角的“依赖设置”会先读取本机环境：

- 优先通过 `nvidia-smi` 读取 GPU、显卡驱动版本、CUDA 支持版本。
- 如果 `nvidia-smi` 不可用，会退回 Windows `Win32_VideoController` 读取显卡名称和驱动版本。
- 根据当前推理后端和缺失 DLL，界面会显示对应的下载入口。
- 对公开可直链下载的依赖，界面会显示“下载/继续”，下载到程序目录的 `downloads\`。
- 未完成文件保存为 `.part`，再次点击同一下载项会使用 HTTP Range 断点续传。
- CUDA、cuDNN、TensorRT、NVIDIA 驱动通常需要登录、接受协议或手动选择硬件版本，界面只打开官网，不伪造直链。

## DLL 来源

| DLL / 文件组 | 来源 |
| --- | --- |
| `onnxruntime.dll`、`onnxruntime_providers_cuda.dll`、`onnxruntime_providers_tensorrt.dll` | [ONNX Runtime Releases](https://github.com/microsoft/onnxruntime/releases) |
| `cudart64_12.dll`、`cublas*.dll`、`nvrtc*.dll`、`cusolver*.dll`、`cusparse*.dll` | [CUDA Toolkit Archive](https://developer.nvidia.com/cuda-toolkit-archive) |
| `cudnn*.dll` | [cuDNN Archive](https://developer.nvidia.com/cudnn-archive) |
| `nvinfer*.dll`、`nvonnxparser_10.dll` | [TensorRT Download](https://developer.nvidia.com/tensorrt/download) |
| `opencv_world*.dll` | [OpenCV Releases](https://opencv.org/releases/) |

## 程序内自动下载支持

当前支持直接下载：

- ONNX Runtime Windows x64 GPU / CPU zip。
- OpenCV Windows 安装包或压缩包入口。

当前不直接下载：

- NVIDIA Driver。
- CUDA Toolkit。
- cuDNN。
- TensorRT。

原因是这些页面通常需要用户选择硬件版本、登录 NVIDIA 账户、接受许可协议，或下载链接会随版本和会话变化。

## 版本选择

- `cudart64_12.dll`、`cublas64_12.dll`、`cusparse64_12.dll` 表示 CUDA 12 系列。
- `cudnn64_9.dll` 表示 cuDNN 9 系列。
- `nvinfer_10.dll`、`nvonnxparser_10.dll` 表示 TensorRT 10 系列。
- 如果 `nvidia-smi` 显示的 CUDA 支持版本低于你下载的 CUDA 12.x 版本，请先更新 NVIDIA 显卡驱动，或改用更低版本且互相匹配的一整套 ONNX Runtime / CUDA / cuDNN / TensorRT。

## 推荐放置方式

最简单的方式是把后端需要的 DLL 放进：

```text
host\bin\Release\net9.0-windows\backend\
```

也可以按类别放到固定目录：

```text
deps\onnxruntime\
deps\onnxruntime\runtimes\win-x64\native\
deps\opencv\x64\vc16\bin\
deps\opencv\build\x64\vc16\bin\
deps\cuda\bin\
deps\tensorrt\bin\
deps\tensorrt\lib\
```

如果不想移动文件，可以在程序右上角点“设置依赖”，对缺失卡片使用“选择位置”手动指定目录。

## CPU 后端最小依赖

CPU 后端只需要：

- `onnxruntime.dll`
- `opencv_world*.dll`
- 模型 `.onnx`

如果你只是想先确认程序能启动，建议先用 CPU 后端。

## CUDA 后端常见 DLL

CUDA 后端通常需要：

- `onnxruntime.dll`
- `onnxruntime_providers_shared.dll`
- `onnxruntime_providers_cuda.dll`
- `cudart64_12.dll`
- `cublas64_12.dll`
- `cublasLt64_12.dll`
- `cudnn*.dll`
- 其他随 ONNX Runtime/CUDA/cuDNN 版本要求的 DLL

## TensorRT 后端常见 DLL

TensorRT 后端通常需要：

- CUDA 后端全部 DLL。
- `onnxruntime_providers_tensorrt.dll`
- `nvinfer_10.dll`
- `nvinfer_plugin_10.dll`
- `nvonnxparser_10.dll`
- TensorRT 包内其他相关 DLL。

