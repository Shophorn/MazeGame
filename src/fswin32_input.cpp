/*
Leo Tamminen

Windows platform input functions
*/

struct Win32KeyboardInput
{
    bool32 left, right, up, down;
    bool32 space;
    bool32 enter, escape;

    v2      mousePosition;
    f32     mouseScroll;
    bool32  leftMouseButtonDown;
};

struct Win32XInput
{
    DWORD xinputLastPacketNumber;
};

// Todo(Leo): hack to get two controller on single pc and use only 1st controller when online
global_variable int globalXinputControllerIndex;

// XInput things.
using XInputGetStateFunc = decltype(XInputGetState);
internal XInputGetStateFunc * xinput_get_state;

using XInputSetStateFunc = decltype(XInputSetState);
internal XInputSetStateFunc * xinput_set_state;

internal void
load_xinput()
{
    // TODO(Leo): check other xinput versions and pick most proper
    HMODULE xinputModule = LoadLibraryA("xinput1_3.dll");

    if (xinputModule != nullptr)
    {
        xinput_get_state = reinterpret_cast<XInputGetStateFunc *> (GetProcAddress(xinputModule, "XInputGetState"));
        xinput_set_state = reinterpret_cast<XInputSetStateFunc *> (GetProcAddress(xinputModule, "XInputSetState"));
    }
    else
    {
        xinput_get_state = [](DWORD, XINPUT_STATE*){ return (DWORD)ERROR_DEVICE_NOT_CONNECTED; };
        xinput_set_state = [](DWORD, XINPUT_VIBRATION*){ return (DWORD)ERROR_DEVICE_NOT_CONNECTED; };
    }
}

internal float
xinput_convert_joystick_value(s16 value)
{
    float deadZone = 0.2f;
    float result = static_cast<float>(value) / max_value_s16;

    float sign = sign_f32(result);
    result *= sign; // cheap abs()
    result -= deadZone;
    result = max_f32(0.0f, result);
    result /= (1.0f - deadZone);
    result *= sign;

    return result;
}

internal bool32
xinput_is_used(Win32XInput & currentInputState, XINPUT_STATE & xinputState)
{
    bool32 used = (xinputState.Gamepad.wButtons != 0)
                || (xinputState.Gamepad.bLeftTrigger != 0)
                || (xinputState.Gamepad.bRightTrigger != 0)
                || (abs_f32(xinput_convert_joystick_value(xinputState.Gamepad.sThumbLX)) > 0.0f)
                || (abs_f32(xinput_convert_joystick_value(xinputState.Gamepad.sThumbLY)) > 0.0f)
                || (abs_f32(xinput_convert_joystick_value(xinputState.Gamepad.sThumbRX)) > 0.0f)
                || (abs_f32(xinput_convert_joystick_value(xinputState.Gamepad.sThumbRY)) > 0.0f);

    used = used || currentInputState.xinputLastPacketNumber != xinputState.dwPacketNumber;
    currentInputState.xinputLastPacketNumber = xinputState.dwPacketNumber;

    return used;
}

enum {
    NINTENDO_GAMEPAD_X = XINPUT_GAMEPAD_Y,
    NINTENDO_GAMEPAD_Y = XINPUT_GAMEPAD_X,
    NINTENDO_GAMEPAD_B = XINPUT_GAMEPAD_A,
    NINTENDO_GAMEPAD_A = XINPUT_GAMEPAD_B,
};

