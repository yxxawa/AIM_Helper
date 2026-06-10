// AIM Helper backend entry aggregation.
// The backend is split by feature area under backend/src. These includes keep the
// first refactor as one translation unit so behavior and initialization order stay unchanged.

#include "backend/src/core/Pch.h"
#include "backend/src/core/Types.h"
#include "backend/src/runtime/Config.cpp"
#include "backend/src/input/InputBackends.cpp"
#include "backend/src/capture/ScreenCapture.cpp"
#include "backend/src/detection/ObjectDetector.cpp"
#include "backend/src/aim/AimFilters.cpp"
#include "backend/src/aim/AimAssistant.cpp"
#include "backend/src/runtime/SelfTest.cpp"
#include "backend/src/runtime/AppMain.cpp"
