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
	f = f / abs_f32(f);
	return f;
}

f32 magnitude_v1(f32 f)
{
	f = abs_f32(f);
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
	f32 m = square_root_f32(v.x * v.x + v.y * v.y);
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

v2 rotate_v2 (v2 vec, f32 angle)
{
	f32 cos = cosine(angle);
	f32 sin = sine(angle);

	vec =
	{
		vec.x * cos - vec.y * sin,
		vec.x * sin + vec.y * cos
	};
	return vec;
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
	struct { f32 x, y, z; };
	struct { f32 r, g, b; };
	struct { v2 xy; f32 ignored_; };
};

constexpr v3 v3_right 	= {1,0,0};
constexpr v3 v3_forward = {0,1,0};
constexpr v3 v3_up 		= {0,0,1};

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

constexpr v3 operator - (v3 vec)
{
	vec.x = -vec.x;
	vec.y = -vec.y;
	vec.z = -vec.z;
	return vec;
}

f32 v3_length(v3 v)
{
	f32 m = square_root_f32(square_f32(v.x) + square_f32(v.y) + square_f32(v.z));
	return m;
}

f32 square_v3_length(v3 v)
{
	f32 sqrMagnitude = square_f32(v.x) + square_f32(v.y) + square_f32(v.z);
	return sqrMagnitude;
}

v3 normalize_v3(v3 v)
{
	v = v / v3_length(v);
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

v3 lerp_v3(v3 a, v3 b, f32 t)
{
	a = a + t * (b - a);
	return a;
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

	f32 denominator = square_root_f32(square_v3_length(from) * square_v3_length(to));
	if (denominator < v3_sqr_epsilon)
		return 0.0f;

	f32 dot = clamp_f32(dot_v3(from, to) / denominator, -1.0f, 1.0f);
	f32 angle = arc_cosine(dot);
	return angle;
}

f32 signed_angle_v3(v3 from, v3 to, v3 axis)
{
	f32 unsignedAngle 	= angle_v3(from, to);
	f32 sign 			= dot_v3(axis, cross_v3(from, to));
	return unsignedAngle * sign;
}


v3 make_uniform_v3(f32 value)
{
	v3 result = {value, value, value};
	return result;
}


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

internal v4 operator * (v4 v, f32 f)
{
	v.x *= f;
	v.y *= f;
	v.z *= f;
	v.w *= f;

	return v;
}

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

v4 v4_lerp(v4 const & a, v4 const & b, float t)
{
	// Todo(Leo): simd
	f32 omt = 1.0f - t;
	v4 result =
	{
		omt * a.x + t * b.x,
		omt * a.y + t * b.y,
		omt * a.z + t * b.z,
		omt * a.w + t * b.w,
	};
	return result;
}

// ----------- Overloads for Vector types -------------------

internal void string_parse(String string, v2 * outValue)
{
	String part0 = string_extract_until_character(string, ',');
	String part1 = string;

	string_parse(part0, &outValue->x);
	string_parse(part1, &outValue->y);
}

internal void string_append(String & string, s32 capacity, v2 value)
{
	string_append_f32(string, capacity, value.x);	
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.y);	
}

internal void string_parse(String string, v3 * outValue)
{
	String part0 = string_extract_until_character(string, ',');
	String part1 = string_extract_until_character(string, ',');
	String part2 = string;

	string_parse(part0, &outValue->x);
	string_parse(part1, &outValue->y);
	string_parse(part2, &outValue->z);
}

internal void string_append(String & string, s32 capacity, v3 value)
{
	string_append_f32(string, capacity, value.x);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.y);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.z);
}

internal void string_parse(String string, v4 * outValue)
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

internal void string_append(String & string, s32 capacity, v4 value)
{
	string_append_f32(string, capacity, value.x);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.y);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.z);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.w);
}
