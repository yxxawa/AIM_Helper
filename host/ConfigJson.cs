using System.Diagnostics;
using System.Globalization;
using System.Text.Json;

namespace AIM_Helper.Host;

public sealed partial class MainForm
{
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
}
