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

struct PlayerInput
{
	v2 		movement;
	bool8 	jump;
	bool8 	crouch;
	bool8 	interact;
	bool8 	pickupOrDrop;
};

static PlayerInput player_input_from_platform_input(PlatformInput * platformInput)
{
	PlayerInput playerInput = {};

	if (input_is_device_used(platformInput, InputDevice_gamepad))
	{

		playerInput.movement.x 	= input_axis_get_value(platformInput, InputAxis_move_x);
		playerInput.movement.y 	= input_axis_get_value(platformInput, InputAxis_move_y);
	
		constexpr InputButton jumpButton 			= InputButton_nintendo_x;
		constexpr InputButton crouchButton 			= InputButton_invalid;
		constexpr InputButton interactButton 		= InputButton_nintendo_b;
		constexpr InputButton pickupOrDropButton 	= InputButton_nintendo_a;

		playerInput.jump			= input_button_went_down(platformInput, jumpButton);
		playerInput.crouch 			= input_button_went_down(platformInput, crouchButton);
		playerInput.interact 		= input_button_went_down(platformInput, interactButton);
		playerInput.pickupOrDrop	= input_button_went_down(platformInput, pickupOrDropButton);
	}
	else
	{
		playerInput.movement.x 	= input_button_is_down(platformInput, InputButton_wasd_right)
								- input_button_is_down(platformInput, InputButton_wasd_left);

		playerInput.movement.y 	= input_button_is_down(platformInput, InputButton_wasd_up)
								- input_button_is_down(platformInput, InputButton_wasd_down);

		constexpr InputButton jumpButton 			= InputButton_keyboard_space;
		constexpr InputButton crouchButton 			= InputButton_invalid;
		constexpr InputButton interactButton 		= InputButton_mouse_0;
		constexpr InputButton pickupOrDropButton 	= InputButton_mouse_1;

		playerInput.jump			= input_button_went_down(platformInput, InputButton_keyboard_space);
		playerInput.crouch 			= input_button_went_down(platformInput, crouchButton);
		playerInput.interact 		= input_button_went_down(platformInput, InputButton_mouse_0);
		playerInput.pickupOrDrop	= input_button_went_down(platformInput, InputButton_mouse_1);
	}

	playerInput.movement 	= clamp_length_v2(playerInput.movement, 1);

	

	return playerInput;
}

CharacterInput update_player_input(	PlayerInputState & 	state,
									Camera & 			playerCamera,
									PlayerInput const & input)
{
	v3 viewForward 		= get_forward(&playerCamera);
	viewForward.z 		= 0;
	viewForward 		= v3_normalize(viewForward);
	v3 viewRight 		= v3_cross(viewForward, v3_up);

	v3 worldSpaceInput 	= viewRight * input.movement.x
						+ viewForward * input.movement.y;

	// Note(Leo): As long as we don't trigger these anywhere else, this works
	state.events 				= {};
	state.events.pickupOrDrop	= input.pickupOrDrop;
	state.events.interact 		= input.interact;

	CharacterInput result 	= {worldSpaceInput, input.jump, input.crouch};
	result.climbInput 		= input.movement;
	return result;
} 