/*=============================================================================
Leo Tamminen

Matrix structure declaration and definition.
Matrices are column major.
=============================================================================*/
#define MATRIX_TEMPLATE template <typename ValueType, int RowsCount, int ColumnsCount>
#define MATRIX_TYPE MatrixBase<ValueType, RowsCount, ColumnsCount>

#define MATRIX_LOOP_ELEMENTS for (int col = 0; col < columns_count; ++col) for (int row = 0; row < rows_count; ++row)

#define STATIC_ASSERT_SQUARE_MATRIX static_assert(RowsCount == ColumnsCount, "Matrix must be square matrix")
#define STATIC_ASSERT_TRANSFORM_MATRIX static_assert((RowsCount == 4) && (ColumnsCount == 4), "Matrix must be 4x4 transform matrix")

/* This is a tag to mark variables explicitly as not initialized.
By convention variables should be initialized or marked MATRIX_NO_INIT */
#define MATRIX_NO_INIT

MATRIX_TEMPLATE
union MatrixBase
{
	using value_type 					= ValueType;
	constexpr static int rows_count 	= RowsCount;
	constexpr static int columns_count 	= ColumnsCount;

	using matrix_type = MatrixBase<value_type, rows_count, columns_count>;

	using row_vector_type = VectorBase<ValueType, ColumnsCount>;
	using column_vector_type = VectorBase<ValueType, RowsCount>;


	/// -------- CONTENTS -------------------
	// Todo(Leo): Is this better than columns only?
	column_vector_type 	columns [columns_count];
	value_type 			values 	[rows_count * columns_count];


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
		
		MATRIX_NO_INIT transpose_type result;

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
		result[3] = Vector4{translate.x, translate.y, translate.z, 1.0f};
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

		real32 	x = rotation.x,
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
	Transform(Vector3 translation, real32 uniformScale = 1.0f)
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
	Transform(Vector3 translation, Quaternion rotation, real32 uniformScale = 1.0f)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		real32 	x = rotation.x,
				y = rotation.y,
				z = rotation.z,
				w = rotation.w;

		real32 s = uniformScale;

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
	Transform(Vector3 translation, Quaternion rotation, Vector3 scale)
	{
		STATIC_ASSERT_TRANSFORM_MATRIX;

		auto result = Translate(translation) * Rotate(rotation) * Scale(scale);
	
		return result;	
	}

};


template <typename ValueType, int LeftRows, int InnerSize, int RightColumns>
auto operator * (
	MatrixBase<ValueType, LeftRows, InnerSize> lhs,
	MatrixBase<ValueType, InnerSize, RightColumns> rhs
){
	using LeftMatrixType = decltype(lhs);
	using RightMatrixType = decltype (rhs);

	static_assert(
		std::is_same_v<typename LeftMatrixType::value_type, typename RightMatrixType::value_type>,
		"Cannot multiply matrices of different value types");

	static_assert(
		LeftMatrixType::columns_count == RightMatrixType::rows_count,
		"Matrices' inner sizes must be equal");

	using value_type = typename LeftMatrixType::value_type;
	constexpr int rows_count = LeftMatrixType::rows_count;
	constexpr int columns_count = RightMatrixType::columns_count;

	using result_type = MatrixBase<value_type, rows_count, columns_count>;

	/* Todo(Leo): this would be better getting columns from transpose instead of
	GetRow in each iteration */
	result_type result;
	MATRIX_LOOP_ELEMENTS { result[col][row] = Dot(lhs.GetRow(row), rhs[col]); }
	return result;
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


using Matrix44 = MatrixBase<real32, 4, 4>;


// Todo (Leo): make template
inline Vector3 
operator * (const Matrix44 & mat, Vector3 vec)
{
	MatrixBase<real32, 4, 1> vecMatrix = { vec.x, vec.y, vec.z, 1.0	};
	MatrixBase<real32, 4, 1> product = mat * vecMatrix;

	// std::cout << vec << ", " << product << "\n";

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
