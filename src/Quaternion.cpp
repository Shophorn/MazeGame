struct quaternion
{
	union
	{
		struct {float x, y, z; };
		v3 vector;
	};
	
	float w;

	constexpr static quaternion	identity();
	static quaternion axis_angle(v3 axis, float angleInRadians);

	static quaternion euler_angles(float x, float y, float z);
	static quaternion euler_angles(v3 eulerRotation);
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

constexpr quaternion
quaternion::identity()
{
	constexpr quaternion identity = {0, 0, 0, 1};
	return identity;
}


quaternion
quaternion::axis_angle(v3 axis, float angleInRadians)
{
	angleInRadians *= -1;
	float halfAngle = angleInRadians / 2.0f;

	quaternion result = {};

	result.w = cosine(halfAngle);
	result.vector = axis * sine(halfAngle);

	return result;
}

quaternion
quaternion::euler_angles(v3 eulerRotation)
{
	return euler_angles(eulerRotation.x, eulerRotation.y, eulerRotation.z);
}

quaternion
quaternion::euler_angles(float eulerX, float eulerY, float eulerZ)
{
	/* Note(Leo): I found these somewhere in the web, but they seem to 
	yield left-handed rotations, which is something we do not want.
	Definetly better way to compute from euler angles though. */
    
    // Abbreviations for the various angular functions
    // float cy = cosf(eulerRotation.z * 0.5);
    // float sy = sinf(eulerRotation.z * 0.5);
    // float cp = cosf(eulerRotation.y * 0.5);
    // float sp = sinf(eulerRotation.y * 0.5);
    // float cr = cosf(eulerRotation.x * 0.5);
    // float sr = sinf(eulerRotation.x * 0.5);

    // quaternion q;
    // q.w = cy * cp * cr + sy * sp * sr;
    // q.x = cy * cp * sr - sy * sp * cr;
    // q.y = sy * cp * sr + cy * sp * cr;
    // q.z = sy * cp * cr - cy * sp * sr;

    // return q;


	quaternion x = axis_angle({1,0,0}, eulerX);
	quaternion y = axis_angle({0,1,0}, eulerY);
	quaternion z = axis_angle({0,0,1}, eulerZ);

	return z * y * x;	
}

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

	quaternion difference = inverse(from) * to;

	// Convert to rodriques rotation
	v3 axis 		= normalize_or_zero(difference.vector);
	float angle 	= arc_cosine(difference.w) * 2 * t;

	quaternion result = {
		.vector = sine(angle / 2.0f) * axis,
		.w 		= cosine(angle / 2.0f)
	};

	return from * result;
}