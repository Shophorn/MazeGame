/*=============================================================================
Leo Tamminen
shophorn @ github
=============================================================================*/
struct Transform3D
{
	v3 position 		= {0, 0, 0};
	v3 scale 			= {1, 1, 1,};
	quaternion rotation = identity_quaternion;
};

internal constexpr Transform3D identity_transform = { 	.position 	= {0, 0, 0}, 
														.scale 		= {1, 1, 1}, 
														.rotation 	= {0, 0, 0, 1}};

internal m44 transform_matrix(Transform3D const & transform)
{
	/* Study(Leo): members of this struct are ordered so that position and scale would
	occupy first half of struct, and rotation the other half by itself. Does this matter,
	and furthermore does that matter in this function call, both in order arguments are put
	there as well as is the order same as this' members order. */
	m44 result = transform_matrix(transform.position, transform.rotation, transform.scale);
	return result;
}

// Todo(Leo): Use quaternion multiplications
internal v3 get_forward(Transform3D const & transform)
{
	return multiply_point(rotation_matrix(transform.rotation), forward_v3);
}

internal v3 get_right(Transform3D const & transform)
{
	return multiply_point(rotation_matrix(transform.rotation), right_v3);
}

internal v3 get_up(Transform3D const & transform)
{
	return multiply_point(rotation_matrix(transform.rotation), up_v3);
}