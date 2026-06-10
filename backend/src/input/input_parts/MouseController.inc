// MouseController facade and humanized movement worker

class MouseController {
public:
    #include "mouse_controller_parts/PublicApi.inc"

private:
    #include "mouse_controller_parts/HumanizedMovement.inc"
    #include "mouse_controller_parts/State.inc"
};
