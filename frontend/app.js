const root = document.querySelector(".app-shell");
const eventLog = document.querySelector("#eventLog");

const defaults = {
    configVersion: 18,
    backend: "cpu",
    inputBackend: "dd",
    modelPath: "",
    driverDllPath: "",
    driverType: 1,
    dependencyPaths: {},
    cropSize: 320,
    lockRadius: 260,
    confidence: 0.5,
    aimHotkey: 6,
    aimHotkey2: 0,
    smoothing: 1.0,
    aimMode: "atan",
    aimGain: 0.45,
    deadzone: 1.5,
    aimFilter: "none",
    pidKp: 1.0,
    pidKi: 0.0,
    pidKd: 0.0,
    pidIntegralLimit: 120,
    oneEuroMinCutoff: 10.0,
    oneEuroBeta: 0.0,
    oneEuroDCutoff: 0.1,
    enablePrediction: false,
    predictionMode: "kalman",
    predictionLeadMs: 20,
    predictionSmoothing: 0.12,
    predictionAccelerationSmoothing: 0.18,
    predictionAlpha: 0.45,
    predictionBeta: 0.06,
    predictionKalmanMeasurementNoise: 34.0,
    predictionKalmanProcessNoise: 72.0,
    predictionMaxPixels: 60,
    predictionResetPixels: 70,
    predictionNoisePixels: 1.5,
    predictionOutputSmoothing: 0.20,
    predictionServoGain: 0.65,
    enableDroneTracking: false,
    droneTrackGain: 1.35,
    droneTrackVelocityGain: 0.55,
    droneTrackDamping: 0.18,
    droneTrackSmoothing: 0.78,
    droneTrackMaxMove: 220,
    droneTrackDeadzone: 0.3,
    targetX: 0.5,
    targetY: 0.3,
    enableAutoAimPart: true,
    aimPartPriority: "distance",
    enemyCamp: "all",
    detectionPart: "all",
    maxMove: 60,
    enableTrackingBoost: true,
    trackingBoostThreshold: 4,
    trackingBoostGain: 2.0,
    trackingBoostMaxMove: 120,
    enableHumanSlide: false,
    humanSlideMaxStep: 50,
    humanSlideJitter: 0.5,
    humanSlideDelayMin: 5,
    humanSlideDelayMax: 20,
    enableAutoClick: false,
    autoClickDelayMin: 0,
    autoClickDelayMax: 0,
    autoClickIntervalMin: 0,
    autoClickIntervalMax: 0,
    autoClickTolerance: 3.0,
    enableAutoStop: false,
    autoStopMode: "counter_tap",
    autoStopHoldMs: 75,
    autoStopSettleMs: 15,
    enableCapture: true,
    enableMouseMove: true,
    enableHoldToAim: true,
    enableVisualization: true,
    enableConsoleStats: false,
    boundedMovement: true
};

const state = {
    running: false,
    backendPhase: "stopped",
    view: "home",
    dependencies: {
        ok: false,
        missingCount: 0,
        system: null,
        downloads: [],
        items: []
    },
    downloadProgress: {},
    config: { ...defaults }
};

const filterDefaults = {
    aimFilter: defaults.aimFilter,
    pidKp: defaults.pidKp,
    pidKi: defaults.pidKi,
    pidKd: defaults.pidKd,
    pidIntegralLimit: defaults.pidIntegralLimit,
    oneEuroMinCutoff: defaults.oneEuroMinCutoff,
    oneEuroBeta: defaults.oneEuroBeta,
    oneEuroDCutoff: defaults.oneEuroDCutoff
};

const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => Array.from(document.querySelectorAll(selector));

const controls = {
    runBtn: $("#runBtn"),
    stopBtn: $("#stopBtn"),
    saveBtn: $("#saveBtn"),
    resetBtn: $("#resetBtn"),
    resetFilterBtn: $("#resetFilterBtn"),
    testBackendBtn: $("#testBackendBtn"),
    clearLogBtn: $("#clearLogBtn"),
    logPanel: $("#logPanel"),
    runtimeState: $("#runtimeState"),
    driverState: $("#driverState"),
    dependencyState: $("#dependencyState"),
    configState: $("#configState"),
    pageNav: $("#pageNav"),
    backHomeBtn: $("#backHomeBtn"),
    pageTitle: $("#pageTitle"),
    pageHint: $("#pageHint"),
    backendLabel: $("#backendLabel"),
    filterLabel: $("#filterLabel"),
    inputBackendLabel: $("#inputBackendLabel"),
    dependencySummary: $("#dependencySummary"),
    dependencySystem: $("#dependencySystem"),
    dependencyDownloads: $("#dependencyDownloads"),
    dependencyList: $("#dependencyList"),
    aimHotkeySwitchLabel: $("#aimHotkeySwitchLabel"),
    humanSlideSwitchLabel: $("#humanSlideSwitchLabel"),
    predictionSwitchLabel: $("#predictionSwitchLabel"),
    droneTrackingSwitchLabel: $("#droneTrackingSwitchLabel"),
    autoClickSwitchLabel: $("#autoClickSwitchLabel"),
    modelPath: $("#modelPath"),
    inputBackend: $("#inputBackend"),
    driverDllPath: $("#driverDllPath"),
    driverType: $("#driverType"),
    aimHotkeyBtn: $("#aimHotkeyBtn"),
    aimHotkey2Btn: $("#aimHotkey2Btn"),
    humanSlideSlot: $("#humanSlideSlot"),
    humanSlideLabel: $("#humanSlideLabel"),
    humanSlideMaxStep: $("#humanSlideMaxStep"),
    humanSlideJitter: $("#humanSlideJitter"),
    humanSlideDelayMin: $("#humanSlideDelayMin"),
    humanSlideDelayMax: $("#humanSlideDelayMax"),
    autoClickSlot: $("#autoClickSlot"),
    autoClickLabel: $("#autoClickLabel"),
    autoClickDelayRange: $("#autoClickDelayRange"),
    autoClickDelayMin: $("#autoClickDelayMin"),
    autoClickDelayMax: $("#autoClickDelayMax"),
    autoClickDelayMinValue: $("#autoClickDelayMinValue"),
    autoClickDelayMaxValue: $("#autoClickDelayMaxValue"),
    autoClickIntervalRange: $("#autoClickIntervalRange"),
    autoClickIntervalMin: $("#autoClickIntervalMin"),
    autoClickIntervalMax: $("#autoClickIntervalMax"),
    autoClickIntervalMinValue: $("#autoClickIntervalMinValue"),
    autoClickIntervalMaxValue: $("#autoClickIntervalMaxValue"),
    autoClickTolerance: $("#autoClickTolerance"),
    autoClickToleranceValue: $("#autoClickToleranceValue"),
    enableAutoStop: $("#enableAutoStop"),
    autoStopMode: $("#autoStopMode"),
    autoStopModeField: $("#autoStopModeField"),
    autoStopHoldField: $("#autoStopHoldField"),
    autoStopSettleField: $("#autoStopSettleField"),
    autoStopHoldMs: $("#autoStopHoldMs"),
    autoStopSettleMs: $("#autoStopSettleMs"),
    autoStopHoldMsValue: $("#autoStopHoldMsValue"),
    autoStopSettleMsValue: $("#autoStopSettleMsValue"),
    filterSlot: $("#filterSlot"),
    aimFilter: $("#aimFilter"),
    pidKp: $("#pidKp"),
    pidKi: $("#pidKi"),
    pidKd: $("#pidKd"),
    pidIntegralLimit: $("#pidIntegralLimit"),
    oneEuroMinCutoff: $("#oneEuroMinCutoff"),
    oneEuroBeta: $("#oneEuroBeta"),
    oneEuroDCutoff: $("#oneEuroDCutoff"),
    predictionSlot: $("#predictionSlot"),
    predictionMode: $("#predictionMode"),
    predictionLeadMs: $("#predictionLeadMs"),
    predictionSmoothing: $("#predictionSmoothing"),
    predictionAccelerationSmoothing: $("#predictionAccelerationSmoothing"),
    predictionAlpha: $("#predictionAlpha"),
    predictionBeta: $("#predictionBeta"),
    predictionKalmanMeasurementNoise: $("#predictionKalmanMeasurementNoise"),
    predictionKalmanProcessNoise: $("#predictionKalmanProcessNoise"),
    predictionMaxPixels: $("#predictionMaxPixels"),
    predictionResetPixels: $("#predictionResetPixels"),
    predictionNoisePixels: $("#predictionNoisePixels"),
    predictionOutputSmoothing: $("#predictionOutputSmoothing"),
    predictionServoGain: $("#predictionServoGain"),
    droneTrackingSlot: $("#droneTrackingSlot"),
    droneTrackingLabel: $("#droneTrackingLabel"),
    droneTrackGain: $("#droneTrackGain"),
    droneTrackVelocityGain: $("#droneTrackVelocityGain"),
    droneTrackDamping: $("#droneTrackDamping"),
    droneTrackSmoothing: $("#droneTrackSmoothing"),
    droneTrackMaxMove: $("#droneTrackMaxMove"),
    droneTrackDeadzone: $("#droneTrackDeadzone"),
    cropSize: $("#cropSize"),
    lockRadius: $("#lockRadius"),
    confidence: $("#confidence"),
    smoothing: $("#smoothing"),
    aimMode: $("#aimMode"),
    aimGain: $("#aimGain"),
    deadzone: $("#deadzone"),
    targetX: $("#targetX"),
    targetY: $("#targetY"),
    aimPartPriority: $("#aimPartPriority"),
    enemyCamp: $("#enemyCamp"),
    detectionPart: $("#detectionPart"),
    maxMove: $("#maxMove"),
    trackingBoostThreshold: $("#trackingBoostThreshold"),
    trackingBoostGain: $("#trackingBoostGain"),
    trackingBoostMaxMove: $("#trackingBoostMaxMove"),
    cropValue: $("#cropValue"),
    radiusValue: $("#radiusValue"),
    confidenceValue: $("#confidenceValue"),
    smoothValue: $("#smoothValue"),
    aimGainValue: $("#aimGainValue"),
    deadzoneValue: $("#deadzoneValue"),
    targetXValue: $("#targetXValue"),
    targetYValue: $("#targetYValue"),
    targetCard: $("#targetCard"),
    aimPartPriorityCard: $("#aimPartPriorityCard"),
    maxMoveValue: $("#maxMoveValue"),
    pidKpValue: $("#pidKpValue"),
    pidKiValue: $("#pidKiValue"),
    pidKdValue: $("#pidKdValue"),
    pidIntegralLimitValue: $("#pidIntegralLimitValue"),
    oneEuroMinCutoffValue: $("#oneEuroMinCutoffValue"),
    oneEuroBetaValue: $("#oneEuroBetaValue"),
    oneEuroDCutoffValue: $("#oneEuroDCutoffValue"),
    predictionLabel: $("#predictionLabel"),
    predictionLeadMsValue: $("#predictionLeadMsValue"),
    predictionSmoothingValue: $("#predictionSmoothingValue"),
    predictionAccelerationSmoothingValue: $("#predictionAccelerationSmoothingValue"),
    predictionAlphaValue: $("#predictionAlphaValue"),
    predictionBetaValue: $("#predictionBetaValue"),
    predictionKalmanMeasurementNoiseValue: $("#predictionKalmanMeasurementNoiseValue"),
    predictionKalmanProcessNoiseValue: $("#predictionKalmanProcessNoiseValue"),
    predictionMaxPixelsValue: $("#predictionMaxPixelsValue"),
    predictionResetPixelsValue: $("#predictionResetPixelsValue"),
    predictionNoisePixelsValue: $("#predictionNoisePixelsValue"),
    predictionOutputSmoothingValue: $("#predictionOutputSmoothingValue"),
    predictionServoGainValue: $("#predictionServoGainValue"),
    droneTrackGainValue: $("#droneTrackGainValue"),
    droneTrackVelocityGainValue: $("#droneTrackVelocityGainValue"),
    droneTrackDampingValue: $("#droneTrackDampingValue"),
    droneTrackSmoothingValue: $("#droneTrackSmoothingValue"),
    droneTrackMaxMoveValue: $("#droneTrackMaxMoveValue"),
    droneTrackDeadzoneValue: $("#droneTrackDeadzoneValue"),
    humanSlideMaxStepValue: $("#humanSlideMaxStepValue"),
    humanSlideJitterValue: $("#humanSlideJitterValue"),
    humanSlideDelayMinValue: $("#humanSlideDelayMinValue"),
    humanSlideDelayMaxValue: $("#humanSlideDelayMaxValue"),
    rangeCircle: $("#rangeCircle"),
    rangePreviewLabel: $("#rangePreviewLabel"),
    rangeCropMetric: $("#rangeCropMetric"),
    rangeRadiusMetric: $("#rangeRadiusMetric"),
    targetPoint: $("#targetPoint"),
    targetPreviewLabel: $("#targetPreviewLabel"),
    enemyCampLabel: $("#enemyCampLabel"),
    detectionPartLabel: $("#detectionPartLabel"),
    trackingBoostFields: $("#trackingBoostFields"),
    trackingBoostThresholdValue: $("#trackingBoostThresholdValue"),
    trackingBoostGainValue: $("#trackingBoostGainValue"),
    trackingBoostMaxMoveValue: $("#trackingBoostMaxMoveValue"),
    switches: {
        enableCapture: $("#enableCapture"),
        enableAutoAimPart: $("#enableAutoAimPart"),
        enableMouseMove: $("#enableMouseMove"),
        enableTrackingBoost: $("#enableTrackingBoost"),
        enableHumanSlide: $("#enableHumanSlide"),
        enablePrediction: $("#enablePrediction"),
        enableDroneTracking: $("#enableDroneTracking"),
        enableAutoClick: $("#enableAutoClick"),
        enableHoldToAim: $("#enableHoldToAim"),
        enableVisualization: $("#enableVisualization"),
        enableConsoleStats: $("#enableConsoleStats"),
        boundedMovement: $("#boundedMovement")
    }
};

