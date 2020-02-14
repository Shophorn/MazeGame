struct quaternion
{
	union
	{
		struct {float x, y, z; };
		v3 vector;
	};
	
	float w;

	constexpr static quaternion
	Identity()
	{
		constexpr quaternion identity = {0, 0, 0, 1};
		return identity;
	}

	static quaternion
	AxisAngle(v3 axis, float angle)
	{
		angle *= -1;
		float halfAngle = angle / 2.0f;

		quaternion result = {};

		result.w = Cosine(halfAngle);
		result.vector = axis * Sine(halfAngle);

		return result;
	}
};
