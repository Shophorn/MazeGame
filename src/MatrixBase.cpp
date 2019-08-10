/*=============================================================================
Leo Tamminen

Matrix structure declaration and definition.
Matrices are column major.
=============================================================================*/
#define MATRIX_TEMPLATE template <typename ValueType, int RowsCount, int ColumnsCount>
#define MATRIX_TYPE MatrixBase<ValueType, RowsCount, ColumnsCount>

#define MATRIX_LOOP_ELEMENTS for (int col = 0; col < columns_count; ++col) for (int row = 0; row < rows_count; ++row)

#define ASSERT_SQUARE_MATRIX static_assert(RowsCount == ColumnsCount, "Matrix must be square matrix")
#define ASSERT_TRANSFORM_MATRIX static_assert((RowsCount == 4) && (ColumnsCount == 4), "Matrix must be 4x4 transform matrix")

MATRIX_TEMPLATE
union MatrixBase
{
	using value_type 					= ValueType;
	constexpr static int rows_count 	= RowsCount;
	constexpr static int columns_count 	= ColumnsCount;

	using row_vector_type = VectorBase<ValueType, ColumnsCount>;
	using column_vector_type = VectorBase<ValueType, RowsCount>;

	// -------- CONTENTS -------------------
	// Todo(Leo): Is this better than columns only?
	ValueType values[columns_count * rows_count];
	column_vector_type columns[ColumnsCount];

	column_vector_type & operator [] (int index) { return columns[index]; }
	column_vector_type operator [] (int index) const { return columns[index]; }

	class_member MATRIX_TYPE
	Diagonal(ValueType value)
	{
		ASSERT_SQUARE_MATRIX;

		MATRIX_TYPE result = {};
		for (int i = 0; i < RowsCount; ++i)
		{
			result [i][i] = value;
		}
		return result;
	}

	class_member MATRIX_TYPE
	Identity()
	{
		ASSERT_SQUARE_MATRIX;

		MATRIX_TYPE result = Diagonal(1);
		return result;
	}

	class_member MATRIX_TYPE
	Translate(VectorBase<ValueType, 3> translation)
	{
		ASSERT_TRANSFORM_MATRIX;

		MATRIX_TYPE result = Diagonal(1);
		result[3] = Vector4{translation.x, translation.y, translation.z, 1.0f};
		return result;
	}

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
};

template<typename MatrixType> MatrixType
Diagonal(typename MatrixType::value_type value)
{
	MatrixType result = {};
	for (int i = 0; i < MatrixType::rows_count; ++i)
	{
		result(i, i) = value;
	}
	return result;
}

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

	result_type result;
	MATRIX_LOOP_ELEMENTS { result[col][row] = Dot(lhs.GetRow(row), rhs[col]); }
	return result;
}


#undef MATRIX_TEMPLATE
#undef MATRIX_TYPE

#undef ASSERT_SQUARE_MATRIX
#undef ASSERT_TRANSFORM_MATRIX

using Matrix44 = MatrixBase<real32, 4, 4>;
