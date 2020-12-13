/*
Leo Tamminen
Windows platform input functions
*/

/// --------- TYPE DEFINITIONS ----------------

enum Win32InputButtonState : s8
{
	InputButtonState_is_up,
	InputButtonState_went_down,
	InputButtonState_is_down,
	InputButtonState_went_up,
};

struct PlatformInput
{
	f32 					axes[InputAxisCount];
	Win32InputButtonState 	buttons[InputButtonCount];

	bool32 gamepadInputUsed;
	bool32 mouseAndKeyboardInputUsed;

	v2 mousePosition;

	decltype(XInputGetState) * xinput_get_state;
	decltype(XInputSetState) * xinput_set_state;

	s32 	xinputControllerIndex;
	DWORD 	xinputLastPacketNumber;
};

using Win32PlatformInput = PlatformInput;


/// --------- PLATFORM API FUNCTIONS --------------
// Note(Leo): These functions are declared in platform api file

f32 input_axis_get_value(Win32PlatformInput * input, InputAxis axis)
{
	f32 value = input->axes[axis];
	return value;
}

v2 input_cursor_get_position(Win32PlatformInput * input)
{
	return input->mousePosition;
}

static bool32 input_button_went_down(Win32PlatformInput * input, InputButton button)
{
	Win32InputButtonState state = input->buttons[button];
	return (state == InputButtonState_went_down);
}

static bool32 input_button_is_down (Win32PlatformInput * input, InputButton button)
{
	Win32InputButtonState state = input->buttons[button];
	return (state == InputButtonState_is_down) || (state == InputButtonState_went_down);
}

static bool32 input_button_went_up (Win32PlatformInput * input, InputButton button)
{
	Win32InputButtonState state = input->buttons[button];
	return state == InputButtonState_went_up;
}

static bool32 input_is_device_used (Win32PlatformInput * input, InputDevice device)
{
	switch(device)
	{
		case InputDevice_gamepad: 				return input->gamepadInputUsed;
		case InputDevice_mouse_and_keyboard: 	return input->mouseAndKeyboardInputUsed;
	}
	return false;
}

/// -------- FUNCTIONS FOR WIN32 USAGE --------------------

static Win32PlatformInput win32_platform_input_create()
{
	PlatformInput input = {};

	{
		// Note(Leo): We do not store this for freeing, because we would only free it on app exit. If that changes, it changes :)
		HMODULE xinputModule = LoadLibrary("xinput1_3.dll");

		if (xinputModule != nullptr)
		{
			input.xinput_get_state = reinterpret_cast<decltype(XInputGetState)*>(GetProcAddress(xinputModule, "XInputGetState"));
			input.xinput_set_state = reinterpret_cast<decltype(XInputSetState)*>(GetProcAddress(xinputModule, "XInputSetState"));
		}
		else
		{	
			input.xinput_get_state = [](DWORD, XINPUT_STATE*){ return (DWORD)ERROR_DEVICE_NOT_CONNECTED; };
			input.xinput_set_state = [](DWORD, XINPUT_VIBRATION*){ return (DWORD)ERROR_DEVICE_NOT_CONNECTED; };
		}
	}

	return input;
}

static void win32_input_reset(Win32PlatformInput & input)
{
	// Note(Leo): This must be reseted manually
	input.mouseAndKeyboardInputUsed		= false;
	input.axes[InputAxis_mouse_scroll]   = 0;

	for(auto & button : input.buttons)
	{
		if (button == InputButtonState_went_down)
		{
			button = InputButtonState_is_down;
		}

		else if (button == InputButtonState_went_up)
		{
			button = InputButtonState_is_up;
		}
	}
}

