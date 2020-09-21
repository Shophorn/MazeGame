struct quaternion
{
	union
	{
		v3 vector;
		struct {f32 x, y, z; };
	};
	f32 w;
};

static_assert(std::is_aggregate_v<quaternion>, "");
static_assert(std::is_standard_layout_v<quaternion>, "");
static_assert(std::is_trivial_v<quaternion>, "");

internal constexpr quaternion identity_quaternion = {0, 0, 0, 1};


quaternion operator * (quaternion lhs, quaternion rhs);


f32 magnitude_quaternion (quaternion q)
{
	return square_root_f32(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
}

f32 square_magnitude_quaternion (quaternion q)
{
	return q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
}

bool is_unit_quaternion(quaternion q)
{
	// TODO(Leo): is this good epsilon for this kind of thing?
	bool result = abs_f32(square_magnitude_quaternion(q) - 1.0f) < 0.00001f;
	return result;
}

internal quaternion axis_angle_quaternion(v3 normalizedAxis, f32 angleInRadians)
{
	// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO 
	// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO 
	// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO 
	// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO 
	// Todo(Leo): This seems horribly wrong, but as everything breaks when this is
	// removed, we probably have mitigated this everywhere.
	angleInRadians 		*= -1;



	f32 halfAngle 		= angleInRadians / 2.0f;

	quaternion result =
	{
		normalizedAxis * sine(halfAngle),
		cosine(halfAngle)
	};

	return result;
}

internal v3 rotate_v3(quaternion const & q, v3 v)
{
	// Study(Leo):
	// https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion

	// Todo(Leo): cross product order changes direction (obviously), original code was the
	// "wrong" way, but it may as well be that our code is the wrong way

	v3 u = q.vector;
	f32 s = q.w;

	v = 2.0f * dot_v3(u, v) * u
		+ (s * s - dot_v3(u, u)) * v
		+ 2.0f * s * cross_v3(v, u);

	return v;
}

internal quaternion euler_angles_quaternion(f32 eulerX, f32 eulerY, f32 eulerZ)
{
	/* Note(Leo): I found these somewhere in the web, but they seem to 
	yield left-handed rotations, which is something we do not want.
	Definetly better way to compute from euler angles though. */
    
    // Abbreviations for the various angular functions
    // f32 cy = cosf(eulerRotation.z * 0.5);
    // f32 sy = sinf(eulerRotation.z * 0.5);
    // f32 cp = cosf(eulerRotation.y * 0.5);
    // f32 sp = sinf(eulerRotation.y * 0.5);
    // f32 cr = cosf(eulerRotation.x * 0.5);
    // f32 sr = sinf(eulerRotation.x * 0.5);

    // quaternion q;
    // q.w = cy * cp * cr + sy * sp * sr;
    // q.x = cy * cp * sr - sy * sp * cr;
    // q.y = sy * cp * sr + cy * sp * cr;
    // q.z = sy * cp * cr - cy * sp * sr;

    // return q;


	quaternion x = axis_angle_quaternion({1,0,0}, eulerX);
	quaternion y = axis_angle_quaternion({0,1,0}, eulerY);
	quaternion z = axis_angle_quaternion({0,0,1}, eulerZ);

	return z * y * x;	
}

internal quaternion euler_angles_quaternion(v3 eulerRotation)
{
	return euler_angles_quaternion(eulerRotation.x, eulerRotation.y, eulerRotation.z);
}

internal quaternion inverse_quaternion(quaternion q)
{
	Assert(is_unit_quaternion(q));

	quaternion result = { -q.x, -q.y, -q.z, q.w };
	return result;
}

quaternion inverse_non_unit_quaternion(quaternion q)
{
	f32 vectorMagnitude = magnitude_v3(q.vector);

	f32 conjugate = (q.w * q.w) + (vectorMagnitude * vectorMagnitude);

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
	
	// https://www.youtube.com/watch?v=BXajpAy5-UI
	quaternion result = {
		.vector = rhs.vector * lhs.w + lhs.vector * rhs.w + cross_v3(lhs.vector, rhs.vector),
		.w 		= lhs.w * rhs.w - dot_v3(lhs.vector, rhs.vector)
	};

	

	return result;
}

quaternion & operator *= (quaternion & lhs, quaternion const & rhs)
{
	lhs = {
		.vector = rhs.vector * lhs.w + lhs.vector * rhs.w + cross_v3(lhs.vector, rhs.vector),
		.w 		= lhs.w * rhs.w - dot_v3(lhs.vector, rhs.vector)
	};
	return lhs;
}

f32 dot_quaternion(quaternion a, quaternion b)
{
	f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
	return result;
}

quaternion normalize_quaternion(quaternion q)
{
	f32 magnitude = magnitude_quaternion(q);

	quaternion result = { 	q.x / magnitude,
							q.y / magnitude,
							q.z / magnitude,
							q.w / magnitude};

	Assert(result.x == result.x && result.y == result.y && result.z == result.z && result.w == result.w && "probably a nan");
	Assert(is_unit_quaternion(result));
	return result;
}

quaternion interpolate_quaternion(quaternion from, quaternion to, f32 t)
{
	// Assert(t == t && "probably a nan");
	if (t < -0.00001f || t > 1.00001f)
	{
		logDebug(0) << "häx: " << t;
		Assert(false);
	}
	Assert(from.x == from.x && from.y == from.y && from.z == from.z && from.w == from.w && "probably a nan");
	Assert(to.x == to.x && to.y == to.y && to.z == to.z && to.w == to.w && "probably a nan");


	// Note(Leo): This ensures that rotation takes the shorter path
	f32 dot = dot_quaternion(from, to);
	if (dot < 0)
	{
		to.vector = -to.vector;
		to.w = -to.w;
		dot = -dot;
	}
// /

	if (dot > 0.99)
	{
		quaternion result = {	interpolate(from.x, to.x, t),
								interpolate(from.y, to.y, t),
								interpolate(from.z, to.z, t),
								interpolate(from.w, to.w, t)};

		// quaternion result;

		// f32 * resultPtr 	= &result.x;
		// f32 * fromPtr 	= &from.x;
		// f32 * toPtr 		= &to.x;

		// for (int i = 0; i < 4; ++i)
		// {
		// 	resultPtr[i] = interpolate(fromPtr[i], toPtr[i], t);
		// }
		
		if(result.x == result.x && result.y == result.y && result.z == result.z && result.w == result.w)
		{

		}
		else
		{
			logDebug(0) << "invalid interpolate quaternion, from: " << from << ", to: " << to << ", t: " << t;
			Assert(false);
		}
		// Assert(result.x == result.x && result.y == result.y && result.z == result.z && result.w == result.w && "probably a nan");
		result = normalize_quaternion(result);
		return result;
	}

	quaternion difference = inverse_non_unit_quaternion(from) * to;

	// https://theory.org/software/qfa/writeup/node12.html
	// https://fi.wikipedia.org/wiki/Hyperbolinen_funktio

	// Convert to rodriques rotation
	v3 axis;

	f32 magnitude = magnitude_v3(difference.vector);
	if (magnitude < 0.0001f)
	{
		axis = difference.vector;
	}
	else
	{
		axis = difference.vector / magnitude;
	}

	// v3 axis 		= normalize_or_zero(difference.vector);
	f32 angle 	= arc_cosine(difference.w) * 2 * t;

	quaternion result = {
		.vector = axis * sine(angle / 2.0f),
		.w 		= cosine(angle / 2.0f)
	};

	return from * result;
}

// Todo(Leo): use this function rather than interpolate_quaternion, since we may also need lerp_quaternion, and thus make difference
quaternion slerp_quaternion(quaternion from, quaternion to, f32 t)
{
	return interpolate_quaternion(from, to, t);
}

quaternion quaternion_from_to(v3 from, v3 to)
{
	f32 angle = angle_v3(from, to);

	v3 axis = normalize_v3(cross_v3(from, to));

    f32 cross_x = from.y * to.z - from.z * to.y;
    f32 cross_y = from.z * to.x - from.x * to.z;
    f32 cross_z = from.x * to.y - from.y * to.x;
    f32 sign = axis.x * cross_x + axis.y * cross_y + axis.z * cross_z;
	
	sign = sign < 0 ? -1 : 1;

	// Todo(Leo): this was also copied from unity, which has wrong handed coordinate-system, multiply by -1 for that
	angle = -1 * sign * angle;

	quaternion result = axis_angle_quaternion(axis, angle);
	return result;
}

// ---------------- String overloads -------------------------

internal void string_parse(String string, quaternion * outValue)
{
	String part0 = string_extract_until_character(string, ',');
	String part1 = string_extract_until_character(string, ',');
	String part2 = string_extract_until_character(string, ',');
	String part3 = string;

	string_parse(part0, &outValue->x);
	string_parse(part1, &outValue->y);
	string_parse(part2, &outValue->z);
	string_parse(part3, &outValue->w);
}

internal void string_append(String & string, s32 capacity, quaternion value)
{
	string_append_f32(string, capacity, value.x);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.y);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.z);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.w);
}
