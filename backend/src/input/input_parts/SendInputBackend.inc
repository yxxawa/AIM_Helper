// InputBackend interface and Win32 SendInput backend

// --- 2. Mouse input backends ---
static bool SendKeyboardInputVk(int vk, bool key_down, const char* context) {
    if (vk <= 0) {
        return false;
    }

    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = static_cast<WORD>(vk);
    input.ki.dwFlags = key_down ? 0 : KEYEVENTF_KEYUP;
    if (::SendInput(1, &input, sizeof(INPUT)) != 1) {
        const DWORD error = GetLastError();
        std::cerr << "\n[WARN] " << context << " SendInput key "
            << (key_down ? "down" : "up")
            << " failed for " << KeyNameFromVk(vk)
            << ". GetLastError=" << error << std::endl;
        return false;
    }
    return true;
}

class InputBackend {
public:
    virtual ~InputBackend() = default;
    virtual void MoveMouse(int x, int y) = 0;
    virtual void Click(int button) = 0;
    virtual bool MouseDown(int button) = 0;
    virtual bool MouseUp(int button) = 0;
    virtual void Scroll(int delta) = 0;
    virtual void KeyDown(int vk) = 0;
    virtual void KeyUp(int vk) = 0;
    virtual const char* Name() const = 0;
    virtual bool IsDriverBackend() const { return false; }
};

class SendInputBackend final : public InputBackend {
public:
    SendInputBackend() {
        std::cout << "--- Mouse input initialized: Win32 SendInput (no driver) ---" << std::endl;
    }

    void MoveMouse(int x, int y) override {
        if (x == 0 && y == 0) {
            return;
        }

        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dx = static_cast<LONG>(x);
        input.mi.dy = static_cast<LONG>(y);
        input.mi.dwFlags = MOUSEEVENTF_MOVE;

        if (::SendInput(1, &input, sizeof(INPUT)) != 1) {
            const DWORD error = GetLastError();
            std::cerr << "\n[WARN] SendInput mouse move failed. GetLastError=" << error << std::endl;
        }
    }

    static bool ButtonFlag(int button, bool down, DWORD& flag) {
        if (button == 0) {
            flag = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        }
        else if (button == 1) {
            flag = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        }
        else if (button == 2) {
            flag = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        }
        else {
            return false;
        }
        return true;
    }

    bool SendMouseButton(int button, bool down) {
        DWORD flag = 0;
        if (!ButtonFlag(button, down, flag)) {
            std::cerr << "\n[WARN] Unsupported SendInput mouse button: " << button << std::endl;
            return false;
        }

        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = flag;
        if (::SendInput(1, &input, sizeof(INPUT)) != 1) {
            const DWORD error = GetLastError();
            std::cerr << "\n[WARN] SendInput mouse "
                << (down ? "down" : "up")
                << " failed. GetLastError=" << error << std::endl;
            return false;
        }
        return true;
    }

    void Click(int button) override {
        DWORD down_flag = 0;
        DWORD up_flag = 0;
        if (!ButtonFlag(button, true, down_flag) || !ButtonFlag(button, false, up_flag)) {
            std::cerr << "\n[WARN] Unsupported SendInput mouse button: " << button << std::endl;
            return;
        }

        INPUT inputs[2]{};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = down_flag;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = up_flag;
        if (::SendInput(2, inputs, sizeof(INPUT)) != 2) {
            const DWORD error = GetLastError();
            std::cerr << "\n[WARN] SendInput click failed. GetLastError=" << error << std::endl;
        }
    }

    bool MouseDown(int button) override {
        return SendMouseButton(button, true);
    }

    bool MouseUp(int button) override {
        return SendMouseButton(button, false);
    }

    void Scroll(int delta) override {
        if (delta == 0) {
            return;
        }

        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.mouseData = static_cast<DWORD>(delta);
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        if (::SendInput(1, &input, sizeof(INPUT)) != 1) {
            const DWORD error = GetLastError();
            std::cerr << "\n[WARN] SendInput scroll failed. GetLastError=" << error << std::endl;
        }
    }

    void KeyDown(int vk) override {
        SendKeyboardInputVk(vk, true, "SendInput");
    }

    void KeyUp(int vk) override {
        SendKeyboardInputVk(vk, false, "SendInput");
    }

    const char* Name() const override {
        return "SendInput";
    }
};

