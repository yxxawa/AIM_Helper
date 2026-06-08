# 驱动输入后端扩展说明

当前项目默认使用 Win32 `SendInput` 作为鼠标输入后端。同时保留一个可选 `DriverBackend` 适配层，用来加载用户自行提供的驱动桥接 DLL。

本文只说明驱动桥接边界，不提供反作弊绕过、厂商漏洞利用、隐藏行为或公共预编译驱动 DLL。

## 推荐驱动 DLL 导出函数

在 WebView 设置里选择的 DLL 推荐导出下面 5 个函数：

```cpp
extern "C" {
    bool __stdcall DriverInit();
    void __stdcall DriverMove(int x, int y);
    void __stdcall DriverClick(int button);
    void __stdcall DriverScroll(int delta);
    void __stdcall DriverCleanup();
}
```

`DriverClick` 的按钮约定：

- `0`：左键
- `1`：右键
- `2`：中键

如果 DLL 加载失败、导出函数缺失，或 `DriverInit()` 返回 `false`，`DriverBackend` 会记录警告并自动回退到 `SendInputBackend`。

驱动 DLL 必须由用户自行提供、自行签名、自行承担使用风险。本项目不包含、不分发、不安装、不下载、不推荐任何驱动二进制文件。

## 兼容的历史 ABI

`DriverBackend` 还尝试兼容两类历史 DLL ABI。

### `logi_driver.dll` / `g.dll` 风格

```cpp
int __stdcall device_open();
void __stdcall device_close();
int __stdcall moveR(int x, int y);
int __stdcall mouse_down(int button); // 1=左键, 2=右键, 3=中键
int __stdcall mouse_up(int button);
```

该 ABI 没有滚轮导出函数，因此 `Scroll(delta)` 会被忽略并记录警告。

### `lgmouse.dll` 风格

```cpp
void* __stdcall LgMouseOpen();
void __stdcall LgMouseClose(void* handle);
int __stdcall LgMouseMoveEx(void* handle, int x, int y); // 或 LgMouseMove
int __stdcall LgMouseLeftClick(void* handle, int delay_ms);
int __stdcall LgMouseRightClick(void* handle, int delay_ms);
int __stdcall LgMouseMiddleClick(void* handle, int delay_ms);
int __stdcall LgMouseScroll(void* handle, int delta);
```

当前进程是 x64，因此不能直接加载 x86 DLL。DLL 位数必须和进程一致。

## 边界保持简单

应用其他部分不应该关心底层是 Win32、HID、DD 还是其他受控实现。输入层接口应保持接近：

```cpp
class InputBackend {
public:
    virtual void MoveMouse(int x, int y) = 0;
    virtual void Click(int button) = 0;
    virtual void Scroll(int delta) = 0;
};
```

当前 `main.cpp` 的主要移动路径已经集中到相对移动调用。因此截图、模型推理、目标选择、可视化和计时代码不需要因为输入后端变化而改动。

## 后端优先级建议

1. `SendInput`：默认后端，已经实现。
2. `DriverBackend`：通过 WebView 设置或命令行显式选择。
3. 测试用虚拟 HID 或官方 WDK 示例改造的鼠标过滤后端。
4. 经过审查的内部驱动后端，仍然隔离在相同接口之后。

不要把公开厂商驱动技巧、漏洞 PoC 或不明来源 DLL 作为生产依赖。

## 不建议做的事情

- 从未知仓库复制 DLL 直接加载。
- 把 Razer/GHub/MouClass 等 PoC 当成生产代码。
- 加入隐藏服务、持久化、自启动、规避检测或绕过逻辑。
- 在日常主力电脑上做驱动实验。
- 把驱动初始化逻辑混进截图、检测、推理代码。

## 更安全的驱动研究资料

- Microsoft mouse filter sample: https://learn.microsoft.com/en-us/samples/microsoft/windows-driver-samples/mouse-input-wdf-filter-driver-moufiltr/
- Windows driver samples index: https://learn.microsoft.com/en-us/windows-hardware/drivers/samples/
- Windows HLK 和驱动签名文档。

## 命令行选择

WebView 宿主会把类似下面的参数传给 C++ 后端：

```powershell
AIM_Helper_Backend.exe --input-backend=driver --driver-dll="D:\path\to\YourDriverBridge.dll"
```

手动指定 `SendInput`：

```powershell
AIM_Helper_Backend.exe --input-backend=sendinput
```

## 推荐后续重构

建议把当前单文件后端拆成：

- `app/AimAssistant.*`
- `capture/ScreenCapturer.*`
- `detector/ObjectDetector.*`
- `input/InputBackend.*`
- `input/SendInputBackend.*`
- `input/DriverBackend.*`
- `config/Config.*`

然后用工厂函数创建输入后端：

```cpp
std::unique_ptr<InputBackend> CreateInputBackend(const Config& cfg);
```

这样可以把驱动相关代码隔离在 `input/` 模块里，并保持无驱动路径为默认路径。

## 运行安全检查

- 默认后端保持 `SendInput`。
- 驱动后端必须通过配置显式启用。
- 启动时打印当前实际输入后端。
- 鼠标移动有边界限制。
- 有明确可见的停止/退出路径。
- 运行时不需要网络下载。
- 模型或 DLL 只有在用户明确选择或放置后才加载。
