/*=============================================================================
Leo Tamminen

Matrix structure declaration and definition.
Matrices are column major.
=============================================================================*/
template <typename Scalar, u32 Rows, u32 Columns>
struct MatrixBase
{
	using scalar_type 					= Scalar;
	constexpr static u32 rows_count 	= Rows;
	constexpr static u32 columns_count 	= Columns;

	using column_type = VectorBase<Scalar, Rows>;

	/// -------- CONTENTS -------------------
	column_type columns [columns_count];

	/// ------- ACCESSORS ----------------------------
	column_type & operator [] (s32 index) { return columns[index]; }
	column_type operator [] (s32 index) const { return columns[index]; }
};

namespace
{
	template<typename TMatrix>
	constexpr bool is_square_matrix = (TMatrix::columns_count == TMatrix::rows_count);

	template<typename TLeft, typename TRight>
	constexpr bool is_same_scalar_type = std::is_same_v<typename TLeft::scalar_type, typename TRight::scalar_type>;

	template <typename TMatrix>
	using RowVectorType = VectorBase<typename TMatrix::scalar_type, TMatrix::columns_count>;

	template<typename TMatrix>
	using ColumnVectorType = VectorBase<typename TMatrix::scalar_type, TMatrix::rows_count>;

	template<typename TMatrix>
	using TransposeMatrixType = MatrixBase<typename TMatrix::scalar_type,
											TMatrix::columns_count,
											TMatrix::rows_count>;

	template<typename TLeft, typename TRight>
	using ProductMatrixType = MatrixBase<typename TLeft::scalar_type,
											TLeft::rows_count,
											TRight::columns_count>;
}

namespace matrix
{
	template<typename TMatrix> TMatrix make_identity();

	template<typename TMatrix> RowVectorType<TMatrix> get_row(TMatrix const & matrix, s32 row);
	template<typename TMatrix> TransposeMatrixType<TMatrix> transpose(TMatrix const & matrix);
	template<typename TLeft, typename TRight> ProductMatrixType<TLeft, TRight> multiply(TLeft, TRight const &);	
}

using Matrix44 = MatrixBase<float, 4, 4>;

Matrix44
make_translation_matrix(vector3 translation)
{
	Matrix44 result = matrix::make_identity<Matrix44>();
	result[3] = vector4{translation.x, translation.y, translation.z, 1.0f};
	return result;
}

Matrix44
make_rotation_matrix(quaternion rotation)
{
	float 	x = rotation.x,
			y = rotation.y,
			z = rotation.z,
			w = rotation.w;

	Matrix44 result =
	{
		1 - 2*y*y - 2*z*z, 	2*x*y-2*w*z, 		2*x*z + 2*w*y, 		0,
		2*x*y + 2*w*z, 		1 - 2*x*x - 2*z*z,	2*y*z - 2*w*x,		0,
		2*x*z - 2*w*y,		2*y*z + 2*w*x,		1 - 2*x*x - 2*y*y,	0,
		0,					0,					0,					1
	};

	return result;	
}

Matrix44
make_scale_matrix(vector3 scale)
{
	Matrix44 result = {};
	result[0][0] = scale [0];
	result[1][1] = scale [1];
	result[2][2] = scale [2];
	result[3][3] = 1.0f;
	return result;	
}

Matrix44
make_transform_matrix(vector3 translation, float uniformScale = 1.0f)
{
	Matrix44 result = {};

	result[0][0] = uniformScale;
	result[1][1] = uniformScale;
	result[2][2] = uniformScale;

	result[3] = { translation.x, translation.y, translation.z, 1.0f };

	return result;
}

Matrix44
make_transform_matrix(vector3 translation, quaternion rotation, float uniformScale = 1.0f)
{
	float 	x = rotation.x,
			y = rotation.y,
			z = rotation.z,
			w = rotation.w;

	float s = uniformScale;

	Matrix44 result =
	{
		s * (1 - 2*y*y - 2*z*z), 	s * (2*x*y-2*w*z), 			s * (2*x*z + 2*w*y),		0,
		s * (2*x*y + 2*w*z), 		s * (1 - 2*x*x - 2*z*z),	s * (2*y*z - 2*w*x),		0,
		s * (2*x*z - 2*w*y),		s * (2*y*z + 2*w*x),		s * (1 - 2*x*x - 2*y*y),	0,
		translation.x,				translation.y,				translation.z,				1
	};

	return result;	
}

template<typename TMatrix> TMatrix 
matrix::make_identity()
{
	static_assert(is_square_matrix<TMatrix>, "No identity available for non-square matrices.");

	TMatrix result = {};
	for (s32 i = 0; i < TMatrix::rows_count; ++i)
	{
		result [i][i] = 1;
	}
	return result;
}

template<typename TMatrix> RowVectorType<TMatrix>
matrix::get_row(TMatrix const & matrix, s32 row)
{
	RowVectorType<TMatrix> result;
	for (s32 col = 0; col < TMatrix::columns_count; ++col)
	{
		result[col] = matrix[col][row];
	}
	return result;
}

/* Todo(Leo): we COULD do some smart tricks with this depending
on shape of the matrix. */
template<typename TMatrix> TransposeMatrixType<TMatrix>
matrix::transpose(TMatrix const & matrix)
{
	TransposeMatrixType<TMatrix> result;

	for (u32 col = 0; col < TMatrix::columns_count; ++col)
		for (u32 row = 0; row < TMatrix::rows_count; ++row)
		{
			result[row][col] = matrix[col][row];
		}
	return result;
}

template<typename TLeft, typename TRight> ProductMatrixType<TLeft, TRight>
matrix::multiply(TLeft lhs, TRight const & rhs)
{
	static_assert(is_same_scalar_type<TLeft, TRight>, "Cannot multiply matrices of different scalar types.");

	using namespace vector;

	// Note(Leo): this is done to get easy access to rows
	lhs = transpose(lhs);

	ProductMatrixType<TLeft, TRight> product;
	constexpr u32 columns 	= decltype(product)::columns_count;
	constexpr u32 rows 		= decltype(product)::rows_count;

	for (u32 col = 0; col < columns; ++col)
		for (u32 row = 0; row < rows; ++row)
		{
			product[col][row] = dot(lhs[row], rhs[col]);
		}

	return product;
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

// Todo (Leo): make template
inline vector3 
operator * (const Matrix44 & mat, vector3 vec)
{
	MatrixBase<float,4,1> vecMatrix = { vec.x, vec.y, vec.z, 1.0	};
	MatrixBase<float,4,1> product = matrix::multiply(mat, vecMatrix);//mat * vecMatrix;

	vec = {
		product[0][0] / product[0][3],
		product[0][1] / product[0][3],
		product[0][2] / product[0][3]
	};
	return vec;
}