static void win32_input_update(Win32PlatformInput & input, Win32PlatformWindow const & window)
{
	HWND foregroundWindow = GetForegroundWindow();
	bool32 windowIsActive = window.hwnd == foregroundWindow;

	if (windowIsActive && window.isCursorVisible == false)
	{
		f32 cursorX = window.width / 2;
		f32 cursorY = window.height / 2;

		POINT currentCursorPosition;
		GetCursorPos(&currentCursorPosition);

		v2 mouseMovement = {currentCursorPosition.x - cursorX, currentCursorPosition.y - cursorY};

		input.axes[InputAxis_mouse_move_x] = mouseMovement.x;
		input.axes[InputAxis_mouse_move_y] = mouseMovement.y;

		SetCursorPos((s32)cursorX, (s32)cursorY);
	}
	else
	{
		// Todo(Leo): this may not be good though, but works now
		input.axes[InputAxis_mouse_move_x] = 0;
		input.axes[InputAxis_mouse_move_y] = 0;
	}

	/* Note(Leo): Only get input from a single controller, locally this is a single
	player game. Use global controller index depending on network status to test
	multiplayering */
   /* TODO [Input](Leo): Test controller and index behaviour when being connected and
	disconnected */

	XINPUT_STATE xinputState;
	bool32 xinputReceived = input.xinput_get_state(0, &xinputState) == ERROR_SUCCESS;

	bool32 xinputUsed = false;

	f32 leftJoystickHorizontal;
	f32 leftJoystickVertical;
	f32 rightJoystickHorizontal;
	f32 rightJoystickVertical;

	if (xinputReceived)
	{
		// todo(Leo): maybe this is too compressed to put this function here
		auto xinput_process_joystick_value = [](s16 value) -> f32
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
		};

		leftJoystickHorizontal 	= xinput_process_joystick_value(xinputState.Gamepad.sThumbLX);
		leftJoystickVertical 	= xinput_process_joystick_value(xinputState.Gamepad.sThumbLY);
		rightJoystickHorizontal = xinput_process_joystick_value(xinputState.Gamepad.sThumbRX);
		rightJoystickVertical 	= xinput_process_joystick_value(xinputState.Gamepad.sThumbRY);

		bool32 used = (xinputState.Gamepad.wButtons != 0)
				|| (xinputState.Gamepad.bLeftTrigger != 0)
				|| (xinputState.Gamepad.bRightTrigger != 0)
				|| (abs_f32(leftJoystickHorizontal) > 0.0f)
				|| (abs_f32(leftJoystickVertical) > 0.0f)
				|| (abs_f32(rightJoystickHorizontal) > 0.0f)
				|| (abs_f32(rightJoystickVertical) > 0.0f);
		used = used || input.xinputLastPacketNumber != xinputState.dwPacketNumber;
		input.xinputLastPacketNumber = xinputState.dwPacketNumber;

		xinputUsed = used;
	}

	if (xinputUsed)
	{
		input.gamepadInputUsed = true;
			
		v2 move = clamp_length_v2({leftJoystickHorizontal, leftJoystickVertical}, 1.0f); 
		v2 look = clamp_length_v2({rightJoystickHorizontal, rightJoystickVertical}, 1.0f); 

		input.axes[InputAxis_move_x] = move.x;
		input.axes[InputAxis_move_y] = move.y;
		input.axes[InputAxis_look_x] = look.x;
		input.axes[InputAxis_look_y] = look.y;

		auto update_button_state = [](Win32InputButtonState & buttonState, bool32 isDown)
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
		};

		// Todo(Leo): remove these too like keyboard stuff when we find controller
		update_button_state(input.buttons[InputButton_xbox_a],  (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_A));
		update_button_state(input.buttons[InputButton_xbox_b],  (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_B));
		update_button_state(input.buttons[InputButton_xbox_y],  (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_Y));
		update_button_state(input.buttons[InputButton_xbox_x],  (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_X));

		update_button_state(input.buttons[InputButton_start],       (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_START));
		update_button_state(input.buttons[InputButton_select],      (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK));
		update_button_state(input.buttons[InputButton_zoom_in],     (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER));
		update_button_state(input.buttons[InputButton_zoom_out],    (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER));

		update_button_state(input.buttons[InputButton_dpad_left],   (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT));
		update_button_state(input.buttons[InputButton_dpad_right],  (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT));
		update_button_state(input.buttons[InputButton_dpad_down],   (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN));
		update_button_state(input.buttons[InputButton_dpad_up],     (xinputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP));
	}
	else
	{
		input.gamepadInputUsed = false;
	}
}

static InputButton win32_input_button_map(WPARAM key)
{
	switch(key)
	{
		case 'W': return InputButton_keyboard_w;
		case 'A': return InputButton_keyboard_a;
		case 'S': return InputButton_keyboard_s;
		case 'D': return InputButton_keyboard_d;
		
		case '1': return InputButton_keyboard_1;
		case '2': return InputButton_keyboard_2;
		case '3': return InputButton_keyboard_3;
		case '4': return InputButton_keyboard_4;
		case '5': return InputButton_keyboard_5;
		case '6': return InputButton_keyboard_6;
		case '7': return InputButton_keyboard_7;
		case '8': return InputButton_keyboard_8;
		case '9': return InputButton_keyboard_9;
		case '0': return InputButton_keyboard_0;

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
