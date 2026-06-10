using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace AIM_Helper.Host;

public sealed partial class MainForm
{
    private void ImportModel(JsonElement root)
    {
        RememberConfig(root);

        using OpenFileDialog dialog = new()
        {
            CheckFileExists = true,
            Multiselect = false,
            Title = "导入 ONNX 模型",
            Filter = "ONNX 模型 (*.onnx)|*.onnx|所有文件 (*.*)|*.*"
        };

        if (dialog.ShowDialog(this) != DialogResult.OK)
        {
            return;
        }

        try
        {
            ModelHistoryEntry entry = ImportModelFile(dialog.FileName);
            List<ModelHistoryEntry> history = LoadModelHistory();
            string payload = JsonSerializer.Serialize(new
            {
                entry,
                history
            });
            PostJson("host:modelImported", payload);
            PostLog($"model imported: {entry.displayName}");
        }
        catch (Exception ex)
        {
            PostLog($"model import failed: {ex.Message}");
            string payload = JsonSerializer.Serialize(new { message = ex.Message });
            PostJson("host:modelImportFailed", payload);
        }
    }

    private void PostModelHistory()
    {
        List<ModelHistoryEntry> history = LoadModelHistory();
        string payload = JsonSerializer.Serialize(new { history });
        PostJson("host:modelHistory", payload);
    }

    private void SelectModelHistory(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) || payload.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        string id = GetString(payload, "id", string.Empty);
        ModelHistoryEntry? entry = LoadModelHistory()
            .FirstOrDefault(item => string.Equals(item.id, id, StringComparison.OrdinalIgnoreCase));
        if (entry is null)
        {
            return;
        }

