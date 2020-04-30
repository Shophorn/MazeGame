/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/

struct PlayerInputState
{
	bool32 	crouching;
	s32 	inputArrayIndex;
};

void update_player_input(	PlayerInputState & 			playerState,
							Array<CharacterInput> & 	inputs,
							Camera & 					playerCamera,
							game::Input & 				platformInput)
{
	v3 viewForward 		= get_forward(&playerCamera);
	viewForward.z 		= 0;
	viewForward 		= viewForward.normalized();
	v3 viewRight 		= vector::cross(viewForward, world::up);

	v3 worldSpaceInput 	= viewRight * platformInput.move.x + viewForward * platformInput.move.y;

	bool32 jumpInput 	= is_clicked(platformInput.X);

	if(is_clicked(platformInput.B))
	{
		playerState.crouching = !playerState.crouching;
	}
	bool32 crouchInput 	= playerState.crouching;

	inputs[playerState.inputArrayIndex] = {worldSpaceInput, jumpInput, crouchInput};
} 