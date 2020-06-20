/*=============================================================================
Leo Tamminen

2d, 3d and 4d vector types

TODO(Leo):
	- Full testing
	- SIMD / SSE / AVX
=============================================================================*/

/// ------- v1 ------------
// Note(Leo): testing out conceptual idea that scalars are just 1d vectors
f32 normalize_v1(f32 f)
{
	f = f / math::absolute(f);
	return f;
}

f32 magnitude_v1(f32 f)
{
	f = math::absolute(f);
	return f;
}

/// ------- v2 ------------

struct v2
{	
	f32 x, y;
};

v2 operator + (v2 a, v2 b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

v2 & operator += (v2 & a, v2 b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

v2 operator - (v2 lhs, v2 rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

v2 & operator -= (v2 & lhs, v2 rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}

v2 operator * (v2 vec, f32 f)
{
	vec.x *= f;
	vec.y *= f;
	return vec;
}

v2 operator / (v2 vec, f32 f)
{
	vec.x /= f;
	vec.y /= f;
	return vec;
}

v2 operator -(v2 vec)
{
	vec.x = -vec.x;
	vec.y = vec.y;
	return vec;
}

f32 dot_v2(v2 a, v2 b)
{
	f32 p = { a.x * b.x + a.y * b.y };
	return p;
}

f32 magnitude_v2(v2 v)
{
	f32 m = math::square_root(v.x * v.x + v.y * v.y);
	return m;
}

f32 square_magnitude_v2(v2 v)
{
	f32 result = v.x * v.x + v.y * v.y;
	return result;
}

v2 normalize_v2(v2 v)
{
	v = v / magnitude_v2(v);
	return v;
}

v2 scale_v2 (v2 a, v2 b)
{
	a.x *= b.x;
	a.y *= b.y;
	return a;
}

v2 clamp_length_v2(v2 vec, f32 length)
{
	f32 magnitude = magnitude_v2(vec);
	if (magnitude > length)
	{
		vec = vec / magnitude;
		vec = vec * length;
	}
	return vec;
}

f32 v2_signed_angle(v2 from, v2 to)
{
	// https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors/16544330#16544330
	
	f32 dot = dot_v2(from, to);
	f32 det = from.x * to.y - from.y * to.x;
	f32 angle = arctan2(det, dot);

	return angle;
}

// ------------ v3 ----------------

union v3
{
	struct
	{
		f32 x, y, z;
	};

	struct
	{
		v2 xy;
		f32 ignored_;
	};
};

constexpr v3 right_v3 	= {1,0,0};
constexpr v3 forward_v3 = {0,1,0};
constexpr v3 up_v3 		= {0,0,1};

v3 operator + (v3 a, v3 b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return a;
}

v3 & operator += (v3 & a, v3 b)
{
	a = a + b;
	return a;
}

v3 operator - (v3 lhs, v3 rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	return lhs;
}

v3 & operator -= (v3 & lhs, v3 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}

v3 operator * (v3 vec, f32 f)
{
	vec.x *= f;
	vec.y *= f;
	vec.z *= f;
	return vec;
}

v3 operator * (f32 f, v3 vec)
{
	return vec * f;
}

v3 operator *= (v3 vec, f32 f)
{
	vec = vec * f;
	return vec;
}

v3 operator / (v3 vec, f32 f)
{
	vec.x /= f;
	vec.y /= f;
	vec.z /= f;
	return vec;
}

v3 operator - (v3 vec)
{
	vec.x = -vec.x;
	vec.y = -vec.y;
	vec.z = -vec.z;
	return vec;
}

f32 magnitude_v3(v3 v)
{
	using namespace math;

	f32 m = square_root(pow2(v.x) + pow2(v.y) + pow2(v.z));
	return m;
}

f32 square_magnitude_v3(v3 v)
{
	using namespace math;

	f32 sqrMagnitude = pow2(v.x) + pow2(v.y) + pow2(v.z);
	return sqrMagnitude;
}

v3 normalize_v3(v3 v)
{
	v = v / magnitude_v3(v);
	return v;
}

f32 dot_v3(v3 a, v3 b)
{
	f32 dot = a.x * b.x + a.y * b.y + a.z * b.z;
	return dot;
}

v3 cross_v3(v3 lhs, v3 rhs)
{
	lhs = {	lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x };
	return lhs;
}

v3 interpolate_v3(v3 from, v3 to, f32 t)
{
	from = from * (1 - t) + to * t;
	return from;
}

v3 rotate_v3(v3 vec, v3 axis, f32 angleInRadians)
{
	v3 projection = axis * dot_v3(axis, vec);
	v3 rejection = vec - projection;

	v3 rotatedRejection = rejection * cosine(angleInRadians) + cross_v3(axis, rejection) * sine(angleInRadians);

	vec = projection + rotatedRejection;
	return vec;	
}

constexpr f32 v3_sqr_epsilon = 1e-15f;

f32 angle_v3(v3 from, v3 to)
{
	// Note(Leo): copied from unity3d vector.cs...

	f32 denominator = math::square_root(square_magnitude_v3(from) * square_magnitude_v3(to));
	if (denominator < v3_sqr_epsilon)
		return 0.0f;

	f32 dot = math::clamp(dot_v3(from, to) / denominator, -1.0f, 1.0f);
	f32 angle = arc_cosine(dot);
	return angle;
}

// // Todo(Leo): Not tested...
// f32 v3_signed_angle(v3 from, v3 to, v3 normalizedReferenceUp)
// {
// 	f32 angle = arctan2(dot_v3(cross_v3(from, to), normalizedReferenceUp), dot_v3(from, to));
// 	return angle;
// }


v3 make_uniform_v3(f32 value)
{
	v3 result = {value, value, value};
	return result;
}

#if MAZEGAME_INCLUDE_STD_IOSTREAM

std::ostream & operator << (std::ostream & os, v3 vec)
{
	os << "(" << vec.x << "," << vec.y << "," << vec.z << ")";
	return os;
}

#endif

// ----------- v4 ----------------

union v4
{
	struct { f32 x, y, z, w; };
	struct
	{
		v3 xyz;
		f32 ignored_1;
	};

	struct
	{
		v2 xy, zw;
	};

	struct { f32 r, g, b, a; };
	struct
	{
		v3 rgb; 
		f32 ignored_2;
	};
};

f32 dot_v4(v4 a, v4 b)
{
	f32 dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	return dot;
}

v4 v3_to_v4(v3 vec, f32 w)
{
	v4 result = {vec.x, vec.y, vec.z, w};
	return result;
}