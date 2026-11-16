#include "../Xbox360/Xbox360InputAdapter.h"

int main()
{
    stk360::XInputState source;
    source.buttons = stk360::X360_A | stk360::X360_B |
                     stk360::X360_START | stk360::X360_DPAD_LEFT;
    source.left_trigger = 255;
    source.right_trigger = 128;
    source.thumb_lx = 1234;
    source.thumb_ly = -2345;
    source.thumb_rx = -32768;
    source.thumb_ry = 32767;

    stk360::STKControllerState result;
    stk360::translateXInput(source, &result);

    if ((result.buttons & (1u << stk360::STK_BUTTON_A)) == 0) return 1;
    if ((result.buttons & (1u << stk360::STK_BUTTON_B)) == 0) return 2;
    if ((result.buttons & (1u << stk360::STK_BUTTON_START)) == 0) return 3;
    if ((result.buttons & (1u << stk360::STK_BUTTON_DPAD_LEFT)) == 0) return 4;
    if (result.axes[stk360::STK_AXIS_LEFT_X] != 1234) return 5;
    if (result.axes[stk360::STK_AXIS_LEFT_Y] != 2345) return 6;
    if (result.axes[stk360::STK_AXIS_RIGHT_X] != -32768) return 7;
    if (result.axes[stk360::STK_AXIS_RIGHT_Y] != -32767) return 8;
    if (result.axes[stk360::STK_AXIS_LEFT_TRIGGER] != 32767) return 9;
    if (result.axes[stk360::STK_AXIS_RIGHT_TRIGGER] < 16400 ||
        result.axes[stk360::STK_AXIS_RIGHT_TRIGGER] > 16500) return 10;

    stk360::translateXInput(source, 0);
    return 0;
}
