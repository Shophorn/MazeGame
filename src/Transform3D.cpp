/*=============================================================================
Leo Tamminen
shophorn @ github
=============================================================================*/
struct Transform3D
{
	v3 position 		= {0, 0, 0};
	real32 scale 		= 1.0f;
	quaternion rotation = quaternion::Identity();

	// Todo(Leo): this won't work if we are to realloc any memory.
	Transform3D * parent;
};

// template<>
// struct Handle<Transform3D> {};

m44
get_matrix(Transform3D * transform)
{
	/* Study(Leo): members of this struct are ordered so that position and scale would
	occupy first half of struct, and rotation the other half by itself. Does this matter,
	and furthermore does that matter in this function call, both in order arguments are put
	there as well as is the order same as this' members order. */
	m44 result = make_transform_matrix(transform->position, transform->rotation, transform->scale);
	if (transform->parent != nullptr)
	{
		result = matrix::multiply(result, get_matrix(transform->parent));
	}
	return result;
}

v3
get_world_position(Transform3D * transform)
{
	if (transform->parent != nullptr)
	{
		return get_matrix(transform->parent) * transform->position;
	}	
	return transform->position;
}

// Todo(Leo): make these const
v3
get_forward(Transform3D * transform)
{
	return make_rotation_matrix(transform->rotation) * world::forward;
}

v3
get_right(Transform3D * transform)
{
	return make_rotation_matrix(transform->rotation) * world::right;
}

v3
get_up(Transform3D * transform)
{
	return make_rotation_matrix(transform->rotation) * world::up;
}