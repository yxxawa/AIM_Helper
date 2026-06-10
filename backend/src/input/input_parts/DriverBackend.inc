// User-provided DLL driver backend adapters

class DriverBackend final : public InputBackend {
public:
    #include "driver_backend_parts/PublicApi.inc"

private:
    #include "driver_backend_parts/AbiTypes.inc"
    #include "driver_backend_parts/ButtonAndKeyHelpers.inc"
    #include "driver_backend_parts/AbiBinding.inc"
    #include "driver_backend_parts/LoggingAndCleanup.inc"
    #include "driver_backend_parts/State.inc"
};
