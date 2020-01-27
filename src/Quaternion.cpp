struct quaternion
{
	union
	{
		struct {real32 x, y, z; };
		vector3 vector;
	};
	
	real32 w;

	constexpr class_member quaternion
	Identity()
	{
		constexpr quaternion identity = {0, 0, 0, 1};
		return identity;
	}

	class_member quaternion
	AxisAngle(vector3 axis, real32 angle)
	{
		angle *= -1;
		real32 halfAngle = angle / 2.0f;

		quaternion result = {};

		result.w = Cosine(halfAngle);
		result.vector = axis * Sine(halfAngle);

		return result;
	}
};
