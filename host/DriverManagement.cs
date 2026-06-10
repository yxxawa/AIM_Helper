using System.Text.Json;
using System.Windows.Forms;

namespace AIM_Helper.Host;

public sealed partial class MainForm
{
    private void ChooseDriverDll(JsonElement root)
    {
        RememberConfig(root);

        using OpenFileDialog dialog = new()
        {
            CheckFileExists = true,
            Multiselect = false,
            Title = "选择驱动 DLL",
            Filter = "驱动 DLL (*.dll)|*.dll|所有文件 (*.*)|*.*"
        };

        string currentPath = string.Empty;
        using (JsonDocument config = ParseConfig(lastConfigJson))
        {
            currentPath = GetString(config.RootElement, "driverDllPath", string.Empty);
        }
        if (!string.IsNullOrWhiteSpace(currentPath))
        {
            string? directory = Path.GetDirectoryName(currentPath);
            if (!string.IsNullOrWhiteSpace(directory) && Directory.Exists(directory))
            {
                dialog.InitialDirectory = directory;
            }
        }

        if (dialog.ShowDialog(this) != DialogResult.OK)
        {
            return;
        }

        DriverHistoryEntry entry = AddDriverHistoryEntry(dialog.FileName);
        string payload = JsonSerializer.Serialize(new
        {
            entry,
            history = LoadDriverHistory()
        });
        PostJson("host:driverSelected", payload);
        PostLog($"driver dll selected: {entry.displayName}");
    }

    private void PostDriverHistory()
    {
        string payload = JsonSerializer.Serialize(new { history = LoadDriverHistory() });
        PostJson("host:driverHistory", payload);
    }

    private void SelectDriverHistory(JsonElement root)
    {
        if (!root.TryGetProperty("payload", out JsonElement payload) || payload.ValueKind != JsonValueKind.Object)
        {
            return;
        }

        string id = GetString(payload, "id", string.Empty);
        DriverHistoryEntry? entry = LoadDriverHistory()
            .FirstOrDefault(item => string.Equals(item.id, id, StringComparison.OrdinalIgnoreCase) &&
                                    File.Exists(item.path));
        if (entry is null)
        {
            PostDriverHistory();
            return;
        }

        string response = JsonSerializer.Serialize(new
        {
            entry,
            history = LoadDriverHistory()
        });
        PostJson("host:driverSelected", response);
        PostLog($"driver dll selected from history: {entry.displayName}");
    }

    private static DriverHistoryEntry AddDriverHistoryEntry(string path)
    {
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("驱动 DLL 不存在", path);
        }

        string sha256 = ComputeFileSha256(path);
        string architecture = PeArchitecture(path);
        DriverHistoryEntry entry = new(
            id: sha256[..16],
            displayName: Path.GetFileName(path),
            path: path,
            selectedAt: DateTimeOffset.Now.ToString("yyyy-MM-dd HH:mm:ss"),
            sha256: sha256,
            architecture: architecture);

        List<DriverHistoryEntry> history = LoadDriverHistory();
        history.RemoveAll(item =>
            string.Equals(item.id, entry.id, StringComparison.OrdinalIgnoreCase) ||
            string.Equals(item.path, entry.path, StringComparison.OrdinalIgnoreCase));
        history.Insert(0, entry);
        SaveDriverHistory(history.Take(30).ToList());
        return entry;
    }

    private static List<DriverHistoryEntry> LoadDriverHistory()
    {
        try
        {
            if (!File.Exists(DriverHistoryJsonPath))
            {
                return [];
            }

            using JsonDocument document = JsonDocument.Parse(File.ReadAllText(DriverHistoryJsonPath));
            if (!document.RootElement.TryGetProperty("drivers", out JsonElement drivers) ||
                drivers.ValueKind != JsonValueKind.Array)
            {
                return [];
            }

            List<DriverHistoryEntry> result = [];
            foreach (JsonElement item in drivers.EnumerateArray())
            {
                DriverHistoryEntry? entry = ReadDriverEntry(item);
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

    private static DriverHistoryEntry? ReadDriverEntry(JsonElement item)
    {
        if (item.ValueKind != JsonValueKind.Object)
        {
            return null;
        }

        string id = GetString(item, "id", string.Empty);
        string path = GetString(item, "path", string.Empty);
        if (string.IsNullOrWhiteSpace(id) || string.IsNullOrWhiteSpace(path))
        {
            return null;
        }

        return new DriverHistoryEntry(
            id: id,
            displayName: GetString(item, "displayName", Path.GetFileName(path)),
            path: path,
            selectedAt: GetString(item, "selectedAt", string.Empty),
            sha256: GetString(item, "sha256", id),
            architecture: GetString(item, "architecture", File.Exists(path) ? PeArchitecture(path) : "unknown"));
    }

    private static void SaveDriverHistory(List<DriverHistoryEntry> history)
    {
        Directory.CreateDirectory(DriversDirectory);
        string json = JsonSerializer.Serialize(new { drivers = history }, new JsonSerializerOptions
        {
            WriteIndented = true
        });
        File.WriteAllText(DriverHistoryJsonPath, json);
    }

    private static string PeArchitecture(string path)
    {
        try
        {
            using FileStream stream = File.OpenRead(path);
            using BinaryReader reader = new(stream);
            stream.Seek(0x3c, SeekOrigin.Begin);
            int peOffset = reader.ReadInt32();
            stream.Seek(peOffset + 4, SeekOrigin.Begin);
            ushort machine = reader.ReadUInt16();
            return machine switch
            {
                0x8664 => "x64",
                0x014c => "x86",
                0xaa64 => "arm64",
                _ => $"0x{machine:X4}"
            };
        }
        catch
        {
            return "unknown";
        }
    }
}
