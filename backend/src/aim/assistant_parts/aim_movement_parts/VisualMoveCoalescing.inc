    bool coalesceVisualMouseMove(int input_x, int input_y, bool bypass, int& output_x, int& output_y) {
        output_x = input_x;
        output_y = input_y;
        resetVisualMoveCoalescer();
        return input_x != 0 || input_y != 0;
    }
