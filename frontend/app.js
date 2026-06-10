const root = document.querySelector(".app-shell");
const eventLog = document.querySelector("#eventLog");

const defaults = {
    configVersion: 34,
    backend: "cpu",
    inputBackend: "dd",
    modelPath: "",
    modelId: "",
    modelName: "",
    modelClasses: [],
    modelInputWidth: 0,
    modelInputHeight: 0,
    trtCachePath: "",
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
    enableCrosshairColor: false,
    crosshairColorR: 0,
    crosshairColorG: 255,
    crosshairColorB: 0,
    crosshairColorTolerance: 42,
    crosshairMinArea: 1,
    crosshairMaxArea: 900,
    crosshairSmoothing: 0.20,
    targetX: 0.5,
    targetY: 0.3,
    enableAutoAimPart: true,
    targetEntityPriority: "distance",
    aimPartPriority: "distance",
    enemyCamp: "all",
    detectionPart: "all",
    maxMove: 60,
    enableHumanSlide: false,
    humanSlideMaxStep: 50,
    humanSlideJitter: 0.5,
    humanSlideDelayMin: 5,
    humanSlideDelayMax: 20,
    enableAutoClick: false,
    autoClickHoldMode: false,
    autoClickDelayMin: 0,
    autoClickDelayMax: 0,
    autoClickHoldDelayMin: 0,
    autoClickHoldDelayMax: 0,
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
    yoloFpsLimit: 200,
    enableConsoleStats: false,
    boundedMovement: true,
    enableInstantSnap: false,
    smoothSlideMaxStep: 18,
    enableAntiSnap: false,
    antiSnapMaxDelta: 90,
    enableFallenTargetFilter: false,
    enableSmallLockOnly: false,
    smallLockRadius: 35
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
    modelHistory: [],
    driverHistory: [],
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
    helpTooltip: $("#helpTooltip"),
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
    crosshairColorSwitchLabel: $("#crosshairColorSwitchLabel"),
    autoClickSwitchLabel: $("#autoClickSwitchLabel"),
    antiSnapSwitchLabel: $("#antiSnapSwitchLabel"),
    smallLockSwitchLabel: $("#smallLockSwitchLabel"),
    fallenTargetFilterSwitchLabel: $("#fallenTargetFilterSwitchLabel"),
    modelPath: $("#modelPath"),
    importModelBtn: $("#importModelBtn"),
    modelHistoryBtn: $("#modelHistoryBtn"),
    modelNameLabel: $("#modelNameLabel"),
    modelPathLabel: $("#modelPathLabel"),
    modelClassSummary: $("#modelClassSummary"),
    modelClassList: $("#modelClassList"),
    modalLayer: $("#modalLayer"),
    modalTitle: $("#modalTitle"),
    modalBody: $("#modalBody"),
    modalActions: $("#modalActions"),
    modalCloseBtn: $("#modalCloseBtn"),
    inputBackend: $("#inputBackend"),
    driverDllPath: $("#driverDllPath"),
    driverDllPathLabel: $("#driverDllPathLabel"),
    chooseDriverDllBtn: $("#chooseDriverDllBtn"),
    driverHistoryBtn: $("#driverHistoryBtn"),
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
    autoClickHoldDelayRange: $("#autoClickHoldDelayRange"),
    autoClickHoldDelayMin: $("#autoClickHoldDelayMin"),
    autoClickHoldDelayMax: $("#autoClickHoldDelayMax"),
    autoClickHoldDelayMinValue: $("#autoClickHoldDelayMinValue"),
    autoClickHoldDelayMaxValue: $("#autoClickHoldDelayMaxValue"),
    autoClickIntervalRange: $("#autoClickIntervalRange"),
    autoClickIntervalMin: $("#autoClickIntervalMin"),
    autoClickIntervalMax: $("#autoClickIntervalMax"),
    autoClickIntervalMinValue: $("#autoClickIntervalMinValue"),
    autoClickIntervalMaxValue: $("#autoClickIntervalMaxValue"),
    autoClickTolerance: $("#autoClickTolerance"),
    autoClickToleranceValue: $("#autoClickToleranceValue"),
    autoClickMode: $("#autoClickMode"),
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
    crosshairColorSlot: $("#crosshairColorSlot"),
    crosshairColorLabel: $("#crosshairColorLabel"),
    pickCrosshairColorBtn: $("#pickCrosshairColorBtn"),
    crosshairColorR: $("#crosshairColorR"),
    crosshairColorG: $("#crosshairColorG"),
    crosshairColorB: $("#crosshairColorB"),
    crosshairColorTolerance: $("#crosshairColorTolerance"),
    crosshairMinArea: $("#crosshairMinArea"),
    crosshairMaxArea: $("#crosshairMaxArea"),
    crosshairSmoothing: $("#crosshairSmoothing"),
    cropSize: $("#cropSize"),
    lockRadius: $("#lockRadius"),
    confidence: $("#confidence"),
    smoothing: $("#smoothing"),
    aimMode: $("#aimMode"),
    aimGain: $("#aimGain"),
    deadzone: $("#deadzone"),
    targetX: $("#targetX"),
    targetY: $("#targetY"),
    targetEntityPriority: $("#targetEntityPriority"),
    aimPartPriority: $("#aimPartPriority"),
    enemyCamp: $("#enemyCamp"),
    detectionPart: $("#detectionPart"),
    maxMove: $("#maxMove"),
    smoothSlideMaxStep: $("#smoothSlideMaxStep"),
    antiSnapMaxDelta: $("#antiSnapMaxDelta"),
    smallLockRadius: $("#smallLockRadius"),
    yoloFpsLimit: $("#yoloFpsLimit"),
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
    smoothSlideMaxStepValue: $("#smoothSlideMaxStepValue"),
    antiSnapMaxDeltaValue: $("#antiSnapMaxDeltaValue"),
    smallLockRadiusValue: $("#smallLockRadiusValue"),
    yoloFpsLimitValue: $("#yoloFpsLimitValue"),
    yoloFpsLimitLabel: $("#yoloFpsLimitLabel"),
    pidKpValue: $("#pidKpValue"),
    pidKiValue: $("#pidKiValue"),
    pidKdValue: $("#pidKdValue"),
    pidIntegralLimitValue: $("#pidIntegralLimitValue"),
    oneEuroMinCutoffValue: $("#oneEuroMinCutoffValue"),
    oneEuroBetaValue: $("#oneEuroBetaValue"),
    oneEuroDCutoffValue: $("#oneEuroDCutoffValue"),
    crosshairColorSwatch: $("#crosshairColorSwatch"),
    crosshairColorRValue: $("#crosshairColorRValue"),
    crosshairColorGValue: $("#crosshairColorGValue"),
    crosshairColorBValue: $("#crosshairColorBValue"),
    crosshairColorToleranceValue: $("#crosshairColorToleranceValue"),
    crosshairMinAreaValue: $("#crosshairMinAreaValue"),
    crosshairMaxAreaValue: $("#crosshairMaxAreaValue"),
    crosshairSmoothingValue: $("#crosshairSmoothingValue"),
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
    switches: {
        enableCapture: $("#enableCapture"),
        enableAutoAimPart: $("#enableAutoAimPart"),
        enableMouseMove: $("#enableMouseMove"),
        enableHumanSlide: $("#enableHumanSlide"),
        enableCrosshairColor: $("#enableCrosshairColor"),
        enableAutoClick: $("#enableAutoClick"),
        enableHoldToAim: $("#enableHoldToAim"),
        enableVisualization: $("#enableVisualization"),
        enableConsoleStats: $("#enableConsoleStats"),
        boundedMovement: $("#boundedMovement"),
        enableInstantSnap: $("#enableInstantSnap"),
        enableAntiSnap: $("#enableAntiSnap"),
        enableFallenTargetFilter: $("#enableFallenTargetFilter"),
        enableSmallLockOnly: $("#enableSmallLockOnly")
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
        title: "鼠标移动",
        hint: "热键、移动参数、平滑步长、倒地残留过滤"
    },
    target: {
        title: "自动选择部位",
        hint: "锁定部位优先级"
    },
    humanSlide: {
        title: "仿人类滑动",
        hint: "分段、随机抖动、移动延迟"
    },
    crosshairColor: {
        title: "准心图色",
        hint: "用准心色块中心代替固定屏幕中心"
    },
    autoClick: {
        title: "自动扳机",
        hint: "单点 / 长按、触发延迟、到位容差"
    },
    movementGuard: {
        title: "防抽",
        hint: "异常移动丢弃阈值"
    },
    smallLock: {
        title: "仅小锁+压枪",
        hint: "只在目标进入小锁半径后移动"
    },
    visualization: {
        title: "显示 YOLO 窗口",
        hint: "可视化窗口与帧率限制"
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
    const ownCamp = normalizeEnemyCampSelection(camp);
    if (ownCamp !== "all") {
        return `排除 ${classCampLabel(ownCamp)}`;
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

function normalizeTargetEntityPriority(mode) {
    return ["distance", "head"].includes(mode) ? mode : defaults.targetEntityPriority;
}

function normalizeAimPartPriority(mode) {
    return ["distance", "head", "other"].includes(mode) ? mode : defaults.aimPartPriority;
}

function normalizeClassRole(role) {
    return ["head", "body", "other"].includes(role) ? role : "other";
}

function normalizeClassCamp(camp) {
    const raw = String(camp || "")
        .replace(/[;,:=|\r\n\t]+/g, " ")
        .replace(/\s+/g, " ")
        .trim();
    if (!raw || raw.toLowerCase() === "unknown") {
        return "敌方";
    }
    const lowered = raw.toLowerCase();
    if (lowered === "ct" || lowered === "t") {
        return lowered;
    }
    return raw.slice(0, 24);
}

function normalizeEnemyCampSelection(camp) {
    const raw = String(camp || "").trim();
    if (!raw || raw.toLowerCase() === "all") {
        return "all";
    }
    return normalizeClassCamp(raw);
}

function modelCampOptions() {
    const seen = new Set();
    const options = [{ value: "all", label: "ALL（全部）" }];
    normalizeModelClasses(state.config.modelClasses).forEach((item) => {
        const value = normalizeClassCamp(item.camp);
        const key = value.toLowerCase();
        if (!seen.has(key)) {
            seen.add(key);
            options.push({ value, label: classCampLabel(value) });
        }
    });
    return options;
}

function renderEnemyCampOptions() {
    const current = normalizeEnemyCampSelection(state.config.enemyCamp);
    const options = modelCampOptions();
    const hasCurrent = options.some((item) => item.value.toLowerCase() === current.toLowerCase());

    controls.enemyCamp.replaceChildren();
    options.forEach((item) => {
        const option = document.createElement("option");
        option.value = item.value;
        option.textContent = item.label;
        controls.enemyCamp.append(option);
    });
    controls.enemyCamp.value = hasCurrent ? current : "all";
    state.config.enemyCamp = controls.enemyCamp.value || "all";
}

function normalizeModelClasses(classes) {
    if (!Array.isArray(classes)) {
        return [];
    }
    return classes
        .map((item) => {
            const id = Number(item?.id);
            if (!Number.isInteger(id) || id < 0) {
                return null;
            }
            const name = String(item?.name || `class-${id}`).trim() || `class-${id}`;
            return {
                id,
                name,
                role: normalizeClassRole(String(item?.role || "other").toLowerCase()),
                camp: normalizeClassCamp(item?.camp),
                enabled: item?.enabled !== false
            };
        })
        .filter(Boolean)
        .sort((a, b) => a.id - b.id);
}

function classRoleLabel(role) {
    if (role === "head") return "头部";
    if (role === "body") return "身体";
    return "其他";
}

function classCampLabel(camp) {
    if (camp === "ct") return "CT";
    if (camp === "t") return "T";
    return normalizeClassCamp(camp);
}

function fileNameFromPath(path) {
    const text = String(path || "");
    const parts = text.split(/[\\/]/).filter(Boolean);
    return parts.at(-1) || "";
}

function shortPath(path) {
    const text = String(path || "");
    if (!text) {
        return "留空时按固定位置自动查找";
    }
    if (text.length <= 72) {
        return text;
    }
    return `...${text.slice(-69)}`;
}

function closeModal() {
    controls.modalLayer.hidden = true;
    controls.modalBody.replaceChildren();
    controls.modalActions.replaceChildren();
    state.activeModal = "";
}

function openModal(title, body, actions = []) {
    controls.modalTitle.textContent = title;
    controls.modalBody.replaceChildren(body);
    controls.modalActions.replaceChildren();
    actions.forEach((action) => controls.modalActions.append(action));
    controls.modalLayer.hidden = false;
    state.activeModal = title;
}

function modalButton(text, className, onClick) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = className || "command compact";
    button.textContent = text;
    button.addEventListener("click", onClick);
    return button;
}

function showInfoModal(title, message) {
    const body = document.createElement("div");
    body.className = "modal-message";
    body.textContent = message;
    openModal(title, body, [
        modalButton("确定", "command compact primary", closeModal)
    ]);
}

function showNumberInputModal(label, initialValue, onCommit) {
    const body = document.createElement("label");
    body.className = "field modal-field";
    const caption = document.createElement("span");
    caption.textContent = label;
    const input = document.createElement("input");
    input.type = "number";
    input.value = initialValue;
    input.autocomplete = "off";
    body.append(caption, input);

    const commit = () => {
        onCommit(input.value);
        closeModal();
    };
    input.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            event.preventDefault();
            commit();
        }
    });

    openModal(label, body, [
        modalButton("取消", "command compact", closeModal),
        modalButton("确定", "command compact primary", commit)
    ]);
    setTimeout(() => {
        input.focus();
        input.select();
    }, 0);
}

