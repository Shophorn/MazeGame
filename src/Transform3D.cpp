/*=============================================================================
Leo Tamminen
shophorn @ github
=============================================================================*/
struct Transform3D
{
	Vector3 position 	= {0, 0, 0};
	real32 scale 		= 1.0f;
	Quaternion rotation = Quaternion::Identity();

	Handle<Transform3D> parent;

	Matrix44 get_matrix()
	{
		/* Study(Leo): members of this struct are ordered so that position and scale would
		occupy first half of struct, and rotation the other half by itself. Does this matter,
		and furthermore does that matter in this function call, both in order arguments are put
		there as well as is the order same as this' members order. */
		Matrix44 result = Matrix44::Transform(position, rotation, scale);

		if (is_handle_valid(parent))
		{
			result = result * parent->get_matrix();
		}

		return result;
	}

	Vector3 get_world_position()
	{
		if (is_handle_valid(parent))
		{
			return parent->get_matrix() * position;
		}

		return position;
	}
};

