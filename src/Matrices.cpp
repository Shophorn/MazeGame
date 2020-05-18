/*=============================================================================
Leo Tamminen

Matrix structure declaration and definition.
Matrices are column major.
=============================================================================*/

struct m44
{
	v4 columns [4];

	v4 & operator [] (s32 column)
	{ 
		return columns[column];
	}

	v4 operator [] (s32 column) const
	{ 
		return columns[column];
	}
};

internal constexpr m44 identity_m44 = {	1, 0, 0, 0,
										0, 1, 0, 0,
										0, 0, 1, 0,
										0, 0, 0, 1 };

internal m44 operator * (m44 lhs, m44 const & rhs)
{
	lhs = transpose_m44(lhs);

	lhs = { dot_v4(lhs[0], rhs[0]), dot_v4(lhs[1], rhs[0]), dot_v4(lhs[2], rhs[0]), dot_v4(lhs[3], rhs[0]),
			dot_v4(lhs[0], rhs[1]), dot_v4(lhs[1], rhs[1]), dot_v4(lhs[2], rhs[1]), dot_v4(lhs[3], rhs[1]),
			dot_v4(lhs[0], rhs[2]), dot_v4(lhs[1], rhs[2]), dot_v4(lhs[2], rhs[2]), dot_v4(lhs[3], rhs[2]),
			dot_v4(lhs[0], rhs[3]), dot_v4(lhs[1], rhs[3]), dot_v4(lhs[2], rhs[3]), dot_v4(lhs[3], rhs[3]) };

	return lhs;
}

internal m44 transpose_m44(m44 mat)
{
	mat = {	mat[0].x, mat[1].x, mat[2].x, mat[3].x,
			mat[0].y, mat[1].y, mat[2].y, mat[3].y,
			mat[0].z, mat[1].z, mat[2].z, mat[3].z,
			mat[0].w, mat[1].w, mat[2].w, mat[3].w };
	return mat;
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

	direction = { 	dot_v3(mat[0].xyz, direction),
					dot_v3(mat[1].xyz, direction),
					dot_v3(mat[2].xyz, direction) };
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

