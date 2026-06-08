using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace AIM_Helper.Host;

public sealed partial class MainForm
{
    private void PostDependencyStatus()
    {
        using JsonDocument document = ParseConfig(lastConfigJson);
        JsonElement config = document.RootElement;
        List<DependencyCheck> checks = BuildDependencyChecks(config, DependencyCheckScope.Full);
        SystemProbe system = ProbeSystem();
        checks.Add(BuildNvidiaDriverCheck(system));
        List<DependencyDownload> downloads = BuildDependencyDownloads(config, checks, system);
        int missingCount = checks.Count(item => item.Required && !item.Ok);
        string payload = JsonSerializer.Serialize(new
        {
            ok = missingCount == 0,
            missingCount,
            system,
            downloads,
            items = checks.Select(item => item.Payload)
        });
        PostJson("host:dependencies", payload);
    }

    private static List<DependencyCheck> BuildDependencyChecks(JsonElement config, DependencyCheckScope scope)
    {
        List<DependencyCheck> checks = [];
        string? backendPath = ResolveBackendPath(config);
        string workingDirectory = backendPath is null
            ? AppContext.BaseDirectory
            : ResolveRuntimeDirectory(backendPath);
        List<string> runtimeDirectories = BuildDependencySearchDirectories(workingDirectory, config).ToList();
        string inferenceBackend = GetString(config, "backend", "cpu").ToLowerInvariant();
        string inputBackend = GetString(config, "inputBackend", "sendinput").ToLowerInvariant();
        bool checkCudaDependencies = scope == DependencyCheckScope.Full || inferenceBackend is "cuda" or "tensorrt";
        bool checkTensorRtDependencies = scope == DependencyCheckScope.Full || inferenceBackend == "tensorrt";

        AddDependency(checks,
            key: "backendExe",
            title: "C++ 后端程序",
            description: "AIM_Helper_Backend.exe",
            detail: "未找到后端程序，程序无法启动后端。",
            required: true,
            ok: backendPath is not null,
            path: backendPath,
            expected: Path.Combine(AppContext.BaseDirectory, "backend", "AIM_Helper_Backend.exe"),
            kind: "file",
            downloadUrl: string.Empty);

        string modelPath = ResolveModelPathForBackend(config);
        AddDependency(checks,
            key: "modelFile",
            title: "ONNX 模型文件",
            description: "当前 modelPath 指向的 .onnx 文件",
            detail: "未找到模型文件，YOLO 无法推理。",
            required: true,
            ok: File.Exists(modelPath),
            path: File.Exists(modelPath) ? modelPath : null,
            expected: modelPath,
            kind: "file",
            downloadUrl: string.Empty);

        string? onnxRuntime = FindFileInDirectories(runtimeDirectories, "onnxruntime.dll");
        AddDependency(checks,
            key: "onnxRuntimeDir",
            title: "ONNX Runtime",
            description: "onnxruntime.dll 所在目录",
            detail: "未找到 onnxruntime.dll，后端无法加载推理运行时。",
            required: true,
            ok: onnxRuntime is not null,
            path: onnxRuntime,
            expected: "onnxruntime.dll",
            kind: "folder",
            downloadUrl: OnnxRuntimeDownloadUrl);

        string? opencv = FindFilePatternInDirectories(runtimeDirectories, "opencv_world*.dll");
        AddDependency(checks,
            key: "opencvDir",
            title: "OpenCV Runtime",
            description: "opencv_world*.dll 所在目录",
            detail: "未找到 OpenCV 运行库，后端无法启动图像处理。",
            required: true,
            ok: opencv is not null,
            path: opencv,
            expected: "opencv_world*.dll",
            kind: "folder",
            downloadUrl: OpenCvDownloadUrl);

        if (checkCudaDependencies)
        {
            string? onnxRuntimeDirectory = string.IsNullOrWhiteSpace(onnxRuntime)
                ? null
                : Path.GetDirectoryName(onnxRuntime);
            string? cudaProvider = string.IsNullOrWhiteSpace(onnxRuntimeDirectory)
                ? null
                : FindFileInDirectories([onnxRuntimeDirectory], "onnxruntime_providers_cuda.dll");
            AddDependency(checks,
                key: "onnxRuntimeDir",
                title: "ONNX Runtime CUDA Provider",
                description: "onnxruntime_providers_cuda.dll",
                detail: "当前推理后端需要 CUDA Provider DLL，且必须与当前 onnxruntime.dll 在同一目录。",
                required: true,
                ok: cudaProvider is not null,
                path: cudaProvider,
                expected: string.IsNullOrWhiteSpace(onnxRuntimeDirectory)
                    ? "onnxruntime_providers_cuda.dll"
                    : Path.Combine(onnxRuntimeDirectory, "onnxruntime_providers_cuda.dll"),
                kind: "folder",
                downloadUrl: OnnxRuntimeDownloadUrl);

            string? cudaRuntime = FindFilePatternInDirectories(runtimeDirectories, "cudart64*.dll");
            AddDependency(checks,
                key: "cudaDir",
                title: "CUDA Runtime",
                description: "cudart64*.dll 所在目录",
                detail: "当前推理后端需要 NVIDIA CUDA 运行库。",
                required: true,
                ok: cudaRuntime is not null,
                path: cudaRuntime,
                expected: "cudart64*.dll",
                kind: "folder",
                downloadUrl: CudaToolkitArchiveUrl);

            string? cublas = FindFilePatternInDirectories(runtimeDirectories, "cublas64*.dll");
            string? cublasLt = FindFilePatternInDirectories(runtimeDirectories, "cublasLt64*.dll");
            AddDependency(checks,
                key: "cudaDir",
                title: "CUDA cuBLAS",
                description: "cublas64*.dll / cublasLt64*.dll",
                detail: "当前推理后端需要 CUDA BLAS 运行库。",
                required: true,
                ok: cublas is not null && cublasLt is not null,
                path: cublas ?? cublasLt,
                expected: "cublas64*.dll + cublasLt64*.dll",
                kind: "folder",
                downloadUrl: CudaToolkitArchiveUrl);

            string? nvrtc = FindFilePatternInDirectories(runtimeDirectories, "nvrtc64*.dll");
            string? nvrtcBuiltins = FindFilePatternInDirectories(runtimeDirectories, "nvrtc-builtins*.dll");
            AddDependency(checks,
                key: "cudaDir",
                title: "CUDA NVRTC",
                description: "nvrtc*.dll",
                detail: "TensorRT/CUDA 可能需要 NVRTC 运行时编译库。",
                required: true,
                ok: nvrtc is not null && nvrtcBuiltins is not null,
                path: nvrtc ?? nvrtcBuiltins,
                expected: "nvrtc64*.dll + nvrtc-builtins*.dll",
                kind: "folder",
                downloadUrl: CudaToolkitArchiveUrl);

            string? cusolver = FindFilePatternInDirectories(runtimeDirectories, "cusolver64*.dll");
            string? cusparse = FindFilePatternInDirectories(runtimeDirectories, "cusparse64*.dll");
            AddDependency(checks,
                key: "cudaDir",
                title: "CUDA Solver/Sparse",
                description: "cusolver*.dll / cusparse*.dll",
                detail: "当前推理后端需要 CUDA Solver/Sparse 运行库。",
                required: true,
                ok: cusolver is not null && cusparse is not null,
                path: cusolver ?? cusparse,
                expected: "cusolver64*.dll + cusparse64*.dll",
                kind: "folder",
                downloadUrl: CudaToolkitArchiveUrl);

            string? cudnnCore = FindFilePatternInDirectories(runtimeDirectories, "cudnn64_*.dll");
            string? cudnnOps = FindFilePatternInDirectories(runtimeDirectories, "cudnn_ops64_*.dll");
            AddDependency(checks,
                key: "cudaDir",
                title: "cuDNN Runtime",
                description: "cudnn*.dll",
                detail: "ONNX Runtime CUDA Provider 需要 cuDNN；解压后请保持所有 cudnn*.dll 在同一目录。",
                required: true,
                ok: cudnnCore is not null && cudnnOps is not null,
                path: cudnnCore ?? cudnnOps,
                expected: "cudnn64_*.dll + cudnn_ops64_*.dll",
                kind: "folder",
                downloadUrl: CudnnArchiveUrl);
        }

        if (checkTensorRtDependencies)
        {
            string? onnxRuntimeDirectory = string.IsNullOrWhiteSpace(onnxRuntime)
                ? null
                : Path.GetDirectoryName(onnxRuntime);
            string? tensorrtProvider = string.IsNullOrWhiteSpace(onnxRuntimeDirectory)
                ? null
                : FindFileInDirectories([onnxRuntimeDirectory], "onnxruntime_providers_tensorrt.dll");
            AddDependency(checks,
                key: "onnxRuntimeDir",
                title: "ONNX Runtime TensorRT Provider",
                description: "onnxruntime_providers_tensorrt.dll",
                detail: "TensorRT 后端需要 ONNX Runtime TensorRT Provider，且必须与当前 onnxruntime.dll 在同一目录。",
                required: true,
                ok: tensorrtProvider is not null,
                path: tensorrtProvider,
                expected: string.IsNullOrWhiteSpace(onnxRuntimeDirectory)
                    ? "onnxruntime_providers_tensorrt.dll"
                    : Path.Combine(onnxRuntimeDirectory, "onnxruntime_providers_tensorrt.dll"),
                kind: "folder",
                downloadUrl: OnnxRuntimeDownloadUrl);

            string? tensorrtRuntime = FindFilePatternInDirectories(runtimeDirectories, "nvinfer*.dll");
            AddDependency(checks,
                key: "tensorrtDir",
                title: "TensorRT Runtime",
                description: "nvinfer*.dll 所在目录",
                detail: "TensorRT 后端需要 NVIDIA TensorRT 运行库。",
                required: true,
                ok: tensorrtRuntime is not null,
                path: tensorrtRuntime,
                expected: "nvinfer*.dll",
                kind: "folder",
                downloadUrl: TensorRtDownloadUrl);

            string? tensorrtParser = FindFileInDirectories(runtimeDirectories, "nvonnxparser_10.dll");
            AddDependency(checks,
                key: "tensorrtDir",
                title: "TensorRT ONNX Parser",
                description: "nvonnxparser_10.dll",
                detail: "TensorRT 后端解析 ONNX 模型需要 NVIDIA ONNX Parser。",
                required: true,
                ok: tensorrtParser is not null,
                path: tensorrtParser,
                expected: "nvonnxparser_10.dll",
                kind: "folder",
                downloadUrl: TensorRtDownloadUrl);
        }

        if (inputBackend is "driver" or "dd")
        {
            string driverDllPath = ResolveDriverDllPathForBackend(config);
            bool driverDllOk = !string.IsNullOrWhiteSpace(driverDllPath) && File.Exists(driverDllPath);
            AddDependency(checks,
                key: "driverDll",
                title: inputBackend == "dd" ? "DD 驱动 DLL" : "输入驱动 DLL",
                description: "可手动选择，也可放入固定 drivers 目录自动识别",
                detail: "当前输入后端需要有效的 DLL；路径为空时会自动查找固定目录。",
                required: true,
                ok: driverDllOk,
                path: driverDllOk ? driverDllPath : null,
                expected: driverDllOk ? driverDllPath : DriverDllExpectedText(inputBackend),
                kind: "file",
                downloadUrl: string.Empty);
        }

        return checks;
    }