internal void
update_controller_input(PlatformInput * input, XINPUT_STATE * xinput)
{
    input->move = { 
        xinput_convert_joystick_value(xinput->Gamepad.sThumbLX),
        xinput_convert_joystick_value(xinput->Gamepad.sThumbLY)};
    input->move = clamp_length_v2(input->move, 1.0f);

    input->look = {
        xinput_convert_joystick_value(xinput->Gamepad.sThumbRX),
        xinput_convert_joystick_value(xinput->Gamepad.sThumbRY)};
    input->look = clamp_length_v2(input->look, 1.0f);

    u16 pressedButtons = xinput->Gamepad.wButtons;
    auto is_down = [pressedButtons](u32 button) -> bool32
    {
        bool32 isDown = (pressedButtons & button) != 0;
        return isDown;
    };

    input->jump     = update_button_state(input->jump,      is_down(XINPUT_GAMEPAD_A));
    input->confirm  = update_button_state(input->confirm,   is_down(XINPUT_GAMEPAD_B));
    input->interact = input->confirm;

    input->A = update_button_state(input->A, is_down(NINTENDO_GAMEPAD_A));
    input->B = update_button_state(input->B, is_down(NINTENDO_GAMEPAD_B));
    input->Y = update_button_state(input->Y, is_down(NINTENDO_GAMEPAD_Y));
    input->X = update_button_state(input->X, is_down(NINTENDO_GAMEPAD_X));


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
update_keyboard_input(PlatformInput * platformInput, Win32KeyboardInput * keyboardInput)
{
    auto bool_to_float = [](bool32 value) { return value ? 1.0f : 0.1f; };

    platformInput->move = 
    {
        .x = bool_to_float(keyboardInput->right) - bool_to_float(keyboardInput->left),
        .y = bool_to_float(keyboardInput->up) - bool_to_float(keyboardInput->down),
    };
    platformInput->look = {};

    platformInput->jump     = update_button_state(platformInput->jump,      keyboardInput->space);
    platformInput->confirm  = update_button_state(platformInput->confirm,   keyboardInput->enter);
    platformInput->interact = update_button_state(platformInput->interact,  false);

    platformInput->start    = update_button_state(platformInput->start,     keyboardInput->escape);
    platformInput->select   = update_button_state(platformInput->select,    false);
    platformInput->zoomIn   = update_button_state(platformInput->zoomIn,    false);
    platformInput->zoomOut  = update_button_state(platformInput->zoomOut,   false);

    platformInput->left     = update_button_state(platformInput->left,      keyboardInput->left);
    platformInput->right    = update_button_state(platformInput->right,     keyboardInput->right);
    platformInput->down     = update_button_state(platformInput->down,      keyboardInput->down);
    platformInput->up       = update_button_state(platformInput->up,        keyboardInput->up);
}

internal void
update_unused_input(PlatformInput * platformInput)
{
    platformInput->move = {};
    platformInput->look = {};

    platformInput->jump     = update_button_state(platformInput->jump,      false);
    platformInput->confirm  = update_button_state(platformInput->confirm,   false);
    platformInput->interact = platformInput->confirm;

    platformInput->start    = update_button_state(platformInput->start,     false);
    platformInput->select   = update_button_state(platformInput->select,    false);
    platformInput->zoomIn   = update_button_state(platformInput->zoomIn,    false);
    platformInput->zoomOut  = update_button_state(platformInput->zoomOut,   false);

    platformInput->left     = update_button_state(platformInput->left,      false);
    platformInput->right    = update_button_state(platformInput->right,     false);
    platformInput->down     = update_button_state(platformInput->down,      false);
    platformInput->up       = update_button_state(platformInput->up,        false);
}

internal void
process_keyboard_input(Win32KeyboardInput & keyboardInput, WPARAM keycode, bool32 isDown)
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
            keyboardInput.left = isDown;
            break;

        case VKEY_D:
        case VK_RIGHT:
            keyboardInput.right = isDown;
            break;

        case VKEY_W:
        case VK_UP:
            keyboardInput.up = isDown;
            break;

        case VKEY_S:
        case VK_DOWN:
            keyboardInput.down = isDown;
            break;

        case VK_RETURN:
            keyboardInput.enter = isDown;
            break;

        case VK_ESCAPE:
            keyboardInput.escape = isDown;
            break;

        case VK_SPACE:
            keyboardInput.space = isDown;
            break;
    }
}