const bridge = {
    available() {
        return Boolean(window.chrome && window.chrome.webview);
    },
    send(type, payload = {}) {
        const message = { type, payload, sentAt: Date.now() };
        if (this.available()) {
            window.chrome.webview.postMessage(message);
        } else {
            logEvent(`preview:${type}`);
        }
    }
};

const hotkeyLabels = new Map([
    [0, "NONE"],
    [1, "鼠标左键"],
    [2, "鼠标右键"],
    [4, "鼠标中键"],
    [5, "鼠标 X1"],
    [6, "鼠标 X2"],
    [8, "Backspace"],
    [9, "Tab"],
    [13, "Enter"],
    [16, "Shift"],
    [17, "Ctrl"],
    [18, "Alt"],
    [20, "CapsLock"],
    [27, "Esc"],
    [32, "Space"],
    [33, "PageUp"],
    [34, "PageDown"],
    [35, "End"],
    [36, "Home"],
    [37, "Left"],
    [38, "Up"],
    [39, "Right"],
    [40, "Down"],
    [45, "Insert"],
    [46, "Delete"]
]);

let pendingHotkeyField = null;

const hotkeyButtons = {
    aimHotkey: () => controls.aimHotkeyBtn,
    aimHotkey2: () => controls.aimHotkey2Btn
};

const hotkeyFieldLabels = {
    aimHotkey: "primary",
    aimHotkey2: "secondary"
};

const settingsPages = {
    home: {
        title: "功能首页",
        hint: "公共配置 / 功能开关"
    },
    mouse: {
        title: "鼠标与防抖",
        hint: "热键、移动参数、输入后端"
    },
    target: {
        title: "自动选择部位",
        hint: "锁定部位优先级"
    },
    humanSlide: {
        title: "仿人类滑动",
        hint: "分段、随机抖动、移动延迟"
    },
    prediction: {
        title: "预瞄",
        hint: "线性、Alpha-Beta、卡尔曼参数"
    },
    droneTracking: {
        title: "仿无人机追踪",
        hint: "视觉伺服、速度前馈、单帧限幅"
    },
    autoClick: {
        title: "自动扳机",
        hint: "按下延迟、点击间隔、到位容差"
    },
    dependencies: {
        title: "依赖设置",
        hint: "本机配置、缺失环境、下载入口"
    }
};

function hotkeyLabel(vk) {
    const code = Number(vk) || 0;
    if (hotkeyLabels.has(code)) {
        return hotkeyLabels.get(code);
    }
    if (code >= 48 && code <= 57) {
        return String.fromCharCode(code);
    }
    if (code >= 65 && code <= 90) {
        return String.fromCharCode(code);
    }
    if (code >= 96 && code <= 105) {
        return `Num ${code - 96}`;
    }
    if (code >= 112 && code <= 135) {
        return `F${code - 111}`;
    }
    return `VK ${code}`;
}

function enemyCampTargetLabel(camp) {
    if (camp === "ct") {
        return "T-body / T-head";
    }
    if (camp === "t") {
        return "CT-body / CT-head";
    }
    return "ALL";
}

function detectionPartTargetLabel(part) {
    if (part === "head") {
        return "HEAD";
    }
    if (part === "body") {
        return "BODY";
    }
    return "ALL";
}

function normalizeAutoStopMode(mode) {
    return ["counter_tap", "ad_pair", "crouch"].includes(mode) ? mode : defaults.autoStopMode;
}

function autoStopModeLabel(mode) {
    if (mode === "ad_pair") {
        return "AD 同按";
    }
    if (mode === "crouch") {
        return "见面蹲";
    }
    return "反向轻点";
}

function normalizeAimMode(mode) {
    return ["atan", "linear"].includes(mode) ? mode : defaults.aimMode;
}

function sanitizeConfig(config) {
    const incomingVersion = Number(config?.configVersion) || 0;
    const merged = { ...defaults, ...config };
    merged.configVersion = defaults.configVersion;
    if (incomingVersion < defaults.configVersion && merged.modelPath === "cs2yolomaax.onnx") {
        merged.modelPath = "";
    }
    merged.aimMode = normalizeAimMode(merged.aimMode);
    merged.autoStopMode = normalizeAutoStopMode(merged.autoStopMode);
    merged.dependencyPaths = {
        ...(defaults.dependencyPaths || {}),
        ...((config && typeof config.dependencyPaths === "object" && !Array.isArray(config.dependencyPaths))
            ? config.dependencyPaths
            : {})
    };
    return Object.fromEntries(
        Object.entries(merged).filter(([key]) => Object.prototype.hasOwnProperty.call(defaults, key))
    );
}

