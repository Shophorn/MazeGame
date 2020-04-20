/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/

void update_player_input(	s32 						inputIndex,
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
	bool32 crouchInput 	= is_pressed(platformInput.B);

	inputs[inputIndex] = {worldSpaceInput, jumpInput, crouchInput};
} 