function showTextInputModal(label, initialValue, onCommit, placeholder = "") {
    const body = document.createElement("label");
    body.className = "field modal-field";
    const caption = document.createElement("span");
    caption.textContent = label;
    const input = document.createElement("input");
    input.type = "text";
    input.value = initialValue;
    input.placeholder = placeholder;
    input.autocomplete = "off";
    input.maxLength = 24;
    body.append(caption, input);

    const commit = () => {
        onCommit(input.value);
        closeModal();
    };
    input.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            event.preventDefault();
            commit();
        }
    });

    openModal(label, body, [
        modalButton("取消", "command compact", closeModal),
        modalButton("确定", "command compact primary", commit)
    ]);
    setTimeout(() => {
        input.focus();
        input.select();
    }, 0);
}

function normalizeModelEntry(entry) {
    if (!entry || typeof entry !== "object" || Array.isArray(entry)) {
        return null;
    }
    const localPath = String(entry.localPath || entry.modelPath || "").trim();
    const displayName = String(entry.displayName || entry.modelName || fileNameFromPath(localPath) || "未命名模型").trim();
    return {
        id: String(entry.id || entry.sha256 || localPath || displayName),
        displayName,
        originalPath: String(entry.originalPath || ""),
        localPath,
        importedAt: String(entry.importedAt || ""),
        sha256: String(entry.sha256 || ""),
        inputWidth: Math.max(0, Number(entry.inputWidth) || 0),
        inputHeight: Math.max(0, Number(entry.inputHeight) || 0),
        classes: normalizeModelClasses(entry.classes),
        enemyCamp: normalizeEnemyCampSelection(entry.enemyCamp),
        detectionPart: ["all", "head", "body"].includes(String(entry.detectionPart || "").toLowerCase())
            ? String(entry.detectionPart).toLowerCase()
            : "all"
    };
}

