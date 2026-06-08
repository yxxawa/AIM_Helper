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
            : @"drivers\DriverBridge.dll / driver_bridge.dll / driver.dll / mouse_driver.dll / logi_driver.dll";
    }
}
