// --- 5. 核心协调器 (AimAssistant) ---

class AimAssistant {
public:
    #include "aim_assistant_parts/Lifecycle.inc"
    #include "aim_assistant_parts/RunLoop.inc"

private:
    #include "assistant_parts/RuntimeConfig.inc"
    #include "assistant_parts/CrosshairTracking.inc"
    #include "assistant_parts/TargetSelection.inc"
    #include "assistant_parts/AimMovement.inc"
    #include "assistant_parts/AimSampleLogging.inc"
    #include "assistant_parts/AutoTrigger.inc"
    #include "assistant_parts/TelemetryVisualization.inc"
    #include "aim_assistant_parts/State.inc"
};
