/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/

struct PlayerInputState
{
	bool32 	crouching;
	// s32 	inputArrayIndex;

	struct
	{
		bool32 pickupOrDrop	= false;
		bool32 interact		= false;
	} events;
};


CharacterInput update_player_input(	PlayerInputState & 	state,
									Camera & 			playerCamera,
									PlatformInput * 	platformInput)
{
	v3 viewForward 		= get_forward(&playerCamera);
	viewForward.z 		= 0;
	viewForward 		= normalize_v3(viewForward);
	v3 viewRight 		= cross_v3(viewForward, v3_up);

	constexpr PlatformInputButton interactButton 		= InputButton_nintendo_b;
	constexpr PlatformInputButton pickupOrDropButton 	= InputButton_nintendo_a;

	v2 inputComponents;

	if (input_is_device_used(platformInput, PlatformInputDevice_gamepad))
	{
		inputComponents.x 	= input_axis_get_value(platformInput, InputAxis_move_x);
		inputComponents.y 	= input_axis_get_value(platformInput, InputAxis_move_y);
	}
	else
	{
		inputComponents.x = input_button_is_down(platformInput, InputButton_keyboard_wasd_right)
								- input_button_is_down(platformInput, InputButton_keyboard_wasd_left);

		inputComponents.y = input_button_is_down(platformInput, InputButton_keyboard_wasd_up)
								- input_button_is_down(platformInput, InputButton_keyboard_wasd_down);
	}

	inputComponents 	= clamp_length_v2(inputComponents, 1);
	v3 worldSpaceInput 	= viewRight * inputComponents.x + viewForward * inputComponents.y;
	bool32 jumpInput 	= input_button_went_down(platformInput, InputButton_nintendo_x);
	bool32 crouchInput 	= false;

	state.events = {};

	// Note(Leo): As long as we don't trigger these anywhere else, this works
	state.events.pickupOrDrop	= input_button_went_down(platformInput, pickupOrDropButton)
								|| input_button_went_down(platformInput, InputButton_mouse_1);

	state.events.interact 		= input_button_went_down(platformInput, interactButton)
								|| input_button_went_down(platformInput, InputButton_mouse_0);

	return {worldSpaceInput, jumpInput, crouchInput};
} 