    private static DependencyCheck BuildNvidiaDriverCheck(SystemProbe system)
    {
        bool hasNvidiaGpu = !string.IsNullOrWhiteSpace(system.gpuName) &&
            system.gpuName.Contains("NVIDIA", StringComparison.OrdinalIgnoreCase);
        bool hasDriver = !string.IsNullOrWhiteSpace(system.driverVersion);
        bool ok = hasNvidiaGpu && hasDriver;
        string? detected = ok
            ? $"{system.gpuName} | 驱动 {system.driverVersion}"
            : null;

        return new DependencyCheck(ok, true, new DependencyPayload(
            key: "nvidiaDriver",
            title: "NVIDIA 显卡驱动",
            description: "本机 NVIDIA GPU 驱动",
            detail: ok
                ? "已检测到 NVIDIA 显卡驱动。"
                : "未检测到 NVIDIA 显卡驱动；CUDA/TensorRT 需要匹配的 NVIDIA 驱动。",
            required: true,
            ok,
            path: detected,
            expected: "NVIDIA GPU + 显卡驱动",
            kind: "none",
            downloadUrl: NvidiaDriverDownloadUrl));
    }

    private static void AddDependency(
        List<DependencyCheck> checks,
        string key,
        string title,
        string description,
        string detail,
        bool required,
        bool ok,
        string? path,
        string expected,
        string kind,
        string downloadUrl)
    {
        checks.Add(new DependencyCheck(ok, required, new DependencyPayload(
            key,
            title,
            description,
            detail,
            required,
            ok,
            path,
            expected,
            kind,
            downloadUrl)));
    }

