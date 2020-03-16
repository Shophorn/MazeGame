/*=============================================================================
Leo Tamminen

Matrix structure declaration and definition.
Matrices are column major.
=============================================================================*/
template <typename Scalar, u32 Rows, u32 Columns>
struct MatrixBase
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
};



namespace matrix_meta_
{
	template<typename TMatrix>
	using ScalarType = typename TMatrix::scalar_type;

	template<typename TMatrix>
	using TransposeType = MatrixBase<typename TMatrix::scalar_type,
											TMatrix::columns_count,
											TMatrix::rows_count>;

	template<typename TLeft, typename TRight>
	using ProductType = MatrixBase<typename TLeft::scalar_type,
											TLeft::rows_count,
											TRight::columns_count>;

	template<typename> constexpr bool32 isMatrixType = false;
	template<typename S, u32 R, u32 C> constexpr bool32 isMatrixType<MatrixBase<S,R,C>> = true;
}

namespace matrix
{
	using namespace matrix_meta_;

	template<typename TMatrix> TMatrix 										make_identity();

	template<typename TMatrix> TransposeType<TMatrix> 						transpose(TMatrix const &);
	template<typename TLeft, typename TRight> ProductType<TLeft, TRight> 	multiply(TLeft const &, TRight const &);	
}

using m33 = MatrixBase<float, 3, 3>;
using m44 = MatrixBase<float, 4, 4>;

v3 multiply_point(m44 const & mat, v3 point)
{
	MatrixBase<float,4,1> vecMatrix = { point.x, point.y, point.z, 1.0	};
	MatrixBase<float,4,1> product = matrix::multiply(mat, vecMatrix);

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

	auto lhs = matrix::transpose(mat);

	direction = {
		dot(convert_to<v3>(lhs[0]), direction),
		dot(convert_to<v3>(lhs[1]), direction),
		dot(convert_to<v3>(lhs[2]), direction)
	};
	return direction;	
}

m44
make_translation_matrix(v3 translation)
{
	m44 result = matrix::make_identity<m44>();
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
	matrix = matrix::transpose(matrix);

	translation = -multiply_point(matrix, translation);

	matrix[3] = { translation.x, translation.y, translation.z, 1 };
	matrix(3,3) = 1;
	return matrix;
}

template<typename TMatrix> TMatrix 
matrix::make_identity()
{
	static_assert(TMatrix::columns_count == TMatrix::rows_count, "No identity available for non-square matrices.");

	TMatrix result = {};
	for (s32 i = 0; i < TMatrix::rows_count; ++i)
	{
		result(i, i) = 1;
	}
	return result;
}

/* Todo(Leo): we COULD do some smart tricks with this depending
on shape of the matrix. */
template<typename TMatrix> matrix::TransposeType<TMatrix>
matrix::transpose(TMatrix const & matrix)
{
	TransposeType<TMatrix> result;

	for (u32 col = 0; col < TMatrix::columns_count; ++col)
		for (u32 row = 0; row < TMatrix::rows_count; ++row)
		{
			result(col, row) = matrix(row, col);
		}
	return result;
}

template<typename Scalar, u32 LeftRows, u32 InnerSize, u32 RightColumns>
MatrixBase<Scalar, LeftRows, RightColumns>
operator * (MatrixBase<Scalar, LeftRows, InnerSize> const & lhs,
			MatrixBase<Scalar, InnerSize, RightColumns> const & rhs)
{
	auto lhsTranspose = matrix::transpose(lhs);

	MatrixBase<Scalar, LeftRows, RightColumns> product;

	for(u32 col = 0; col < RightColumns; ++col)
		for (u32 row = 0; row < LeftRows; ++row)
		{
			product(col, row) = vector::dot(lhsTranspose[row], rhs[col]);
		}

	return product;
}  

template<typename TLeft, typename TRight> matrix::ProductType<TLeft, TRight>
matrix::multiply(TLeft const & lhs, TRight const & rhs)
{
	return lhs * rhs;
}

#if MAZEGAME_INCLUDE_STD_IOSTREAM
namespace std
{
	template<typename S, u32 C, u32 R> ostream &
	operator << (ostream & os, MatrixBase<S, C, R> mat)
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