        string response = JsonSerializer.Serialize(entry);
        PostJson("host:modelSelected", response);
    }

    private void SaveModelPreset(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) || payload.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        string id = GetString(payload, "id", string.Empty);
        if (string.IsNullOrWhiteSpace(id))
        {
            return;
        }

        List<ModelHistoryEntry> history = LoadModelHistory();
        int index = history.FindIndex(item => string.Equals(item.id, id, StringComparison.OrdinalIgnoreCase));
        if (index < 0)
        {
            return;
        }

        ModelHistoryEntry current = history[index];
        ModelClassInfo[] classes = ReadModelClasses(payload, current.classes);
        string enemyCamp = GetString(payload, "enemyCamp", current.enemyCamp);
        string detectionPart = GetString(payload, "detectionPart", current.detectionPart);
        history[index] = current with
        {
            classes = classes,
            enemyCamp = NormalizeFrontendCamp(enemyCamp),
            detectionPart = NormalizeFrontendDetectionPart(detectionPart)
        };
        SaveModelHistory(history);
        PostModelHistory();
    }

    private ModelHistoryEntry ImportModelFile(string sourcePath)
    {
        if (!File.Exists(sourcePath))
        {
            throw new FileNotFoundException("模型文件不存在", sourcePath);
        }

        Directory.CreateDirectory(ModelsDirectory);
        string sha256 = ComputeFileSha256(sourcePath);
        List<ModelHistoryEntry> history = LoadModelHistory();
        ModelHistoryEntry? existing = history.FirstOrDefault(item =>
            string.Equals(item.sha256, sha256, StringComparison.OrdinalIgnoreCase) &&
            File.Exists(item.localPath));
        if (existing is not null)
        {
            return existing;
        }

        string sourceName = Path.GetFileNameWithoutExtension(sourcePath);
        string displayName = string.IsNullOrWhiteSpace(sourceName) ? "model" : sourceName;
        string localName = $"{SafeFileName(displayName)}_{sha256[..8]}.onnx";
        string localPath = Path.Combine(ModelsDirectory, localName);
        if (!File.Exists(localPath))
        {
            File.Copy(sourcePath, localPath, overwrite: false);
        }

        ModelClassInfo[] classes = ExtractOnnxClassMap(localPath);
        OnnxInputShape inputShape = ExtractOnnxInputShape(localPath);
        using JsonDocument config = ParseConfig(lastConfigJson);
        string enemyCamp = config.RootElement.ValueKind == JsonValueKind.Object
            ? GetString(config.RootElement, "enemyCamp", "all")
            : "all";
        string detectionPart = config.RootElement.ValueKind == JsonValueKind.Object
            ? GetString(config.RootElement, "detectionPart", "all")
            : "all";

        ModelHistoryEntry entry = new(
            id: sha256[..16],
            displayName: displayName,
            originalPath: sourcePath,
            localPath: localPath,
            importedAt: DateTimeOffset.Now.ToString("yyyy-MM-dd HH:mm:ss"),
            sha256: sha256,
            inputWidth: inputShape.width,
            inputHeight: inputShape.height,
            classes: classes,
            enemyCamp: NormalizeFrontendCamp(enemyCamp),
            detectionPart: NormalizeFrontendDetectionPart(detectionPart));

        history.RemoveAll(item => string.Equals(item.id, entry.id, StringComparison.OrdinalIgnoreCase));
        history.Insert(0, entry);
        SaveModelHistory(history.Take(30).ToList());
        return entry;
    }

    private static string NormalizeFrontendCamp(string value)
    {
        if (string.IsNullOrWhiteSpace(value) || value.Equals("all", StringComparison.OrdinalIgnoreCase))
        {
            return "all";
        }

        string normalized = NormalizeCamp(value);
        return normalized.Equals("all", StringComparison.OrdinalIgnoreCase) ? "all" : normalized;
    }

    private static string NormalizeFrontendDetectionPart(string value)
    {
        string normalized = value.Trim().ToLowerInvariant();
        return normalized is "head" or "body" ? normalized : "all";
    }

    private static List<ModelHistoryEntry> LoadModelHistory()
    {
        try
        {
            if (!File.Exists(ModelHistoryJsonPath))
            {
                return [];
            }

            using JsonDocument document = JsonDocument.Parse(File.ReadAllText(ModelHistoryJsonPath));
            if (!document.RootElement.TryGetProperty("models", out JsonElement models) ||
                models.ValueKind != JsonValueKind.Array)
            {
                return [];
            }

            List<ModelHistoryEntry> result = [];
            foreach (JsonElement model in models.EnumerateArray())
            {
                ModelHistoryEntry? entry = ReadModelEntry(model);
                if (entry is not null)
                {
                    result.Add(entry);
                }
            }
            return result;
        }
        catch
        {
            return [];
        }
    }

    private static ModelHistoryEntry? ReadModelEntry(JsonElement model)
    {
        if (model.ValueKind != JsonValueKind.Object)
        {
            return null;
        }

        string id = GetString(model, "id", string.Empty);
        string localPath = GetString(model, "localPath", string.Empty);
        if (string.IsNullOrWhiteSpace(id) || string.IsNullOrWhiteSpace(localPath))
        {
            return null;
        }

        int inputWidth = GetInt(model, "inputWidth", 0);
        int inputHeight = GetInt(model, "inputHeight", 0);
        if ((inputWidth <= 0 || inputHeight <= 0) && File.Exists(localPath))
        {
            OnnxInputShape shape = ExtractOnnxInputShape(localPath);
            inputWidth = shape.width;
            inputHeight = shape.height;
        }

        return new ModelHistoryEntry(
            id: id,
            displayName: GetString(model, "displayName", Path.GetFileNameWithoutExtension(localPath)),
            originalPath: GetString(model, "originalPath", string.Empty),
            localPath: localPath,
            importedAt: GetString(model, "importedAt", string.Empty),
            sha256: GetString(model, "sha256", id),
            inputWidth: inputWidth,
            inputHeight: inputHeight,
            classes: ReadModelClasses(model, []),
            enemyCamp: NormalizeFrontendCamp(GetString(model, "enemyCamp", "all")),
            detectionPart: NormalizeFrontendDetectionPart(GetString(model, "detectionPart", "all")));
    }

    private static ModelClassInfo[] ReadModelClasses(JsonElement root, ModelClassInfo[] fallback)
    {
        if (!root.TryGetProperty("classes", out JsonElement classes) || classes.ValueKind != JsonValueKind.Array)
        {
            return fallback;
        }

        List<ModelClassInfo> result = [];
        foreach (JsonElement item in classes.EnumerateArray())
        {
            if (item.ValueKind != JsonValueKind.Object)
            {
                continue;
            }

            int id = GetInt(item, "id", -1);
            if (id < 0)
            {
                continue;
            }

            string name = GetString(item, "name", $"class-{id}");
            result.Add(new ModelClassInfo(
                id,
                name,
                NormalizeRole(GetString(item, "role", InferRole(name))),
                NormalizeCamp(GetString(item, "camp", InferCamp(name))),
                GetBool(item, "enabled", true)));
        }

        return result.Count > 0 ? result.OrderBy(item => item.id).ToArray() : fallback;
    }

    private static void SaveModelHistory(List<ModelHistoryEntry> history)
    {
        Directory.CreateDirectory(ModelsDirectory);
        JsonObject root = new()
        {
            ["version"] = 1,
            ["updatedAt"] = DateTimeOffset.Now.ToString("yyyy-MM-dd HH:mm:ss"),
            ["models"] = JsonSerializer.SerializeToNode(history)
        };
        WriteTextAtomically(ModelHistoryJsonPath, root.ToJsonString(new JsonSerializerOptions { WriteIndented = true }));
    }

    private static string ComputeFileSha256(string path)
    {
        using FileStream stream = File.OpenRead(path);
        byte[] hash = SHA256.HashData(stream);
        return Convert.ToHexString(hash).ToLowerInvariant();
    }

    private static string SafeFileName(string value)
    {
        string clean = string.Join("_", value.Split(Path.GetInvalidFileNameChars(), StringSplitOptions.RemoveEmptyEntries));
        clean = Regex.Replace(clean, @"\s+", "_").Trim('_');
        return string.IsNullOrWhiteSpace(clean) ? "model" : clean;
    }

    private static ModelClassInfo[] ExtractOnnxClassMap(string path)
    {
        try
        {
            Dictionary<string, string> metadata = ReadOnnxMetadataProps(path);
            foreach (string key in new[] { "names", "class_names", "classes", "labels" })
            {
                if (!metadata.TryGetValue(key, out string? value) || string.IsNullOrWhiteSpace(value))
                {
                    continue;
                }

                ModelClassInfo[] classes = ParseClassMapText(value);
                if (classes.Length > 0)
                {
                    return classes;
                }
            }
        }
        catch
        {
        }

        return DefaultGenericClassMap();
    }

    private sealed record OnnxInputShape(int width, int height);

    private static OnnxInputShape ExtractOnnxInputShape(string path)
    {
        try
        {
            byte[] data = File.ReadAllBytes(path);
            int offset = 0;
            while (offset < data.Length)
            {
                ulong tag = ReadVarint(data, ref offset);
                if (tag == 0)
                {
                    break;
                }

                int field = (int)(tag >> 3);
                int wire = (int)(tag & 7);
                if (field == 7 && wire == 2)
                {
                    int length = checked((int)ReadVarint(data, ref offset));
                    int end = Math.Min(data.Length, offset + length);
                    OnnxInputShape shape = ReadGraphInputShape(data, offset, end);
                    if (shape.width > 0 && shape.height > 0)
                    {
                        return shape;
                    }
                    offset = end;
                    continue;
                }

                SkipProtobufField(data, ref offset, wire);
            }
        }
        catch
        {
        }

        return new OnnxInputShape(0, 0);
    }

    private static OnnxInputShape ReadGraphInputShape(byte[] data, int offset, int end)
    {
        while (offset < end)
        {
            ulong tag = ReadVarint(data, ref offset);
            int field = (int)(tag >> 3);
            int wire = (int)(tag & 7);
            if (field == 11 && wire == 2)
            {
                int length = checked((int)ReadVarint(data, ref offset));
                int valueEnd = Math.Min(end, offset + length);
                OnnxInputShape shape = ReadValueInfoInputShape(data, offset, valueEnd);
                if (shape.width > 0 && shape.height > 0)
                {
                    return shape;
                }
                offset = valueEnd;
                continue;
            }

            SkipProtobufField(data, ref offset, wire, end);
        }

        return new OnnxInputShape(0, 0);
    }

    private static OnnxInputShape ReadValueInfoInputShape(byte[] data, int offset, int end)
    {
        while (offset < end)
        {
            ulong tag = ReadVarint(data, ref offset);
            int field = (int)(tag >> 3);
            int wire = (int)(tag & 7);
            if (field == 2 && wire == 2)
            {
                int length = checked((int)ReadVarint(data, ref offset));
                int valueEnd = Math.Min(end, offset + length);
                OnnxInputShape shape = ReadTypeInputShape(data, offset, valueEnd);
                if (shape.width > 0 && shape.height > 0)
                {
                    return shape;
                }
                offset = valueEnd;
                continue;
            }

            SkipProtobufField(data, ref offset, wire, end);
        }

        return new OnnxInputShape(0, 0);
    }

    private static OnnxInputShape ReadTypeInputShape(byte[] data, int offset, int end)
    {
        while (offset < end)
        {
            ulong tag = ReadVarint(data, ref offset);
            int field = (int)(tag >> 3);
            int wire = (int)(tag & 7);
            if (field == 1 && wire == 2)
            {
                int length = checked((int)ReadVarint(data, ref offset));
                int valueEnd = Math.Min(end, offset + length);
                OnnxInputShape shape = ReadTensorTypeInputShape(data, offset, valueEnd);
                if (shape.width > 0 && shape.height > 0)
                {
                    return shape;
                }
                offset = valueEnd;
                continue;
            }

            SkipProtobufField(data, ref offset, wire, end);
        }

        return new OnnxInputShape(0, 0);
    }

    private static OnnxInputShape ReadTensorTypeInputShape(byte[] data, int offset, int end)
    {
        while (offset < end)
        {
            ulong tag = ReadVarint(data, ref offset);
            int field = (int)(tag >> 3);
            int wire = (int)(tag & 7);
            if (field == 2 && wire == 2)
            {
                int length = checked((int)ReadVarint(data, ref offset));
                int valueEnd = Math.Min(end, offset + length);
                OnnxInputShape shape = ReadTensorShape(data, offset, valueEnd);
                if (shape.width > 0 && shape.height > 0)
                {
                    return shape;
                }
                offset = valueEnd;
                continue;
            }

            SkipProtobufField(data, ref offset, wire, end);
        }

        return new OnnxInputShape(0, 0);
    }

    private static OnnxInputShape ReadTensorShape(byte[] data, int offset, int end)
    {
        List<long> dims = [];
        while (offset < end)
        {
            ulong tag = ReadVarint(data, ref offset);
            int field = (int)(tag >> 3);
            int wire = (int)(tag & 7);
            if (field == 1 && wire == 2)
            {
                int length = checked((int)ReadVarint(data, ref offset));
                int valueEnd = Math.Min(end, offset + length);
                long dim = ReadDimensionValue(data, offset, valueEnd);
                dims.Add(dim);
                offset = valueEnd;
                continue;
            }

            SkipProtobufField(data, ref offset, wire, end);
        }

        if (dims.Count >= 4)
        {
            if (dims[1] > 8 && dims[2] > 8 && dims[3] > 0 && dims[3] <= 8)
            {
                return new OnnxInputShape(checked((int)dims[2]), checked((int)dims[1]));
            }

            if (dims[2] > 0 && dims[3] > 0)
            {
                return new OnnxInputShape(checked((int)dims[3]), checked((int)dims[2]));
            }

            if (dims[1] > 0 && dims[2] > 0)
            {
                return new OnnxInputShape(checked((int)dims[2]), checked((int)dims[1]));
            }
        }

        return new OnnxInputShape(0, 0);
    }

    private static long ReadDimensionValue(byte[] data, int offset, int end)
    {
        while (offset < end)
        {
            ulong tag = ReadVarint(data, ref offset);
            int field = (int)(tag >> 3);
            int wire = (int)(tag & 7);
            if (field == 1 && wire == 0)
            {
                ulong value = ReadVarint(data, ref offset);
                return value <= int.MaxValue ? (long)value : 0;
            }

            SkipProtobufField(data, ref offset, wire, end);
        }

        return 0;
    }

    private static Dictionary<string, string> ReadOnnxMetadataProps(string path)
    {
        byte[] data = File.ReadAllBytes(path);
        Dictionary<string, string> metadata = new(StringComparer.OrdinalIgnoreCase);
        int offset = 0;
        while (offset < data.Length)
        {
            ulong tag = ReadVarint(data, ref offset);
            if (tag == 0)
            {
                break;
            }

            int field = (int)(tag >> 3);
            int wire = (int)(tag & 7);
            if (field == 14 && wire == 2)
            {
                int length = checked((int)ReadVarint(data, ref offset));
                int end = Math.Min(data.Length, offset + length);
                (string key, string value) = ParseMetadataEntry(data, offset, end);
                if (!string.IsNullOrWhiteSpace(key))
                {
                    metadata[key.Trim()] = value;
                }
                offset = end;
                continue;
            }

            SkipProtobufField(data, ref offset, wire);
        }

        return metadata;
    }

    private static (string key, string value) ParseMetadataEntry(byte[] data, int offset, int end)
    {
        string key = string.Empty;
        string value = string.Empty;
        while (offset < end)
        {
            ulong tag = ReadVarint(data, ref offset);
            int field = (int)(tag >> 3);
            int wire = (int)(tag & 7);
            if (wire == 2 && (field == 1 || field == 2))
            {
                int length = checked((int)ReadVarint(data, ref offset));
                int stringEnd = Math.Min(end, offset + length);
                string text = Encoding.UTF8.GetString(data, offset, stringEnd - offset);
                if (field == 1)
                {
                    key = text;
                }
                else
                {
                    value = text;
                }
                offset = stringEnd;
                continue;
            }

            SkipProtobufField(data, ref offset, wire, end);
        }

        return (key, value);
    }

    private static ulong ReadVarint(byte[] data, ref int offset)
    {
        ulong result = 0;
        int shift = 0;
        while (offset < data.Length && shift < 64)
        {
            byte current = data[offset++];
            result |= (ulong)(current & 0x7F) << shift;
            if ((current & 0x80) == 0)
            {
                return result;
            }
            shift += 7;
        }
        return result;
    }

    private static void SkipProtobufField(byte[] data, ref int offset, int wire, int? limit = null)
    {
        int end = limit ?? data.Length;
        switch (wire)
        {
            case 0:
                _ = ReadVarint(data, ref offset);
                break;
            case 1:
                offset = Math.Min(end, offset + 8);
                break;
            case 2:
                int length = checked((int)ReadVarint(data, ref offset));
                offset = Math.Min(end, offset + length);
                break;
            case 5:
                offset = Math.Min(end, offset + 4);
                break;
            default:
                offset = end;
                break;
        }
    }

    private static ModelClassInfo[] ParseClassMapText(string text)
    {
        string trimmed = text.Trim();
        Dictionary<int, string> names = [];

        if (trimmed.StartsWith("{") || trimmed.StartsWith("["))
        {
            try
            {
                using JsonDocument document = JsonDocument.Parse(trimmed);
                if (document.RootElement.ValueKind == JsonValueKind.Object)
                {
                    foreach (JsonProperty property in document.RootElement.EnumerateObject())
                    {
                        if (int.TryParse(property.Name, out int id) && property.Value.ValueKind == JsonValueKind.String)
                        {
                            names[id] = property.Value.GetString() ?? $"class-{id}";
                        }
                    }
                }
                else if (document.RootElement.ValueKind == JsonValueKind.Array)
                {
                    int index = 0;
                    foreach (JsonElement item in document.RootElement.EnumerateArray())
                    {
                        if (item.ValueKind == JsonValueKind.String)
                        {
                            names[index] = item.GetString() ?? $"class-{index}";
                        }
                        ++index;
                    }
                }
            }
            catch
            {
            }
        }

        foreach (Match match in Regex.Matches(trimmed, @"['""]?(?<id>\d+)['""]?\s*:\s*['""](?<name>[^'""]+)['""]"))
        {
            if (int.TryParse(match.Groups["id"].Value, out int id))
            {
                names[id] = match.Groups["name"].Value;
            }
        }

        if (names.Count == 0)
        {
            int index = 0;
            foreach (Match match in Regex.Matches(trimmed, @"['""](?<name>[^'""]+)['""]"))
            {
                names[index++] = match.Groups["name"].Value;
            }
        }

        if (names.Count == 0 && trimmed.Contains('\n'))
        {
            int index = 0;
            foreach (string line in trimmed.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
            {
                names[index++] = line;
            }
        }

        return names
            .OrderBy(item => item.Key)
            .Select(item => new ModelClassInfo(
                item.Key,
                item.Value,
                InferRole(item.Value),
                InferCamp(item.Value),
                true))
            .ToArray();
    }

    private static ModelClassInfo[] DefaultGenericClassMap()
    {
        return
        [
            new(0, "class-0", "other", "敌方", true),
            new(1, "class-1", "other", "敌方", true),
            new(2, "class-2", "other", "敌方", true),
            new(3, "class-3", "other", "敌方", true)
        ];
    }

    private static string InferRole(string name)
    {
        string value = name.ToLowerInvariant();
        if (value.Contains("head") || value.Contains("头"))
        {
            return "head";
        }
        if (value.Contains("body") || value.Contains("身体") || value.Contains("person") || value.Contains("player"))
        {
            return "body";
        }
        return "other";
    }

    private static string InferCamp(string name)
    {
        string value = name.ToLowerInvariant();
        if (Regex.IsMatch(value, @"(^|[^a-z])ct([^a-z]|$)") || value.Contains("counter"))
        {
            return "ct";
        }
        if (Regex.IsMatch(value, @"(^|[^a-z])t([^a-z]|$)") || value.Contains("terror"))
        {
            return "t";
        }
        return "unknown";
    }

    private static string NormalizeRole(string role)
    {
        string value = role.Trim().ToLowerInvariant();
        return value is "head" or "body" or "other" ? value : "other";
    }

    private static string NormalizeCamp(string camp)
    {
        string value = Regex.Replace(camp.Trim(), @"[;,:=|\r\n\t]+", " ");
        value = Regex.Replace(value, @"\s+", " ").Trim();
        if (value.Equals("all", StringComparison.OrdinalIgnoreCase))
        {
            return "all";
        }
        if (string.IsNullOrWhiteSpace(value) || value.Equals("unknown", StringComparison.OrdinalIgnoreCase))
        {
            return "敌方";
        }

        string lowered = value.ToLowerInvariant();
        if (lowered is "ct" or "t")
        {
            return lowered;
        }

        return value.Length > 24 ? value[..24] : value;
    }

    private static string GetTargetClassIds(JsonElement config)
    {
        if (!config.TryGetProperty("modelClasses", out JsonElement classes) ||
            classes.ValueKind != JsonValueKind.Array)
        {
            return string.Empty;
        }

        string enemyCamp = NormalizeFrontendCamp(GetString(config, "enemyCamp", "all"));
        string detectionPart = NormalizeFrontendDetectionPart(GetString(config, "detectionPart", "all"));
        List<int> accepted = [];
        int total = 0;
        foreach (JsonElement item in classes.EnumerateArray())
        {
            if (item.ValueKind != JsonValueKind.Object)
            {
                continue;
            }

            int id = GetInt(item, "id", -1);
            if (id < 0)
            {
                continue;
            }

            ++total;
            if (!GetBool(item, "enabled", true))
            {
                continue;
            }

            string camp = NormalizeCamp(GetString(item, "camp", InferCamp(GetString(item, "name", string.Empty))));
            string role = NormalizeRole(GetString(item, "role", InferRole(GetString(item, "name", string.Empty))));
            if (!ModelClassMatchesCamp(camp, enemyCamp) || !ModelClassMatchesPart(role, detectionPart))
            {
                continue;
            }

            accepted.Add(id);
        }

        if (total == 0 || accepted.Count == total)
        {
            return string.Empty;
        }
        if (accepted.Count == 0)
        {
            return "-999999";
        }

        accepted.Sort();
        return string.Join(",", accepted.Distinct());
    }

    private static string GetModelClassRoles(JsonElement config)
    {
        return GetModelClassMap(config, static item =>
        {
            string name = GetString(item, "name", string.Empty);
            return NormalizeRole(GetString(item, "role", InferRole(name)));
        });
    }

    private static string GetModelClassNames(JsonElement config)
    {
        return GetModelClassMap(config, static item =>
        {
            string name = GetString(item, "name", string.Empty).Trim();
            return string.IsNullOrWhiteSpace(name) ? string.Empty : name;
        });
    }

    private static string GetModelClassCamps(JsonElement config)
    {
        return GetModelClassMap(config, static item =>
        {
            string name = GetString(item, "name", string.Empty);
            return NormalizeCamp(GetString(item, "camp", InferCamp(name)));
        });
    }

    private static string GetModelClassMap(JsonElement config, Func<JsonElement, string> valueSelector)
    {
        if (!config.TryGetProperty("modelClasses", out JsonElement classes) ||
            classes.ValueKind != JsonValueKind.Array)
        {
            return string.Empty;
        }

        List<string> pairs = [];
        foreach (JsonElement item in classes.EnumerateArray())
        {
            if (item.ValueKind != JsonValueKind.Object)
            {
                continue;
            }

            int id = GetInt(item, "id", -1);
            if (id < 0)
            {
                continue;
            }

            string value = valueSelector(item);
            if (!string.IsNullOrWhiteSpace(value))
            {
                pairs.Add($"{id}:{value}");
            }
        }

        return string.Join(";", pairs);
    }

    private static bool ModelClassMatchesCamp(string classCamp, string ownCamp)
    {
        ownCamp = NormalizeFrontendCamp(ownCamp);
        if (ownCamp.Equals("all", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        classCamp = NormalizeCamp(classCamp);
        return !classCamp.Equals(ownCamp, StringComparison.OrdinalIgnoreCase);
    }

    private static bool ModelClassMatchesPart(string role, string detectionPart)
    {
        detectionPart = NormalizeFrontendDetectionPart(detectionPart);
        if (detectionPart == "all")
        {
            return true;
        }

        return NormalizeRole(role) == detectionPart;
    }
}
