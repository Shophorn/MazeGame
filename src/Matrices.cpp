/*=============================================================================
Leo Tamminen

Matrix structure declaration and definition.
Matrices are column major.
=============================================================================*/
template <typename Scalar, u32 Rows, u32 Columns>
struct Matrix
{
	constexpr static u32 rows_count 	= Rows;
	constexpr static u32 columns_count 	= Columns;

	using scalar_type 					= Scalar;
	using column_type 					= Vector<Scalar, Rows>;

	/// -------- CONTENTS -------------------
	column_type columns [columns_count];

	/// ------- ACCESSORS ----------------------------
	column_type & operator [] (s32 column) 				{ return columns[column]; }
	column_type operator [] (s32 column) const 			{ return columns[column]; }
	
	scalar_type & operator()(u32 column, u32 row) 		{ return vector::begin(&columns[column])[row];}
	scalar_type operator() (u32 column, u32 row) const 	{ return vector::const_begin(&columns[column])[row];	}

	constexpr static Matrix identity();
	constexpr Matrix<Scalar, Columns, Rows> transpose() const;
};

template<typename Scalar, u32 LeftRows, u32 InnerSize, u32 RightColumns>
Matrix<Scalar, LeftRows, RightColumns>
operator * (Matrix<Scalar, LeftRows, InnerSize> const & lhs,
			Matrix<Scalar, InnerSize, RightColumns> const & rhs)
{
	auto lhsTranspose = lhs.transpose();

	Matrix<Scalar, LeftRows, RightColumns> product;

	for(u32 col = 0; col < RightColumns; ++col)
		for (u32 row = 0; row < LeftRows; ++row)
		{
			product(col, row) = vector::dot(lhsTranspose[row], rhs[col]);
		}

	return product;
}  

using m33 = Matrix<float, 3, 3>;
using m44 = Matrix<float, 4, 4>;

v3 multiply_point(m44 const & mat, v3 point)
{
	Matrix<float,4,1> vecMatrix = { point.x, point.y, point.z, 1.0	};
	Matrix<float,4,1> product = mat * vecMatrix;

	point = {
		product(0,0) / product(0,3),
		product(0,1) / product(0,3),
		product(0,2) / product(0,3)
	};
	return point;
}

v3 multiply_direction(const m44 & mat, v3 direction)
{
	using namespace vector;

	auto lhs = mat.transpose();

	direction = {
		dot(convert_to<v3>(lhs[0]), direction),
		dot(convert_to<v3>(lhs[1]), direction),
		dot(convert_to<v3>(lhs[2]), direction)
	};
	return direction;	
}


template<>
constexpr m44 m44::identity()
{
	constexpr m44 identity_ = {	1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1};
	return identity_;
}

m44
make_translation_matrix(v3 translation)
{
	m44 result = m44::identity();
	result[3] = v4{translation.x, translation.y, translation.z, 1.0f};
	return result;
}

m44
make_rotation_matrix(quaternion rotation)
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

m44
make_scale_matrix(v3 scale)
{
	m44 result = {};
	result(0,0) = scale.x;
	result(1,1) = scale.y;
	result(2,2) = scale.z;
	result(3,3) = 1.0f;
	return result;	
}

m44
make_transform_matrix(v3 translation, float uniformScale = 1.0f)
{
	m44 result = {};

	result(0,0) = uniformScale;
	result(1,1) = uniformScale;
	result(2,2) = uniformScale;

	result[3] = { translation.x, translation.y, translation.z, 1.0f };

	return result;
}

m44
make_transform_matrix(v3 translation, quaternion rotation, float uniformScale = 1.0f)
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

m44 invert_transform_matrix(m44 matrix)
{
	v3 translation = {matrix[3].x, matrix[3].y, matrix[3].z};

	matrix[3] = {0,0,0,1};
	// matrix = matrix::transpose(matrix);
	matrix = matrix.transpose();

	translation = -multiply_point(matrix, translation);

	matrix[3] = { translation.x, translation.y, translation.z, 1 };
	matrix(3,3) = 1;
	return matrix;
}

template<typename S, u32 R, u32 C>
constexpr Matrix<S, C, R> Matrix<S,R,C>::transpose() const
{
	Matrix<S, C, R> result;
	for (u32 col = 0; col < C; ++col)
		for (u32 row = 0; row < R; ++row)
		{
			result(col, row) = (*this)(row, col);
		}
	return result;
}

#if MAZEGAME_INCLUDE_STD_IOSTREAM
namespace std
{
	template<typename S, u32 C, u32 R> ostream &
	operator << (ostream & os, Matrix<S, C, R> mat)
	{
		os << "(" << mat[0];
		for (s32 i = 1; i < C; ++i)
		{
			os << ";" << mat[i];
		}
		os << ")";
		return os;
	}
}
#endif
