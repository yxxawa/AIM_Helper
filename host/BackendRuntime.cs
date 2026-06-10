using System.Diagnostics;
using System.Text.Json;

namespace AIM_Helper.Host;

public sealed partial class MainForm
{
    private static ProcessStartInfo CreateBackendStartInfo(string backendPath, string workingDirectory, string configJson)
    {
        ProcessStartInfo startInfo = new()
        {
            FileName = backendPath,
            WorkingDirectory = workingDirectory,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        SetEnvironmentPath(startInfo, BuildRuntimePath(workingDirectory, GetEnvironmentPath(startInfo), configJson));
        return startInfo;
    }

    private static string? ResolveBackendPath(string configJson)
    {
        using JsonDocument document = ParseConfig(configJson);
        return ResolveBackendPath(document.RootElement);
    }

    private static string? ResolveBackendPath(JsonElement config)
    {
        string configuredBackend = GetDependencyPath(config, "backendExe");
        if (!string.IsNullOrWhiteSpace(configuredBackend) && File.Exists(configuredBackend))
        {
            return configuredBackend;
        }

        string[] directories =
        [
            Path.Combine(AppContext.BaseDirectory, "backend"),
            Path.Combine(RepoRoot, "build_ninja2"),
            Path.Combine(RepoRoot, "build_ninja"),
            Path.Combine(RepoRoot, "build", "Release"),
            Path.Combine(RepoRoot, "build")
        ];

        return FindFirstExistingFile(directories, BackendExecutableNames);
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
        string[] runtimeDirectories =
        [
            appDirectory,
            workingDirectory,
            backendDirectory,
            RepoRoot,
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
            Path.Combine(RepoRoot, "runtime"),
            Path.Combine(RepoRoot, "deps"),
            Path.Combine(RepoRoot, "deps", "onnxruntime"),
            Path.Combine(RepoRoot, "deps", "onnxruntime", "runtimes", "win-x64", "native"),
            Path.Combine(RepoRoot, "deps", "opencv"),
            Path.Combine(RepoRoot, "deps", "opencv", "x64", "vc16", "bin"),
            Path.Combine(RepoRoot, "deps", "opencv", "build", "x64", "vc16", "bin"),
            Path.Combine(RepoRoot, "deps", "cuda", "bin"),
            Path.Combine(RepoRoot, "deps", "tensorrt", "bin"),
            Path.Combine(RepoRoot, "deps", "tensorrt", "lib"),
            GetDependencyPath(config, "onnxRuntimeDir"),
            GetDependencyPath(config, "opencvDir"),
            GetDependencyPath(config, "cudaDir"),
            GetDependencyPath(config, "tensorrtDir")
        ];

        return ExistingDistinctDirectories(runtimeDirectories);
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
        string modelPath = GetString(config, "modelPath", string.Empty).Trim();
        if (!string.IsNullOrWhiteSpace(modelPath) && Path.IsPathRooted(modelPath))
        {
            return modelPath;
        }

        string modelId = GetString(config, "modelId", string.Empty).Trim();
        if (!string.IsNullOrWhiteSpace(modelId))
        {
            ModelHistoryEntry? historyEntry = LoadModelHistory()
                .FirstOrDefault(item => string.Equals(item.id, modelId, StringComparison.OrdinalIgnoreCase) &&
                                        !string.IsNullOrWhiteSpace(item.localPath) &&
                                        File.Exists(item.localPath));
            if (historyEntry is not null)
            {
                return historyEntry.localPath;
            }
        }

        string configuredModel = GetDependencyPath(config, "modelFile");
        if (!string.IsNullOrWhiteSpace(configuredModel) && File.Exists(configuredModel))
        {
            return configuredModel;
        }

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
            RepoRoot,
            Path.Combine(RepoRoot, "models")
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
        if (!string.IsNullOrWhiteSpace(configuredPath))
        {
            return configuredMatch is not null && IsDriverDllCompatibleWithBackend(configuredMatch, config)
                ? configuredMatch
                : string.Empty;
        }

        string[] dllNames = inputBackend == "dd" ? DdDriverDllNames : GenericDriverDllNames;
        return FindFirstCompatibleDriverDll(BuildDriverDllSearchDirectories(config), dllNames, config) ?? string.Empty;
    }

    private static IEnumerable<string> BuildDriverDllSearchDirectories(JsonElement config)
    {
        string appDirectory = AppContext.BaseDirectory;
        string backendDirectory = Path.GetDirectoryName(ResolveBackendPath(config) ?? string.Empty) ?? string.Empty;
        string configuredDirectory = Path.GetDirectoryName(GetString(config, "driverDllPath", string.Empty)) ?? string.Empty;

        string[] directories =
        [
            configuredDirectory,
            Path.Combine(appDirectory, "drivers"),
            Path.Combine(backendDirectory, "drivers"),
            Path.Combine(RepoRoot, "drivers"),
            backendDirectory,
            appDirectory,
            RepoRoot
        ];

        return ExistingDistinctDirectories(directories);
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

    private static string? FindFirstCompatibleDriverDll(IEnumerable<string> directories, IEnumerable<string> fileNames, JsonElement config)
    {
        foreach (string directory in directories)
        {
            foreach (string fileName in fileNames)
            {
                string candidate = Path.Combine(directory, fileName);
                if (File.Exists(candidate) && IsDriverDllCompatibleWithBackend(candidate, config))
                {
                    return candidate;
                }
            }
        }

        return null;
    }

    private static bool IsDriverDllCompatibleWithBackend(string dllPath, JsonElement config)
    {
        ushort? dllMachine = ReadPeMachine(dllPath);
        string? backendPath = ResolveBackendPath(config);
        ushort? backendMachine = string.IsNullOrWhiteSpace(backendPath) ? null : ReadPeMachine(backendPath);
        if (dllMachine is null || backendMachine is null)
        {
            return true;
        }
        return dllMachine.Value == backendMachine.Value;
    }

    private static ushort? ReadPeMachine(string path)
    {
        try
        {
            using FileStream stream = File.OpenRead(path);
            using BinaryReader reader = new(stream);
            if (stream.Length < 0x40 || reader.ReadUInt16() != 0x5A4D)
            {
                return null;
            }
            stream.Position = 0x3C;
            int peOffset = reader.ReadInt32();
            if (peOffset <= 0 || peOffset + 6 > stream.Length)
            {
                return null;
            }
            stream.Position = peOffset;
            if (reader.ReadUInt32() != 0x00004550)
            {
                return null;
            }
            return reader.ReadUInt16();
        }
        catch
        {
            return null;
        }
    }

    private static IEnumerable<string> ExistingDistinctDirectories(IEnumerable<string> directories)
    {
        return directories
            .Where(path => !string.IsNullOrWhiteSpace(path))
            .Select(Path.GetFullPath)
            .Where(Directory.Exists)
            .Distinct(StringComparer.OrdinalIgnoreCase);
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

    private static string DriverDllExpectedText(string inputBackend)
    {
        return inputBackend == "dd"
            ? @"drivers\dd60300.dll 或 backend\dd60300.dll"
            : @"drivers\logi_driver_direct.dll / DriverBridge.dll / driver_bridge.dll / driver.dll / mouse_driver.dll / logi_driver.dll / g.dll / razer.dll / lgmouse.dll（必须与后端同架构，当前发布版通常为 x64）";
    }
}
