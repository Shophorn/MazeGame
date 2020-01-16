/*=============================================================================
Leo Tamminen
shophorn @ internet

Windows platform input functions
=============================================================================*/

// XInput things.
using XInputGetStateFunc = decltype(XInputGetState);
internal XInputGetStateFunc * XInputGetState_;
DWORD WINAPI XInputGetStateStub (DWORD, XINPUT_STATE *)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

/*Todo(Leo): see if we should get rid of #define below by creating
a proper named struct/namespace to hold this pointer */
#define XInputGetState XInputGetState_

using XInputSetStateFunc = decltype(XInputSetState);
internal XInputSetStateFunc * XInputSetState_;
DWORD WINAPI XInputSetStateStub (DWORD, XINPUT_VIBRATION *)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

/*Todo(Leo): see if we should get rid of #define below by creating
a proper named struct/namespace to hold this pointer */
#define XInputSetState XInputSetState_

internal void
load_xinput()
{
    // TODO(Leo): check other xinput versions and pick most proper
    HMODULE xinputModule = LoadLibraryA("xinput1_3.dll");

    if (xinputModule != nullptr)
    {
        XInputGetState_ = reinterpret_cast<XInputGetStateFunc *> (GetProcAddress(xinputModule, "XInputGetState"));
        XInputSetState_ = reinterpret_cast<XInputSetStateFunc *> (GetProcAddress(xinputModule, "XInputSetState"));
    }
    else
    {
        XInputGetState_ = XInputGetStateStub;
        XInputSetState_ = XInputSetStateStub;
    }
}

internal float
xinput_convert_joystick_value(s16 value)
{
    float deadZone = 0.2f;
    float result = static_cast<float>(value) / maxValue<s16>;

    float sign = Sign(result);
    result *= sign; // cheap abs()
    result -= deadZone;
    result = Max(0.0f, result);
    result /= (1.0f - deadZone);
    result *= sign;

    return result;
}

internal bool32
xinput_is_used(winapi::State * winState, XINPUT_STATE * xinputState)
{
    bool32 used = (xinputState->Gamepad.wButtons != 0)
                || (xinputState->Gamepad.bLeftTrigger != 0)
                || (xinputState->Gamepad.bRightTrigger != 0)
                || (Abs(xinput_convert_joystick_value(xinputState->Gamepad.sThumbLX)) > 0.0f)
                || (Abs(xinput_convert_joystick_value(xinputState->Gamepad.sThumbLY)) > 0.0f)
                || (Abs(xinput_convert_joystick_value(xinputState->Gamepad.sThumbRX)) > 0.0f)
                || (Abs(xinput_convert_joystick_value(xinputState->Gamepad.sThumbRY)) > 0.0f);

    used = used || winState->xinputLastPacketNumber != xinputState->dwPacketNumber;
    winState->xinputLastPacketNumber = xinputState->dwPacketNumber;

    return used;
}

internal void
update_controller_input(game::Input * input, XINPUT_STATE * xinput)
{
    input->move = { 
        xinput_convert_joystick_value(xinput->Gamepad.sThumbLX),
        xinput_convert_joystick_value(xinput->Gamepad.sThumbLY)};
    input->move = vector::clamp_length(input->move, 1.0f);

    input->look = {
        xinput_convert_joystick_value(xinput->Gamepad.sThumbRX),
        xinput_convert_joystick_value(xinput->Gamepad.sThumbRY)};
    input->look = vector::clamp_length(input->look, 1.0f);

    u16 pressedButtons = xinput->Gamepad.wButtons;
    auto is_down = [pressedButtons](u32 button) -> bool32
    {
        bool32 isDown = (pressedButtons & button) != 0;
        return isDown;
    };

    input->jump     = update_button_state(input->jump,      is_down(XINPUT_GAMEPAD_A));
    input->confirm  = update_button_state(input->confirm,   is_down(XINPUT_GAMEPAD_B));
    input->interact = input->confirm;

    input->start    = update_button_state(input->start,     is_down(XINPUT_GAMEPAD_START));
    input->select   = update_button_state(input->select,    is_down(XINPUT_GAMEPAD_BACK));
    input->zoomIn   = update_button_state(input->zoomIn,    is_down(XINPUT_GAMEPAD_LEFT_SHOULDER));
    input->zoomOut  = update_button_state(input->zoomOut,   is_down(XINPUT_GAMEPAD_RIGHT_SHOULDER));

    input->left     = update_button_state(input->left,      is_down(XINPUT_GAMEPAD_DPAD_LEFT));
    input->right    = update_button_state(input->right,     is_down(XINPUT_GAMEPAD_DPAD_RIGHT));
    input->down     = update_button_state(input->down,      is_down(XINPUT_GAMEPAD_DPAD_DOWN));
    input->up       = update_button_state(input->up,        is_down(XINPUT_GAMEPAD_DPAD_UP));
}

