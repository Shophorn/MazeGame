/*=============================================================================
Leo Tamminen

Matrix structure declaration and definition.
Matrices are column major.

Todo(Leo):
	- make matrix calls free functions
=============================================================================*/
#define MATRIX_TEMPLATE template <typename ValueType, int RowsCount, int ColumnsCount>
#define MATRIX_TYPE MatrixBase<ValueType, RowsCount, ColumnsCount>

#define MATRIX_LOOP_ELEMENTS for (int col = 0; col < columns_count; ++col) for (int row = 0; row < rows_count; ++row)

#define STATIC_ASSERT_SQUARE_MATRIX static_assert(RowsCount == ColumnsCount, "Matrix must be square matrix")
#define STATIC_ASSERT_TRANSFORM_MATRIX static_assert((RowsCount == 4) && (ColumnsCount == 4), "Matrix must be 4x4 transform matrix")


MATRIX_TEMPLATE
struct MatrixBase
{
	using value_type 					= ValueType;
	constexpr static int rows_count 	= RowsCount;
	constexpr static int columns_count 	= ColumnsCount;

	using matrix_type = MatrixBase<value_type, rows_count, columns_count>;

	using row_vector_type = VectorBase<ValueType, ColumnsCount>;
	using column_vector_type = VectorBase<ValueType, RowsCount>;


	/// -------- CONTENTS -------------------
	column_vector_type 	columns [columns_count];


	/// ------- ACCESSORS ----------------------------
	column_vector_type & operator [] (int index) { return columns[index]; }
	column_vector_type operator [] (int index) const { return columns[index]; }
	
	row_vector_type
	GetRow(int row)
	{
		row_vector_type result;
		for (int col = 0; col < columns_count; ++col)
		{
			result[col] = columns[col][row];
		}
		return result;
	}

	/// ------- BASIC CONSTRUCTORS ------------------------
	constexpr class_member matrix_type
	Diagonal(ValueType value)
	{
		STATIC_ASSERT_SQUARE_MATRIX;

		matrix_type result = {};
		for (int i = 0; i < RowsCount; ++i)
		{
			result [i][i] = value;
		}
		return result;
	}

	constexpr class_member matrix_type
	Identity()
	{
		STATIC_ASSERT_SQUARE_MATRIX;

		matrix_type result = Diagonal(1);
		return result;
	}

	auto Transpose()
	{
		using transpose_type = MatrixBase<ValueType, columns_count, rows_count>;
		
		transpose_type result;
		for (int row = 0; row < rows_count; ++row)
		{
			result.columns[row] = GetRow(row);
		}
		return result;
	}

	/// --------- TRANSFORMS ---------------------------
	// Note(Leo): These are only available for 4x4 transform matrices
	class_member matrix_type
	Translate(VectorBase<ValueType, 3> translate)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		matrix_type result = Diagonal(1);
		result[3] = vector4{translate.x, translate.y, translate.z, 1.0f};
		return result;
	}

	class_member matrix_type
	Scale(VectorBase<ValueType, 3> scale)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		matrix_type result = {};
		result[0][0] = scale [0];
		result[1][1] = scale [1];
		result[2][2] = scale [2];
		result[3][3] = 1.0f;
		return result;
	}

	class_member matrix_type
	Scale(ValueType value)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		matrix_type result = {};
		result[0][0] = value;
		result[1][1] = value;
		result[2][2] = value;
		result[3][3] = 1.0f;
		return result;
	}

	class_member matrix_type
	Rotate(Quaternion rotation)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		float 	x = rotation.x,
				y = rotation.y,
				z = rotation.z,
				w = rotation.w;

		matrix_type result =
		{
			1 - 2*y*y - 2*z*z, 	2*x*y-2*w*z, 		2*x*z + 2*w*y, 		0,
			2*x*y + 2*w*z, 		1 - 2*x*x - 2*z*z,	2*y*z - 2*w*x,		0,
			2*x*z - 2*w*y,		2*y*z + 2*w*x,		1 - 2*x*x - 2*y*y,	0,
			0,					0,					0,					1
		};

		return result;	
	}


	class_member matrix_type
	Transform(vector3 translation, float uniformScale = 1.0f)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		matrix_type result = {};

		result[0][0] = uniformScale;
		result[1][1] = uniformScale;
		result[2][2] = uniformScale;

		result[3] = { translation.x, translation.y, translation.z, 1.0f };

		return result;
	}

	class_member matrix_type
	Transform(vector3 translation, Quaternion rotation, float uniformScale = 1.0f)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		float 	x = rotation.x,
				y = rotation.y,
				z = rotation.z,
				w = rotation.w;

		float s = uniformScale;

		// Note(Leo): Using SSE could be faster than this.
		matrix_type result =
		{
			s * (1 - 2*y*y - 2*z*z), 	s * (2*x*y-2*w*z), 			s * (2*x*z + 2*w*y),		0,
			s * (2*x*y + 2*w*z), 		s * (1 - 2*x*x - 2*z*z),	s * (2*y*z - 2*w*x),		0,
			s * (2*x*z - 2*w*y),		s * (2*y*z + 2*w*x),		s * (1 - 2*x*x - 2*y*y),	0,
			translation.x,				translation.y,				translation.z,				1
		};

		return result;	
	}

	// TODO(Leo): Manually inline calculations
	class_member matrix_type
	Transform(vector3 translation, Quaternion rotation, vector3 scale)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		auto result = Translate(translation) * Rotate(rotation) * Scale(scale);
	
		return result;	
	}

};

