/*=============================================================================
Leo Tamminen
shophorn @ github
=============================================================================*/
struct Transform3D
{
	v3 position 		= {0, 0, 0};
	float scale 		= 1.0f;
	quaternion rotation = quaternion::identity();

	// Todo(Leo): this won't work if we are to realloc any memory.
	Transform3D * parent;

	static Transform3D identity();
};

Transform3D Transform3D::identity()
{
	Transform3D transform = {	{0,0,0},
								1.0f,
								quaternion::identity()};
	return transform;
}

Transform3D interpolate(Transform3D const & lhs, Transform3D const & rhs, float t)
{
	// Note(Leo): Transforms should not store hierarchy themselves, interpolating does not support that
	assert(lhs.parent == nullptr && rhs.parent == nullptr);

	Transform3D result = {};
	result.position = vector::interpolate(lhs.position, rhs.position, t);
	result.rotation = interpolate(lhs.rotation, rhs.rotation, t);
	result.scale = interpolate(lhs.scale, rhs.scale, t);
	return result;
}

m44 get_matrix(Transform3D const & transform)
{
	/* Study(Leo): members of this struct are ordered so that position and scale would
	occupy first half of struct, and rotation the other half by itself. Does this matter,
	and furthermore does that matter in this function call, both in order arguments are put
	there as well as is the order same as this' members order. */
	m44 result = make_transform_matrix(transform.position, transform.rotation, transform.scale);
	if (transform.parent != nullptr)
	{
		result = result * get_matrix(*transform.parent);
	}
	return result;
}

v3 get_world_position(Transform3D const & transform)
{
	if (transform.parent != nullptr)
	{
		return multiply_point(get_matrix(*transform.parent), transform.position);
	}	
	return transform.position;
}

// Todo(Leo): Use quaternion multiplications
v3 get_forward(Transform3D const & transform)
{
	return multiply_point(make_rotation_matrix(transform.rotation), world::forward);
}

v3 get_right(Transform3D const & transform)
{
	return multiply_point(make_rotation_matrix(transform.rotation), world::right);
}

v3 get_up(Transform3D const & transform)
{
	return multiply_point(make_rotation_matrix(transform.rotation), world::up);
}