function setView(view) {
    const nextView = Object.prototype.hasOwnProperty.call(settingsPages, view) ? view : "home";
    const page = settingsPages[nextView];
    state.view = nextView;
    root.dataset.view = nextView;
    root.dataset.detail = String(nextView !== "home");
    controls.pageTitle.textContent = page.title;
    controls.pageHint.textContent = page.hint;
    controls.pageNav.hidden = nextView === "home";
    updateLogVisibility();
}

function updateLogVisibility() {
    controls.logPanel.hidden = !state.config.enableConsoleStats || state.view !== "home";
}

function openSettingsPage(page) {
    setView(page);
}

function predictionModeLabel(mode) {
    if (mode === "arc") {
        return "ARC";
    }
    if (mode === "hybrid") {
        return "HYBRID";
    }
    if (mode === "adaptive") {
        return "ADAPTIVE";
    }
    if (mode === "servo") {
        return "SERVO";
    }
    if (mode === "alphabeta") {
        return "ALPHA-BETA";
    }
    if (mode === "kalman") {
        return "KALMAN";
    }
    return "LINEAR";
}

function keyboardEventToVk(event) {
    if (["Escape", "Backspace", "Delete"].includes(event.code)) {
        return 0;
    }
    if (/^Key[A-Z]$/.test(event.code)) {
        return event.code.charCodeAt(3);
    }
    if (/^Digit[0-9]$/.test(event.code)) {
        return event.code.charCodeAt(5);
    }
    if (/^Numpad[0-9]$/.test(event.code)) {
        return 96 + Number(event.code.slice(6));
    }
    if (/^F([1-9]|1[0-9]|2[0-4])$/.test(event.code)) {
        return 111 + Number(event.code.slice(1));
    }
    const named = {
        Space: 32,
        Enter: 13,
        Tab: 9,
        ShiftLeft: 16,
        ShiftRight: 16,
        ControlLeft: 17,
        ControlRight: 17,
        AltLeft: 18,
        AltRight: 18,
        CapsLock: 20,
        PageUp: 33,
        PageDown: 34,
        End: 35,
        Home: 36,
        ArrowLeft: 37,
        ArrowUp: 38,
        ArrowRight: 39,
        ArrowDown: 40,
        Insert: 45
    };
    return named[event.code] ?? null;
}

function mouseEventToVk(event) {
    if (event.button === 0) return 1;
    if (event.button === 1) return 4;
    if (event.button === 2) return 2;
    if (event.button === 3) return 5;
    if (event.button === 4) return 6;
    return null;
}

function beginHotkeyCapture(field) {
    endHotkeyCapture(false);
    pendingHotkeyField = field;
    const button = hotkeyButtons[field]?.();
    if (!button) {
        pendingHotkeyField = null;
        return;
    }
    button.textContent = "等待输入...";
    button.classList.add("recording");
    setTimeout(() => {
        window.addEventListener("keydown", captureHotkeyKey, true);
        window.addEventListener("mousedown", captureHotkeyMouse, true);
        window.addEventListener("contextmenu", blockHotkeyContextMenu, true);
    }, 0);
}

function endHotkeyCapture(refresh = true) {
    window.removeEventListener("keydown", captureHotkeyKey, true);
    window.removeEventListener("mousedown", captureHotkeyMouse, true);
    window.removeEventListener("contextmenu", blockHotkeyContextMenu, true);
    pendingHotkeyField = null;
    Object.values(hotkeyButtons).forEach((buttonFactory) => buttonFactory()?.classList.remove("recording"));
    if (refresh) {
        updateLabels();
    }
}

function commitHotkey(vk) {
    if (!pendingHotkeyField) {
        return;
    }
    const field = pendingHotkeyField;
    syncConfigFromUi();
    state.config[field] = Number(vk) || 0;
    endHotkeyCapture(false);
    updateLabels();
    setDirty(false);
    localStorage.setItem("offline-yolo-switchboard", JSON.stringify(state.config));
    bridge.send("ui:updateConfig", state.config);
    logEvent(`${hotkeyFieldLabels[field] || "hotkey"} hotkey: ${hotkeyLabel(vk)}`);
}

function captureHotkeyKey(event) {
    const vk = keyboardEventToVk(event);
    if (vk === null) {
        return;
    }
    event.preventDefault();
    event.stopPropagation();
    commitHotkey(vk);
}

function captureHotkeyMouse(event) {
    const vk = mouseEventToVk(event);
    if (vk === null) {
        return;
    }
    event.preventDefault();
    event.stopPropagation();
    commitHotkey(vk);
}

function blockHotkeyContextMenu(event) {
    if (!pendingHotkeyField) {
        return;
    }
    event.preventDefault();
    event.stopPropagation();
}

function loadConfig() {
    const saved = localStorage.getItem("offline-yolo-switchboard");
    if (!saved) {
        return;
    }
    try {
        const parsed = JSON.parse(saved);
        state.config = { ...defaults, ...parsed };
        if ((Number(parsed.configVersion) || 0) < defaults.configVersion) {
            state.config.enableConsoleStats = defaults.enableConsoleStats;
            state.config.configVersion = defaults.configVersion;
        }
        state.config = sanitizeConfig(state.config);
        localStorage.setItem("offline-yolo-switchboard", JSON.stringify(state.config));
    } catch {
        localStorage.removeItem("offline-yolo-switchboard");
    }
}

function applyHostConfig(config) {
    if (!config || typeof config !== "object" || Array.isArray(config)) {
        return;
    }
    state.config = sanitizeConfig(config);
    syncUiFromConfig();
    localStorage.setItem("offline-yolo-switchboard", JSON.stringify(state.config));
    setDirty(false);
    bridge.send("ui:updateConfig", state.config);
    logEvent("saved config restored");
    requestDependencyCheck();
}

function updateDependencyStatus(status) {
    const normalized = status && typeof status === "object" ? status : {};
    state.dependencies = {
        ok: Boolean(normalized.ok),
        missingCount: Number(normalized.missingCount) || 0,
        system: normalized.system && typeof normalized.system === "object" ? normalized.system : null,
        downloads: Array.isArray(normalized.downloads) ? normalized.downloads : [],
        items: Array.isArray(normalized.items) ? normalized.items : []
    };
    renderDependencyBadge();
    renderDependencySystem();
    renderDependencyDownloads();
    renderDependencyList();
}

function renderDependencyBadge(checking = false) {
    if (checking) {
        controls.dependencyState.textContent = "检测依赖";
        controls.dependencyState.dataset.state = "checking";
        controls.dependencySummary.textContent = "CHECKING";
        return;
    }

    const ok = Boolean(state.dependencies.ok);
    controls.dependencyState.textContent = ok ? "环境正常" : "设置依赖";
    controls.dependencyState.dataset.state = ok ? "ok" : "missing";
    controls.dependencySummary.textContent = ok
        ? "OK"
        : `${state.dependencies.missingCount || 0} MISSING`;
}

function dependencyPathText(item) {
    if (item.path) {
        return item.path;
    }
    if (item.expected) {
        return `期望: ${item.expected}`;
    }
    return "未找到可用路径";
}

function dependencyDirectDownloadKey(item) {
    const url = String(item?.downloadUrl || "").toLowerCase();
    if (url.includes("github.com/microsoft/onnxruntime")) {
        return "onnxruntime";
    }
    if (url.includes("developer.nvidia.com/cuda-toolkit")) {
        return "cuda";
    }
    if (url.includes("developer.nvidia.com/cudnn")) {
        return "cudnn";
    }
    if (url.includes("opencv.org")) {
        return "opencv";
    }
    return "";
}

function valueOrUnknown(value) {
    return value ? String(value) : "未知";
}

function renderDependencySystem() {
    controls.dependencySystem.replaceChildren();
    const system = state.dependencies.system;
    if (!system) {
        controls.dependencySystem.hidden = true;
        return;
    }

    controls.dependencySystem.hidden = false;
    const card = document.createElement("article");
    card.className = "dependency-system-card";

    const header = document.createElement("div");
    header.className = "dependency-system-header";
    const title = document.createElement("strong");
    title.textContent = "本机配置";
    const source = document.createElement("small");
    source.textContent = system.source ? `来源: ${system.source}` : "来源: 未知";
    header.append(title, source);

    const grid = document.createElement("div");
    grid.className = "dependency-system-grid";
    [
        ["GPU", valueOrUnknown(system.gpuName)],
        ["驱动", valueOrUnknown(system.driverVersion)],
        ["CUDA", valueOrUnknown(system.cudaVersion)],
        ["系统", valueOrUnknown(system.osDescription)],
        ["架构", valueOrUnknown(system.architecture)]
    ].forEach(([label, value]) => {
        const item = document.createElement("span");
        const key = document.createElement("b");
        key.textContent = label;
        const text = document.createElement("em");
        text.textContent = value;
        item.append(key, text);
        grid.append(item);
    });

    card.append(header, grid);
    if (Array.isArray(system.notes) && system.notes.length > 0) {
        const notes = document.createElement("ul");
        notes.className = "dependency-notes";
        system.notes.forEach((note) => {
            const li = document.createElement("li");
            li.textContent = note;
            notes.append(li);
        });
        card.append(notes);
    }

    controls.dependencySystem.append(card);
}

