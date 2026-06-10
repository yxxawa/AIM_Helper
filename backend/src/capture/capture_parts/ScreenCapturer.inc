// D3D/DXGI screen duplication and cropped frame readback

class ScreenCapturer {
public:
    #include "screen_capturer_parts/PublicApi.inc"

private:
    #include "screen_capturer_parts/OutputSelection.inc"
    #include "screen_capturer_parts/State.inc"
};

#include "screen_capturer_parts/CenterCropRegion.inc"
