/*=============================================================================
Leo Tamminen
shophorn @ github
=============================================================================*/
struct Transform3D
{
	vector3 position 	= {0, 0, 0};
	real32 scale 		= 1.0f;
	Quaternion rotation = Quaternion::Identity();

	Handle<Transform3D> parent;

	// Matrix44 get_matrix()
	// {
	// 	 Study(Leo): members of this struct are ordered so that position and scale would
	// 	occupy first half of struct, and rotation the other half by itself. Does this matter,
	// 	and furthermore does that matter in this function call, both in order arguments are put
	// 	there as well as is the order same as this' members order. 
	// 	Matrix44 result = Matrix44::Transform(position, rotation, scale);

	// 	if (is_handle_valid(parent))
	// 	{
	// 		result = result * parent->get_matrix();
	// 	}

	// 	return result;
	// }

	// vector3 get_world_position()
	// {
	// 	if (is_handle_valid(parent))
	// 	{
	// 		return parent->get_matrix() * position;
	// 	}

	// 	return position;
	// }
};

Matrix44
get_matrix(Transform3D * transform)
{
	/* Study(Leo): members of this struct are ordered so that position and scale would
	occupy first half of struct, and rotation the other half by itself. Does this matter,
	and furthermore does that matter in this function call, both in order arguments are put
	there as well as is the order same as this' members order. */
	Matrix44 result = Matrix44::Transform(transform->position, transform->rotation, transform->scale);
	if (is_handle_valid(transform->parent))
	{
		result = result * get_matrix(transform->parent);
	}
	return result;
}

vector3
get_world_position(Transform3D * transform)
{
	if (is_handle_valid(transform->parent))
	{
		return get_matrix(transform->parent) * transform->position;
	}	
	return transform->position;
}

vector3
get_forward(Transform3D * transform)
{
	return get_rotation_matrix(transform->rotation) * world::forward;
}

vector3
get_right(Transform3D * transform)
{
	return get_rotation_matrix(transform->rotation) * world::right;
}

vector3
get_up(Transform3D * transform)
{
	return get_rotation_matrix(transform->rotation) * world::up;
}