function clampCropSize(value) {
    const numeric = Number(value) || defaults.cropSize;
    const snapped = Math.round(numeric / 32) * 32;
    return Math.max(160, Math.min(960, snapped));
}

function autoCropSizeForModel(model) {
    const width = Math.max(0, Number(model?.inputWidth) || 0);
    const height = Math.max(0, Number(model?.inputHeight) || 0);
    const target = Math.max(width, height);
    if (target <= 0) {
        return defaults.cropSize;
    }
    return clampCropSize(target);
}

function normalizeModelHistory(history) {
    return (Array.isArray(history) ? history : [])
        .map(normalizeModelEntry)
        .filter(Boolean);
}

function normalizeDriverEntry(entry) {
    if (!entry || typeof entry !== "object" || Array.isArray(entry)) {
        return null;
    }
    const path = String(entry.path || entry.driverDllPath || "").trim();
    if (!path) {
        return null;
    }
    const displayName = String(entry.displayName || fileNameFromPath(path) || "驱动 DLL").trim();
    return {
        id: String(entry.id || entry.sha256 || path || displayName),
        displayName,
        path,
        selectedAt: String(entry.selectedAt || ""),
        sha256: String(entry.sha256 || ""),
        architecture: String(entry.architecture || "unknown")
    };
}

function normalizeDriverHistory(history) {
    return (Array.isArray(history) ? history : [])
        .map(normalizeDriverEntry)
        .filter(Boolean);
}

function applyModelEntry(entry, { applyPreset = true } = {}) {
    const model = normalizeModelEntry(entry);
    if (!model) {
        return;
    }
    state.config.modelId = model.id;
    state.config.modelName = model.displayName;
    state.config.modelPath = model.localPath;
    state.config.modelClasses = normalizeModelClasses(model.classes);
    state.config.modelInputWidth = Math.max(0, Number(model.inputWidth) || 0);
    state.config.modelInputHeight = Math.max(0, Number(model.inputHeight) || 0);
    state.config.trtCachePath = "";
    state.config.dependencyPaths = {
        ...(state.config.dependencyPaths || {}),
        modelFile: model.localPath
    };
    const autoCrop = autoCropSizeForModel(model);
    if (autoCrop > 0) {
        state.config.cropSize = autoCrop;
    }
    if (applyPreset) {
        state.config.enemyCamp = model.enemyCamp;
        state.config.detectionPart = model.detectionPart;
    }
    syncUiFromConfig();
    saveConfig({ notifyHost: false });
    bridge.send("ui:updateConfig", state.config);
    requestDependencyCheck();
}

function saveModelPreset() {
    if (!state.config.modelId) {
        return;
    }
    bridge.send("ui:saveModelPreset", {
        id: state.config.modelId,
        classes: state.config.modelClasses,
        enemyCamp: state.config.enemyCamp,
        detectionPart: state.config.detectionPart
    });
}

function requestModelHistory() {
    bridge.send("ui:getModelHistory", {});
}

function importModel() {
    syncConfigFromUi();
    saveConfig({ notifyHost: false });
    bridge.send("ui:importModel", state.config);
}

function requestDriverHistory() {
    bridge.send("ui:getDriverHistory", {});
}

function chooseDriverDll() {
    syncConfigFromUi();
    saveConfig({ notifyHost: false });
    bridge.send("ui:chooseDriverDll", state.config);
}

function updateDriverHistory(payload) {
    const history = payload && typeof payload === "object" ? payload.history : payload;
    state.driverHistory = normalizeDriverHistory(history);
}

function applyDriverEntry(entry) {
    const driver = normalizeDriverEntry(entry);
    if (!driver) {
        return;
    }
    state.config.driverDllPath = driver.path;
    state.config.dependencyPaths = {
        ...(state.config.dependencyPaths || {}),
        driverDll: driver.path
    };
    syncUiFromConfig();
    saveConfig({ notifyHost: false });
    bridge.send("ui:updateConfig", state.config);
    requestDependencyCheck();
    logEvent(`driver dll selected: ${driver.displayName}`);
}

function modelClassSummaryText(classes) {
    if (!classes.length) {
        return "ALL";
    }
    const enabled = classes.filter((item) => item.enabled !== false);
    if (enabled.length === classes.length) {
        return `ALL ${classes.length}`;
    }
    if (enabled.length === 0) {
        return "NONE";
    }
    return `${enabled.length}/${classes.length}`;
}

