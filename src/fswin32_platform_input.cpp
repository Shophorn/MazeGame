/*
Leo Tamminen

Windows platform input functions
*/

// Todo(Leo): These buttonstate things are implementation details and may even vary per platform,
// so consider moving them elsewhere. Maybe only expose is_clicked etc. as
enum InputButtonState : s8
{
	InputButtonState_is_up,
	InputButtonState_went_down,
	InputButtonState_is_down,
	InputButtonState_went_up,
};

struct PlatformInput
{
	f32 axes[InputAxisCount];
	InputButtonState buttons [InputButtonCount];
	v2  mousePosition;
};


f32 input_axis_get_value(PlatformInput * input, PlatformInputAxis axis)
{
	f32 value = input->axes[axis];
	return value;
}

v2 input_cursor_get_position(PlatformInput * input)
{
	return input->mousePosition;
}


internal bool32 input_button_went_down(PlatformInput * input, PlatformInputButton button)
{
	InputButtonState state = input->buttons[button];
	return state == InputButtonState_went_down;
}

internal bool32 input_button_is_down (PlatformInput * input, PlatformInputButton button)
{
	InputButtonState state = input->buttons[button];
	return (state == InputButtonState_is_down) || (state == InputButtonState_went_down);
}

internal bool32 input_button_went_up (PlatformInput * input, PlatformInputButton button)
{
	InputButtonState state = input->buttons[button];
	return state == InputButtonState_went_up;
}

internal void fswin32_input_update_button_state(InputButtonState & buttonState, bool32 isDown)
{
	switch(buttonState)
	{
		case InputButtonState_is_up:
			buttonState = isDown ? InputButtonState_went_down : InputButtonState_is_up;
			break;

		case InputButtonState_went_down:
			buttonState = isDown ? InputButtonState_is_down : InputButtonState_went_up;
			break;

		case InputButtonState_is_down:
			buttonState = isDown ? InputButtonState_is_down : InputButtonState_went_up;
			break;

		case InputButtonState_went_up:
			buttonState = isDown ? InputButtonState_went_down : InputButtonState_is_up;
			break;

	}
}


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
	v2 move = { 
		xinput_convert_joystick_value(xinput->Gamepad.sThumbLX),
		xinput_convert_joystick_value(xinput->Gamepad.sThumbLY)};
	move = clamp_length_v2(move, 1.0f);

	v2 look = {
		xinput_convert_joystick_value(xinput->Gamepad.sThumbRX),
		xinput_convert_joystick_value(xinput->Gamepad.sThumbRY)};
	look = clamp_length_v2(look, 1.0f);

	input->axes[InputAxis_move_x] = move.x;//xinput_convert_joystick_value(xinput->Gamepad.sThumbLX);
	input->axes[InputAxis_move_y] = move.y;//xinput_convert_joystick_value(xinput->Gamepad.sThumbLY);
	input->axes[InputAxis_look_x] = look.x;//xinput_convert_joystick_value(xinput->Gamepad.sThumbRX);
	input->axes[InputAxis_look_y] = look.y;//xinput_convert_joystick_value(xinput->Gamepad.sThumbRY);

	u16 pressedButtons = xinput->Gamepad.wButtons;
	auto is_down = [pressedButtons](u32 button) -> bool32
	{
		bool32 isDown = (pressedButtons & button) != 0;
		return isDown;
	};


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

/* Todo(Leo): this may not be thread-safe because windows callbacks can probably come anytime.
Maybe make 2 structs and toggle between them, so when we start reading, we use the other one for writing*/
internal void
update_keyboard_input(PlatformInput * platformInput, Win32KeyboardInput * keyboardInput)
{
	auto bool_to_float = [](bool32 value) { return value ? 1.0f : 0.1f; };

	platformInput->axes[InputAxis_move_x] = bool_to_float(keyboardInput->right) - bool_to_float(keyboardInput->left);
	platformInput->axes[InputAxis_move_y] = bool_to_float(keyboardInput->up) - bool_to_float(keyboardInput->down);

	platformInput->axes[InputAxis_look_x] = 0;
	platformInput->axes[InputAxis_look_y] = 0;

	// platformInput->move = 
	// {
	// 	.x = bool_to_float(keyboardInput->right) - bool_to_float(keyboardInput->left),
	// 	.y = bool_to_float(keyboardInput->up) - bool_to_float(keyboardInput->down),
	// };
	// platformInput->look = {};

	fswin32_input_update_button_state(platformInput->buttons[InputButton_start],     keyboardInput->escape);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_select],    false);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_zoom_in],   false);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_zoom_out],  false);

	fswin32_input_update_button_state(platformInput->buttons[InputButton_dpad_left],      keyboardInput->left);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_dpad_right],     keyboardInput->right);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_dpad_down],      keyboardInput->down);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_dpad_up],        keyboardInput->up);
}

internal void
update_unused_input(PlatformInput * platformInput)
{
	platformInput->axes[InputAxis_move_x] = 0;
	platformInput->axes[InputAxis_move_y] = 0;

	platformInput->axes[InputAxis_look_x] = 0;
	platformInput->axes[InputAxis_look_y] = 0;

	fswin32_input_update_button_state(platformInput->buttons[InputButton_start],     false);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_select],    false);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_zoom_in],   false);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_zoom_out],  false);

	fswin32_input_update_button_state(platformInput->buttons[InputButton_dpad_left],      false);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_dpad_right],     false);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_dpad_down],      false);
	fswin32_input_update_button_state(platformInput->buttons[InputButton_dpad_up],        false);
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
