namespace experimental
{
	template<typename T>
	struct IteratorRange
	{
		T * begin_;
		T * end_;

		T * begin() { return begin_; }
		T * end() { return end_; }

		T const * begin() const { return begin_; }
		T const * end() const { return end_; }
	};

	template<typename T> IteratorRange<T> fixed_range (Array2<T> & array)
	{
		return IteratorRange<T> {array.begin(), array.end()};
	}

	template <typename T> IteratorRange<T> fixed_range(T * memory, s32 count)
	{
		return IteratorRange<T>{memory, memory + count};
	}
}