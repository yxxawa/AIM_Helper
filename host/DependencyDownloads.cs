using System.Collections.Concurrent;
using System.Diagnostics;
using System.IO.Compression;
using System.Net;
using System.Net.Http.Headers;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace AIM_Helper.Host;

public sealed partial class MainForm
{
    private readonly ConcurrentDictionary<string, byte> activeDownloads = new(StringComparer.OrdinalIgnoreCase);
    private static readonly HttpClient DependencyHttpClient = CreateDependencyHttpClient();

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
}
