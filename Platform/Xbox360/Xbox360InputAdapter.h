#ifndef STK360_XBOX360_INPUT_ADAPTER_H
#define STK360_XBOX360_INPUT_ADAPTER_H

/*
 * Xbox 360 controller state translation for SuperTuxKart.
 *
 * This file deliberately has no XDK or SDL dependency. The platform polling
 * layer reads XINPUT_STATE (or an equivalent controller API) and fills
 * XInputState below. The adapter then exposes SDL_GameController-compatible
 * button and axis ordering for the STK input bridge.
 *
 * C++03 compatible. Stick deadzones and user remapping remain STK policy.
 */

namespace stk360
{
    enum
    {
        X360_DPAD_UP        = 0x0001,
        X360_DPAD_DOWN      = 0x0002,
        X360_DPAD_LEFT      = 0x0004,
        X360_DPAD_RIGHT     = 0x0008,
        X360_START          = 0x0010,
        X360_BACK           = 0x0020,
        X360_LEFT_THUMB     = 0x0040,
        X360_RIGHT_THUMB    = 0x0080,
        X360_LEFT_SHOULDER  = 0x0100,
        X360_RIGHT_SHOULDER = 0x0200,
        X360_A              = 0x1000,
        X360_B              = 0x2000,
        X360_X              = 0x4000,
        X360_Y              = 0x8000
    };

    enum
    {
        STK_BUTTON_A = 0,
        STK_BUTTON_B,
        STK_BUTTON_X,
        STK_BUTTON_Y,
        STK_BUTTON_BACK,
        STK_BUTTON_GUIDE,
        STK_BUTTON_START,
        STK_BUTTON_LEFT_STICK,
        STK_BUTTON_RIGHT_STICK,
        STK_BUTTON_LEFT_SHOULDER,
        STK_BUTTON_RIGHT_SHOULDER,
        STK_BUTTON_DPAD_UP,
        STK_BUTTON_DPAD_RIGHT,
        STK_BUTTON_DPAD_DOWN,
        STK_BUTTON_DPAD_LEFT,
        STK_BUTTON_COUNT
    };

    enum
    {
        STK_AXIS_LEFT_X = 0,
        STK_AXIS_LEFT_Y,
        STK_AXIS_RIGHT_X,
        STK_AXIS_RIGHT_Y,
        STK_AXIS_LEFT_TRIGGER,
        STK_AXIS_RIGHT_TRIGGER,
        STK_AXIS_COUNT
    };

    struct XInputState
    {
        unsigned short buttons;
        unsigned char left_trigger;
        unsigned char right_trigger;
        short thumb_lx;
        short thumb_ly;
        short thumb_rx;
        short thumb_ry;
    };

    struct STKControllerState
    {
        unsigned int buttons;
        short axes[STK_AXIS_COUNT];
    };

    inline short triggerToAxis(unsigned char value)
    {
        return (short)(((unsigned int)value * 32767u) / 255u);
    }

    inline void translateXInput(const XInputState& source,
                                STKControllerState* destination)
    {
        if (!destination)
            return;

        unsigned int b = 0;
        if (source.buttons & X360_A)              b |= 1u << STK_BUTTON_A;
        if (source.buttons & X360_B)              b |= 1u << STK_BUTTON_B;
        if (source.buttons & X360_X)              b |= 1u << STK_BUTTON_X;
        if (source.buttons & X360_Y)              b |= 1u << STK_BUTTON_Y;
        if (source.buttons & X360_BACK)           b |= 1u << STK_BUTTON_BACK;
        if (source.buttons & X360_START)          b |= 1u << STK_BUTTON_START;
        if (source.buttons & X360_LEFT_THUMB)     b |= 1u << STK_BUTTON_LEFT_STICK;
        if (source.buttons & X360_RIGHT_THUMB)    b |= 1u << STK_BUTTON_RIGHT_STICK;
        if (source.buttons & X360_LEFT_SHOULDER)  b |= 1u << STK_BUTTON_LEFT_SHOULDER;
        if (source.buttons & X360_RIGHT_SHOULDER) b |= 1u << STK_BUTTON_RIGHT_SHOULDER;
        if (source.buttons & X360_DPAD_UP)        b |= 1u << STK_BUTTON_DPAD_UP;
        if (source.buttons & X360_DPAD_RIGHT)     b |= 1u << STK_BUTTON_DPAD_RIGHT;
        if (source.buttons & X360_DPAD_DOWN)      b |= 1u << STK_BUTTON_DPAD_DOWN;
        if (source.buttons & X360_DPAD_LEFT)      b |= 1u << STK_BUTTON_DPAD_LEFT;

        destination->buttons = b;
        destination->axes[STK_AXIS_LEFT_X] = source.thumb_lx;
        destination->axes[STK_AXIS_LEFT_Y] = (short)-source.thumb_ly;
        destination->axes[STK_AXIS_RIGHT_X] = source.thumb_rx;
        destination->axes[STK_AXIS_RIGHT_Y] = (short)-source.thumb_ry;
        destination->axes[STK_AXIS_LEFT_TRIGGER] = triggerToAxis(source.left_trigger);
        destination->axes[STK_AXIS_RIGHT_TRIGGER] = triggerToAxis(source.right_trigger);
    }
}

#endif
