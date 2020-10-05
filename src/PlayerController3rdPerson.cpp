/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/

struct PlayerInputEvents
{
	bool32 pickupOrDrop	= false;
	bool32 interact		= false;
};


struct PlayerInputState
{
	bool32 	crouching;
	// s32 	inputArrayIndex;

	PlayerInputEvents events;
};


CharacterInput update_player_input(	PlayerInputState & 	state,
									Camera & 			playerCamera,
									PlatformInput * 	platformInput)
{
	v3 viewForward 		= get_forward(&playerCamera);
	viewForward.z 		= 0;
	viewForward 		= normalize_v3(viewForward);
	v3 viewRight 		= cross_v3(viewForward, v3_up);

	v3 worldSpaceInput 	= viewRight * input_axis_get_value(platformInput, INPUT_AXIS_move_x) 
						+ viewForward * input_axis_get_value(platformInput, INPUT_AXIS_move_y);

	bool32 jumpInput 	= input_button_went_down(platformInput, INPUT_BUTTON_nintendo_x);


	bool32 crouchInput 	= false;

	state.events = {};

	// Note(Leo): As long as we don't trigger these anywhere else, this works
	state.events.pickupOrDrop	= input_button_went_down(platformInput, INPUT_BUTTON_nintendo_b);
	state.events.interact 		= input_button_went_down(platformInput, INPUT_BUTTON_nintendo_a);


	return {worldSpaceInput, jumpInput, crouchInput};
} 