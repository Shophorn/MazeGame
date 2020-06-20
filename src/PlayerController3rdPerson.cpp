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
							PlatformInput const & 		platformInput)
{
	v3 viewForward 		= get_forward(&playerCamera);
	viewForward.z 		= 0;
	viewForward 		= normalize_v3(viewForward);
	v3 viewRight 		= cross_v3(viewForward, up_v3);

	v3 worldSpaceInput 	= viewRight * platformInput.move.x + viewForward * platformInput.move.y;

	bool32 jumpInput 	= is_clicked(platformInput.X);

	if(is_clicked(platformInput.B))
	{
		playerState.crouching = !playerState.crouching;
	}
	bool32 crouchInput 	= playerState.crouching;

	inputs[playerState.inputArrayIndex] = {worldSpaceInput, jumpInput, crouchInput};
} 