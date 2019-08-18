struct Quaternion
{
	union
	{
		struct {real32 x, y, z; };
		Vector3 vector;
	};
	
	real32 w;

	class_member Quaternion
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

	Matrix44
	ToRotationMatrix()
	{
		Matrix44 result = {
			1 - 2*y*y - 2*z*z, 	2*x*y-2*w*z, 		2*x*z + 2*w*y, 		0,
			2*x*y + 2*w*z, 		1 - 2*x*x - 2*z*z,	2*y*z - 2*w*x,		0,
			2*x*z - 2*w*y,		2*y*z + 2*w*x,		1 - 2*x*x - 2*y*y,	0,
			0,					0,					0,					1
		};

		return result;
	}
};