    private static List<DependencyDownload> BuildDependencyDownloads(
        JsonElement config,
        List<DependencyCheck> checks,
        SystemProbe system)
    {
        List<DependencyDownload> downloads = [];
        if (HasMissingDownload(checks, NvidiaDriverDownloadUrl))
        {
            string gpuDetail = string.IsNullOrWhiteSpace(system.gpuName)
                ? "当前没有读取到 NVIDIA GPU 信息；如果要使用 CUDA/TensorRT，请先安装匹配显卡的 NVIDIA 驱动。"
                : $"检测到 {system.gpuName}，驱动 {TextOrUnknown(system.driverVersion)}，nvidia-smi CUDA {TextOrUnknown(system.cudaVersion)}。";
            AddDownload(downloads, "nvidia-driver", "NVIDIA 显卡驱动", gpuDetail, NvidiaDriverDownloadUrl, canDownload: false, fileName: string.Empty);
        }

        if (HasMissingDownload(checks, OnnxRuntimeDownloadUrl))
        {
            string packageHint = HasMissingGpuOnnxRuntimeProvider(checks)
                ? "可自动下载 Windows x64 GPU 包，文件名通常是 onnxruntime-win-x64-gpu-*.zip。"
                : "可自动下载 Windows x64 CPU 包，文件名通常是 onnxruntime-win-x64-*.zip。";
            AddDownload(downloads, "onnxruntime", "ONNX Runtime", packageHint, OnnxRuntimeDownloadUrl, canDownload: true, fileName: "onnxruntime.zip");
        }

        if (HasMissingDownload(checks, CudaToolkitArchiveUrl))
        {
            string cudaHint = string.IsNullOrWhiteSpace(system.cudaVersion)
                ? "将从 NVIDIA 官方 redist 直链下载 CUDA 12.x 组件 zip，并自动解压缺失 DLL。"
                : $"nvidia-smi 报告最高支持 CUDA {system.cudaVersion}；将下载 CUDA 12.x 官方 redist 组件并自动解压。";
            AddDownload(downloads, "cuda", "CUDA Runtime DLLs", cudaHint, CudaToolkitArchiveUrl, canDownload: true, fileName: "cuda-redist.zip");
        }

        if (HasMissingDownload(checks, CudnnArchiveUrl))
        {
            AddDownload(downloads, "cudnn", "cuDNN Runtime DLLs", "将从 NVIDIA 官方 redist 直链下载 cuDNN 9.x for CUDA 12.x，并自动解压缺失 DLL。", CudnnArchiveUrl, canDownload: true, fileName: "cudnn-redist.zip");
        }

        if (HasMissingDownload(checks, TensorRtDownloadUrl))
        {
            AddDownload(downloads, "tensorrt", "TensorRT", "TensorRT 需要登录 NVIDIA 并接受协议；下载 TensorRT 10.x Windows x64 zip。", TensorRtDownloadUrl, canDownload: false, fileName: string.Empty);
        }

        if (HasMissingDownload(checks, OpenCvDownloadUrl))
        {
            AddDownload(downloads, "opencv", "OpenCV", "可自动下载 Windows 安装包；完成后安装或解压，再选择 opencv_world*.dll 所在目录。", OpenCvDownloadUrl, canDownload: true, fileName: "opencv-windows.exe");
        }

        return downloads;
    }

