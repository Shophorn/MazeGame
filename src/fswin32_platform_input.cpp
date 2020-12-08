/*
Leo Tamminen

Windows platform input functions
*/

enum InputButtonState : s8
{
	InputButtonState_is_up,
	InputButtonState_went_down,
	InputButtonState_is_down,
	InputButtonState_went_up,
};

struct PlatformInput
{
	f32 				axes[InputAxisCount];
	InputButtonState 	buttons[InputButtonCount];

	v2 mousePosition;

	bool32 gamepadInputUsed;
	bool32 mouseAndKeyboardInputUsed;
};

using Win32PlatformInput = PlatformInput;

f32 input_axis_get_value(Win32PlatformInput * input, PlatformInputAxis axis)
{
	f32 value = input->axes[axis];
	return value;
}

v2 input_cursor_get_position(Win32PlatformInput * input)
{
	return input->mousePosition;
}

internal bool32 input_button_went_down(Win32PlatformInput * input, PlatformInputButton button)
{
	InputButtonState state = input->buttons[button];
	return (state == InputButtonState_went_down);
}

internal bool32 input_button_is_down (Win32PlatformInput * input, PlatformInputButton button)
{
	InputButtonState state = input->buttons[button];
	return (state == InputButtonState_is_down) || (state == InputButtonState_went_down);
}

internal bool32 input_button_went_up (Win32PlatformInput * input, PlatformInputButton button)
{
	InputButtonState state = input->buttons[button];
	return state == InputButtonState_went_up;
}

internal bool32 input_is_device_used (Win32PlatformInput * input, PlatformInputDevice device)
{
	switch(device)
	{
		case PlatformInputDevice_gamepad: 				return input->gamepadInputUsed;
		case PlatformInputDevice_mouse_and_keyboard: 	return input->mouseAndKeyboardInputUsed;
	}
	return false;
}

internal void fswin32_input_update_button_state(InputButtonState & buttonState, bool32 isDown)
{
	switch(buttonState)
	{
		case InputButtonState_is_up:
		case InputButtonState_went_up:
			buttonState = isDown ? InputButtonState_went_down : InputButtonState_is_up;
			break;

		case InputButtonState_went_down:
		case InputButtonState_is_down:
			buttonState = isDown ? InputButtonState_is_down : InputButtonState_went_up; 
			break;
	}
}

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
update_controller_input(Win32PlatformInput * input, XINPUT_STATE * xinput)
{
	v2 move = { 
		xinput_convert_joystick_value(xinput->Gamepad.sThumbLX),
		xinput_convert_joystick_value(xinput->Gamepad.sThumbLY)};
	move = clamp_length_v2(move, 1.0f);

	v2 look = {
		xinput_convert_joystick_value(xinput->Gamepad.sThumbRX),
		xinput_convert_joystick_value(xinput->Gamepad.sThumbRY)};
	look = clamp_length_v2(look, 1.0f);

	input->axes[InputAxis_move_x] = move.x;
	input->axes[InputAxis_move_y] = move.y;
	input->axes[InputAxis_look_x] = look.x;
	input->axes[InputAxis_look_y] = look.y;

	u16 pressedButtons = xinput->Gamepad.wButtons;
	auto is_down = [pressedButtons](u32 button) -> bool32
	{
		bool32 isDown = (pressedButtons & button) != 0;
		return isDown;
	};

	// Todo(Leo): remove these too like keyboard stuff when we find controller
	fswin32_input_update_button_state(input->buttons[InputButton_nintendo_a],  is_down(NINTENDO_GAMEPAD_A));
	fswin32_input_update_button_state(input->buttons[InputButton_nintendo_b],  is_down(NINTENDO_GAMEPAD_B));
	fswin32_input_update_button_state(input->buttons[InputButton_nintendo_y],  is_down(NINTENDO_GAMEPAD_Y));
	fswin32_input_update_button_state(input->buttons[InputButton_nintendo_x],  is_down(NINTENDO_GAMEPAD_X));

	fswin32_input_update_button_state(input->buttons[InputButton_start],       is_down(XINPUT_GAMEPAD_START));
	fswin32_input_update_button_state(input->buttons[InputButton_select],      is_down(XINPUT_GAMEPAD_BACK));
	fswin32_input_update_button_state(input->buttons[InputButton_zoom_in],     is_down(XINPUT_GAMEPAD_LEFT_SHOULDER));
	fswin32_input_update_button_state(input->buttons[InputButton_zoom_out],    is_down(XINPUT_GAMEPAD_RIGHT_SHOULDER));

	fswin32_input_update_button_state(input->buttons[InputButton_dpad_left],   is_down(XINPUT_GAMEPAD_DPAD_LEFT));
	fswin32_input_update_button_state(input->buttons[InputButton_dpad_right],  is_down(XINPUT_GAMEPAD_DPAD_RIGHT));
	fswin32_input_update_button_state(input->buttons[InputButton_dpad_down],   is_down(XINPUT_GAMEPAD_DPAD_DOWN));
	fswin32_input_update_button_state(input->buttons[InputButton_dpad_up],     is_down(XINPUT_GAMEPAD_DPAD_UP));
}

static PlatformInputButton win32_input_button_map(WPARAM key)
{
	switch(key)
	{
		case 'W': return InputButton_keyboard_w;
		case 'A': return InputButton_keyboard_a;
		case 'S': return InputButton_keyboard_s;
		case 'D': return InputButton_keyboard_d;

		case VK_LEFT: 	return InputButton_keyboard_left;
		case VK_RIGHT: 	return InputButton_keyboard_right;
		case VK_UP:		return InputButton_keyboard_up;
		case VK_DOWN: 	return InputButton_keyboard_down;

		case VK_RETURN: return InputButton_keyboard_enter;
		case VK_ESCAPE: return InputButton_keyboard_escape;
		case VK_BACK: 	return InputButton_keyboard_backspace;
		case VK_SPACE: 	return InputButton_keyboard_space;

		case VK_F1: return InputButton_keyboard_f1;
		case VK_F2: return InputButton_keyboard_f2;
		case VK_F3: return InputButton_keyboard_f3;
		case VK_F4: return InputButton_keyboard_f4;

		default:
			return InputButton_invalid;
	}
}