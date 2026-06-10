// Console startup controls

    void print_instructions() {
        std::cout << "\n--- Controls ---" << std::endl;
        if (cfg.enable_hold_to_aim) {
            std::cout << "    [Hold " << KeyNameFromVk(cfg.smooth_aim_key);
            if (cfg.smooth_aim_secondary_key > 0) {
                std::cout << " or " << KeyNameFromVk(cfg.smooth_aim_secondary_key);
            }
            std::cout << "] to smoothly lock onto a target." << std::endl;
        }
        else {
            std::cout << "    [Auto Move] Hold-to-aim is OFF; target lock moves without a mouse button." << std::endl;
        }
        std::cout << "    [Press F9] to test active mouse backend (+20px then -20px)." << std::endl;
        std::cout << "    [Press V in visualization window] to toggle visualization ON/OFF." << std::endl;
        std::cout << "    [Press ESC in visualization window] to quit." << std::endl;
    }