function renderDependencyDownloads() {
    controls.dependencyDownloads.replaceChildren();
    const downloads = (state.dependencies.downloads || []).filter((item) => item && item.url);
    if (downloads.length === 0) {
        controls.dependencyDownloads.hidden = true;
        return;
    }

    controls.dependencyDownloads.hidden = false;
    const header = document.createElement("div");
    header.className = "dependency-download-header";
    const copy = document.createElement("div");
    const title = document.createElement("strong");
    title.textContent = "推荐下载页";
    const detail = document.createElement("small");
    const downloadableCount = downloads.filter((item) => item.canDownload).length;
    detail.textContent = downloadableCount > 0
        ? `可直接下载 ${downloadableCount} 项，其余需要官网登录或手动选择`
        : "这些项目需要官网登录或手动选择";
    copy.append(title, detail);

    const downloadAll = document.createElement("button");
    downloadAll.className = "command compact";
    downloadAll.type = "button";
    downloadAll.textContent = "下载可获取项";
    downloadAll.dataset.dependencyDownloadAll = "true";
    downloadAll.disabled = downloadableCount === 0;

    const openAll = document.createElement("button");
    openAll.className = "command compact";
    openAll.type = "button";
    openAll.textContent = "打开推荐页";
    openAll.dataset.dependencyOpenRecommended = "true";
    const headerActions = document.createElement("div");
    headerActions.className = "dependency-download-header-actions";
    headerActions.append(downloadAll, openAll);
    header.append(copy, headerActions);
    controls.dependencyDownloads.append(header);

    const list = document.createElement("div");
    list.className = "dependency-download-list";
    downloads.forEach((item) => {
        const card = document.createElement("article");
        card.className = "dependency-download-card";
        card.dataset.downloadable = String(Boolean(item.canDownload));

        const body = document.createElement("div");
        body.className = "dependency-download-body";
        const name = document.createElement("strong");
        name.textContent = item.title || item.key || "下载页";
        const desc = document.createElement("small");
        desc.textContent = item.detail || item.url;
        body.append(name, desc);

        const actions = document.createElement("div");
        actions.className = "dependency-download-actions";
        if (item.canDownload) {
            const download = document.createElement("button");
            download.className = "command compact";
            download.type = "button";
            download.textContent = "下载/继续";
            download.dataset.dependencyDownload = item.key || "";
            actions.append(download);
        }
        const open = document.createElement("button");
        open.className = "command compact";
        open.type = "button";
        open.textContent = "打开官网";
        open.dataset.dependencyUrl = item.url;
        actions.append(open);

        const progress = renderDownloadProgress(item);
        card.append(body, actions, progress);
        list.append(card);
    });
    controls.dependencyDownloads.append(list);
}

function formatBytes(bytes) {
    const value = Number(bytes) || 0;
    if (value >= 1024 * 1024 * 1024) {
        return `${(value / 1024 / 1024 / 1024).toFixed(2)} GB`;
    }
    if (value >= 1024 * 1024) {
        return `${(value / 1024 / 1024).toFixed(1)} MB`;
    }
    if (value >= 1024) {
        return `${(value / 1024).toFixed(1)} KB`;
    }
    return `${value.toFixed(0)} B`;
}

function renderDownloadProgress(item) {
    const progress = state.downloadProgress[item.key] || null;
    const wrap = document.createElement("div");
    wrap.className = "dependency-progress";
    if (!progress) {
        wrap.hidden = true;
        return wrap;
    }

    const percent = Math.max(0, Math.min(100, Number(progress.percent) || 0));
    wrap.dataset.status = progress.status || "idle";
    const meta = document.createElement("div");
    meta.className = "dependency-progress-meta";
    const status = document.createElement("span");
    status.textContent = progress.message || progress.status || "等待下载";
    const bytes = document.createElement("b");
    const received = formatBytes(progress.receivedBytes);
    const total = Number(progress.totalBytes) > 0 ? formatBytes(progress.totalBytes) : "未知大小";
    bytes.textContent = `${percent.toFixed(1)}% · ${received} / ${total}`;
    meta.append(status, bytes);

    const bar = document.createElement("div");
    bar.className = "dependency-progress-track";
    const fill = document.createElement("span");
    fill.className = "dependency-progress-fill";
    fill.style.width = `${percent}%`;
    bar.append(fill);

    wrap.append(meta, bar);
    return wrap;
}

function renderDependencyList() {
    controls.dependencyList.replaceChildren();
    const items = state.dependencies.items || [];
    const displayItems = items.filter((item) => item && item.required !== false);
    const missingItems = displayItems.filter((item) => !item.ok);

    if (state.dependencies.ok || missingItems.length === 0) {
        const empty = document.createElement("div");
        empty.className = "dependency-empty";
        empty.textContent = "环境正常";
        controls.dependencyList.append(empty);
        return;
    }

    const orderedItems = [
        ...missingItems,
        ...displayItems.filter((item) => item.ok)
    ];

    orderedItems.forEach((item) => {
        const card = document.createElement("article");
        card.className = "dependency-card";
        card.dataset.ok = String(Boolean(item.ok));

        const copy = document.createElement("div");
        copy.className = "dependency-copy";
        const title = document.createElement("strong");
        title.textContent = item.title || item.key || "缺失依赖";
        const detail = document.createElement("small");
        detail.textContent = item.detail || item.description || "需要手动配置路径";
        copy.append(title, detail);

        const path = document.createElement("div");
        path.className = "dependency-path";
        path.title = dependencyPathText(item);
        path.textContent = dependencyPathText(item);

        const actions = document.createElement("div");
        actions.className = "dependency-actions";
        const status = document.createElement("span");
        status.className = "dependency-status";
        status.textContent = item.ok ? "正常" : "缺失";
        actions.append(status);

        if (!item.ok) {
            if ((item.kind || "folder") !== "none") {
                const choose = document.createElement("button");
                choose.className = "command compact";
                choose.type = "button";
                choose.textContent = "选择位置";
                choose.dataset.dependencyChoose = item.key || "";
                choose.dataset.dependencyKind = item.kind || "folder";
                actions.append(choose);
            }
            if (item.downloadUrl) {
                const download = document.createElement("button");
                download.className = "command compact";
                download.type = "button";
                const directKey = dependencyDirectDownloadKey(item);
                if (directKey) {
                    download.textContent = "下载修复";
                    download.dataset.dependencyDownload = directKey;
                } else {
                    download.textContent = "打开官网";
                    download.dataset.dependencyUrl = item.downloadUrl;
                }
                actions.append(download);
            }
        }

        card.append(copy, path, actions);
        controls.dependencyList.append(card);
    });
}

function requestDependencyCheck() {
    syncConfigFromUi();
    state.config = sanitizeConfig(state.config);
    localStorage.setItem("offline-yolo-switchboard", JSON.stringify(state.config));
    renderDependencyBadge(true);
    bridge.send("ui:checkDependencies", state.config);
}

function commitDependencyPath(key, path) {
    if (!key || !path) {
        return;
    }
    state.config.dependencyPaths = {
        ...(state.config.dependencyPaths || {}),
        [key]: path
    };
    if (key === "modelFile") {
        state.config.modelPath = path;
        controls.modelPath.value = path;
    }
    if (key === "driverDll") {
        state.config.driverDllPath = path;
        controls.driverDllPath.value = path;
    }
    syncUiFromConfig();
    saveConfig({ notifyHost: false });
    bridge.send("ui:updateConfig", state.config);
    requestDependencyCheck();
}

function openDependencyUrl(url) {
    if (!url) {
        return;
    }
    if (bridge.available()) {
        bridge.send("ui:openUrl", { url });
    } else {
        window.open(url, "_blank", "noopener");
    }
}

function openDependencyUrls(urls) {
    const uniqueUrls = Array.from(new Set((urls || []).filter(Boolean)));
    if (uniqueUrls.length === 0) {
        return;
    }
    if (bridge.available()) {
        bridge.send("ui:openUrls", { urls: uniqueUrls });
    } else {
        uniqueUrls.forEach((url) => window.open(url, "_blank", "noopener"));
    }
}

function downloadDependency(key) {
    if (!key) {
        return;
    }
    if (bridge.available()) {
        bridge.send("ui:downloadDependency", { key });
    } else {
        logEvent(`preview download:${key}`);
    }
}

function downloadDependencies(keys) {
    const uniqueKeys = Array.from(new Set((keys || []).filter(Boolean)));
    if (uniqueKeys.length === 0) {
        return;
    }
    if (bridge.available()) {
        bridge.send("ui:downloadDependencies", { keys: uniqueKeys });
    } else {
        logEvent(`preview downloads:${uniqueKeys.join(",")}`);
    }
}

function saveConfig({ notifyHost = true } = {}) {
    syncConfigFromUi();
    localStorage.setItem("offline-yolo-switchboard", JSON.stringify(state.config));
    setDirty(false);
    if (notifyHost) {
        bridge.send("ui:saveConfig", state.config);
        logEvent("switches saved");
    }
}

function resetConfig() {
    state.config = { ...defaults };
    syncUiFromConfig();
    saveConfig();
    bridge.send("ui:updateConfig", state.config);
    logEvent("defaults restored");
}

function resetFilterConfig() {
    syncConfigFromUi();
    state.config = { ...state.config, ...filterDefaults };
    syncUiFromConfig();
    saveConfig({ notifyHost: false });
    bridge.send("ui:updateConfig", state.config);
    logEvent("filter defaults restored");
}

