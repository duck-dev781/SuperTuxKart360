# Xbox 360 controller input

## Adapter status

`Platform/Xbox360/Xbox360InputAdapter.h` is a C++03-compatible translation
layer. It accepts the button/axis fields read by the Xbox 360 platform polling
code and converts them to the button and axis ordering used by SDL's standard
game-controller layout, which is the layout STK's existing controller
configuration understands.

The adapter maps:
- A, B, X, Y, Back, Start, stick clicks, shoulders, and D-pad
- Left/right sticks, with Y inverted to match SDL's positive-down convention
- Both analog triggers from 0..255 to 0..32767

The Guide button is intentionally not mapped to an in-game control. The Xbox
shell owns that button.

## Platform integration still required

The Xbox-specific source file must:
1. Poll controller slots using the Xbox 360 platform's controller API.
2. Detect connect/disconnect and stop emitting input for disconnected slots.
3. Copy the platform state fields into `stk360::XInputState`.
4. Call `translateXInput` once per poll.
5. Convert changed button/axis values into STK's existing
   `irr::SEvent::SJoystickEvent` / `InputManager` dispatch path.
6. Implement rumble through the platform API if supported.

This repository does not yet contain the Xbox SDK polling call or the final
STK `InputManager` bridge. Those must be compiled with the actual Xbox 360
toolchain; the portable adapter tests do not claim that hardware integration
is complete.

## Upstream basis

The uploaded STK source archive contains `src/input/sdl_controller.hpp` and
`src/input/sdl_controller.cpp`. The existing controller path tracks button
states, axes, hats, device connection/configuration, and rumble. This adapter
preserves a standard controller layout so the platform-specific input source
can feed the existing configuration model rather than inventing new gameplay
controls.
