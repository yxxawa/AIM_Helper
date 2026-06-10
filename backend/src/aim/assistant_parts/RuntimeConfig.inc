// Runtime config reload, validation and startup instructions.
// Split into runtime_config_parts to keep validation, polling, apply and startup text separate.

#include "runtime_config_parts/ValidationAndReload.inc"
#include "runtime_config_parts/LiveConfigPolling.inc"
#include "runtime_config_parts/LiveConfigApply.inc"
#include "runtime_config_parts/StartupInstructions.inc"
