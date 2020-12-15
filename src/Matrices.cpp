/*=============================================================================
Leo Tamminen

Matrix structure declaration and definition.
Matrices are column major.
=============================================================================*/

internal constexpr m44 identity_m44 = {	1, 0, 0, 0,
										0, 1, 0, 0,
										0, 0, 1, 0,
										0, 0, 0, 1 };

internal m44 transpose_m44(m44 mat)
{
	mat = {	mat[0].x, mat[1].x, mat[2].x, mat[3].x,
			mat[0].y, mat[1].y, mat[2].y, mat[3].y,
			mat[0].z, mat[1].z, mat[2].z, mat[3].z,
			mat[0].w, mat[1].w, mat[2].w, mat[3].w };
	return mat;
}

internal m44 operator * (m44 lhs, m44 const & rhs)
{
	lhs = transpose_m44(lhs);

	lhs = { dot_v4(lhs[0], rhs[0]), dot_v4(lhs[1], rhs[0]), dot_v4(lhs[2], rhs[0]), dot_v4(lhs[3], rhs[0]),
			dot_v4(lhs[0], rhs[1]), dot_v4(lhs[1], rhs[1]), dot_v4(lhs[2], rhs[1]), dot_v4(lhs[3], rhs[1]),
			dot_v4(lhs[0], rhs[2]), dot_v4(lhs[1], rhs[2]), dot_v4(lhs[2], rhs[2]), dot_v4(lhs[3], rhs[2]),
			dot_v4(lhs[0], rhs[3]), dot_v4(lhs[1], rhs[3]), dot_v4(lhs[2], rhs[3]), dot_v4(lhs[3], rhs[3]) };

	return lhs;
}

internal v3 multiply_point(m44 mat, v3 point)
{
	v4 vec4 = v3_to_v4(point, 1.0f);

	mat = transpose_m44(mat);

	f32 w = dot_v4(mat[3], vec4);

	point = { 	dot_v4(mat[0], vec4) / w,
				dot_v4(mat[1], vec4) / w,
				dot_v4(mat[2], vec4) / w }; 

	return point;
}

internal v3 multiply_direction(m44 mat, v3 direction)
{
	mat = transpose_m44(mat);

	direction = { 	v3_dot(mat[0].xyz, direction),
					v3_dot(mat[1].xyz, direction),
					v3_dot(mat[2].xyz, direction) };
	return direction;	
}


internal m44 translation_matrix(v3 translation)
{
	m44 result =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		translation.x, translation.y, translation.z, 1
	};
	return result;
}


internal m44 rotation_matrix(quaternion rotation)
{
	// Todo(Leo): Study matrix multiplication order, and order of axes in here
	float 	x = rotation.x,
			y = rotation.y,
			z = rotation.z,
			w = rotation.w;

	m44 result =
	{
		1 - 2*y*y - 2*z*z, 	2*x*y-2*w*z, 		2*x*z + 2*w*y, 		0,
		2*x*y + 2*w*z, 		1 - 2*x*x - 2*z*z,	2*y*z - 2*w*x,		0,
		2*x*z - 2*w*y,		2*y*z + 2*w*x,		1 - 2*x*x - 2*y*y,	0,
		0,					0,					0,					1
	};

	return result;	
}

internal m44 scale_matrix(v3 scale)
{
	m44 result = {	scale.x, 0, 0, 0,
					0, scale.y, 0, 0,
					0, 0, scale.z, 0,
					0, 0, 0, 1.0f };
	return result;	
}

internal m44 make_transform_matrix(v3 translation, float scale = 1.0f)
{
	m44 result = {	scale, 0, 0, 0,
					0, scale, 0, 0,
					0, 0, scale, 0,
					translation.x, translation.y,  translation.z, 1.0f };
	return result;
}

internal m44 make_transform_matrix(v3 translation, quaternion rotation, float uniformScale = 1.0f)
{
	float 	x = rotation.x,
			y = rotation.y,
			z = rotation.z,
			w = rotation.w;

	float s = uniformScale;

	m44 result =
	{
		s * (1 - 2*y*y - 2*z*z), 	s * (2*x*y-2*w*z), 			s * (2*x*z + 2*w*y),		0,
		s * (2*x*y + 2*w*z), 		s * (1 - 2*x*x - 2*z*z),	s * (2*y*z - 2*w*x),		0,
		s * (2*x*z - 2*w*y),		s * (2*y*z + 2*w*x),		s * (1 - 2*x*x - 2*y*y),	0,
		translation.x,				translation.y,				translation.z,				1
	};

	return result;	
}

internal m44 transform_matrix(v3 translation, quaternion rotation, v3 scale)
{
	float 	x = rotation.x,
			y = rotation.y,
			z = rotation.z,
			w = rotation.w,

			sx = scale.x,
			sy = scale.y,
			sz = scale.z;

	m44 result =
	{
		sx * (1 - 2*y*y - 2*z*z), 	sx * (2*x*y-2*w*z), 		sx * (2*x*z + 2*w*y),		0,
		sy * (2*x*y + 2*w*z), 		sy * (1 - 2*x*x - 2*z*z),	sy * (2*y*z - 2*w*x),		0,
		sz * (2*x*z - 2*w*y),		sz * (2*y*z + 2*w*x),		sz * (1 - 2*x*x - 2*y*y),	0,
		translation.x,				translation.y,				translation.z,				1
	};

	return result;	
}