function renderModelClassList() {
    controls.modelClassList.replaceChildren();
    const classes = normalizeModelClasses(state.config.modelClasses);
    controls.modelClassSummary.textContent = modelClassSummaryText(classes);
    if (classes.length === 0) {
        const empty = document.createElement("div");
        empty.className = "model-class-empty";
        empty.textContent = "未读取到类别映射时默认不过滤类别。";
        controls.modelClassList.append(empty);
        return;
    }

    classes.forEach((item) => {
        const row = document.createElement("article");
        row.className = "model-class-item";
        row.dataset.classId = String(item.id);

        const enabled = document.createElement("input");
        enabled.type = "checkbox";
        enabled.checked = item.enabled !== false;
        enabled.dataset.modelClassEnabled = String(item.id);

        const info = document.createElement("div");
        info.className = "model-class-info";
        const title = document.createElement("strong");
        title.textContent = `${item.id} · ${item.name}`;
        const meta = document.createElement("small");
        meta.textContent = `${classCampLabel(item.camp)} / ${classRoleLabel(item.role)}`;
        info.append(title, meta);

        const metaControls = document.createElement("div");
        metaControls.className = "model-class-meta";

        const camp = document.createElement("button");
        camp.type = "button";
        camp.className = "model-class-camp-button";
        camp.dataset.modelClassCampButton = String(item.id);
        camp.textContent = classCampLabel(item.camp);
        camp.title = "点击编辑阵营名称";

        const role = document.createElement("select");
        role.dataset.modelClassRole = String(item.id);
        [
            ["other", "其他"],
            ["head", "头部"],
            ["body", "身体"]
        ].forEach(([value, label]) => {
            const option = document.createElement("option");
            option.value = value;
            option.textContent = label;
            role.append(option);
        });
        role.value = item.role;

        metaControls.append(camp, role);
        row.append(enabled, info, metaControls);
        controls.modelClassList.append(row);
    });
}

function renderModelCard() {
    const name = state.config.modelName || fileNameFromPath(state.config.modelPath) || "未导入模型";
    const inputWidth = Number(state.config.modelInputWidth) || 0;
    const inputHeight = Number(state.config.modelInputHeight) || 0;
    const inputText = inputWidth > 0 && inputHeight > 0 ? ` · 输入 ${inputWidth}x${inputHeight}` : "";
    controls.modelNameLabel.textContent = name;
    controls.modelPathLabel.textContent = `${shortPath(state.config.modelPath)}${inputText}`;
    controls.modelPathLabel.title = state.config.modelPath || "";
    renderEnemyCampOptions();
    renderModelClassList();
}

function renderDriverPathSummary() {
    const path = String(state.config.driverDllPath || "").trim();
    controls.driverDllPathLabel.textContent = path ? shortPath(path) : "留空时自动识别 drivers 目录";
    controls.driverDllPathLabel.title = path || "留空时自动识别 drivers 目录";
}

function openModelHistoryModal() {
    const body = document.createElement("div");
    body.className = "model-history-list";
    const history = normalizeModelHistory(state.modelHistory);
    if (history.length === 0) {
        const empty = document.createElement("div");
        empty.className = "modal-message";
        empty.textContent = "还没有导入过模型。点击“导入模型”后会自动复制到 models 目录，并在这里显示历史记录。";
        body.append(empty);
    } else {
        history.forEach((entry) => {
            const button = document.createElement("button");
            button.type = "button";
            button.className = "model-history-item";
            button.dataset.modelId = entry.id;
            const copy = document.createElement("span");
            const name = document.createElement("strong");
            name.textContent = entry.displayName;
            const path = document.createElement("small");
            path.textContent = shortPath(entry.localPath);
            copy.append(name, path);
            const meta = document.createElement("b");
            const inputText = entry.inputWidth > 0 && entry.inputHeight > 0
                ? ` · ${entry.inputWidth}x${entry.inputHeight}`
                : "";
            meta.textContent = `${entry.classes.length || "ALL"} 类${inputText} · ${entry.importedAt || "未知时间"}`;
            button.append(copy, meta);
            button.addEventListener("click", () => {
                bridge.send("ui:selectModelHistory", { id: entry.id });
                closeModal();
            });
            body.append(button);
        });
    }

    openModal("模型历史", body, [
        modalButton("关闭", "command compact", closeModal),
        modalButton("刷新", "command compact primary", () => {
            requestModelHistory();
            openModelHistoryModal();
        })
    ]);
}

function openDriverHistoryModal() {
    const body = document.createElement("div");
    body.className = "model-history-list";
    const history = normalizeDriverHistory(state.driverHistory);
    if (history.length === 0) {
        const empty = document.createElement("div");
        empty.className = "modal-message";
        empty.textContent = "还没有选择过驱动 DLL。点击“选择文件”后会自动保存到历史记录。";
        body.append(empty);
    } else {
        history.forEach((entry) => {
            const button = document.createElement("button");
            button.type = "button";
            button.className = "model-history-item";
            button.dataset.driverId = entry.id;
            const copy = document.createElement("span");
            const name = document.createElement("strong");
            name.textContent = entry.displayName;
            const path = document.createElement("small");
            path.textContent = shortPath(entry.path);
            copy.append(name, path);
            const meta = document.createElement("b");
            meta.textContent = `${entry.architecture || "unknown"} · ${entry.selectedAt || "未知时间"}`;
            button.append(copy, meta);
            button.addEventListener("click", () => {
                bridge.send("ui:selectDriverHistory", { id: entry.id });
                closeModal();
            });
            body.append(button);
        });
    }

    openModal("驱动历史", body, [
        modalButton("关闭", "command compact", closeModal),
        modalButton("刷新", "command compact primary", () => {
            requestDriverHistory();
            openDriverHistoryModal();
        })
    ]);
}