function clampNumber(value, min, max, fallback) {
    const numeric = Number(value);
    if (!Number.isFinite(numeric)) {
        return fallback;
    }
    return Math.min(max, Math.max(min, numeric));
}

function stepDecimals(input) {
    const step = String(input.step || "1");
    const dot = step.indexOf(".");
    return dot === -1 ? 0 : step.length - dot - 1;
}

function formatForInput(input, value) {
    return Number(value).toFixed(stepDecimals(input));
}

function normalizeRangePair(minInput, maxInput, changedSide = "min") {
    const minLimit = Number(minInput.min);
    const maxLimit = Number(minInput.max);
    let minValue = clampNumber(minInput.value, minLimit, maxLimit, Number(minInput.value));
    let maxValue = clampNumber(maxInput.value, minLimit, maxLimit, Number(maxInput.value));

    if (minValue > maxValue) {
        if (changedSide === "max") {
            minValue = maxValue;
        } else {
            maxValue = minValue;
        }
    }

    minInput.value = formatForInput(minInput, minValue);
    maxInput.value = formatForInput(maxInput, maxValue);
    updateDualRangeFill(minInput, maxInput);
    return { minValue, maxValue };
}

function updateDualRangeFill(minInput, maxInput) {
    const range = minInput.closest(".dual-range");
    if (!range) {
        return;
    }
    const minLimit = Number(minInput.min);
    const maxLimit = Number(minInput.max);
    const span = Math.max(1, maxLimit - minLimit);
    const minValue = Number(minInput.value);
    const maxValue = Number(maxInput.value);
    const start = ((minValue - minLimit) / span) * 100;
    const end = ((maxValue - minLimit) / span) * 100;
    range.style.setProperty("--range-start", `${Math.min(100, Math.max(0, start))}%`);
    range.style.setProperty("--range-end", `${Math.min(100, Math.max(0, end))}%`);
}

function editRangeEndpoint(minInput, maxInput, side, label) {
    const target = side === "max" ? maxInput : minInput;
    const raw = window.prompt(label, target.value);
    if (raw === null) {
        return;
    }
    const value = raw.trim().replace(",", ".");
    const numeric = Number(value);
    if (!Number.isFinite(numeric)) {
        logEvent("invalid range value");
        return;
    }
    target.value = formatForInput(target, clampNumber(numeric, Number(target.min), Number(target.max), Number(target.value)));
    normalizeRangePair(minInput, maxInput, side);
    pushConfigUpdate();
}

function editSingleRangeValue(input, label) {
    const raw = window.prompt(label, input.value);
    if (raw === null) {
        return;
    }
    const value = raw.trim().replace(",", ".");
    const numeric = Number(value);
    if (!Number.isFinite(numeric)) {
        logEvent("invalid numeric value");
        return;
    }
    input.value = formatForInput(input, clampNumber(numeric, Number(input.min), Number(input.max), Number(input.value)));
    pushConfigUpdate();
}

function bindRangePairEvents(minInput, maxInput, minButton, maxButton, label) {
    const minHandler = () => {
        normalizeRangePair(minInput, maxInput, "min");
        pushConfigUpdate();
    };
    const maxHandler = () => {
        normalizeRangePair(minInput, maxInput, "max");
        pushConfigUpdate();
    };
    minInput.addEventListener("input", minHandler);
    minInput.addEventListener("change", minHandler);
    maxInput.addEventListener("input", maxHandler);
    maxInput.addEventListener("change", maxHandler);
    minButton.addEventListener("click", () => editRangeEndpoint(minInput, maxInput, "min", `${label}最小值`));
    maxButton.addEventListener("click", () => editRangeEndpoint(minInput, maxInput, "max", `${label}最大值`));
}

function syncUiFromConfig() {
    state.config = sanitizeConfig(state.config);
    controls.modelPath.value = state.config.modelPath;
    controls.inputBackend.value = state.config.inputBackend;
    controls.driverDllPath.value = state.config.driverDllPath;
    controls.driverType.value = String(state.config.driverType);
    controls.aimHotkeyBtn.textContent = hotkeyLabel(state.config.aimHotkey);
    controls.aimHotkey2Btn.textContent = hotkeyLabel(state.config.aimHotkey2);
    controls.humanSlideMaxStep.value = state.config.humanSlideMaxStep;
    controls.humanSlideJitter.value = state.config.humanSlideJitter;
    controls.humanSlideDelayMin.value = state.config.humanSlideDelayMin;
    controls.humanSlideDelayMax.value = state.config.humanSlideDelayMax;
    controls.autoClickDelayMin.value = state.config.autoClickDelayMin;
    controls.autoClickDelayMax.value = state.config.autoClickDelayMax;
    controls.autoClickIntervalMin.value = state.config.autoClickIntervalMin;
    controls.autoClickIntervalMax.value = state.config.autoClickIntervalMax;
    controls.autoClickTolerance.value = state.config.autoClickTolerance;
    controls.enableAutoStop.checked = Boolean(state.config.enableAutoStop);
    controls.autoStopMode.value = normalizeAutoStopMode(state.config.autoStopMode);
    controls.autoStopHoldMs.value = state.config.autoStopHoldMs;
    controls.autoStopSettleMs.value = state.config.autoStopSettleMs;
    controls.aimFilter.value = state.config.aimFilter;
    controls.pidKp.value = state.config.pidKp;
    controls.pidKi.value = state.config.pidKi;
    controls.pidKd.value = state.config.pidKd;
    controls.pidIntegralLimit.value = state.config.pidIntegralLimit;
    controls.oneEuroMinCutoff.value = state.config.oneEuroMinCutoff;
    controls.oneEuroBeta.value = state.config.oneEuroBeta;
    controls.oneEuroDCutoff.value = state.config.oneEuroDCutoff;
    controls.predictionMode.value = ["linear", "arc", "hybrid", "adaptive", "servo", "alphabeta", "kalman"].includes(state.config.predictionMode)
        ? state.config.predictionMode
        : "linear";
    controls.predictionLeadMs.value = state.config.predictionLeadMs;
    controls.predictionSmoothing.value = state.config.predictionSmoothing;
    controls.predictionAccelerationSmoothing.value = state.config.predictionAccelerationSmoothing;
    controls.predictionAlpha.value = state.config.predictionAlpha;
    controls.predictionBeta.value = state.config.predictionBeta;
    controls.predictionKalmanMeasurementNoise.value = state.config.predictionKalmanMeasurementNoise;
    controls.predictionKalmanProcessNoise.value = state.config.predictionKalmanProcessNoise;
    controls.predictionMaxPixels.value = state.config.predictionMaxPixels;
    controls.predictionResetPixels.value = state.config.predictionResetPixels;
    controls.predictionNoisePixels.value = state.config.predictionNoisePixels;
    controls.predictionOutputSmoothing.value = state.config.predictionOutputSmoothing;
    controls.predictionServoGain.value = state.config.predictionServoGain;
    controls.droneTrackGain.value = state.config.droneTrackGain;
    controls.droneTrackVelocityGain.value = state.config.droneTrackVelocityGain;
    controls.droneTrackDamping.value = state.config.droneTrackDamping;
    controls.droneTrackSmoothing.value = state.config.droneTrackSmoothing;
    controls.droneTrackMaxMove.value = state.config.droneTrackMaxMove;
    controls.droneTrackDeadzone.value = state.config.droneTrackDeadzone;
    controls.cropSize.value = state.config.cropSize;
    controls.lockRadius.value = state.config.lockRadius;
    controls.confidence.value = state.config.confidence;
    controls.smoothing.value = state.config.smoothing;
    controls.aimMode.value = normalizeAimMode(state.config.aimMode);
    controls.aimGain.value = state.config.aimGain;
    controls.deadzone.value = state.config.deadzone;
    controls.targetX.value = state.config.targetX;
    controls.targetY.value = state.config.targetY;
    controls.aimPartPriority.value = state.config.aimPartPriority;
    controls.enemyCamp.value = state.config.enemyCamp;
    controls.detectionPart.value = state.config.detectionPart;
    controls.maxMove.value = state.config.maxMove;
    controls.trackingBoostThreshold.value = state.config.trackingBoostThreshold;
    controls.trackingBoostGain.value = state.config.trackingBoostGain;
    controls.trackingBoostMaxMove.value = state.config.trackingBoostMaxMove;

    Object.entries(controls.switches).forEach(([key, input]) => {
        input.checked = Boolean(state.config[key]);
    });
    enforceDetectionPartCompatibility();

    $$("[data-backend]").forEach((button) => {
        button.classList.toggle("active", button.dataset.backend === state.config.backend);
    });

    updateLabels();
}

function ensureSwitchStateBadges() {
    Object.entries(controls.switches).forEach(([key, input]) => {
        const card = input.closest(".switch-card");
        if (!card || card.querySelector(".switch-state")) {
            return;
        }
        const badge = document.createElement("span");
        badge.className = "switch-state";
        badge.dataset.stateFor = key;
        badge.setAttribute("aria-hidden", "true");
        card.append(badge);
    });
}

function enforceDetectionPartCompatibility() {
    const bodyOnly = state.config.detectionPart === "body";
    if (bodyOnly && state.config.enableAutoAimPart) {
        state.config.enableAutoAimPart = false;
    }

    const autoPartSwitch = controls.switches.enableAutoAimPart;
    autoPartSwitch.checked = Boolean(state.config.enableAutoAimPart);
    autoPartSwitch.disabled = bodyOnly;
    autoPartSwitch.closest(".switch-card")?.classList.toggle("locked", bodyOnly);
}

