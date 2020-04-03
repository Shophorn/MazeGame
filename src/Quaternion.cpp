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

	bool is_unit_quaternion() const;

	float magnitude() const;
	float square_magnitude() const;
	quaternion inverse() const;
	quaternion inverse_non_unit() const;

	quaternion normalized() const;
};

static_assert(std::is_aggregate_v<quaternion>, "");
static_assert(std::is_standard_layout_v<quaternion>, "");
static_assert(std::is_trivial_v<quaternion>, "");

quaternion operator * (quaternion lhs, quaternion rhs);
quaternion interpolate(quaternion from, quaternion to, float t);
float dot_product(quaternion a, quaternion b);

#if MAZEGAME_INCLUDE_STD_IOSTREAM

std::ostream & operator << (std::ostream & os, quaternion q)
{
	os << "(" << q.vector << ", " << q.w << ")";
	return os;
}

#endif

float quaternion::magnitude() const
{
	return math::square_root(x*x + y*y + z*z + w*w);
}

float quaternion::square_magnitude() const
{
	return x*x + y*y + z*z + w*w;
}

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

quaternion quaternion::inverse() const
{
	assert(this->is_unit_quaternion());

	quaternion result = { -x, -y, -z, w };
	return result;
}

quaternion quaternion::inverse_non_unit() const
{
	using namespace vector;
	float vectorMagnitude = vector.magnitude();

	float conjugate = (w * w) + (vectorMagnitude * vectorMagnitude);

	return {
		.vector = (-vector) / conjugate,
		.w 		= w / conjugate
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
		.w 		= lhs.w * rhs.w - vector::dot(lhs.vector, rhs.vector)
	};

	return result;
}

quaternion & operator *= (quaternion & lhs, quaternion const & rhs)
{
	using namespace vector;

	lhs = {
		.vector = lhs.w * rhs.vector + rhs.w * lhs.vector + cross(lhs.vector, rhs.vector),
		.w 		= lhs.w * rhs.w - vector::dot(lhs.vector, rhs.vector)
	};
	return lhs;
}

quaternion interpolate(quaternion from, quaternion to, float t)
{
	using namespace vector;

	// Note(Leo): This ensures that rotation takes the shorter path
	float dot = dot_product(from, to);
	if (dot < 0)
	{
		to.vector = -to.vector;
		to.w = -to.w;
	}

	if (dot > 0.99)
	{
		// quaternion result = {	interpolate(from.x, to.x, t),
		// 						interpolate(from.y, to.y, t),
		// 						interpolate(from.z, to.z, t),
		// 						interpolate(from.w, to.w, t)};

		quaternion result;

		float * resultPtr 	= &result.x;
		float * fromPtr 	= &from.x;
		float * toPtr 		= &to.x;

		for (int i = 0; i < 4; ++i)
		{
			resultPtr[i] = interpolate(fromPtr[i], toPtr[i], t);
		}

		return result.normalized();
	}

	quaternion difference = from.inverse() * to;

	// Convert to rodriques rotation
	v3 axis 		= normalize_or_zero(difference.vector);
	float angle 	= arc_cosine(difference.w) * 2 * t;

	quaternion result = {
		.vector = sine(angle / 2.0f) * axis,
		.w 		= cosine(angle / 2.0f)
	};

	return from * result;
}

float dot_product(quaternion a, quaternion b)
{
	float result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
	return result;
}

bool quaternion::is_unit_quaternion() const
{
	bool result = math::close_enough_small(square_magnitude(), 1.0f);
	return result;
}

quaternion quaternion::normalized() const
{
	float magnitude_ = magnitude();

	quaternion result = { 	x / magnitude_,
							y / magnitude_,
							z / magnitude_,
							w / magnitude_};
	assert(result.is_unit_quaternion());
	return result;
}