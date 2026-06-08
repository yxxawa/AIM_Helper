using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.Text.Json;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace AIM_Helper.Host;

public sealed partial class MainForm : Form
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
    private volatile bool consoleStatsEnabled;
    private bool tensorRtCacheNoticeShown;
    private string lastConfigJson = "{}";
    private static string RuntimeConfigJsonPath => Path.Combine(AppContext.BaseDirectory, "runtime-config.json");
    private static string LiveConfigPath => Path.Combine(AppContext.BaseDirectory, "runtime-config.live");
    private static string RepoRoot => Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));
    private static readonly string[] BackendExecutableNames = ["AIM_Helper_Backend.exe", "offline_yolo_mouse_assistant.exe"];
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
        ProcessStartInfo startInfo = CreateBackendStartInfo(backendPath, workingDirectory, lastConfigJson);
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
        MaybeShowTensorRtCacheNotice(workingDirectory, lastConfigJson);
        ProcessStartInfo startInfo = CreateBackendStartInfo(backendPath, workingDirectory, lastConfigJson);
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

    private void RememberConfig(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) || payload.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        lastConfigJson = payload.GetRawText();
        hasSavedConfig = true;
        consoleStatsEnabled = GetBool(payload, "enableConsoleStats", false);
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
            consoleStatsEnabled = GetBool(document.RootElement, "enableConsoleStats", false);
            WriteTextAtomically(LiveConfigPath, BuildLiveConfigText(document.RootElement));
        }
        catch
        {
            lastConfigJson = "{}";
            hasSavedConfig = false;
            consoleStatsEnabled = false;
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
        AppendLine(builder, "drone_tracking_enabled", GetBool(config, "enableDroneTracking", false));
        AppendLine(builder, "drone_track_gain", GetDouble(config, "droneTrackGain", 1.35));
        AppendLine(builder, "drone_track_velocity_gain", GetDouble(config, "droneTrackVelocityGain", 0.55));
        AppendLine(builder, "drone_track_damping", GetDouble(config, "droneTrackDamping", 0.18));
        AppendLine(builder, "drone_track_smoothing", GetDouble(config, "droneTrackSmoothing", 0.78));
        AppendLine(builder, "drone_track_max_move", GetInt(config, "droneTrackMaxMove", 220));
        AppendLine(builder, "drone_track_deadzone", GetDouble(config, "droneTrackDeadzone", 0.3));
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
        AddArg(startInfo, "drone-tracking", GetBool(config, "enableDroneTracking", false) ? "true" : "false");
        AddArg(startInfo, "drone-track-gain", GetDouble(config, "droneTrackGain", 1.35));
        AddArg(startInfo, "drone-track-velocity-gain", GetDouble(config, "droneTrackVelocityGain", 0.55));
        AddArg(startInfo, "drone-track-damping", GetDouble(config, "droneTrackDamping", 0.18));
        AddArg(startInfo, "drone-track-smoothing", GetDouble(config, "droneTrackSmoothing", 0.78));
        AddArg(startInfo, "drone-track-max-move", GetInt(config, "droneTrackMaxMove", 220));
        AddArg(startInfo, "drone-track-deadzone", GetDouble(config, "droneTrackDeadzone", 0.3));
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

    private void PostProcessLine(string? text)
    {
        if (string.IsNullOrWhiteSpace(text))
        {
            return;
        }

        string line = text.Trim();
        bool backendReadyLine = IsBackendReadyLine(line);
        if (!consoleStatsEnabled && !backendReadyLine && IsHighFrequencyBackendLine(line))
        {
            return;
        }

        PostToUi(() =>
        {
            PostLog(line);
            if (backendReadyLine)
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

    private static bool IsHighFrequencyBackendLine(string line)
    {
        return line.StartsWith("[LIVE]", StringComparison.OrdinalIgnoreCase);
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
        if (!consoleStatsEnabled)
        {
            return;
        }

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