function syncConfigFromUi() {
    state.config.modelPath = controls.modelPath.value.trim();
    state.config.inputBackend = controls.inputBackend.value;
    state.config.driverDllPath = controls.driverDllPath.value.trim();
    state.config.driverType = Number(controls.driverType.value);
    state.config.aimHotkey = Number.isFinite(Number(state.config.aimHotkey)) ? Number(state.config.aimHotkey) : defaults.aimHotkey;
    state.config.aimHotkey2 = Number.isFinite(Number(state.config.aimHotkey2)) ? Number(state.config.aimHotkey2) : defaults.aimHotkey2;
    state.config.humanSlideMaxStep = Number(controls.humanSlideMaxStep.value);
    state.config.humanSlideJitter = Number(controls.humanSlideJitter.value);
    state.config.humanSlideDelayMin = Number(controls.humanSlideDelayMin.value);
    state.config.humanSlideDelayMax = Number(controls.humanSlideDelayMax.value);
    if (state.config.humanSlideDelayMin > state.config.humanSlideDelayMax) {
        [state.config.humanSlideDelayMin, state.config.humanSlideDelayMax] = [
            state.config.humanSlideDelayMax,
            state.config.humanSlideDelayMin
        ];
        controls.humanSlideDelayMin.value = state.config.humanSlideDelayMin;
        controls.humanSlideDelayMax.value = state.config.humanSlideDelayMax;
    }
    const clickDelayRange = normalizeRangePair(controls.autoClickDelayMin, controls.autoClickDelayMax);
    state.config.autoClickDelayMin = Math.round(clickDelayRange.minValue);
    state.config.autoClickDelayMax = Math.round(clickDelayRange.maxValue);
    const clickIntervalRange = normalizeRangePair(controls.autoClickIntervalMin, controls.autoClickIntervalMax);
    state.config.autoClickIntervalMin = Math.round(clickIntervalRange.minValue);
    state.config.autoClickIntervalMax = Math.round(clickIntervalRange.maxValue);
    state.config.autoClickTolerance = Number(controls.autoClickTolerance.value);
    state.config.enableAutoStop = Boolean(controls.enableAutoStop.checked);
    state.config.autoStopMode = normalizeAutoStopMode(controls.autoStopMode.value);
    state.config.autoStopHoldMs = Number(controls.autoStopHoldMs.value);
    state.config.autoStopSettleMs = Number(controls.autoStopSettleMs.value);
    state.config.aimFilter = controls.aimFilter.value;
    state.config.pidKp = Number(controls.pidKp.value);
    state.config.pidKi = Number(controls.pidKi.value);
    state.config.pidKd = Number(controls.pidKd.value);
    state.config.pidIntegralLimit = Number(controls.pidIntegralLimit.value);
    state.config.oneEuroMinCutoff = Number(controls.oneEuroMinCutoff.value);
    state.config.oneEuroBeta = Number(controls.oneEuroBeta.value);
    state.config.oneEuroDCutoff = Number(controls.oneEuroDCutoff.value);
    state.config.predictionMode = controls.predictionMode.value;
    state.config.predictionLeadMs = Number(controls.predictionLeadMs.value);
    state.config.predictionSmoothing = Number(controls.predictionSmoothing.value);
    state.config.predictionAccelerationSmoothing = Number(controls.predictionAccelerationSmoothing.value);
    state.config.predictionAlpha = Number(controls.predictionAlpha.value);
    state.config.predictionBeta = Number(controls.predictionBeta.value);
    state.config.predictionKalmanMeasurementNoise = Number(controls.predictionKalmanMeasurementNoise.value);
    state.config.predictionKalmanProcessNoise = Number(controls.predictionKalmanProcessNoise.value);
    state.config.predictionMaxPixels = Number(controls.predictionMaxPixels.value);
    state.config.predictionResetPixels = Number(controls.predictionResetPixels.value);
    state.config.predictionNoisePixels = Number(controls.predictionNoisePixels.value);
    state.config.predictionOutputSmoothing = Number(controls.predictionOutputSmoothing.value);
    state.config.predictionServoGain = Number(controls.predictionServoGain.value);
    state.config.droneTrackGain = Number(controls.droneTrackGain.value);
    state.config.droneTrackVelocityGain = Number(controls.droneTrackVelocityGain.value);
    state.config.droneTrackDamping = Number(controls.droneTrackDamping.value);
    state.config.droneTrackSmoothing = Number(controls.droneTrackSmoothing.value);
    state.config.droneTrackMaxMove = Number(controls.droneTrackMaxMove.value);
    state.config.droneTrackDeadzone = Number(controls.droneTrackDeadzone.value);
    state.config.cropSize = Number(controls.cropSize.value);
    state.config.lockRadius = Number(controls.lockRadius.value);
    state.config.confidence = Number(controls.confidence.value);
    state.config.smoothing = Number(controls.smoothing.value);
    state.config.aimMode = normalizeAimMode(controls.aimMode.value);
    state.config.aimGain = Number(controls.aimGain.value);
    state.config.deadzone = Number(controls.deadzone.value);
    state.config.targetX = Number(controls.targetX.value);
    state.config.targetY = Number(controls.targetY.value);
    state.config.aimPartPriority = controls.aimPartPriority.value;
    state.config.enemyCamp = controls.enemyCamp.value;
    state.config.detectionPart = controls.detectionPart.value;
    state.config.maxMove = Number(controls.maxMove.value);
    state.config.trackingBoostThreshold = Number(controls.trackingBoostThreshold.value);
    state.config.trackingBoostGain = Number(controls.trackingBoostGain.value);
    state.config.trackingBoostMaxMove = Number(controls.trackingBoostMaxMove.value);
    Object.entries(controls.switches).forEach(([key, input]) => {
        state.config[key] = input.checked;
    });
    enforceDetectionPartCompatibility();
}

