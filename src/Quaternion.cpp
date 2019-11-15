struct Quaternion
{
	union
	{
		struct {real32 x, y, z; };
		Vector3 vector;
	};
	
	real32 w;

	constexpr class_member Quaternion
	Identity()
	{
		constexpr Quaternion identity = {0, 0, 0, 1};
		return identity;
	}

	class_member Quaternion
	AxisAngle(Vector3 axis, real32 angle)
	{
		angle *= -1;
		real32 halfAngle = angle / 2.0f;

		Quaternion result = {};

		result.w = Cosine(halfAngle);
		result.vector = axis * Sine(halfAngle);

		return result;
	}
};