    private static bool HasMissingDownload(List<DependencyCheck> checks, string url)
    {
        return checks.Any(item =>
            item.Required &&
            !item.Ok &&
            string.Equals(item.Payload.downloadUrl, url, StringComparison.OrdinalIgnoreCase));
    }

    private static bool HasMissingGpuOnnxRuntimeProvider(List<DependencyCheck> checks)
    {
        return checks.Any(item =>
            item.Required &&
            !item.Ok &&
            string.Equals(item.Payload.downloadUrl, OnnxRuntimeDownloadUrl, StringComparison.OrdinalIgnoreCase) &&
            (item.Payload.title.Contains("CUDA Provider", StringComparison.OrdinalIgnoreCase) ||
             item.Payload.title.Contains("TensorRT Provider", StringComparison.OrdinalIgnoreCase)));
    }

    private static void AddDownload(
        List<DependencyDownload> downloads,
        string key,
        string title,
        string detail,
        string url,
        bool canDownload,
        string fileName)
    {
        if (downloads.Any(item => string.Equals(item.url, url, StringComparison.OrdinalIgnoreCase)))
        {
            return;
        }

        downloads.Add(new DependencyDownload(key, title, detail, url, canDownload, fileName));
    }

    private static string TextOrUnknown(string value)
    {
        return string.IsNullOrWhiteSpace(value) ? "未知" : value;
    }