function updateLabels() {
    controls.cropValue.textContent = state.config.cropSize;
    controls.radiusValue.textContent = state.config.lockRadius;
    controls.confidenceValue.textContent = state.config.confidence.toFixed(2);
    controls.smoothValue.textContent = state.config.smoothing.toFixed(2);
    controls.aimGainValue.textContent = state.config.aimGain.toFixed(2);
    controls.deadzoneValue.textContent = state.config.deadzone.toFixed(1);
    controls.targetXValue.textContent = state.config.targetX.toFixed(2);
    controls.targetYValue.textContent = state.config.targetY.toFixed(2);
    controls.enemyCampLabel.textContent = enemyCampTargetLabel(state.config.enemyCamp);
    controls.detectionPartLabel.textContent = detectionPartTargetLabel(state.config.detectionPart);
    controls.maxMoveValue.textContent = state.config.maxMove;
    controls.trackingBoostThresholdValue.textContent = state.config.trackingBoostThreshold.toFixed(0);
    controls.trackingBoostGainValue.textContent = state.config.trackingBoostGain.toFixed(2);
    controls.trackingBoostMaxMoveValue.textContent = state.config.trackingBoostMaxMove.toFixed(0);
    controls.trackingBoostFields.hidden = !state.config.enableTrackingBoost;
    controls.filterLabel.textContent = state.config.aimFilter === "pid_oneeuro"
        ? "PID + 1 EURO"
        : state.config.aimFilter === "oneeuro" ? "1 EURO" : state.config.aimFilter.toUpperCase();
    controls.pidKpValue.textContent = state.config.pidKp.toFixed(2);
    controls.pidKiValue.textContent = state.config.pidKi.toFixed(2);
    controls.pidKdValue.textContent = state.config.pidKd.toFixed(2);
    controls.pidIntegralLimitValue.textContent = state.config.pidIntegralLimit.toFixed(0);
    controls.oneEuroMinCutoffValue.textContent = state.config.oneEuroMinCutoff.toFixed(2);
    controls.oneEuroBetaValue.textContent = state.config.oneEuroBeta.toFixed(2);
    controls.oneEuroDCutoffValue.textContent = state.config.oneEuroDCutoff.toFixed(2);
    controls.predictionLeadMsValue.textContent = state.config.predictionLeadMs.toFixed(0);
    controls.predictionSmoothingValue.textContent = state.config.predictionSmoothing.toFixed(2);
    controls.predictionAlphaValue.textContent = state.config.predictionAlpha.toFixed(2);
    controls.predictionBetaValue.textContent = state.config.predictionBeta.toFixed(2);
    controls.predictionKalmanMeasurementNoiseValue.textContent = state.config.predictionKalmanMeasurementNoise.toFixed(1);
    controls.predictionKalmanProcessNoiseValue.textContent = state.config.predictionKalmanProcessNoise.toFixed(1);
    controls.predictionMaxPixelsValue.textContent = state.config.predictionMaxPixels.toFixed(0);
    controls.predictionResetPixelsValue.textContent = state.config.predictionResetPixels.toFixed(0);
    controls.predictionNoisePixelsValue.textContent = state.config.predictionNoisePixels.toFixed(1);
    controls.predictionOutputSmoothingValue.textContent = state.config.predictionOutputSmoothing.toFixed(2);
    controls.predictionServoGainValue.textContent = state.config.predictionServoGain.toFixed(2);
    controls.predictionAccelerationSmoothingValue.textContent = state.config.predictionAccelerationSmoothing.toFixed(2);
    controls.droneTrackGainValue.textContent = state.config.droneTrackGain.toFixed(2);
    controls.droneTrackVelocityGainValue.textContent = state.config.droneTrackVelocityGain.toFixed(2);
    controls.droneTrackDampingValue.textContent = state.config.droneTrackDamping.toFixed(2);
    controls.droneTrackSmoothingValue.textContent = state.config.droneTrackSmoothing.toFixed(2);
    controls.droneTrackMaxMoveValue.textContent = state.config.droneTrackMaxMove.toFixed(0);
    controls.droneTrackDeadzoneValue.textContent = state.config.droneTrackDeadzone.toFixed(1);
    controls.droneTrackingLabel.textContent = state.config.enableDroneTracking
        ? `${state.config.droneTrackGain.toFixed(2)} / ${state.config.droneTrackMaxMove.toFixed(0)}px`
        : "OFF";
    controls.droneTrackingSwitchLabel.textContent = controls.droneTrackingLabel.textContent;
    const predictionModeText = predictionModeLabel(state.config.predictionMode);
    controls.predictionLabel.textContent = state.config.enablePrediction
        ? `${predictionModeText} ${state.config.predictionLeadMs.toFixed(0)}ms`
        : "OFF";
    controls.predictionSwitchLabel.textContent = controls.predictionLabel.textContent;
    controls.predictionSlot.dataset.predictionEnabled = String(state.config.enablePrediction);
    controls.predictionSlot.dataset.predictionMode = ["linear", "arc", "hybrid", "adaptive", "servo", "alphabeta", "kalman"].includes(state.config.predictionMode)
        ? state.config.predictionMode
        : "linear";
    controls.droneTrackingSlot.dataset.droneTrackingEnabled = String(state.config.enableDroneTracking);
    controls.humanSlideMaxStepValue.textContent = state.config.humanSlideMaxStep.toFixed(0);
    controls.humanSlideJitterValue.textContent = state.config.humanSlideJitter.toFixed(1);
    controls.humanSlideDelayMinValue.textContent = state.config.humanSlideDelayMin.toFixed(0);
    controls.humanSlideDelayMaxValue.textContent = state.config.humanSlideDelayMax.toFixed(0);
    controls.autoClickDelayMinValue.textContent = state.config.autoClickDelayMin.toFixed(0);
    controls.autoClickDelayMaxValue.textContent = state.config.autoClickDelayMax.toFixed(0);
    controls.autoClickIntervalMinValue.textContent = state.config.autoClickIntervalMin.toFixed(0);
    controls.autoClickIntervalMaxValue.textContent = state.config.autoClickIntervalMax.toFixed(0);
    controls.autoClickToleranceValue.textContent = state.config.autoClickTolerance.toFixed(1);
    updateDualRangeFill(controls.autoClickDelayMin, controls.autoClickDelayMax);
    updateDualRangeFill(controls.autoClickIntervalMin, controls.autoClickIntervalMax);
    controls.autoClickSlot.dataset.autoStopEnabled = String(state.config.enableAutoStop);
    controls.autoStopModeField.hidden = !state.config.enableAutoStop;
    controls.autoStopHoldField.hidden = !state.config.enableAutoStop;
    controls.autoStopSettleField.hidden = !state.config.enableAutoStop;
    controls.autoStopHoldMsValue.textContent = state.config.autoStopHoldMs.toFixed(0);
    controls.autoStopSettleMsValue.textContent = state.config.autoStopSettleMs.toFixed(0);
    const usesPid = state.config.aimFilter === "pid" || state.config.aimFilter === "pid_oneeuro";
    const usesOneEuro = state.config.aimFilter === "oneeuro" || state.config.aimFilter === "pid_oneeuro";
    controls.filterSlot.dataset.pidVisible = String(usesPid);
    controls.filterSlot.dataset.oneEuroVisible = String(usesOneEuro);
    const radiusPercent = Math.min(100, Math.max(2, (state.config.lockRadius * 2 / state.config.cropSize) * 100));
    controls.rangeCircle.style.setProperty("--range-size", `${radiusPercent}%`);
    controls.rangePreviewLabel.textContent = `${state.config.cropSize} / ${state.config.lockRadius}`;
    controls.rangeCropMetric.textContent = state.config.cropSize;
    controls.rangeRadiusMetric.textContent = state.config.lockRadius;
    const usesAutoAimPart = Boolean(state.config.enableAutoAimPart);
    controls.targetPoint.style.setProperty("--target-x", String(state.config.targetX * 100));
    controls.targetPoint.style.setProperty("--target-y", String(state.config.targetY * 100));
    controls.targetPreviewLabel.textContent = `${state.config.targetX.toFixed(2)} / ${state.config.targetY.toFixed(2)}`;
    controls.targetCard.dataset.autoPart = String(usesAutoAimPart);
    controls.aimPartPriorityCard.dataset.autoPart = String(usesAutoAimPart);
    controls.targetX.disabled = usesAutoAimPart;
    controls.targetY.disabled = usesAutoAimPart;
    controls.aimPartPriority.disabled = !usesAutoAimPart;
    controls.aimHotkeyBtn.textContent = pendingHotkeyField === "aimHotkey" ? "等待输入..." : hotkeyLabel(state.config.aimHotkey);
    controls.aimHotkey2Btn.textContent = pendingHotkeyField === "aimHotkey2" ? "等待输入..." : hotkeyLabel(state.config.aimHotkey2);
    controls.aimHotkeySwitchLabel.textContent = `${hotkeyLabel(state.config.aimHotkey)} / ${hotkeyLabel(state.config.aimHotkey2)}`;
    controls.humanSlideSwitchLabel.textContent = state.config.enableHumanSlide
        ? `${state.config.humanSlideMaxStep.toFixed(0)}px / ${state.config.humanSlideJitter.toFixed(1)}`
        : "OFF";
    controls.humanSlideLabel.textContent = state.config.enableHumanSlide ? "HUMAN" : "DIRECT";
    const autoStopText = state.config.enableAutoStop
        ? `${autoStopModeLabel(state.config.autoStopMode)} ${state.config.autoStopHoldMs}-${state.config.autoStopSettleMs}ms`
        : "无急停";
    controls.autoClickSwitchLabel.textContent = state.config.enableAutoClick
        ? `${state.config.autoClickDelayMin}-${state.config.autoClickDelayMax}ms / ${autoStopText}`
        : "OFF";
    controls.autoClickLabel.textContent = state.config.enableAutoClick
        ? `${state.config.autoClickDelayMin}-${state.config.autoClickDelayMax} / ${state.config.autoClickIntervalMin}-${state.config.autoClickIntervalMax}ms · ${autoStopText}`
        : "OFF";
    controls.backendLabel.textContent = state.config.backend.toUpperCase();
    controls.inputBackendLabel.textContent = state.config.inputBackend === "dd"
        ? "DD"
        : state.config.inputBackend === "driver" ? "DRIVER" : "SendInput";
    controls.driverState.textContent = state.config.inputBackend === "dd" ? "DD"
        : state.config.inputBackend === "driver" ? "DRIVER" : "SENDINPUT";
    updateLogVisibility();
    document.querySelector(".driver-slot").dataset.driverVisible = String(state.config.inputBackend === "driver" || state.config.inputBackend === "dd");
    document.querySelector(".driver-slot").dataset.driverTypeVisible = String(state.config.inputBackend === "driver");
    controls.humanSlideSlot.dataset.humanSlideEnabled = String(state.config.enableHumanSlide);
    controls.autoClickSlot.dataset.autoClickEnabled = String(state.config.enableAutoClick);
    Object.entries(controls.switches).forEach(([key, input]) => {
        const badge = document.querySelector(`.switch-state[data-state-for="${key}"]`);
        if (!badge) {
            return;
        }
        const enabled = Boolean(input.checked);
        badge.textContent = enabled ? "开启" : "未开启";
        badge.dataset.enabled = String(enabled);
    });
}

function setDirty(isDirty) {
    controls.configState.textContent = isDirty ? "DIRTY" : "SAVED";
    controls.configState.style.color = isDirty ? "var(--amber)" : "var(--green)";
}

function setRunning(running, phase = null) {
    state.running = Boolean(running);
    state.backendPhase = phase || (state.running ? "running" : "stopped");
    root.dataset.running = String(state.running);
    controls.runtimeState.textContent = state.backendPhase === "starting"
        ? "STARTING"
        : state.running ? "RUNNING" : "STOPPED";
    controls.runBtn.textContent = state.backendPhase === "starting"
        ? "后端启动中"
        : state.running ? "后端运行中" : "启动后端";
}

function startBackend() {
    syncConfigFromUi();
    if (!state.config.enableCapture) {
        logEvent("screen capture disabled");
    }
    if ((state.config.inputBackend === "driver" || state.config.inputBackend === "dd") && !state.config.driverDllPath) {
        logEvent("driver dll path is empty");
    }
    saveConfig({ notifyHost: false });
    bridge.send("ui:start", state.config);
    logEvent("start requested");
}

function stopBackend() {
    bridge.send("ui:stop");
    logEvent("stop requested");
}

function testInputBackend() {
    syncConfigFromUi();
    saveConfig({ notifyHost: false });
    bridge.send("ui:testInputBackend", state.config);
    logEvent("input backend self-test requested");
}

