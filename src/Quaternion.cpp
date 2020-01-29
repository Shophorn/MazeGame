struct quaternion
{
	union
	{
		struct {float x, y, z; };
		vector3 vector;
	};
	
	float w;

	constexpr class_member quaternion
	Identity()
	{
		constexpr quaternion identity = {0, 0, 0, 1};
		return identity;
	}

	class_member quaternion
	AxisAngle(vector3 axis, float angle)
	{
		angle *= -1;
		float halfAngle = angle / 2.0f;

		quaternion result = {};

		result.w = Cosine(halfAngle);
		result.vector = axis * Sine(halfAngle);

		return result;
	}
};