    private static SystemProbe ProbeSystem()
    {
        return CachedSystemProbe.Value;
    }

    private static SystemProbe ProbeSystemInternal()
    {
        List<string> notes = [];
        string gpuName = string.Empty;
        string driverVersion = string.Empty;
        string cudaVersion = string.Empty;
        string source = "unavailable";
        string osDescription = RuntimeInformation.OSDescription.Trim();
        string architecture = RuntimeInformation.OSArchitecture.ToString();

        string? nvidiaSmiOutput = TryReadNvidiaSmi();
        if (!string.IsNullOrWhiteSpace(nvidiaSmiOutput) &&
            TryParseNvidiaSmi(nvidiaSmiOutput, out gpuName, out driverVersion, out cudaVersion))
        {
            source = "nvidia-smi";
        }
        else
        {
            (gpuName, driverVersion) = TryReadWindowsGpuInfo();
            source = string.IsNullOrWhiteSpace(gpuName) ? "unavailable" : "win32-video";
            if (string.IsNullOrWhiteSpace(cudaVersion))
            {
                notes.Add("未读取到 nvidia-smi CUDA 版本，只能给出通用 CUDA 12.x 下载入口。");
            }
        }

        if (string.IsNullOrWhiteSpace(gpuName))
        {
            notes.Add("未检测到显卡名称；如果这台机器没有 NVIDIA 显卡，CUDA/TensorRT 后端不可用。");
        }
        else if (!gpuName.Contains("NVIDIA", StringComparison.OrdinalIgnoreCase) &&
                 source != "nvidia-smi")
        {
            notes.Add("当前显卡信息不是 NVIDIA；CUDA/TensorRT 依赖只适用于 NVIDIA GPU。");
        }

        return new SystemProbe(
            gpuName,
            driverVersion,
            cudaVersion,
            osDescription,
            architecture,
            source,
            notes.ToArray());
    }