function pushConfigUpdate() {
    syncConfigFromUi();
    updateLabels();
    localStorage.setItem("offline-yolo-switchboard", JSON.stringify(state.config));
    setDirty(false);
    bridge.send("ui:updateConfig", state.config);
}

function logEvent(text) {
    if (!state.config.enableConsoleStats) {
        return;
    }
    const item = document.createElement("li");
    const time = document.createElement("time");
    time.textContent = new Date().toLocaleTimeString("zh-CN", { hour12: false });
    item.append(time, ` ${text}`);
    eventLog.prepend(item);
    while (eventLog.children.length > 32) {
        eventLog.lastElementChild.remove();
    }
}

function handleHostMessage(event) {
    const message = event.data || {};
    if (message.type === "host:state") {
        setRunning(Boolean(message.payload.running), message.payload.phase || null);
    }
    if (message.type === "host:config") {
        applyHostConfig(message.payload);
    }
    if (message.type === "host:dependencies") {
        updateDependencyStatus(message.payload);
    }
    if (message.type === "host:downloadProgress") {
        const payload = message.payload || {};
        if (payload.key) {
            state.downloadProgress[payload.key] = payload;
            renderDependencyDownloads();
            if (payload.status === "completed" || payload.status === "failed" || payload.status === "unsupported") {
                logEvent(`${payload.title || payload.key}: ${payload.message || payload.status}`);
            }
        }
    }
    if (message.type === "host:dependencyPath") {
        commitDependencyPath(message.payload?.key, message.payload?.path);
    }
    if (message.type === "host:log") {
        logEvent(message.payload.text || "host event");
    }
}

function bindEvents() {
    controls.runBtn.addEventListener("click", startBackend);
    controls.stopBtn.addEventListener("click", stopBackend);
    controls.saveBtn.addEventListener("click", () => saveConfig());
    controls.resetBtn.addEventListener("click", resetConfig);
    controls.backHomeBtn.addEventListener("click", () => openSettingsPage("home"));
    controls.dependencyState.addEventListener("click", () => {
        openSettingsPage("dependencies");
        requestDependencyCheck();
    });
    controls.resetFilterBtn.addEventListener("click", resetFilterConfig);
    controls.testBackendBtn.addEventListener("click", testInputBackend);
    controls.aimHotkeyBtn.addEventListener("click", (event) => {
        event.preventDefault();
        beginHotkeyCapture("aimHotkey");
    });
    controls.aimHotkey2Btn.addEventListener("click", (event) => {
        event.preventDefault();
        beginHotkeyCapture("aimHotkey2");
    });
    controls.clearLogBtn.addEventListener("click", () => {
        eventLog.replaceChildren();
        logEvent("log cleared");
    });
    controls.dependencyList.addEventListener("click", (event) => {
        const chooseButton = event.target.closest("[data-dependency-choose]");
        if (chooseButton) {
            bridge.send("ui:chooseDependencyPath", {
                key: chooseButton.dataset.dependencyChoose,
                kind: chooseButton.dataset.dependencyKind || "folder"
            });
            return;
        }

        const openButton = event.target.closest("[data-dependency-url]");
        if (openButton) {
            openDependencyUrl(openButton.dataset.dependencyUrl);
            return;
        }

        const downloadButton = event.target.closest("[data-dependency-download]");
        if (downloadButton) {
            downloadDependency(downloadButton.dataset.dependencyDownload);
        }
    });
    controls.dependencyDownloads.addEventListener("click", (event) => {
        const downloadAll = event.target.closest("[data-dependency-download-all]");
        if (downloadAll) {
            downloadDependencies((state.dependencies.downloads || [])
                .filter((item) => item.canDownload)
                .map((item) => item.key));
            return;
        }

        const downloadButton = event.target.closest("[data-dependency-download]");
        if (downloadButton) {
            downloadDependency(downloadButton.dataset.dependencyDownload);
            return;
        }

        const openRecommended = event.target.closest("[data-dependency-open-recommended]");
        if (openRecommended) {
            openDependencyUrls((state.dependencies.downloads || []).map((item) => item.url));
            return;
        }

        const openButton = event.target.closest("[data-dependency-url]");
        if (openButton) {
            openDependencyUrl(openButton.dataset.dependencyUrl);
        }
    });
    const liveControls = [
        controls.modelPath,
        controls.inputBackend,
        controls.driverDllPath,
        controls.driverType,
        controls.humanSlideMaxStep,
        controls.humanSlideJitter,
        controls.humanSlideDelayMin,
        controls.humanSlideDelayMax,
        controls.trackingBoostThreshold,
        controls.trackingBoostGain,
        controls.trackingBoostMaxMove,
        controls.autoClickTolerance,
        controls.enableAutoStop,
        controls.autoStopMode,
        controls.autoStopHoldMs,
        controls.autoStopSettleMs,
        controls.aimFilter,
        controls.pidKp,
        controls.pidKi,
        controls.pidKd,
        controls.pidIntegralLimit,
        controls.oneEuroMinCutoff,
        controls.oneEuroBeta,
        controls.oneEuroDCutoff,
        controls.predictionMode,
        controls.predictionLeadMs,
        controls.predictionSmoothing,
        controls.predictionAccelerationSmoothing,
        controls.predictionAlpha,
        controls.predictionBeta,
        controls.predictionKalmanMeasurementNoise,
        controls.predictionKalmanProcessNoise,
        controls.predictionMaxPixels,
        controls.predictionResetPixels,
        controls.predictionNoisePixels,
        controls.predictionOutputSmoothing,
        controls.predictionServoGain,
        controls.droneTrackGain,
        controls.droneTrackVelocityGain,
        controls.droneTrackDamping,
        controls.droneTrackSmoothing,
        controls.droneTrackMaxMove,
        controls.droneTrackDeadzone,
        controls.cropSize,
        controls.lockRadius,
        controls.confidence,
        controls.smoothing,
        controls.aimMode,
        controls.aimGain,
        controls.deadzone,
        controls.targetX,
        controls.targetY,
        controls.aimPartPriority,
        controls.enemyCamp,
        controls.detectionPart,
        controls.maxMove
    ];
    liveControls.forEach((control) => {
        control.addEventListener("input", pushConfigUpdate);
        control.addEventListener("change", pushConfigUpdate);
    });

    bindRangePairEvents(
        controls.autoClickDelayMin,
        controls.autoClickDelayMax,
        controls.autoClickDelayMinValue,
        controls.autoClickDelayMaxValue,
        "按下延迟"
    );
    bindRangePairEvents(
        controls.autoClickIntervalMin,
        controls.autoClickIntervalMax,
        controls.autoClickIntervalMinValue,
        controls.autoClickIntervalMaxValue,
        "点击间隔"
    );
    controls.autoClickToleranceValue.addEventListener("click", () => editSingleRangeValue(controls.autoClickTolerance, "到位容差"));
    controls.autoClickToleranceValue.addEventListener("keydown", (event) => {
        if (event.key === "Enter" || event.key === " ") {
            event.preventDefault();
            editSingleRangeValue(controls.autoClickTolerance, "到位容差");
        }
    });
    controls.autoStopHoldMsValue.addEventListener("click", () => editSingleRangeValue(controls.autoStopHoldMs, "急停保持"));
    controls.autoStopHoldMsValue.addEventListener("keydown", (event) => {
        if (event.key === "Enter" || event.key === " ") {
            event.preventDefault();
            editSingleRangeValue(controls.autoStopHoldMs, "急停保持");
        }
    });
    controls.autoStopSettleMsValue.addEventListener("click", () => editSingleRangeValue(controls.autoStopSettleMs, "开火前等待"));
    controls.autoStopSettleMsValue.addEventListener("keydown", (event) => {
        if (event.key === "Enter" || event.key === " ") {
            event.preventDefault();
            editSingleRangeValue(controls.autoStopSettleMs, "开火前等待");
        }
    });

    controls.inputBackend.addEventListener("change", () => {
        pushConfigUpdate();
        requestDependencyCheck();
    });

    [
        controls.modelPath,
        controls.driverDllPath,
        controls.driverType
    ].forEach((control) => {
        control.addEventListener("change", requestDependencyCheck);
    });

    Object.values(controls.switches).forEach((input) => {
        input.addEventListener("change", pushConfigUpdate);
    });

    $$("[data-open-page]").forEach((button) => {
        button.addEventListener("mousedown", (event) => {
            event.preventDefault();
            event.stopPropagation();
        });
        button.addEventListener("click", (event) => {
            event.preventDefault();
            event.stopPropagation();
            openSettingsPage(button.dataset.openPage);
        });
    });

    $$("[data-backend]").forEach((button) => {
        button.addEventListener("click", () => {
            state.config.backend = button.dataset.backend;
            $$("[data-backend]").forEach((item) => item.classList.toggle("active", item === button));
            pushConfigUpdate();
            requestDependencyCheck();
        });
    });

    if (bridge.available()) {
        window.chrome.webview.addEventListener("message", handleHostMessage);
    }
}

function boot() {
    loadConfig();
    ensureSwitchStateBadges();
    syncUiFromConfig();
    setView("home");
    bindEvents();
    setRunning(false);
    setDirty(false);
    renderDependencyBadge(true);
    logEvent(bridge.available() ? "webview bridge ready" : "browser preview ready");
    bridge.send("ui:ready", state.config);
}

boot();