/* Todo(Leo): this may not be thread-safe because windows callbacks can probably come anytime.
Maybe make 2 structs and toggle between them, so when we start reading, we use the other one for writing*/
internal void
update_keyboard_input(game::Input * gameInput, winapi::KeyboardInput * keyboardInput)
{
    auto bool_to_float = [](bool32 value) { return value ? 1.0f : 0.1f; };

    gameInput->move = 
    {
        .x = bool_to_float(keyboardInput->right) - bool_to_float(keyboardInput->left),
        .y = bool_to_float(keyboardInput->up) - bool_to_float(keyboardInput->down),
    };
    gameInput->look = {};

    gameInput->jump     = update_button_state(gameInput->jump,      keyboardInput->space);
    gameInput->confirm  = update_button_state(gameInput->confirm,   keyboardInput->enter);
    gameInput->interact = update_button_state(gameInput->interact,  false);

    gameInput->start    = update_button_state(gameInput->start,     keyboardInput->escape);
    gameInput->select   = update_button_state(gameInput->select,    false);
    gameInput->zoomIn   = update_button_state(gameInput->zoomIn,    false);
    gameInput->zoomOut  = update_button_state(gameInput->zoomOut,   false);

    gameInput->left     = update_button_state(gameInput->left,      keyboardInput->left);
    gameInput->right    = update_button_state(gameInput->right,     keyboardInput->right);
    gameInput->down     = update_button_state(gameInput->down,      keyboardInput->down);
    gameInput->up       = update_button_state(gameInput->up,        keyboardInput->up);
}

internal void
update_unused_input(game::Input * gameInput)
{
    gameInput->move = {};
    gameInput->look = {};

    gameInput->jump     = update_button_state(gameInput->jump,      false);
    gameInput->confirm  = update_button_state(gameInput->confirm,   false);
    gameInput->interact = gameInput->confirm;

    gameInput->start    = update_button_state(gameInput->start,     false);
    gameInput->select   = update_button_state(gameInput->select,    false);
    gameInput->zoomIn   = update_button_state(gameInput->zoomIn,    false);
    gameInput->zoomOut  = update_button_state(gameInput->zoomOut,   false);

    gameInput->left     = update_button_state(gameInput->left,      false);
    gameInput->right    = update_button_state(gameInput->right,     false);
    gameInput->down     = update_button_state(gameInput->down,      false);
    gameInput->up       = update_button_state(gameInput->up,        false);
}

internal void
process_keyboard_input(winapi::State * state, WPARAM keycode, bool32 isDown)
{
    enum : WPARAM
    {
        VKEY_A = 0x41,
        VKEY_D = 0x44,
        VKEY_S = 0x53,
        VKEY_W = 0x57
    };

    switch(keycode)
    {
        case VKEY_A:
        case VK_LEFT:
            state->keyboardInput.left = isDown;
            break;

        case VKEY_D:
        case VK_RIGHT:
            state->keyboardInput.right = isDown;
            break;

        case VKEY_W:
        case VK_UP:
            state->keyboardInput.up = isDown;
            break;

        case VKEY_S:
        case VK_DOWN:
            state->keyboardInput.down = isDown;
            break;

        case VK_RETURN:
            state->keyboardInput.enter = isDown;
            break;

        case VK_ESCAPE:
            state->keyboardInput.escape = isDown;
            break;

        case VK_SPACE:
            state->keyboardInput.space = isDown;
            break;
    }

    state->keyboardInputIsUsed = true;
}