function sanitizeConfig(config) {
    const incomingVersion = Number(config?.configVersion) || 0;
    const merged = { ...defaults, ...config };
    if (incomingVersion < defaults.configVersion && merged.modelPath === "cs2yolomaax.onnx") {
        merged.modelPath = "";
    }
    if (incomingVersion < 21) {
        merged.enableCrosshairColor = defaults.enableCrosshairColor;
        merged.crosshairColorR = defaults.crosshairColorR;
        merged.crosshairColorG = defaults.crosshairColorG;
        merged.crosshairColorB = defaults.crosshairColorB;
        merged.crosshairColorTolerance = defaults.crosshairColorTolerance;
        merged.crosshairMinArea = defaults.crosshairMinArea;
        merged.crosshairMaxArea = defaults.crosshairMaxArea;
        merged.crosshairSmoothing = defaults.crosshairSmoothing;
    }
    if (incomingVersion < 24) {
        merged.autoClickHoldMode = defaults.autoClickHoldMode;
    }
    if (incomingVersion < 27) {
        merged.autoClickHoldDelayMin = defaults.autoClickHoldDelayMin;
        merged.autoClickHoldDelayMax = defaults.autoClickHoldDelayMax;
    }
    if (incomingVersion < 31 && Number(merged.yoloFpsLimit) === 60) {
        merged.yoloFpsLimit = defaults.yoloFpsLimit;
    }
    if (merged.aimFilter === "pid_oneeuro") {
        merged.aimFilter = "pid";
    }
    merged.modelId = String(merged.modelId || "");
    merged.modelName = String(merged.modelName || "");
    merged.modelPath = String(merged.modelPath || "");
    merged.modelInputWidth = Math.max(0, Number(merged.modelInputWidth) || 0);
    merged.modelInputHeight = Math.max(0, Number(merged.modelInputHeight) || 0);
    merged.trtCachePath = String(merged.trtCachePath || "");
    merged.modelClasses = normalizeModelClasses(merged.modelClasses);
    merged.cropSize = clampCropSize(merged.cropSize);
    merged.yoloFpsLimit = Math.max(0, Math.min(300, Number(merged.yoloFpsLimit) || defaults.yoloFpsLimit));
    merged.smoothSlideMaxStep = Math.max(1, Math.min(120, Number(merged.smoothSlideMaxStep) || defaults.smoothSlideMaxStep));
    merged.enableCapture = true;
    merged.enableHoldToAim = true;
    merged.aimMode = normalizeAimMode(merged.aimMode);
    merged.targetEntityPriority = normalizeTargetEntityPriority(merged.targetEntityPriority);
    merged.aimPartPriority = normalizeAimPartPriority(merged.aimPartPriority);
    merged.autoStopMode = normalizeAutoStopMode(merged.autoStopMode);
    merged.dependencyPaths = {
        ...(defaults.dependencyPaths || {}),
        ...((config && typeof config.dependencyPaths === "object" && !Array.isArray(config.dependencyPaths))
            ? config.dependencyPaths
            : {})
    };
    merged.configVersion = defaults.configVersion;
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

let helpTimer = null;

function hideHelpTooltip() {
    if (helpTimer) {
        clearTimeout(helpTimer);
        helpTimer = null;
    }
    controls.helpTooltip.hidden = true;
}

function showHelpTooltip(target) {
    const text = target?.dataset?.help || "";
    if (!text) {
        hideHelpTooltip();
        return;
    }
    const rect = target.getBoundingClientRect();
    controls.helpTooltip.textContent = text;
    controls.helpTooltip.hidden = false;

    const tooltipRect = controls.helpTooltip.getBoundingClientRect();
    const margin = 10;
    const left = Math.min(
        window.innerWidth - tooltipRect.width - margin,
        Math.max(margin, rect.left + rect.width / 2 - tooltipRect.width / 2)
    );
    const top = rect.bottom + tooltipRect.height + margin > window.innerHeight
        ? Math.max(margin, rect.top - tooltipRect.height - margin)
        : rect.bottom + margin;
    controls.helpTooltip.style.left = `${left}px`;
    controls.helpTooltip.style.top = `${top}px`;
}

function scheduleHelpTooltip(target) {
    hideHelpTooltip();
    helpTimer = window.setTimeout(() => showHelpTooltip(target), 520);
}

function openSettingsPage(page) {
    setView(page);
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
        state.config = sanitizeConfig(parsed);
        if ((Number(parsed.configVersion) || 0) < defaults.configVersion) {
            state.config.enableConsoleStats = defaults.enableConsoleStats;
        }
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
    showNumberInputModal(label, target.value, (raw) => {
        const value = String(raw || "").trim().replace(",", ".");
        const numeric = Number(value);
        if (!Number.isFinite(numeric)) {
            logEvent("invalid range value");
            return;
        }
        target.value = formatForInput(target, clampNumber(numeric, Number(target.min), Number(target.max), Number(target.value)));
        normalizeRangePair(minInput, maxInput, side);
        pushConfigUpdate();
    });
}

function editSingleRangeValue(input, label) {
    showNumberInputModal(label, input.value, (raw) => {
        const value = String(raw || "").trim().replace(",", ".");
        const numeric = Number(value);
        if (!Number.isFinite(numeric)) {
            logEvent("invalid numeric value");
            return;
        }
        input.value = formatForInput(input, clampNumber(numeric, Number(input.min), Number(input.max), Number(input.value)));
        pushConfigUpdate();
    });
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
    controls.autoClickHoldDelayMin.value = state.config.autoClickHoldDelayMin;
    controls.autoClickHoldDelayMax.value = state.config.autoClickHoldDelayMax;
    controls.autoClickIntervalMin.value = state.config.autoClickIntervalMin;
    controls.autoClickIntervalMax.value = state.config.autoClickIntervalMax;
    controls.autoClickTolerance.value = state.config.autoClickTolerance;
    controls.autoClickMode.value = state.config.autoClickHoldMode ? "hold" : "single";
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
    controls.crosshairColorR.value = state.config.crosshairColorR;
    controls.crosshairColorG.value = state.config.crosshairColorG;
    controls.crosshairColorB.value = state.config.crosshairColorB;
    controls.crosshairColorTolerance.value = state.config.crosshairColorTolerance;
    controls.crosshairMinArea.value = state.config.crosshairMinArea;
    controls.crosshairMaxArea.value = state.config.crosshairMaxArea;
    controls.crosshairSmoothing.value = state.config.crosshairSmoothing;
    controls.cropSize.value = state.config.cropSize;
    controls.lockRadius.value = state.config.lockRadius;
    controls.confidence.value = state.config.confidence;
    controls.smoothing.value = state.config.smoothing;
    controls.aimMode.value = normalizeAimMode(state.config.aimMode);
    controls.aimGain.value = state.config.aimGain;
    controls.deadzone.value = state.config.deadzone;
    controls.targetX.value = state.config.targetX;
    controls.targetY.value = state.config.targetY;
    controls.targetEntityPriority.value = normalizeTargetEntityPriority(state.config.targetEntityPriority);
    controls.aimPartPriority.value = normalizeAimPartPriority(state.config.aimPartPriority);
    renderEnemyCampOptions();
    controls.detectionPart.value = state.config.detectionPart;
    controls.maxMove.value = state.config.maxMove;
    controls.smoothSlideMaxStep.value = state.config.smoothSlideMaxStep;
    controls.antiSnapMaxDelta.value = state.config.antiSnapMaxDelta;
    controls.smallLockRadius.value = state.config.smallLockRadius;
    controls.yoloFpsLimit.value = state.config.yoloFpsLimit;

    Object.entries(controls.switches).forEach(([key, input]) => {
        input.checked = Boolean(state.config[key]);
    });
    enforceDetectionPartCompatibility();

    $$("[data-backend]").forEach((button) => {
        button.classList.toggle("active", button.dataset.backend === state.config.backend);
    });

    renderModelCard();
    renderDriverPathSummary();
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
    state.config.modelClasses = normalizeModelClasses(state.config.modelClasses);
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
    const clickHoldDelayRange = normalizeRangePair(controls.autoClickHoldDelayMin, controls.autoClickHoldDelayMax);
    state.config.autoClickHoldDelayMin = Math.round(clickHoldDelayRange.minValue);
    state.config.autoClickHoldDelayMax = Math.round(clickHoldDelayRange.maxValue);
    const clickIntervalRange = normalizeRangePair(controls.autoClickIntervalMin, controls.autoClickIntervalMax);
    state.config.autoClickIntervalMin = Math.round(clickIntervalRange.minValue);
    state.config.autoClickIntervalMax = Math.round(clickIntervalRange.maxValue);
    state.config.autoClickTolerance = Number(controls.autoClickTolerance.value);
    state.config.autoClickHoldMode = controls.autoClickMode.value === "hold";
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
    state.config.crosshairColorR = Number(controls.crosshairColorR.value);
    state.config.crosshairColorG = Number(controls.crosshairColorG.value);
    state.config.crosshairColorB = Number(controls.crosshairColorB.value);
    state.config.crosshairColorTolerance = Number(controls.crosshairColorTolerance.value);
    state.config.crosshairMinArea = Number(controls.crosshairMinArea.value);
    state.config.crosshairMaxArea = Number(controls.crosshairMaxArea.value);
    if (state.config.crosshairMinArea > state.config.crosshairMaxArea) {
        [state.config.crosshairMinArea, state.config.crosshairMaxArea] = [
            state.config.crosshairMaxArea,
            state.config.crosshairMinArea
        ];
        controls.crosshairMinArea.value = state.config.crosshairMinArea;
        controls.crosshairMaxArea.value = state.config.crosshairMaxArea;
    }
    state.config.crosshairSmoothing = Number(controls.crosshairSmoothing.value);
    state.config.cropSize = clampCropSize(controls.cropSize.value);
    controls.cropSize.value = state.config.cropSize;
    state.config.lockRadius = Number(controls.lockRadius.value);
    state.config.confidence = Number(controls.confidence.value);
    state.config.smoothing = Number(controls.smoothing.value);
    state.config.aimMode = normalizeAimMode(controls.aimMode.value);
    state.config.aimGain = Number(controls.aimGain.value);
    state.config.deadzone = Number(controls.deadzone.value);
    state.config.targetX = Number(controls.targetX.value);
    state.config.targetY = Number(controls.targetY.value);
    state.config.targetEntityPriority = normalizeTargetEntityPriority(controls.targetEntityPriority.value);
    state.config.aimPartPriority = normalizeAimPartPriority(controls.aimPartPriority.value);
    state.config.enemyCamp = normalizeEnemyCampSelection(controls.enemyCamp.value);
    state.config.detectionPart = controls.detectionPart.value;
    state.config.maxMove = Number(controls.maxMove.value);
    state.config.smoothSlideMaxStep = Number(controls.smoothSlideMaxStep.value);
    state.config.antiSnapMaxDelta = Number(controls.antiSnapMaxDelta.value);
    state.config.smallLockRadius = Number(controls.smallLockRadius.value);
    state.config.yoloFpsLimit = Math.max(0, Math.min(300, Number(controls.yoloFpsLimit.value) || 0));
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
    controls.maxMoveValue.textContent = state.config.maxMove;
    controls.smoothSlideMaxStepValue.textContent = state.config.smoothSlideMaxStep.toFixed(0);
    controls.smoothSlideMaxStep.disabled = Boolean(state.config.enableInstantSnap);
    controls.antiSnapMaxDeltaValue.textContent = state.config.antiSnapMaxDelta.toFixed(0);
    controls.smallLockRadiusValue.textContent = state.config.smallLockRadius.toFixed(0);
    controls.yoloFpsLimitValue.textContent = state.config.yoloFpsLimit.toFixed(0);
    controls.yoloFpsLimitLabel.textContent = state.config.yoloFpsLimit > 0
        ? `${state.config.yoloFpsLimit.toFixed(0)} FPS`
        : "不限";
    controls.filterLabel.textContent = state.config.aimFilter === "oneeuro" ? "1 EURO" : state.config.aimFilter.toUpperCase();
    controls.pidKpValue.textContent = state.config.pidKp.toFixed(2);
    controls.pidKiValue.textContent = state.config.pidKi.toFixed(2);
    controls.pidKdValue.textContent = state.config.pidKd.toFixed(2);
    controls.pidIntegralLimitValue.textContent = state.config.pidIntegralLimit.toFixed(0);
    controls.oneEuroMinCutoffValue.textContent = state.config.oneEuroMinCutoff.toFixed(2);
    controls.oneEuroBetaValue.textContent = state.config.oneEuroBeta.toFixed(2);
    controls.oneEuroDCutoffValue.textContent = state.config.oneEuroDCutoff.toFixed(2);
    controls.crosshairColorRValue.textContent = state.config.crosshairColorR.toFixed(0);
    controls.crosshairColorGValue.textContent = state.config.crosshairColorG.toFixed(0);
    controls.crosshairColorBValue.textContent = state.config.crosshairColorB.toFixed(0);
    controls.crosshairColorToleranceValue.textContent = state.config.crosshairColorTolerance.toFixed(0);
    controls.crosshairMinAreaValue.textContent = state.config.crosshairMinArea.toFixed(0);
    controls.crosshairMaxAreaValue.textContent = state.config.crosshairMaxArea.toFixed(0);
    controls.crosshairSmoothingValue.textContent = state.config.crosshairSmoothing.toFixed(2);
    const crosshairColorText = `RGB ${state.config.crosshairColorR.toFixed(0)},${state.config.crosshairColorG.toFixed(0)},${state.config.crosshairColorB.toFixed(0)}`;
    controls.crosshairColorSwatch.style.background = `rgb(${state.config.crosshairColorR}, ${state.config.crosshairColorG}, ${state.config.crosshairColorB})`;
    controls.crosshairColorLabel.textContent = state.config.enableCrosshairColor
        ? `${crosshairColorText} / ±${state.config.crosshairColorTolerance.toFixed(0)}`
        : "OFF";
    controls.crosshairColorSwitchLabel.textContent = controls.crosshairColorLabel.textContent;
    controls.crosshairColorSlot.dataset.crosshairColorEnabled = String(state.config.enableCrosshairColor);
    controls.humanSlideMaxStepValue.textContent = state.config.humanSlideMaxStep.toFixed(0);
    controls.humanSlideJitterValue.textContent = state.config.humanSlideJitter.toFixed(1);
    controls.humanSlideDelayMinValue.textContent = state.config.humanSlideDelayMin.toFixed(0);
    controls.humanSlideDelayMaxValue.textContent = state.config.humanSlideDelayMax.toFixed(0);
    controls.autoClickDelayMinValue.textContent = state.config.autoClickDelayMin.toFixed(0);
    controls.autoClickDelayMaxValue.textContent = state.config.autoClickDelayMax.toFixed(0);
    controls.autoClickHoldDelayMinValue.textContent = state.config.autoClickHoldDelayMin.toFixed(0);
    controls.autoClickHoldDelayMaxValue.textContent = state.config.autoClickHoldDelayMax.toFixed(0);
    controls.autoClickIntervalMinValue.textContent = state.config.autoClickIntervalMin.toFixed(0);
    controls.autoClickIntervalMaxValue.textContent = state.config.autoClickIntervalMax.toFixed(0);
    controls.autoClickToleranceValue.textContent = state.config.autoClickTolerance.toFixed(1);
    updateDualRangeFill(controls.autoClickDelayMin, controls.autoClickDelayMax);
    updateDualRangeFill(controls.autoClickHoldDelayMin, controls.autoClickHoldDelayMax);
    updateDualRangeFill(controls.autoClickIntervalMin, controls.autoClickIntervalMax);
    controls.autoClickSlot.dataset.autoStopEnabled = String(state.config.enableAutoStop);
    controls.autoClickSlot.dataset.triggerMode = state.config.autoClickHoldMode ? "hold" : "single";
    controls.autoStopModeField.hidden = !state.config.enableAutoStop;
    controls.autoStopHoldField.hidden = !state.config.enableAutoStop;
    controls.autoStopSettleField.hidden = !state.config.enableAutoStop;
    controls.autoStopHoldMsValue.textContent = state.config.autoStopHoldMs.toFixed(0);
    controls.autoStopSettleMsValue.textContent = state.config.autoStopSettleMs.toFixed(0);
    const usesPid = state.config.aimFilter === "pid";
    const usesOneEuro = state.config.aimFilter === "oneeuro";
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
    controls.targetEntityPriority.value = normalizeTargetEntityPriority(state.config.targetEntityPriority);
    const entityHeadFirst = usesAutoAimPart && state.config.targetEntityPriority === "head";
    controls.aimPartPriority.disabled = !usesAutoAimPart || entityHeadFirst;
    controls.aimPartPriorityCard.dataset.entityHeadFirst = String(entityHeadFirst);
    controls.aimHotkeyBtn.textContent = pendingHotkeyField === "aimHotkey" ? "等待输入..." : hotkeyLabel(state.config.aimHotkey);
    controls.aimHotkey2Btn.textContent = pendingHotkeyField === "aimHotkey2" ? "等待输入..." : hotkeyLabel(state.config.aimHotkey2);
    if (controls.aimHotkeySwitchLabel) {
        controls.aimHotkeySwitchLabel.textContent = `${hotkeyLabel(state.config.aimHotkey)} / ${hotkeyLabel(state.config.aimHotkey2)}`;
    }
    controls.humanSlideSwitchLabel.textContent = state.config.enableHumanSlide
        ? `${state.config.humanSlideMaxStep.toFixed(0)}px / ${state.config.humanSlideJitter.toFixed(1)}`
        : "OFF";
    controls.antiSnapSwitchLabel.textContent = state.config.enableAntiSnap
        ? `>${state.config.antiSnapMaxDelta.toFixed(0)}px 丢弃`
        : "OFF";
    controls.smallLockSwitchLabel.textContent = state.config.enableSmallLockOnly
        ? `${state.config.smallLockRadius.toFixed(0)}px`
        : "OFF";
    controls.fallenTargetFilterSwitchLabel.textContent = state.config.enableFallenTargetFilter
        ? "ON"
        : "OFF";
    controls.humanSlideLabel.textContent = state.config.enableHumanSlide ? "HUMAN" : "DIRECT";
    const autoStopText = state.config.enableAutoStop
        ? `${autoStopModeLabel(state.config.autoStopMode)} ${state.config.autoStopHoldMs}-${state.config.autoStopSettleMs}ms`
        : "无急停";
    const autoClickTimingText = state.config.autoClickHoldMode
        ? `长按延迟 ${state.config.autoClickHoldDelayMin}-${state.config.autoClickHoldDelayMax}ms`
        : `单点 ${state.config.autoClickDelayMin}-${state.config.autoClickDelayMax} / ${state.config.autoClickIntervalMin}-${state.config.autoClickIntervalMax}ms`;
    controls.autoClickSwitchLabel.textContent = state.config.enableAutoClick
        ? `${autoClickTimingText} / ${autoStopText}`
        : "OFF";
    controls.autoClickLabel.textContent = state.config.enableAutoClick
        ? `${autoClickTimingText} · ${autoStopText}`
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

function pickCrosshairColor() {
    syncConfigFromUi();
    saveConfig({ notifyHost: false });
    bridge.send("ui:pickCrosshairColor", state.config);
    logEvent("crosshair color pick requested");
}

function pushConfigUpdate() {
    syncConfigFromUi();
    updateLabels();
    localStorage.setItem("offline-yolo-switchboard", JSON.stringify(state.config));
    setDirty(false);
    bridge.send("ui:updateConfig", state.config);
}

function applyPickedCrosshairColor(payload) {
    if (!payload || typeof payload !== "object") {
        return;
    }
    const r = Number(payload.r);
    const g = Number(payload.g);
    const b = Number(payload.b);
    if (![r, g, b].every(Number.isFinite)) {
        logEvent("crosshair color pick failed");
        return;
    }
    state.config.crosshairColorR = Math.max(0, Math.min(255, Math.round(r)));
    state.config.crosshairColorG = Math.max(0, Math.min(255, Math.round(g)));
    state.config.crosshairColorB = Math.max(0, Math.min(255, Math.round(b)));
    state.config.enableCrosshairColor = true;
    syncUiFromConfig();
    pushConfigUpdate();
    logEvent(`crosshair color picked rgb=${state.config.crosshairColorR},${state.config.crosshairColorG},${state.config.crosshairColorB}`);
}

function updateModelHistory(payload) {
    const history = payload && typeof payload === "object" ? payload.history : payload;
    state.modelHistory = normalizeModelHistory(history);
}

function applyImportedModel(payload) {
    if (!payload || typeof payload !== "object") {
        return;
    }
    updateModelHistory(payload.history ? payload : { history: state.modelHistory });
    applyModelEntry(payload.entry || payload, { applyPreset: true });
    logEvent(`model imported: ${state.config.modelName || fileNameFromPath(state.config.modelPath)}`);
}

function applySelectedModel(payload) {
    const entry = normalizeModelEntry(payload);
    if (!entry) {
        return;
    }
    applyModelEntry(entry, { applyPreset: true });
    logEvent(`model selected: ${entry.displayName}`);
}

function applySelectedDriver(payload) {
    if (!payload || typeof payload !== "object") {
        return;
    }
    updateDriverHistory(payload.history ? payload : { history: state.driverHistory });
    applyDriverEntry(payload.entry || payload);
}

function updateModelClassFromControl(control) {
    const id = Number(
        control.dataset.modelClassEnabled
        ?? control.dataset.modelClassRole
    );
    if (!Number.isInteger(id)) {
        return false;
    }
    const classes = normalizeModelClasses(state.config.modelClasses);
    const index = classes.findIndex((item) => item.id === id);
    if (index < 0) {
        return false;
    }

    if (control.dataset.modelClassEnabled !== undefined) {
        classes[index].enabled = Boolean(control.checked);
    }
    if (control.dataset.modelClassRole !== undefined) {
        classes[index].role = normalizeClassRole(control.value);
    }

    state.config.modelClasses = classes;
    state.config.enemyCamp = normalizeEnemyCampSelection(state.config.enemyCamp);
    renderModelCard();
    pushConfigUpdate();
    saveModelPreset();
    return true;
}

function editModelClassCamp(button) {
    const id = Number(button.dataset.modelClassCampButton);
    if (!Number.isInteger(id)) {
        return;
    }
    const classes = normalizeModelClasses(state.config.modelClasses);
    const index = classes.findIndex((item) => item.id === id);
    if (index < 0) {
        return;
    }
    showTextInputModal("阵营名称", classes[index].camp, (raw) => {
        classes[index].camp = normalizeClassCamp(raw);
        state.config.modelClasses = classes;
        state.config.enemyCamp = normalizeEnemyCampSelection(state.config.enemyCamp);
        renderModelCard();
        pushConfigUpdate();
        saveModelPreset();
    }, "敌方");
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
    if (message.type === "host:crosshairColor") {
        applyPickedCrosshairColor(message.payload);
    }
    if (message.type === "host:modelHistory") {
        updateModelHistory(message.payload);
        if (state.activeModal === "模型历史") {
            openModelHistoryModal();
        }
    }
    if (message.type === "host:driverHistory") {
        updateDriverHistory(message.payload);
        if (state.activeModal === "驱动历史") {
            openDriverHistoryModal();
        }
    }
    if (message.type === "host:modelImported") {
        applyImportedModel(message.payload);
    }
    if (message.type === "host:modelSelected") {
        applySelectedModel(message.payload);
    }
    if (message.type === "host:driverSelected") {
        applySelectedDriver(message.payload);
    }
    if (message.type === "host:modelImportFailed") {
        showInfoModal("导入失败", message.payload?.message || "模型导入失败");
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
    controls.pickCrosshairColorBtn.addEventListener("click", pickCrosshairColor);
    controls.importModelBtn.addEventListener("click", importModel);
    controls.modelHistoryBtn.addEventListener("click", () => {
        requestModelHistory();
        openModelHistoryModal();
    });
    controls.chooseDriverDllBtn.addEventListener("click", chooseDriverDll);
    controls.driverHistoryBtn.addEventListener("click", () => {
        requestDriverHistory();
        openDriverHistoryModal();
    });
    controls.modalCloseBtn.addEventListener("click", closeModal);
    controls.modalLayer.addEventListener("click", (event) => {
        if (event.target === controls.modalLayer) {
            closeModal();
        }
    });
    window.addEventListener("keydown", (event) => {
        if (event.key === "Escape" && !controls.modalLayer.hidden) {
            closeModal();
        }
    });
    controls.modelClassList.addEventListener("change", (event) => {
        const control = event.target.closest("[data-model-class-enabled], [data-model-class-role]");
        if (control) {
            updateModelClassFromControl(control);
        }
    });
    controls.modelClassList.addEventListener("click", (event) => {
        const button = event.target.closest("[data-model-class-camp-button]");
        if (button) {
            editModelClassCamp(button);
        }
    });
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
        controls.autoClickTolerance,
        controls.autoClickMode,
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
        controls.crosshairColorR,
        controls.crosshairColorG,
        controls.crosshairColorB,
        controls.crosshairColorTolerance,
        controls.crosshairMinArea,
        controls.crosshairMaxArea,
        controls.crosshairSmoothing,
        controls.cropSize,
        controls.lockRadius,
        controls.confidence,
        controls.smoothing,
        controls.aimMode,
        controls.aimGain,
        controls.deadzone,
        controls.targetX,
        controls.targetY,
        controls.targetEntityPriority,
        controls.aimPartPriority,
        controls.enemyCamp,
        controls.detectionPart,
        controls.maxMove,
        controls.smoothSlideMaxStep,
        controls.antiSnapMaxDelta,
        controls.smallLockRadius,
        controls.yoloFpsLimit
    ];
    liveControls.forEach((control) => {
        control.addEventListener("input", pushConfigUpdate);
        control.addEventListener("change", pushConfigUpdate);
    });

    [controls.enemyCamp, controls.detectionPart].forEach((control) => {
        control.addEventListener("change", saveModelPreset);
    });

    bindRangePairEvents(
        controls.autoClickDelayMin,
        controls.autoClickDelayMax,
        controls.autoClickDelayMinValue,
        controls.autoClickDelayMaxValue,
        "单点按下延迟"
    );
    bindRangePairEvents(
        controls.autoClickHoldDelayMin,
        controls.autoClickHoldDelayMax,
        controls.autoClickHoldDelayMinValue,
        controls.autoClickHoldDelayMaxValue,
        "长按延迟"
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

    $$("[data-help]").forEach((item) => {
        item.addEventListener("mouseenter", () => scheduleHelpTooltip(item));
        item.addEventListener("mousemove", () => {
            if (!controls.helpTooltip.hidden) {
                showHelpTooltip(item);
            }
        });
        item.addEventListener("mouseleave", hideHelpTooltip);
        item.addEventListener("focusin", () => scheduleHelpTooltip(item));
        item.addEventListener("focusout", hideHelpTooltip);
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