using Matrix44 = MatrixBase<float, 4, 4>;


// Note(Leo): new style functions here
// Todo(Leo): make all like this
Matrix44
get_rotation_matrix(Quaternion quaternion)
{
	Matrix44 result = Matrix44::Rotate(quaternion);
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
make_transform_matrix(vector3 translation, Quaternion rotation, float uniformScale = 1.0f)
{
	float 	x = rotation.x,
			y = rotation.y,
			z = rotation.z,
			w = rotation.w;

	float s = uniformScale;

	// Note(Leo): Using SSE could be faster than this.
	Matrix44 result =
	{
		s * (1 - 2*y*y - 2*z*z), 	s * (2*x*y-2*w*z), 			s * (2*x*z + 2*w*y),		0,
		s * (2*x*y + 2*w*z), 		s * (1 - 2*x*x - 2*z*z),	s * (2*y*z - 2*w*x),		0,
		s * (2*x*z - 2*w*y),		s * (2*y*z + 2*w*x),		s * (1 - 2*x*x - 2*y*y),	0,
		translation.x,				translation.y,				translation.z,				1
	};

	return result;	
}

template<typename LeftMatrixType, typename RightMatrixType>
using ProductType = MatrixBase<typename LeftMatrixType::value_type,
								LeftMatrixType::rows_count,
								RightMatrixType::columns_count>;	

template <typename ValueType, int LeftRows, int InnerSize, int RightColumns>
auto operator * (
	MatrixBase<ValueType, LeftRows, InnerSize> lhs,
	MatrixBase<ValueType, InnerSize, RightColumns> rhs
){
	// SETUP AND ASSERTIONS :D
	using LeftMatrixType = decltype(lhs);
	using RightMatrixType = decltype (rhs);

	static_assert(	std::is_same_v<typename LeftMatrixType::value_type, typename RightMatrixType::value_type>,
					"Cannot multiply matrices of different value types");

	static_assert(	LeftMatrixType::columns_count == RightMatrixType::rows_count,
					"Matrices' inner sizes must be equal");

	using product_type = ProductType<LeftMatrixType, RightMatrixType>;

	// COMPUTATATION
	auto leftTranspose = lhs.Transpose();
	product_type product;

	for (int col = 0; col < product_type::columns_count; ++col)
		for (int row = 0; row < product_type::rows_count; ++row)
		{ 
			product[col][row] = vector::dot(leftTranspose[row], rhs[col]);
		}

	return product;
}


#if MAZEGAME_INCLUDE_STD_IOSTREAM
namespace std
{
	MATRIX_TEMPLATE ostream &
	operator << (ostream & os, MATRIX_TYPE mat)
	{
		os << "(" << mat[0];
		for (int i = 1; i < ColumnsCount; ++i)
		{
			os << ";" << mat[i];
		}
		os << ")";
		return os;
	}
}
#endif


template<u32 Rows, u32 Columns>
using matrixf = MatrixBase<float, Rows, Columns>;

// Todo (Leo): make template
inline vector3 
operator * (const Matrix44 & mat, vector3 vec)
{
	matrixf<4,1> vecMatrix = { vec.x, vec.y, vec.z, 1.0	};
	matrixf<4,1> product = mat * vecMatrix;

	vec = {
		product[0][0] / product[0][3],
		product[0][1] / product[0][3],
		product[0][2] / product[0][3]
	};
	return vec;
}


#undef MATRIX_TEMPLATE
#undef MATRIX_TYPE

#undef MATRIX_LOOP_ELEMENTS

#undef STATIC_ASSERT_SQUARE_MATRIX
#undef STATIC_ASSERT_TRANSFORM_MATRIX