internal m44 inverse_transform_matrix(v3 translation, quaternion rotation, v3 scale)
{
	translation = -translation;
	rotation 	= inverse_quaternion(rotation);
	scale 		= { 1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z };

	m44 result 		= scale_matrix(scale)
					* rotation_matrix(rotation)
					* translation_matrix(translation);

	// float 	x = rotation.x,
	// 		y = rotation.y,
	// 		z = rotation.z,
	// 		w = rotation.w,

	// 		sx = scale.x,
	// 		sy = scale.y,
	// 		sz = scale.z;

	// m44 result =
	// {
	// 	sx * (1 - 2*y*y - 2*z*z), 	sx * (2*x*y-2*w*z), 		sx * (2*x*z + 2*w*y),		0,
	// 	sy * (2*x*y + 2*w*z), 		sy * (1 - 2*x*x - 2*z*z),	sy * (2*y*z - 2*w*x),		0,
	// 	sz * (2*x*z - 2*w*y),		sz * (2*y*z + 2*w*x),		sz * (1 - 2*x*x - 2*y*y),	0,
	// 	translation.x,				translation.y,				translation.z,				1
	// };

	return result;	
}

internal v3 get_translation(m44 matrix)
{
	// Todo(Leo): Learn more about matrices, and maybe just return the last column
	v3 translation = multiply_point(matrix, {0,0,0});
	return translation;
}

// From: https://stackoverflow.com/questions/2624422/efficient-4x4-matrix-inverse-affine-transform
// public static double[,] GetInverse(double[,] a)
static m44 m44_inverse(m44 & m)
{
	// f32  *  * a = reinterpret_cast<f32  *  *>(&m);
	auto & a = m;

    f32 s0 = a[0].x * a[1].y - a[1].x * a[0].y;
    f32 s1 = a[0].x * a[1].z - a[1].x * a[0].z;
    f32 s2 = a[0].x * a[1].w - a[1].x * a[0].w;
    f32 s3 = a[0].y * a[1].z - a[1].y * a[0].z;
    f32 s4 = a[0].y * a[1].w - a[1].y * a[0].w;
    f32 s5 = a[0].z * a[1].w - a[1].z * a[0].w;

    f32 c5 = a[2].z * a[3].w - a[3].z * a[2].w;
    f32 c4 = a[2].y * a[3].w - a[3].y * a[2].w;
    f32 c3 = a[2].y * a[3].z - a[3].y * a[2].z;
    f32 c2 = a[2].x * a[3].w - a[3].x * a[2].w;
    f32 c1 = a[2].x * a[3].z - a[3].x * a[2].z;
    f32 c0 = a[2].x * a[3].y - a[3].x * a[2].y;

    // Should check for 0 determinant
    f32 invdet = 1.0 / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

    m44 result;
    auto & b = result;
    b[0].x = ( a[1].y * c5 - a[1].z * c4 + a[1].w * c3) * invdet;
    b[0].y = (-a[0].y * c5 + a[0].z * c4 - a[0].w * c3) * invdet;
    b[0].z = ( a[3].y * s5 - a[3].z * s4 + a[3].w * s3) * invdet;
    b[0].w = (-a[2].y * s5 + a[2].z * s4 - a[2].w * s3) * invdet;

    b[1].x = (-a[1].x * c5 + a[1].z * c2 - a[1].w * c1) * invdet;
    b[1].y = ( a[0].x * c5 - a[0].z * c2 + a[0].w * c1) * invdet;
    b[1].z = (-a[3].x * s5 + a[3].z * s2 - a[3].w * s1) * invdet;
    b[1].w = ( a[2].x * s5 - a[2].z * s2 + a[2].w * s1) * invdet;

    b[2].x = ( a[1].x * c4 - a[1].y * c2 + a[1].w * c0) * invdet;
    b[2].y = (-a[0].x * c4 + a[0].y * c2 - a[0].w * c0) * invdet;
    b[2].z = ( a[3].x * s4 - a[3].y * s2 + a[3].w * s0) * invdet;
    b[2].w = (-a[2].x * s4 + a[2].y * s2 - a[2].w * s0) * invdet;

    b[3].x = (-a[1].x * c3 + a[1].y * c1 - a[1].z * c0) * invdet;
    b[3].y = ( a[0].x * c3 - a[0].y * c1 + a[0].z * c0) * invdet;
    b[3].z = (-a[3].x * s3 + a[3].y * s1 - a[3].z * s0) * invdet;
    b[3].w = ( a[2].x * s3 - a[2].y * s1 + a[2].z * s0) * invdet;

    return result;
}