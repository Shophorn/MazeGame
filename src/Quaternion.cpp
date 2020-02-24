struct quaternion
{
	union
	{
		struct {float x, y, z; };
		v3 vector;
	};
	
	float w;

	constexpr static quaternion
	identity()
	{
		constexpr quaternion identity = {0, 0, 0, 1};
		return identity;
	}

	static quaternion
	axis_angle(v3 axis, float angleInRadians)
	{
		angleInRadians *= -1;
		float halfAngle = angleInRadians / 2.0f;

		quaternion result = {};

		result.w = cosine(halfAngle);
		result.vector = axis * sine(halfAngle);

		return result;
	}
};

// quaternion normalize(quaternion q);
quaternion inverse(quaternion q);
quaternion operator * (quaternion lhs, quaternion rhs);
quaternion interpolate(quaternion from, quaternion to, float t);

#if MAZEGAME_INCLUDE_STD_IOSTREAM

std::ostream & operator << (std::ostream & os, quaternion q)
{
	os << "(" << q.vector << ", " << q.w << ")";
	return os;
}

#endif

// quaternion normalize(quaternion q)
// {
// 	float magnitude = Root2(vector::get_sqr_length(q.vector) + (q.w * q.w));
// 	q.vector /= magnitude;
// 	q.w /= magnitude;
// 	return q;
// }

quaternion inverse(quaternion q)
{
	using namespace vector;
	float magnitude = get_length(q.vector);

	float conjugate = q.w * q.w + magnitude * magnitude;

	return {
		.vector = (-q.vector) / conjugate,
		.w 		= q.w / conjugate
	};
}

quaternion operator * (quaternion lhs, quaternion rhs)
{
	// https://www.cprogramming.com/tutorial/3d/quaternions.html
	// quaternion result = {
	// 	lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
	// 	lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
	// 	lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
	// 	lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z
	// };
	
	using namespace vector;
	
	// https://www.youtube.com/watch?v=BXajpAy5-UI
	quaternion result = {
		.vector = lhs.w * rhs.vector + rhs.w * lhs.vector + cross(lhs.vector, rhs.vector),
		.w 		= lhs.w * rhs.w - dot(lhs.vector, rhs.vector)
	};

	return result;
}

quaternion & operator *= (quaternion & lhs, quaternion const & rhs)
{
	using namespace vector;

	lhs = {
		.vector = lhs.w * rhs.vector + rhs.w * lhs.vector + cross(lhs.vector, rhs.vector),
		.w 		= lhs.w * rhs.w - dot(lhs.vector, rhs.vector)
	};
	return lhs;
}

quaternion interpolate(quaternion from, quaternion to, float t)
{
	using namespace vector;

	quaternion difference = to * inverse(from);

	// Convert to rodriques rotation
	v3 axis 		= normalize_or_zero(difference.vector);
	float angle 	= arc_cosine(difference.w) * 2 * t;

	quaternion result = {
		.vector = sine(angle / 2.0f) * axis,
		.w 		= cosine(angle / 2.0f)
	};

	return from * result;
}