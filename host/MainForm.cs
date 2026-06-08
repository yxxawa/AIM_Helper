using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.Collections.Concurrent;
using System.IO.Compression;
using System.Net;
using System.Net.Http.Headers;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace AIM_Helper.Host;

public sealed class MainForm : Form
{
    private static readonly Color ShellBackColor = Color.FromArgb(11, 13, 12);
    private static readonly Color ShellTextColor = Color.FromArgb(241, 244, 236);
    private static readonly Color ShellMutedColor = Color.FromArgb(167, 176, 159);

    private readonly WebView2 webView = new()
    {
        Dock = DockStyle.Fill,
        AllowExternalDrop = false,
        DefaultBackgroundColor = ShellBackColor,
        Visible = false
    };

    private readonly Label loadingStatus = new()
    {
        AutoSize = true,
        ForeColor = ShellMutedColor,
        Font = new Font("Microsoft YaHei UI", 10f, FontStyle.Regular),
        Text = "正在加载界面..."
    };

    private readonly Panel loadingPanel;
    private Process? backendProcess;
    private bool backendReady;
    private bool hasSavedConfig;
    private bool tensorRtCacheNoticeShown;
    private string lastConfigJson = "{}";
    private readonly ConcurrentDictionary<string, byte> activeDownloads = new(StringComparer.OrdinalIgnoreCase);
    private static string RuntimeConfigJsonPath => Path.Combine(AppContext.BaseDirectory, "runtime-config.json");
    private static string LiveConfigPath => Path.Combine(AppContext.BaseDirectory, "runtime-config.live");
    private static readonly string[] DdDriverDllNames = ["dd60300.dll"];
    private static readonly string[] GenericDriverDllNames =
    [
        "DriverBridge.dll",
        "driver_bridge.dll",
        "driver.dll",
        "mouse_driver.dll",
        "logi_driver.dll",
        "dd60300.dll"
    ];

    private const string NvidiaDriverDownloadUrl = "https://www.nvidia.com/Download/index.aspx";
    private const string OnnxRuntimeDownloadUrl = "https://github.com/microsoft/onnxruntime/releases";
    private const string CudaToolkitArchiveUrl = "https://developer.nvidia.com/cuda-toolkit-archive";
    private const string CudnnArchiveUrl = "https://developer.nvidia.com/cudnn-archive";
    private const string TensorRtDownloadUrl = "https://developer.nvidia.com/tensorrt/download";
    private const string OpenCvDownloadUrl = "https://opencv.org/releases/";
    private const string OnnxRuntimePreferredTag = "v1.24.3";
    private const string OpenCvPreferredTag = "4.12.0";

    private static readonly Lazy<SystemProbe> CachedSystemProbe = new(ProbeSystemInternal);
    private static readonly HttpClient DependencyHttpClient = CreateDependencyHttpClient();

    private sealed record DependencyPayload(
        string key,
        string title,
        string description,
        string detail,
        bool required,
        bool ok,
        string? path,
        string expected,
        string kind,
        string downloadUrl);

    private sealed record DependencyCheck(bool Ok, bool Required, DependencyPayload Payload);

    private enum DependencyCheckScope
    {
        Startup,
        Full
    }

    private sealed record SystemProbe(
        string gpuName,
        string driverVersion,
        string cudaVersion,
        string osDescription,
        string architecture,
        string source,
        string[] notes);

    private sealed record DependencyDownload(
        string key,
        string title,
        string detail,
        string url,
        bool canDownload,
        string fileName);

    private sealed record ResolvedDependencyDownload(
        string key,
        string title,
        string url,
        string fileName);

    private sealed record NvidiaRedistComponent(
        string key,
        string title,
        string directoryUrl,
        string assetPattern);

    private static readonly NvidiaRedistComponent[] CudaRedistComponents =
    [
        new("cuda-cudart", "CUDA Runtime", "https://developer.download.nvidia.com/compute/cuda/redist/cuda_cudart/windows-x86_64/", @"href='(?<file>cuda_cudart-windows-x86_64-12\.[^']+-archive\.zip)'"),
        new("cuda-cublas", "CUDA cuBLAS", "https://developer.download.nvidia.com/compute/cuda/redist/libcublas/windows-x86_64/", @"href='(?<file>libcublas-windows-x86_64-12\.[^']+-archive\.zip)'"),
        new("cuda-nvrtc", "CUDA NVRTC", "https://developer.download.nvidia.com/compute/cuda/redist/cuda_nvrtc/windows-x86_64/", @"href='(?<file>cuda_nvrtc-windows-x86_64-12\.[^']+-archive\.zip)'"),
        new("cuda-cusolver", "CUDA Solver", "https://developer.download.nvidia.com/compute/cuda/redist/libcusolver/windows-x86_64/", @"href='(?<file>libcusolver-windows-x86_64-12\.[^']+-archive\.zip)'"),
        new("cuda-cusparse", "CUDA Sparse", "https://developer.download.nvidia.com/compute/cuda/redist/libcusparse/windows-x86_64/", @"href='(?<file>libcusparse-windows-x86_64-12\.[^']+-archive\.zip)'")
    ];

    private static readonly NvidiaRedistComponent[] CudnnRedistComponents =
    [
        new("cudnn", "cuDNN for CUDA 12", "https://developer.download.nvidia.com/compute/cudnn/redist/cudnn/windows-x86_64/", @"href='(?<file>cudnn-windows-x86_64-[^']+_cuda12-archive\.zip)'")
    ];

    public MainForm()
    {
        Text = "AIM_Helper";
        StartPosition = FormStartPosition.CenterScreen;
        MinimumSize = new Size(1120, 760);
        Size = new Size(1480, 900);
        BackColor = ShellBackColor;
        loadingPanel = CreateLoadingPanel();
        Controls.Add(webView);
        Controls.Add(loadingPanel);
        loadingPanel.BringToFront();
        Load += async (_, _) => await InitializeWebViewAsync();
        FormClosing += (_, _) => StopBackend();
    }

