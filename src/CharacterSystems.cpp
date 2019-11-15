/*=============================================================================
Leo Tamminen
shophorn@protonmail.com

Player Character Systems
=============================================================================*/

struct Character
{
	union 
	{
		struct{ Vector3 position; real32 scale; Quaternion rotation; };
		Transform3D transform; 
	};

	real32 zSpeed;
	real32 zRotationRadians;
};

internal Vector3
ProcessCharacterInput(game::Input * input, Camera * camera)
{
	Vector3 viewForward = camera->forward;
	viewForward.z 		= 0;
	viewForward 		= Normalize(viewForward);
	Vector3 viewRight 	= Cross(viewForward, World::Up);

	Vector3 result = viewRight * input->move.x
					+ viewForward * input->move.y;

	return result;
}