    private static string? TryReadNvidiaSmi()
    {
        string systemNvidiaSmi = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.Windows),
            "System32",
            "nvidia-smi.exe");

        if (File.Exists(systemNvidiaSmi))
        {
            string? output = TryReadNvidiaSmiFrom(systemNvidiaSmi);
            if (!string.IsNullOrWhiteSpace(output))
            {
                return output;
            }
        }

        return TryReadNvidiaSmiFrom("nvidia-smi");
    }

    private static string? TryReadNvidiaSmiFrom(string fileName)
    {
        string? gpuOutput = TryCaptureProcess(fileName,
            ["--query-gpu=name,driver_version", "--format=csv,noheader"],
            timeoutMs: 1500);
        if (string.IsNullOrWhiteSpace(gpuOutput))
        {
            return null;
        }

        string cudaVersion = string.Empty;
        string? summaryOutput = TryCaptureProcess(fileName, [], timeoutMs: 1500);
        if (!string.IsNullOrWhiteSpace(summaryOutput))
        {
            Match match = Regex.Match(summaryOutput, @"CUDA Version:\s*([0-9.]+)", RegexOptions.IgnoreCase);
            if (match.Success)
            {
                cudaVersion = match.Groups[1].Value;
            }
        }

        string firstLine = gpuOutput
            .Replace("\r\n", "\n")
            .Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .FirstOrDefault() ?? string.Empty;
        return string.IsNullOrWhiteSpace(cudaVersion) ? firstLine : $"{firstLine}, {cudaVersion}";
    }

    private static bool TryParseNvidiaSmi(
        string output,
        out string gpuName,
        out string driverVersion,
        out string cudaVersion)
    {
        gpuName = string.Empty;
        driverVersion = string.Empty;
        cudaVersion = string.Empty;

        string? line = output
            .Replace("\r\n", "\n")
            .Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .FirstOrDefault();
        if (string.IsNullOrWhiteSpace(line))
        {
            return false;
        }

        string[] parts = line.Split(',', StringSplitOptions.TrimEntries);
        if (parts.Length < 2)
        {
            return false;
        }

        gpuName = parts.ElementAtOrDefault(0) ?? string.Empty;
        driverVersion = parts.ElementAtOrDefault(1) ?? string.Empty;
        cudaVersion = parts.ElementAtOrDefault(2) ?? string.Empty;
        return !string.IsNullOrWhiteSpace(gpuName);
    }

    private static (string GpuName, string DriverVersion) TryReadWindowsGpuInfo()
    {
        const string script = """
            $gpu = Get-CimInstance Win32_VideoController |
              Sort-Object @{ Expression = { $_.Name -notmatch 'NVIDIA' } }, Name |
              Select-Object -First 1 Name, DriverVersion;
            if ($gpu) { $gpu | ConvertTo-Json -Compress }
            """;
        string? output = TryCaptureProcess("powershell.exe",
            ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", script],
            timeoutMs: 1800);
        if (string.IsNullOrWhiteSpace(output))
        {
            return (string.Empty, string.Empty);
        }

        try
        {
            using JsonDocument document = JsonDocument.Parse(output);
            JsonElement root = document.RootElement;
            string name = GetString(root, "Name", string.Empty);
            string driver = GetString(root, "DriverVersion", string.Empty);
            return (name, driver);
        }
        catch
        {
            return (string.Empty, string.Empty);
        }
    }

    private static string? TryCaptureProcess(string fileName, string[] arguments, int timeoutMs)
    {
        try
        {
            ProcessStartInfo startInfo = new()
            {
                FileName = fileName,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true
            };
            foreach (string argument in arguments)
            {
                startInfo.ArgumentList.Add(argument);
            }

            using Process process = new() { StartInfo = startInfo };
            process.Start();
            Task<string> outputTask = process.StandardOutput.ReadToEndAsync();
            Task<string> errorTask = process.StandardError.ReadToEndAsync();
            if (!process.WaitForExit(timeoutMs))
            {
                try
                {
                    process.Kill(entireProcessTree: true);
                }
                catch
                {
                }

                return null;
            }

            string output = outputTask.GetAwaiter().GetResult();
            string error = errorTask.GetAwaiter().GetResult();
            return string.IsNullOrWhiteSpace(output) ? error : output;
        }
        catch
        {
            return null;
        }
    }
}