    private Panel CreateLoadingPanel()
    {
        Label title = new()
        {
            AutoSize = true,
            ForeColor = ShellTextColor,
            Font = new Font("Microsoft YaHei UI", 20f, FontStyle.Bold),
            Text = "功能开关"
        };

        Label subtitle = new()
        {
            AutoSize = true,
            ForeColor = ShellMutedColor,
            Font = new Font("Microsoft YaHei UI", 9f, FontStyle.Regular),
            Text = "AIM_Helper / Local Console"
        };

        FlowLayoutPanel stack = new()
        {
            Anchor = AnchorStyles.None,
            AutoSize = true,
            BackColor = ShellBackColor,
            FlowDirection = FlowDirection.TopDown,
            WrapContents = false
        };
        stack.Controls.Add(title);
        stack.Controls.Add(subtitle);
        stack.Controls.Add(loadingStatus);

        TableLayoutPanel layout = new()
        {
            BackColor = ShellBackColor,
            ColumnCount = 1,
            Dock = DockStyle.Fill,
            RowCount = 3
        };
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 46f));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 54f));
        layout.Controls.Add(stack, 0, 1);

        Panel panel = new()
        {
            BackColor = ShellBackColor,
            Dock = DockStyle.Fill
        };
        panel.Controls.Add(layout);
        return panel;
    }

    private void ShowLoading(string status)
    {
        loadingStatus.Text = status;
        loadingPanel.Visible = true;
        loadingPanel.BringToFront();
        webView.Visible = false;
    }

    private void ShowWebView()
    {
        webView.Visible = true;
        webView.BringToFront();
        loadingPanel.Visible = false;
    }

    private async Task InitializeWebViewAsync()
    {
        LoadSavedConfig();
        ShowLoading("正在初始化 WebView...");
        await webView.EnsureCoreWebView2Async();
        webView.CoreWebView2.Settings.AreDefaultContextMenusEnabled = true;
        webView.CoreWebView2.Settings.AreDevToolsEnabled = true;
        webView.CoreWebView2.WebMessageReceived += OnWebMessageReceived;
        webView.CoreWebView2.NavigationCompleted += (_, args) =>
        {
            if (args.IsSuccess)
            {
                ShowWebView();
            }
            else
            {
                ShowLoading($"界面加载失败: {args.WebErrorStatus}");
            }
        };

        string frontendPath = Path.Combine(AppContext.BaseDirectory, "frontend", "index.html");
        if (!File.Exists(frontendPath))
        {
            ShowLoading("找不到前端文件");
            MessageBox.Show(this, $"Frontend not found:\n{frontendPath}", "AIM_Helper",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }

        ShowLoading("正在加载界面...");
        webView.CoreWebView2.Navigate(new Uri(frontendPath).AbsoluteUri);
    }

    private void OnWebMessageReceived(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
    {
        using JsonDocument document = JsonDocument.Parse(e.WebMessageAsJson);
        string type = document.RootElement.TryGetProperty("type", out JsonElement typeElement)
            ? typeElement.GetString() ?? string.Empty
            : string.Empty;

        switch (type)
        {
            case "ui:start":
                RememberConfig(document.RootElement);
                StartBackend();
                break;
            case "ui:ready":
                if (hasSavedConfig)
                {
                    PostSavedConfig();
                }
                else
                {
                    RememberConfig(document.RootElement);
                }
                PostDependencyStatus();
                break;
            case "ui:checkDependencies":
                RememberConfig(document.RootElement);
                PostDependencyStatus();
                break;
            case "ui:chooseDependencyPath":
                ChooseDependencyPath(document.RootElement);
                break;
            case "ui:openUrl":
                OpenUrl(document.RootElement);
                break;
            case "ui:openUrls":
                OpenUrls(document.RootElement);
                break;
            case "ui:downloadDependency":
                StartDependencyDownload(document.RootElement);
                break;
            case "ui:downloadDependencies":
                StartDependencyDownloads(document.RootElement);
                break;
            case "ui:stop":
                StopBackend();
                break;
            case "ui:testInputBackend":
                RememberConfig(document.RootElement);
                TestInputBackend();
                break;
            case "ui:updateConfig":
                RememberConfig(document.RootElement);
                break;
            case "ui:saveConfig":
                RememberConfig(document.RootElement);
                PostLog("switches saved");
                break;
        }
    }

    private void TestInputBackend()
    {
        RunBackendSelfTest("input backend self-test", startInfo =>
        {
            AddBackendArguments(startInfo, lastConfigJson);
            startInfo.ArgumentList.Add("--input-self-test");
            if (WillUseDriver(lastConfigJson))
            {
                startInfo.ArgumentList.Add("--require-driver");
            }
        });
    }

    private void RunBackendSelfTest(string testName, Action<ProcessStartInfo> addArguments, int timeoutMs = 5000)
    {
        if (backendProcess is { HasExited: false })
        {
            PostLog($"stop backend before {testName}");
            return;
        }

        string? backendPath = ResolveBackendPath(lastConfigJson);
        if (backendPath is null)
        {
            PostLog("backend executable not found; build the C++ target first");
            return;
        }

        string workingDirectory = ResolveRuntimeDirectory(backendPath);
        MaybeShowTensorRtCacheNotice(workingDirectory, lastConfigJson);
        ProcessStartInfo startInfo = new()
        {
            FileName = backendPath,
            WorkingDirectory = workingDirectory,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        SetEnvironmentPath(startInfo, BuildRuntimePath(workingDirectory, GetEnvironmentPath(startInfo), lastConfigJson));
        PostInputBackendNote(lastConfigJson);
        addArguments(startInfo);
        PostLog($"{testName} started");

        _ = Task.Run(async () =>
        {
            using Process testProcess = new() { StartInfo = startInfo };
            try
            {
                testProcess.Start();
                Task<string> stdoutTask = testProcess.StandardOutput.ReadToEndAsync();
                Task<string> stderrTask = testProcess.StandardError.ReadToEndAsync();
                if (!testProcess.WaitForExit(timeoutMs))
                {
                    testProcess.Kill(entireProcessTree: true);
                    PostProcessLine($"{testName} timed out");
                    return;
                }

                PostProcessText(await stdoutTask);
                PostProcessText(await stderrTask);
                PostProcessLine($"{testName} exited with code {testProcess.ExitCode}");
            }
            catch (Exception ex)
            {
                PostProcessLine($"{testName} failed: {ex.Message}");
            }
        });
    }

    private void StartBackend()
    {
        if (backendProcess is { HasExited: false })
        {
            PostLog(backendReady ? "backend is already running" : "backend is already starting");
            PostState(true, backendReady ? "running" : "starting");
            return;
        }

        string? backendPath = ResolveBackendPath(lastConfigJson);
        if (backendPath is null)
        {
            PostLog("backend executable not found; build the C++ target first");
            PostState(false);
            return;
        }

        if (!CanStartFromConfig(lastConfigJson))
        {
            PostLog("screen capture switch is off; backend not started");
            PostState(false);
            return;
        }

        List<DependencyCheck> missingDependencies = MissingRequiredDependencies(lastConfigJson);
        if (missingDependencies.Count > 0)
        {
            string names = string.Join(", ", missingDependencies.Select(item => item.Payload.title).Distinct().Take(5));
            PostLog($"dependencies missing; backend not started: {names}");
            PostDependencyStatus();
            PostState(false);
            return;
        }

        string workingDirectory = ResolveRuntimeDirectory(backendPath);
        ProcessStartInfo startInfo = new()
        {
            FileName = backendPath,
            WorkingDirectory = workingDirectory,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        SetEnvironmentPath(startInfo, BuildRuntimePath(workingDirectory, GetEnvironmentPath(startInfo), lastConfigJson));
        PostInputBackendNote(lastConfigJson);
        AddBackendArguments(startInfo, lastConfigJson);

        Process process = new() { StartInfo = startInfo, EnableRaisingEvents = true };
        process.OutputDataReceived += (_, args) => PostProcessLine(args.Data);
        process.ErrorDataReceived += (_, args) => PostProcessLine(args.Data);
        process.Exited += (_, _) =>
        {
            int exitCode;
            try
            {
                exitCode = process.ExitCode;
            }
            catch
            {
                exitCode = -1;
            }
            PostToUi(() =>
            {
                if (!ReferenceEquals(backendProcess, process))
                {
                    process.Dispose();
                    return;
                }

                PostLog($"backend exited with code {exitCode}");
                PostState(false);
                backendReady = false;
                backendProcess = null;
                process.Dispose();
            });
        };

        try
        {
            backendProcess = process;
            backendReady = false;
            PostLog($"backend launching: {Path.GetFileName(backendPath)}");
            process.Start();
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();
            PostLog($"backend process started: {Path.GetFileName(backendPath)}");
            PostState(true, "starting");
        }
        catch (Exception ex)
        {
            process.Dispose();
            backendProcess = null;
            backendReady = false;
            PostLog($"backend start failed: {ex.Message}");
            PostState(false);
        }
    }

    private void MaybeShowTensorRtCacheNotice(string workingDirectory, string configJson)
    {
        if (tensorRtCacheNoticeShown)
        {
            return;
        }

        using JsonDocument document = ParseConfig(configJson);
        string inferenceBackend = GetString(document.RootElement, "backend", "cpu").ToLowerInvariant();
        if (inferenceBackend != "tensorrt")
        {
            return;
        }

        string cacheDirectory = Path.GetFullPath(Path.Combine(workingDirectory, "engine_cache"));
        if (HasTensorRtEngineCache(cacheDirectory))
        {
            return;
        }

        tensorRtCacheNoticeShown = true;
        PostLog("TensorRT engine cache not found; first initialization may be slow");
        MessageBox.Show(this,
            "首次使用 TensorRT 时需要生成 engine_cache，初始化速度会比较慢，请耐心等待。\n\n后续启动会复用缓存，速度会明显变快。",
            "首次初始化提醒",
            MessageBoxButtons.OK,
            MessageBoxIcon.Information);
    }

    private static bool HasTensorRtEngineCache(string cacheDirectory)
    {
        try
        {
            return Directory.Exists(cacheDirectory) && Directory.EnumerateFileSystemEntries(cacheDirectory).Any();
        }
        catch
        {
            return false;
        }
    }

    private void StopBackend()
    {
        if (backendProcess is not { HasExited: false })
        {
            PostState(false);
            return;
        }

        try
        {
            backendProcess.Kill(entireProcessTree: true);
            backendProcess.WaitForExit(1500);
            PostLog("backend stopped");
        }
        catch (Exception ex)
        {
            PostLog($"backend stop failed: {ex.Message}");
        }
        finally
        {
            backendProcess?.Dispose();
            backendProcess = null;
            backendReady = false;
            PostState(false);
        }
    }

    private static string? ResolveBackendPath(string configJson)
    {
        using JsonDocument document = ParseConfig(configJson);
        return ResolveBackendPath(document.RootElement);
    }

    private static string? ResolveBackendPath(JsonElement config)
    {
        string repoRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));
        string configuredBackend = GetDependencyPath(config, "backendExe");
        if (!string.IsNullOrWhiteSpace(configuredBackend) && File.Exists(configuredBackend))
        {
            return configuredBackend;
        }

        string[] candidates =
        [
            Path.Combine(AppContext.BaseDirectory, "backend", "AIM_Helper_Backend.exe"),
            Path.Combine(AppContext.BaseDirectory, "backend", "offline_yolo_mouse_assistant.exe"),
            Path.Combine(repoRoot, "build_ninja2", "AIM_Helper_Backend.exe"),
            Path.Combine(repoRoot, "build_ninja2", "offline_yolo_mouse_assistant.exe"),
            Path.Combine(repoRoot, "build_ninja", "AIM_Helper_Backend.exe"),
            Path.Combine(repoRoot, "build_ninja", "offline_yolo_mouse_assistant.exe"),
            Path.Combine(repoRoot, "build", "Release", "AIM_Helper_Backend.exe"),
            Path.Combine(repoRoot, "build", "Release", "offline_yolo_mouse_assistant.exe"),
            Path.Combine(repoRoot, "build", "AIM_Helper_Backend.exe"),
            Path.Combine(repoRoot, "build", "offline_yolo_mouse_assistant.exe")
        ];

        return candidates.FirstOrDefault(File.Exists);
    }

    private static string ResolveRuntimeDirectory(string backendPath)
    {
        return Path.GetDirectoryName(backendPath) ?? AppContext.BaseDirectory;
    }

    private static string BuildRuntimePath(string workingDirectory, string? existingPath, string configJson)
    {
        using JsonDocument document = ParseConfig(configJson);
        string prefix = string.Join(Path.PathSeparator, BuildRuntimeDirectories(workingDirectory, document.RootElement));
        return string.IsNullOrWhiteSpace(existingPath) ? prefix : $"{prefix}{Path.PathSeparator}{existingPath}";
    }

    private static IEnumerable<string> BuildRuntimeDirectories(string workingDirectory, JsonElement config)
    {
        string appDirectory = AppContext.BaseDirectory;
        string backendDirectory = Path.GetDirectoryName(ResolveBackendPath(config) ?? string.Empty) ?? string.Empty;
        string repoRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));
        string[] runtimeDirectories =
        [
            appDirectory,
            workingDirectory,
            backendDirectory,
            repoRoot,
            Path.Combine(appDirectory, "runtime"),
            Path.Combine(appDirectory, "deps"),
            Path.Combine(appDirectory, "deps", "onnxruntime"),
            Path.Combine(appDirectory, "deps", "onnxruntime", "runtimes", "win-x64", "native"),
            Path.Combine(appDirectory, "deps", "opencv"),
            Path.Combine(appDirectory, "deps", "opencv", "x64", "vc16", "bin"),
            Path.Combine(appDirectory, "deps", "opencv", "build", "x64", "vc16", "bin"),
            Path.Combine(appDirectory, "deps", "cuda", "bin"),
            Path.Combine(appDirectory, "deps", "tensorrt", "bin"),
            Path.Combine(appDirectory, "deps", "tensorrt", "lib"),
            Path.Combine(backendDirectory, "runtime"),
            Path.Combine(backendDirectory, "deps"),
            Path.Combine(repoRoot, "runtime"),
            Path.Combine(repoRoot, "deps"),
            Path.Combine(repoRoot, "deps", "onnxruntime"),
            Path.Combine(repoRoot, "deps", "onnxruntime", "runtimes", "win-x64", "native"),
            Path.Combine(repoRoot, "deps", "opencv"),
            Path.Combine(repoRoot, "deps", "opencv", "x64", "vc16", "bin"),
            Path.Combine(repoRoot, "deps", "opencv", "build", "x64", "vc16", "bin"),
            Path.Combine(repoRoot, "deps", "cuda", "bin"),
            Path.Combine(repoRoot, "deps", "tensorrt", "bin"),
            Path.Combine(repoRoot, "deps", "tensorrt", "lib"),
            GetDependencyPath(config, "onnxRuntimeDir"),
            GetDependencyPath(config, "opencvDir"),
            GetDependencyPath(config, "cudaDir"),
            GetDependencyPath(config, "tensorrtDir")
        ];

        return runtimeDirectories
            .Where(path => !string.IsNullOrWhiteSpace(path))
            .Select(Path.GetFullPath)
            .Where(Directory.Exists)
            .Distinct(StringComparer.OrdinalIgnoreCase);
    }

    private static string? GetEnvironmentPath(ProcessStartInfo startInfo)
    {
        string? pathKey = startInfo.Environment.Keys
            .FirstOrDefault(key => string.Equals(key, "PATH", StringComparison.OrdinalIgnoreCase));
        return pathKey is null ? null : startInfo.Environment[pathKey];
    }

    private static void SetEnvironmentPath(ProcessStartInfo startInfo, string value)
    {
        string? pathKey = startInfo.Environment.Keys
            .FirstOrDefault(key => string.Equals(key, "PATH", StringComparison.OrdinalIgnoreCase));
        startInfo.Environment[pathKey ?? "PATH"] = value;
    }

    private static string ResolveModelPathForBackend(JsonElement config)
    {
        string configuredModel = GetDependencyPath(config, "modelFile");
        if (!string.IsNullOrWhiteSpace(configuredModel) && File.Exists(configuredModel))
        {
            return configuredModel;
        }

        string modelPath = GetString(config, "modelPath", string.Empty).Trim();
        if (!string.IsNullOrWhiteSpace(modelPath) && Path.IsPathRooted(modelPath))
        {
            return modelPath;
        }

        string repoRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));
        string backendDirectory = Path.GetDirectoryName(ResolveBackendPath(config) ?? string.Empty) ?? string.Empty;
        string[] modelNames = string.IsNullOrWhiteSpace(modelPath)
            ? ["cs2yolomaax.onnx"]
            : [modelPath];
        string[] searchDirectories =
        [
            AppContext.BaseDirectory,
            Path.Combine(AppContext.BaseDirectory, "models"),
            Path.Combine(AppContext.BaseDirectory, "backend"),
            Path.Combine(AppContext.BaseDirectory, "backend", "models"),
            backendDirectory,
            Path.Combine(backendDirectory, "models"),
            repoRoot,
            Path.Combine(repoRoot, "models")
        ];
        string[] candidates = modelNames
            .SelectMany(name => searchDirectories.Select(directory => Path.Combine(directory, name)))
            .ToArray();

        return candidates.FirstOrDefault(File.Exists) ?? modelNames[0];
    }

    private static string ResolveDriverDllPathForBackend(JsonElement config)
    {
        string inputBackend = GetString(config, "inputBackend", "sendinput").ToLowerInvariant();
        string configuredPath = GetString(config, "driverDllPath", string.Empty);
        if (!IsDriverBackendSelection(inputBackend))
        {
            return configuredPath;
        }

        string? configuredMatch = ResolveConfiguredFilePath(configuredPath, BuildDriverDllSearchDirectories(config));
        if (configuredMatch is not null)
        {
            return configuredMatch;
        }

        string[] dllNames = inputBackend == "dd" ? DdDriverDllNames : GenericDriverDllNames;
        return FindFirstExistingFile(BuildDriverDllSearchDirectories(config), dllNames) ?? string.Empty;
    }

    private static IEnumerable<string> BuildDriverDllSearchDirectories(JsonElement config)
    {
        string appDirectory = AppContext.BaseDirectory;
        string backendDirectory = Path.GetDirectoryName(ResolveBackendPath(config) ?? string.Empty) ?? string.Empty;
        string repoRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));
        string configuredDirectory = Path.GetDirectoryName(GetString(config, "driverDllPath", string.Empty)) ?? string.Empty;

        string[] directories =
        [
            configuredDirectory,
            Path.Combine(appDirectory, "drivers"),
            Path.Combine(backendDirectory, "drivers"),
            Path.Combine(repoRoot, "drivers"),
            backendDirectory,
            appDirectory,
            repoRoot
        ];

        return directories
            .Where(path => !string.IsNullOrWhiteSpace(path))
            .Select(Path.GetFullPath)
            .Where(Directory.Exists)
            .Distinct(StringComparer.OrdinalIgnoreCase);
    }

    private static string? ResolveConfiguredFilePath(string configuredPath, IEnumerable<string> searchDirectories)
    {
        if (string.IsNullOrWhiteSpace(configuredPath))
        {
            return null;
        }

        if (Path.IsPathRooted(configuredPath))
        {
            return File.Exists(configuredPath) ? configuredPath : null;
        }

        return FindFirstExistingFile(searchDirectories, [configuredPath]);
    }

    private static string? FindFirstExistingFile(IEnumerable<string> directories, IEnumerable<string> fileNames)
    {
        foreach (string directory in directories)
        {
            foreach (string fileName in fileNames)
            {
                string candidate = Path.Combine(directory, fileName);
                if (File.Exists(candidate))
                {
                    return candidate;
                }
            }
        }

        return null;
    }

    private static string DriverDllExpectedText(string inputBackend)
    {
        return inputBackend == "dd"
            ? @"drivers\dd60300.dll 或 backend\dd60300.dll"
            : @"drivers\DriverBridge.dll / driver_bridge.dll / driver.dll / mouse_driver.dll / logi_driver.dll";
    }

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

    private static IEnumerable<string> BuildDependencySearchDirectories(string workingDirectory, JsonElement config)
    {
        foreach (string directory in BuildRuntimeDirectories(workingDirectory, config))
        {
            yield return directory;
        }

        string? path = Environment.GetEnvironmentVariable("PATH");
        if (string.IsNullOrWhiteSpace(path))
        {
            yield break;
        }

        foreach (string directory in path.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
        {
            if (Directory.Exists(directory))
            {
                yield return directory;
            }
        }
    }

    private static string? FindFileInDirectories(IEnumerable<string> directories, string fileName)
    {
        foreach (string directory in directories.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            string candidate = Path.Combine(directory, fileName);
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return null;
    }

    private static string? FindFilePatternInDirectories(IEnumerable<string> directories, string pattern)
    {
        foreach (string directory in directories.Distinct(StringComparer.OrdinalIgnoreCase))
        {
            try
            {
                string? match = Directory.GetFiles(directory, pattern).FirstOrDefault();
                if (match is not null)
                {
                    return match;
                }
            }
            catch
            {
            }
        }

        return null;
    }

    private void RememberConfig(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) || payload.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        lastConfigJson = payload.GetRawText();
        hasSavedConfig = true;
        try
        {
            WriteTextAtomically(RuntimeConfigJsonPath, lastConfigJson);
            WriteTextAtomically(LiveConfigPath, BuildLiveConfigText(payload));
        }
        catch (Exception ex)
        {
            PostLog($"config write failed: {ex.Message}");
        }
    }

    private void LoadSavedConfig()
    {
        hasSavedConfig = false;
        if (!File.Exists(RuntimeConfigJsonPath))
        {
            return;
        }

        try
        {
            string text = File.ReadAllText(RuntimeConfigJsonPath);
            using JsonDocument document = JsonDocument.Parse(text);
            if (document.RootElement.ValueKind != JsonValueKind.Object)
            {
                return;
            }

            lastConfigJson = document.RootElement.GetRawText();
            hasSavedConfig = true;
            WriteTextAtomically(LiveConfigPath, BuildLiveConfigText(document.RootElement));
        }
        catch
        {
            lastConfigJson = "{}";
            hasSavedConfig = false;
        }
    }

    private void PostSavedConfig()
    {
        if (!hasSavedConfig)
        {
            return;
        }

        PostJson("host:config", lastConfigJson);
    }

    private void ChooseDependencyPath(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) || payload.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        string key = GetString(payload, "key", string.Empty);
        string kind = GetString(payload, "kind", "folder");
        if (string.IsNullOrWhiteSpace(key))
        {
            return;
        }

        string? selectedPath = kind == "file"
            ? ChooseDependencyFile(key)
            : ChooseDependencyFolder();
        if (string.IsNullOrWhiteSpace(selectedPath))
        {
            return;
        }

        string response = JsonSerializer.Serialize(new { key, path = selectedPath });
        PostJson("host:dependencyPath", response);
    }

    private string? ChooseDependencyFile(string key)
    {
        using OpenFileDialog dialog = new()
        {
            CheckFileExists = true,
            Multiselect = false,
            Title = "选择依赖文件",
            Filter = key switch
            {
                "modelFile" => "ONNX 模型 (*.onnx)|*.onnx|所有文件 (*.*)|*.*",
                "backendExe" => "后端程序 (AIM_Helper_Backend.exe)|AIM_Helper_Backend.exe|EXE 文件 (*.exe)|*.exe|所有文件 (*.*)|*.*",
                "driverDll" => "DLL 文件 (*.dll)|*.dll|所有文件 (*.*)|*.*",
                _ => "所有文件 (*.*)|*.*"
            }
        };

        return dialog.ShowDialog(this) == DialogResult.OK ? dialog.FileName : null;
    }

    private string? ChooseDependencyFolder()
    {
        using FolderBrowserDialog dialog = new()
        {
            Description = "选择依赖所在目录",
            UseDescriptionForTitle = true,
            ShowNewFolderButton = false
        };

        return dialog.ShowDialog(this) == DialogResult.OK ? dialog.SelectedPath : null;
    }

    private void OpenUrl(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) || payload.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        string url = GetString(payload, "url", string.Empty);
        if (!Uri.TryCreate(url, UriKind.Absolute, out Uri? uri) ||
            (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps))
        {
            return;
        }

        try
        {
            Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true });
        }
        catch (Exception ex)
        {
            PostLog($"open dependency url failed: {ex.Message}");
        }
    }

    private void OpenUrls(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) ||
            !payload.TryGetProperty("urls", out JsonElement urls) ||
            urls.ValueKind != JsonValueKind.Array)
        {
            return;
        }

        foreach (JsonElement element in urls.EnumerateArray().Take(8))
        {
            string? url = element.ValueKind == JsonValueKind.String ? element.GetString() : null;
            if (!Uri.TryCreate(url, UriKind.Absolute, out Uri? uri) ||
                (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps))
            {
                continue;
            }

            try
            {
                Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true });
            }
            catch (Exception ex)
            {
                PostLog($"open dependency url failed: {ex.Message}");
            }
        }
    }

    private void StartDependencyDownload(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) || payload.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        string key = GetString(payload, "key", string.Empty);
        if (!string.IsNullOrWhiteSpace(key))
        {
            QueueDependencyDownload(key);
        }
    }

    private void StartDependencyDownloads(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) ||
            !payload.TryGetProperty("keys", out JsonElement keys) ||
            keys.ValueKind != JsonValueKind.Array)
        {
            return;
        }

        foreach (JsonElement element in keys.EnumerateArray())
        {
            string? key = element.ValueKind == JsonValueKind.String ? element.GetString() : null;
            if (!string.IsNullOrWhiteSpace(key))
            {
                QueueDependencyDownload(key);
            }
        }
    }

    private void QueueDependencyDownload(string key)
    {
        if (!activeDownloads.TryAdd(key, 1))
        {
            PostDownloadProgress(key, key, "downloading", "下载已在进行中", 0, 0, 0, null);
            return;
        }

        _ = Task.Run(async () =>
        {
            try
            {
                await DownloadDependencyAsync(key);
            }
            catch (Exception ex)
            {
                PostDownloadProgress(key, key, "failed", ex.Message, 0, 0, 0, null);
            }
            finally
            {
                activeDownloads.TryRemove(key, out _);
            }
        });
    }

    private async Task DownloadDependencyAsync(string key)
    {
        if (string.Equals(key, "cuda", StringComparison.OrdinalIgnoreCase))
        {
            await DownloadNvidiaRedistBundleAsync("cuda", "CUDA Runtime DLLs", CudaRedistComponents);
            return;
        }

        if (string.Equals(key, "cudnn", StringComparison.OrdinalIgnoreCase))
        {
            await DownloadNvidiaRedistBundleAsync("cudnn", "cuDNN Runtime DLLs", CudnnRedistComponents);
            return;
        }

        ResolvedDependencyDownload? download = await ResolveDependencyDownloadAsync(key);
        if (download is null)
        {
            PostDownloadProgress(key, key, "unsupported", "这个依赖需要在官网登录或手动选择版本，无法自动直链下载。", 0, 0, 0, null);
            return;
        }

        string finalPath = await DownloadResolvedFileAsync(download, $"准备下载 {download.fileName}", "下载中");
        string completeMessage = InstallDownloadedDependency(download, finalPath, out string? installedPath);
        PostDownloadProgress(download.key, download.title, "completed", completeMessage, 0, 0, 100, installedPath ?? finalPath);
        PostDependencyStatus();
    }

    private async Task<string> DownloadResolvedFileAsync(
        ResolvedDependencyDownload download,
        string preparingMessage,
        string activeMessage)
    {
        string downloadsDirectory = Path.Combine(AppContext.BaseDirectory, "downloads");
        Directory.CreateDirectory(downloadsDirectory);

        string fileName = SanitizeFileName(download.fileName);
        string finalPath = Path.Combine(downloadsDirectory, fileName);
        string partPath = $"{finalPath}.part";
        long existingBytes = File.Exists(partPath) ? new FileInfo(partPath).Length : 0;

        PostDownloadProgress(download.key, download.title, "resolving", preparingMessage, existingBytes, 0, 0, finalPath);

        using HttpRequestMessage request = new(HttpMethod.Get, download.url);
        if (existingBytes > 0)
        {
            request.Headers.Range = new RangeHeaderValue(existingBytes, null);
        }

        using HttpResponseMessage response = await DependencyHttpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead);
        if (existingBytes > 0 && response.StatusCode != HttpStatusCode.PartialContent)
        {
            existingBytes = 0;
        }

        response.EnsureSuccessStatusCode();

        long contentLength = response.Content.Headers.ContentLength ?? 0;
        long totalBytes = response.StatusCode == HttpStatusCode.PartialContent
            ? existingBytes + contentLength
            : contentLength;
        FileMode fileMode = existingBytes > 0 && response.StatusCode == HttpStatusCode.PartialContent
            ? FileMode.Append
            : FileMode.Create;

        byte[] buffer = new byte[1024 * 128];
        long receivedBytes = existingBytes;
        long lastReportedBytes = -1;
        DateTime lastReport = DateTime.MinValue;

        await using (Stream network = await response.Content.ReadAsStreamAsync())
        await using (FileStream file = new(partPath, fileMode, FileAccess.Write, FileShare.Read, 1024 * 128, useAsync: true))
        {
            while (true)
            {
                int read = await network.ReadAsync(buffer);
                if (read <= 0)
                {
                    break;
                }

                await file.WriteAsync(buffer.AsMemory(0, read));
                receivedBytes += read;

                bool shouldReport = receivedBytes - lastReportedBytes >= 1024 * 1024 ||
                                    DateTime.UtcNow - lastReport > TimeSpan.FromMilliseconds(250);
                if (shouldReport)
                {
                    PostDownloadProgress(download.key, download.title, "downloading", activeMessage, receivedBytes, totalBytes, Percent(receivedBytes, totalBytes), finalPath);
                    lastReportedBytes = receivedBytes;
                    lastReport = DateTime.UtcNow;
                }
            }

            await file.FlushAsync();
        }

        File.Move(partPath, finalPath, overwrite: true);
        return finalPath;
    }

    private async Task DownloadNvidiaRedistBundleAsync(string key, string title, IReadOnlyList<NvidiaRedistComponent> components)
    {
        string targetDirectory = ResolveBackendInstallDirectory();
        Directory.CreateDirectory(targetDirectory);
        int totalDlls = 0;

        for (int index = 0; index < components.Count; index++)
        {
            NvidiaRedistComponent component = components[index];
            ResolvedDependencyDownload? download = await ResolveNvidiaRedistComponentAsync(component);
            if (download is null)
            {
                throw new InvalidOperationException($"无法解析 {component.title} 官方 redist 下载地址。");
            }

            ResolvedDependencyDownload progressDownload = download with { key = key, title = title };
            string finalPath = await DownloadResolvedFileAsync(
                progressDownload,
                $"准备下载 {component.title} ({index + 1}/{components.Count})",
                $"下载 {component.title} ({index + 1}/{components.Count})");

            int extracted = ExtractDllsFromZip(finalPath, targetDirectory);
            totalDlls += extracted;
            PostDownloadProgress(key, title, "installing", $"已解压 {component.title}: {extracted} 个 DLL", 0, 0, ((index + 1) * 100.0) / components.Count, targetDirectory);
        }

        PostDownloadProgress(key, title, "completed", $"已安装 {totalDlls} 个 DLL 到 {targetDirectory}", 0, 0, 100, targetDirectory);
        PostDependencyStatus();
    }

    private string InstallDownloadedDependency(ResolvedDependencyDownload download, string finalPath, out string? installedPath)
    {
        installedPath = finalPath;
        if (string.Equals(download.key, "onnxruntime", StringComparison.OrdinalIgnoreCase) &&
            string.Equals(Path.GetExtension(finalPath), ".zip", StringComparison.OrdinalIgnoreCase))
        {
            string targetDirectory = ResolveBackendInstallDirectory();
            Directory.CreateDirectory(targetDirectory);
            int extracted = ExtractDllsFromZip(finalPath, targetDirectory);
            installedPath = targetDirectory;
            return extracted > 0
                ? $"已解压 {extracted} 个 ONNX Runtime DLL 到 {targetDirectory}"
                : "下载完成，但压缩包内没有找到可安装的 DLL。";
        }

        return "下载完成。这个依赖不是 DLL 压缩包，可能需要安装或手动选择目录。";
    }

    private string ResolveBackendInstallDirectory()
    {
        using JsonDocument document = ParseConfig(lastConfigJson);
        string? backendPath = document.RootElement.ValueKind == JsonValueKind.Object
            ? ResolveBackendPath(document.RootElement)
            : ResolveBackendPath("{}");
        if (!string.IsNullOrWhiteSpace(backendPath))
        {
            return ResolveRuntimeDirectory(backendPath);
        }

        return Path.Combine(AppContext.BaseDirectory, "backend");
    }

    private static int ExtractDllsFromZip(string zipPath, string targetDirectory)
    {
        if (!File.Exists(zipPath))
        {
            return 0;
        }

        Directory.CreateDirectory(targetDirectory);
        int extracted = 0;
        using ZipArchive archive = ZipFile.OpenRead(zipPath);
        foreach (ZipArchiveEntry entry in archive.Entries)
        {
            if (!entry.Name.EndsWith(".dll", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            string destination = Path.Combine(targetDirectory, entry.Name);
            entry.ExtractToFile(destination, overwrite: true);
            extracted++;
        }

        return extracted;
    }

    private static async Task<ResolvedDependencyDownload?> ResolveNvidiaRedistComponentAsync(NvidiaRedistComponent component)
    {
        string html = await DependencyHttpClient.GetStringAsync(component.directoryUrl);
        Regex regex = new(component.assetPattern, RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
        string? fileName = regex.Matches(html)
            .Cast<Match>()
            .Select(match => match.Groups["file"].Value)
            .Where(value => !string.IsNullOrWhiteSpace(value))
            .LastOrDefault();
        if (string.IsNullOrWhiteSpace(fileName))
        {
            return null;
        }

        string url = component.directoryUrl.EndsWith("/", StringComparison.Ordinal)
            ? component.directoryUrl + fileName
            : component.directoryUrl + "/" + fileName;
        return new ResolvedDependencyDownload(component.key, component.title, url, fileName);
    }

    private async Task<ResolvedDependencyDownload?> ResolveDependencyDownloadAsync(string key)
    {
        using JsonDocument document = ParseConfig(lastConfigJson);
        if (string.Equals(key, "onnxruntime", StringComparison.OrdinalIgnoreCase))
        {
            bool gpuPackage = HasMissingGpuOnnxRuntimeProvider(
                BuildDependencyChecks(document.RootElement, DependencyCheckScope.Full));
            string assetPattern = gpuPackage
                ? @"^onnxruntime-win-x64-gpu-[\d.]+\.zip$"
                : @"^onnxruntime-win-x64-[\d.]+\.zip$";
            return await ResolveGitHubAssetAsync(
                key,
                "ONNX Runtime",
                "microsoft",
                "onnxruntime",
                OnnxRuntimePreferredTag,
                assetPattern);
        }

        if (string.Equals(key, "opencv", StringComparison.OrdinalIgnoreCase))
        {
            return await ResolveGitHubAssetAsync(
                key,
                "OpenCV",
                "opencv",
                "opencv",
                OpenCvPreferredTag,
                @"^opencv-[\d.]+-windows\.exe$");
        }

        return null;
    }

    private static async Task<ResolvedDependencyDownload?> ResolveGitHubAssetAsync(
        string key,
        string title,
        string owner,
        string repo,
        string preferredTag,
        string assetPattern)
    {
        string[] endpoints =
        [
            $"https://api.github.com/repos/{owner}/{repo}/releases/tags/{preferredTag}",
            $"https://api.github.com/repos/{owner}/{repo}/releases/latest"
        ];

        Regex regex = new(assetPattern, RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
        foreach (string endpoint in endpoints)
        {
            try
            {
                string json = await DependencyHttpClient.GetStringAsync(endpoint);
                using JsonDocument document = JsonDocument.Parse(json);
                if (!document.RootElement.TryGetProperty("assets", out JsonElement assets) ||
                    assets.ValueKind != JsonValueKind.Array)
                {
                    continue;
                }

                foreach (JsonElement asset in assets.EnumerateArray())
                {
                    string name = GetString(asset, "name", string.Empty);
                    string url = GetString(asset, "browser_download_url", string.Empty);
                    if (!string.IsNullOrWhiteSpace(name) &&
                        !string.IsNullOrWhiteSpace(url) &&
                        regex.IsMatch(name))
                    {
                        return new ResolvedDependencyDownload(key, title, url, name);
                    }
                }
            }
            catch
            {
            }
        }

        return null;
    }

    private void PostDownloadProgress(
        string key,
        string title,
        string status,
        string message,
        long receivedBytes,
        long totalBytes,
        double percent,
        string? path)
    {
        string payload = JsonSerializer.Serialize(new
        {
            key,
            title,
            status,
            message,
            receivedBytes,
            totalBytes,
            percent,
            path
        });
        PostToUi(() => PostJson("host:downloadProgress", payload));
    }

    private static double Percent(long receivedBytes, long totalBytes)
    {
        if (totalBytes <= 0)
        {
            return 0;
        }

        return Math.Max(0, Math.Min(100, receivedBytes * 100.0 / totalBytes));
    }

    private static string SanitizeFileName(string fileName)
    {
        string safe = string.IsNullOrWhiteSpace(fileName) ? "dependency.bin" : fileName;
        foreach (char invalid in Path.GetInvalidFileNameChars())
        {
            safe = safe.Replace(invalid, '_');
        }

        return safe;
    }

    private static HttpClient CreateDependencyHttpClient()
    {
        HttpClient client = new()
        {
            Timeout = Timeout.InfiniteTimeSpan
        };
        client.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("AIM_Helper", "1.0"));
        client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
        return client;
    }

    private static void WriteTextAtomically(string path, string text)
    {
        string tempPath = $"{path}.tmp";
        Exception? lastError = null;
        for (int attempt = 0; attempt < 5; ++attempt)
        {
            try
            {
                File.WriteAllText(tempPath, text);
                File.Move(tempPath, path, overwrite: true);
                return;
            }
            catch (IOException ex)
            {
                lastError = ex;
            }
            catch (UnauthorizedAccessException ex)
            {
                lastError = ex;
            }

            System.Threading.Thread.Sleep(20);
        }

        throw lastError ?? new IOException($"Unable to write {path}");
    }

    private static string BuildLiveConfigText(JsonElement config)
    {
        System.Text.StringBuilder builder = new();
        string inputBackend = GetString(config, "inputBackend", "sendinput");
        string driverDllPath = ResolveDriverDllPathForBackend(config);
        bool captureEnabled = GetBool(config, "enableCapture", true);
        string effectiveInputBackend = IsDriverBackendSelection(inputBackend) && !string.IsNullOrWhiteSpace(driverDllPath)
            ? "driver"
            : "sendinput";

        AppendLine(builder, "model_path", ResolveModelPathForBackend(config));
        AppendLine(builder, "backend", GetString(config, "backend", "cpu"));
        AppendLine(builder, "input_backend", effectiveInputBackend);
        AppendLine(builder, "driver_dll_path", driverDllPath);
        AppendLine(builder, "driver_type", GetInt(config, "driverType", 1));
        AppendLine(builder, "crop_size", GetInt(config, "cropSize", 320));
        AppendLine(builder, "lock_radius", GetInt(config, "lockRadius", 100));
        AppendLine(builder, "confidence", GetDouble(config, "confidence", 0.5));
        AppendLine(builder, "smoothing", GetDouble(config, "smoothing", 1.0));
        AppendLine(builder, "aim_key", GetInt(config, "aimHotkey", 1));
        AppendLine(builder, "aim_key2", GetInt(config, "aimHotkey2", 0));
        AppendLine(builder, "aim_mode", GetAimMode(config));
        AppendLine(builder, "aim_gain", GetDouble(config, "aimGain", 0.45));
        AppendLine(builder, "deadzone", GetDouble(config, "deadzone", 1.5));
        AppendLine(builder, "aim_filter", GetString(config, "aimFilter", "none"));
        AppendLine(builder, "pid_kp", GetDouble(config, "pidKp", 1.0));
        AppendLine(builder, "pid_ki", GetDouble(config, "pidKi", 0.0));
        AppendLine(builder, "pid_kd", GetDouble(config, "pidKd", 0.0));
        AppendLine(builder, "pid_i_limit", GetDouble(config, "pidIntegralLimit", 120.0));
        AppendLine(builder, "one_euro_min_cutoff", GetDouble(config, "oneEuroMinCutoff", 1.0));
        AppendLine(builder, "one_euro_beta", GetDouble(config, "oneEuroBeta", 0.02));
        AppendLine(builder, "one_euro_d_cutoff", GetDouble(config, "oneEuroDCutoff", 1.0));
        string predictionMode = GetBool(config, "enablePrediction", false)
            ? GetString(config, "predictionMode", "linear")
            : "off";
        AppendLine(builder, "prediction_mode", predictionMode);
        AppendLine(builder, "prediction_lead_ms", GetDouble(config, "predictionLeadMs", 20.0));
        AppendLine(builder, "prediction_smoothing", GetDouble(config, "predictionSmoothing", 0.12));
        AppendLine(builder, "prediction_acceleration_smoothing", GetDouble(config, "predictionAccelerationSmoothing", 0.18));
        AppendLine(builder, "prediction_alpha", GetDouble(config, "predictionAlpha", 0.45));
        AppendLine(builder, "prediction_beta", GetDouble(config, "predictionBeta", 0.06));
        AppendLine(builder, "prediction_kalman_measurement_noise", GetDouble(config, "predictionKalmanMeasurementNoise", 34.0));
        AppendLine(builder, "prediction_kalman_process_noise", GetDouble(config, "predictionKalmanProcessNoise", 72.0));
        AppendLine(builder, "prediction_max_pixels", GetInt(config, "predictionMaxPixels", 18));
        AppendLine(builder, "prediction_reset_pixels", GetInt(config, "predictionResetPixels", 70));
        AppendLine(builder, "prediction_noise_pixels", GetDouble(config, "predictionNoisePixels", 1.5));
        AppendLine(builder, "prediction_output_smoothing", GetDouble(config, "predictionOutputSmoothing", 0.20));
        AppendLine(builder, "prediction_servo_gain", GetDouble(config, "predictionServoGain", 0.65));
        AppendLine(builder, "target_x", GetDouble(config, "targetX", 0.5));
        AppendLine(builder, "target_y", GetDouble(config, "targetY", 0.3));
        AppendLine(builder, "auto_target_part", GetBool(config, "enableAutoAimPart", true));
        AppendLine(builder, "aim_part_priority", GetString(config, "aimPartPriority", "distance"));
        AppendLine(builder, "enemy_camp", GetString(config, "enemyCamp", "all"));
        AppendLine(builder, "detection_part", GetString(config, "detectionPart", "all"));
        AppendLine(builder, "max_move", GetInt(config, "maxMove", 60));
        AppendLine(builder, "tracking_boost_enabled", GetBool(config, "enableTrackingBoost", true));
        AppendLine(builder, "tracking_boost_threshold", GetDouble(config, "trackingBoostThreshold", 4.0));
        AppendLine(builder, "tracking_boost_gain", GetDouble(config, "trackingBoostGain", 2.0));
        AppendLine(builder, "tracking_boost_max_move", GetInt(config, "trackingBoostMaxMove", 120));
        AppendLine(builder, "human_slide_enabled", GetBool(config, "enableHumanSlide", false));
        AppendLine(builder, "human_slide_max_step", GetDouble(config, "humanSlideMaxStep", 50.0));
        AppendLine(builder, "human_slide_jitter", GetDouble(config, "humanSlideJitter", 0.5));
        AppendLine(builder, "human_slide_delay_min", GetInt(config, "humanSlideDelayMin", 5));
        AppendLine(builder, "human_slide_delay_max", GetInt(config, "humanSlideDelayMax", 20));
        AppendLine(builder, "auto_click_enabled", GetBool(config, "enableAutoClick", false));
        AppendLine(builder, "auto_click_delay_min", GetInt(config, "autoClickDelayMin", 80));
        AppendLine(builder, "auto_click_delay_max", GetInt(config, "autoClickDelayMax", 160));
        AppendLine(builder, "auto_click_interval_min", GetInt(config, "autoClickIntervalMin", 120));
        AppendLine(builder, "auto_click_interval_max", GetInt(config, "autoClickIntervalMax", 220));
        AppendLine(builder, "auto_click_tolerance", GetDouble(config, "autoClickTolerance", 3.0));
        AppendLine(builder, "auto_stop_enabled", GetBool(config, "enableAutoStop", false));
        AppendLine(builder, "auto_stop_mode", GetString(config, "autoStopMode", "counter_tap"));
        AppendLine(builder, "auto_stop_hold_ms", GetInt(config, "autoStopHoldMs", 75));
        AppendLine(builder, "auto_stop_settle_ms", GetInt(config, "autoStopSettleMs", 15));
        AppendLine(builder, "enable_capture", captureEnabled);
        AppendLine(builder, "enable_mouse_move", GetBool(config, "enableMouseMove", true));
        AppendLine(builder, "enable_hold_to_aim", GetBool(config, "enableHoldToAim", true));
        AppendLine(builder, "enable_visualization", GetBool(config, "enableVisualization", true));
        AppendLine(builder, "console_stats_enabled", GetBool(config, "enableConsoleStats", false));
        AppendLine(builder, "bounded_movement", GetBool(config, "boundedMovement", true));
        return builder.ToString();
    }

    private static void AppendLine(System.Text.StringBuilder builder, string key, string value)
    {
        builder.Append(key).Append('=').AppendLine(value);
    }

    private static void AppendLine(System.Text.StringBuilder builder, string key, int value)
    {
        builder.Append(key).Append('=').AppendLine(value.ToString(CultureInfo.InvariantCulture));
    }

    private static void AppendLine(System.Text.StringBuilder builder, string key, double value)
    {
        builder.Append(key).Append('=').AppendLine(value.ToString(CultureInfo.InvariantCulture));
    }

    private static void AppendLine(System.Text.StringBuilder builder, string key, bool value)
    {
        builder.Append(key).Append('=').AppendLine(value ? "true" : "false");
    }

    private static bool CanStartFromConfig(string configJson)
    {
        using JsonDocument document = ParseConfig(configJson);
        JsonElement config = document.RootElement;
        return GetBool(config, "enableCapture", true);
    }

    private static List<DependencyCheck> MissingRequiredDependencies(string configJson)
    {
        using JsonDocument document = ParseConfig(configJson);
        if (document.RootElement.ValueKind != JsonValueKind.Object)
        {
            return [];
        }

        return BuildDependencyChecks(document.RootElement, DependencyCheckScope.Startup)
            .Where(item => item.Required && !item.Ok)
            .ToList();
    }

    private static bool WillUseDriver(string configJson)
    {
        using JsonDocument document = ParseConfig(configJson);
        JsonElement config = document.RootElement;
        string inputBackend = GetString(config, "inputBackend", "sendinput");
        string driverDllPath = ResolveDriverDllPathForBackend(config);
        return IsDriverBackendSelection(inputBackend) && !string.IsNullOrWhiteSpace(driverDllPath);
    }

    private static bool IsDriverBackendSelection(string inputBackend)
    {
        return inputBackend == "driver" || inputBackend == "dd";
    }

    private static void AddBackendArguments(ProcessStartInfo startInfo, string configJson)
    {
        using JsonDocument document = ParseConfig(configJson);
        JsonElement config = document.RootElement;

        AddArg(startInfo, "model", ResolveModelPathForBackend(config));
        AddArg(startInfo, "live-config", LiveConfigPath);
        AddArg(startInfo, "backend", GetString(config, "backend", "cpu"));
        string inputBackend = GetString(config, "inputBackend", "sendinput");
        string driverDllPath = ResolveDriverDllPathForBackend(config);
        int driverType = GetInt(config, "driverType", 1);
        if (IsDriverBackendSelection(inputBackend) && !string.IsNullOrWhiteSpace(driverDllPath))
        {
            AddArg(startInfo, "input-backend", "driver");
            AddArg(startInfo, "driver-dll", driverDllPath);
            if (inputBackend == "driver")
            {
                AddArg(startInfo, "driver-type", driverType);
            }
        }
        else
        {
            AddArg(startInfo, "input-backend", "sendinput");
        }
        AddArg(startInfo, "crop-size", GetInt(config, "cropSize", 320));
        AddArg(startInfo, "lock-radius", GetInt(config, "lockRadius", 100));
        AddArg(startInfo, "confidence", GetDouble(config, "confidence", 0.5));
        AddArg(startInfo, "smoothing", GetDouble(config, "smoothing", 1.0));
        AddArg(startInfo, "aim-key", GetInt(config, "aimHotkey", 1));
        AddArg(startInfo, "aim-key2", GetInt(config, "aimHotkey2", 0));
        AddArg(startInfo, "aim-mode", GetAimMode(config));
        AddArg(startInfo, "aim-gain", GetDouble(config, "aimGain", 0.45));
        AddArg(startInfo, "deadzone", GetDouble(config, "deadzone", 1.5));
        AddArg(startInfo, "aim-filter", GetString(config, "aimFilter", "none"));
        AddArg(startInfo, "pid-kp", GetDouble(config, "pidKp", 1.0));
        AddArg(startInfo, "pid-ki", GetDouble(config, "pidKi", 0.0));
        AddArg(startInfo, "pid-kd", GetDouble(config, "pidKd", 0.0));
        AddArg(startInfo, "pid-i-limit", GetDouble(config, "pidIntegralLimit", 120.0));
        AddArg(startInfo, "one-euro-min-cutoff", GetDouble(config, "oneEuroMinCutoff", 1.0));
        AddArg(startInfo, "one-euro-beta", GetDouble(config, "oneEuroBeta", 0.02));
        AddArg(startInfo, "one-euro-d-cutoff", GetDouble(config, "oneEuroDCutoff", 1.0));
        string predictionMode = GetBool(config, "enablePrediction", false)
            ? GetString(config, "predictionMode", "linear")
            : "off";
        AddArg(startInfo, "prediction-mode", predictionMode);
        AddArg(startInfo, "prediction-lead-ms", GetDouble(config, "predictionLeadMs", 20.0));
        AddArg(startInfo, "prediction-smoothing", GetDouble(config, "predictionSmoothing", 0.12));
        AddArg(startInfo, "prediction-acceleration-smoothing", GetDouble(config, "predictionAccelerationSmoothing", 0.18));
        AddArg(startInfo, "prediction-alpha", GetDouble(config, "predictionAlpha", 0.45));
        AddArg(startInfo, "prediction-beta", GetDouble(config, "predictionBeta", 0.06));
        AddArg(startInfo, "prediction-kalman-measurement-noise", GetDouble(config, "predictionKalmanMeasurementNoise", 34.0));
        AddArg(startInfo, "prediction-kalman-process-noise", GetDouble(config, "predictionKalmanProcessNoise", 72.0));
        AddArg(startInfo, "prediction-max-pixels", GetInt(config, "predictionMaxPixels", 18));
        AddArg(startInfo, "prediction-reset-pixels", GetInt(config, "predictionResetPixels", 70));
        AddArg(startInfo, "prediction-noise-pixels", GetDouble(config, "predictionNoisePixels", 1.5));
        AddArg(startInfo, "prediction-output-smoothing", GetDouble(config, "predictionOutputSmoothing", 0.20));
        AddArg(startInfo, "prediction-servo-gain", GetDouble(config, "predictionServoGain", 0.65));
        AddArg(startInfo, "target-x", GetDouble(config, "targetX", 0.5));
        AddArg(startInfo, "target-y", GetDouble(config, "targetY", 0.3));
        AddArg(startInfo, "auto-target-part", GetBool(config, "enableAutoAimPart", true) ? "true" : "false");
        AddArg(startInfo, "aim-part-priority", GetString(config, "aimPartPriority", "distance"));
        AddArg(startInfo, "enemy-camp", GetString(config, "enemyCamp", "all"));
        AddArg(startInfo, "detection-part", GetString(config, "detectionPart", "all"));
        AddArg(startInfo, "max-move", GetInt(config, "maxMove", 60));
        AddArg(startInfo, "tracking-boost", GetBool(config, "enableTrackingBoost", true) ? "true" : "false");
        AddArg(startInfo, "tracking-boost-threshold", GetDouble(config, "trackingBoostThreshold", 4.0));
        AddArg(startInfo, "tracking-boost-gain", GetDouble(config, "trackingBoostGain", 2.0));
        AddArg(startInfo, "tracking-boost-max-move", GetInt(config, "trackingBoostMaxMove", 120));
        AddArg(startInfo, "human-slide", GetBool(config, "enableHumanSlide", false) ? "true" : "false");
        AddArg(startInfo, "human-slide-max-step", GetDouble(config, "humanSlideMaxStep", 50.0));
        AddArg(startInfo, "human-slide-jitter", GetDouble(config, "humanSlideJitter", 0.5));
        AddArg(startInfo, "human-slide-delay-min", GetInt(config, "humanSlideDelayMin", 5));
        AddArg(startInfo, "human-slide-delay-max", GetInt(config, "humanSlideDelayMax", 20));
        AddArg(startInfo, "auto-click", GetBool(config, "enableAutoClick", false) ? "true" : "false");
        AddArg(startInfo, "auto-click-delay-min", GetInt(config, "autoClickDelayMin", 80));
        AddArg(startInfo, "auto-click-delay-max", GetInt(config, "autoClickDelayMax", 160));
        AddArg(startInfo, "auto-click-interval-min", GetInt(config, "autoClickIntervalMin", 120));
        AddArg(startInfo, "auto-click-interval-max", GetInt(config, "autoClickIntervalMax", 220));
        AddArg(startInfo, "auto-click-tolerance", GetDouble(config, "autoClickTolerance", 3.0));
        AddArg(startInfo, "auto-stop", GetBool(config, "enableAutoStop", false) ? "true" : "false");
        AddArg(startInfo, "auto-stop-mode", GetString(config, "autoStopMode", "counter_tap"));
        AddArg(startInfo, "auto-stop-hold-ms", GetInt(config, "autoStopHoldMs", 75));
        AddArg(startInfo, "auto-stop-settle-ms", GetInt(config, "autoStopSettleMs", 15));

        if (!GetBool(config, "enableCapture", true))
        {
            startInfo.ArgumentList.Add("--disable-capture");
        }
        if (!GetBool(config, "enableMouseMove", true)) startInfo.ArgumentList.Add("--disable-mouse-move");
        if (!GetBool(config, "enableHoldToAim", true)) startInfo.ArgumentList.Add("--disable-hold-to-aim");
        if (!GetBool(config, "enableVisualization", true)) startInfo.ArgumentList.Add("--disable-visualization");
        AddArg(startInfo, "console-stats", GetBool(config, "enableConsoleStats", false) ? "true" : "false");
        if (!GetBool(config, "boundedMovement", true)) startInfo.ArgumentList.Add("--unbounded-movement");
    }

    private void PostInputBackendNote(string configJson)
    {
        using JsonDocument document = ParseConfig(configJson);
        JsonElement config = document.RootElement;
        string inputBackend = GetString(config, "inputBackend", "sendinput");
        string driverDllPath = ResolveDriverDllPathForBackend(config);
        int driverType = GetInt(config, "driverType", 1);
        if (!IsDriverBackendSelection(inputBackend))
        {
            PostLog("input backend: SendInput");
            return;
        }
        if (string.IsNullOrWhiteSpace(driverDllPath))
        {
            PostLog("driver backend requested but no DLL was found in configured or fixed locations; using SendInput");
            return;
        }
        if (inputBackend == "dd")
        {
            PostLog($"input backend: DD; dll={driverDllPath}");
        }
        else
        {
            PostLog($"input backend: Driver; dll={driverDllPath}; driverType={driverType}");
        }
    }

    private static JsonDocument ParseConfig(string configJson)
    {
        if (string.IsNullOrWhiteSpace(configJson))
        {
            return JsonDocument.Parse("{}");
        }

        try
        {
            return JsonDocument.Parse(configJson);
        }
        catch
        {
            return JsonDocument.Parse("{}");
        }
    }

    private static void AddArg(ProcessStartInfo startInfo, string name, string value)
    {
        if (!string.IsNullOrWhiteSpace(value))
        {
            startInfo.ArgumentList.Add($"--{name}={value}");
        }
    }

    private static void AddArg(ProcessStartInfo startInfo, string name, int value)
    {
        startInfo.ArgumentList.Add($"--{name}={value}");
    }

    private static void AddArg(ProcessStartInfo startInfo, string name, double value)
    {
        startInfo.ArgumentList.Add($"--{name}={value.ToString(CultureInfo.InvariantCulture)}");
    }

    private static string GetString(JsonElement config, string name, string fallback)
    {
        return config.TryGetProperty(name, out JsonElement element) && element.ValueKind == JsonValueKind.String
            ? element.GetString() ?? fallback
            : fallback;
    }

    private static string GetAimMode(JsonElement config)
    {
        string mode = GetString(config, "aimMode", "atan").ToLowerInvariant();
        return mode is "atan" or "linear" ? mode : "atan";
    }

    private static string GetDependencyPath(JsonElement config, string key)
    {
        if (!config.TryGetProperty("dependencyPaths", out JsonElement paths) ||
            paths.ValueKind != JsonValueKind.Object)
        {
            return string.Empty;
        }

        return GetString(paths, key, string.Empty);
    }

    private static bool GetBool(JsonElement config, string name, bool fallback)
    {
        return config.TryGetProperty(name, out JsonElement element) && element.ValueKind is JsonValueKind.True or JsonValueKind.False
            ? element.GetBoolean()
            : fallback;
    }

    private static int GetInt(JsonElement config, string name, int fallback)
    {
        return config.TryGetProperty(name, out JsonElement element) && element.TryGetInt32(out int value)
            ? value
            : fallback;
    }

    private static double GetDouble(JsonElement config, string name, double fallback)
    {
        return config.TryGetProperty(name, out JsonElement element) && element.TryGetDouble(out double value)
            ? value
            : fallback;
    }

    private void PostProcessLine(string? text)
    {
        if (string.IsNullOrWhiteSpace(text))
        {
            return;
        }

        string line = text.Trim();
        PostToUi(() =>
        {
            PostLog(line);
            if (IsBackendReadyLine(line))
            {
                backendReady = true;
                PostState(true, "running");
            }
        });
    }

    private static bool IsBackendReadyLine(string line)
    {
        return line.Contains("Model loaded successfully", StringComparison.OrdinalIgnoreCase)
            || line.Contains("--- Controls ---", StringComparison.OrdinalIgnoreCase);
    }

    private void PostProcessText(string? text)
    {
        if (string.IsNullOrWhiteSpace(text))
        {
            return;
        }

        foreach (string line in text.Replace("\r\n", "\n").Split('\n'))
        {
            PostProcessLine(line);
        }
    }

    private void PostState(bool running, string? phase = null)
    {
        phase ??= running ? "running" : "stopped";
        string payload = JsonSerializer.Serialize(new { running, phase });
        PostJson("host:state", payload);
    }

    private void PostLog(string text)
    {
        string payload = JsonSerializer.Serialize(new { text });
        PostJson("host:log", payload);
    }

    private void PostJson(string type, string payloadJson)
    {
        if (webView.CoreWebView2 is null)
        {
            return;
        }

        string messageJson = $$"""{"type":"{{type}}","payload":{{payloadJson}}}""";
        webView.CoreWebView2.PostWebMessageAsJson(messageJson);
    }

    private void PostToUi(Action action)
    {
        if (IsDisposed)
        {
            return;
        }

        try
        {
            if (InvokeRequired)
            {
                BeginInvoke(action);
            }
            else
            {
                action();
            }
        }
        catch (InvalidOperationException)
        {
        }
    }
}
