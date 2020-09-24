/*
Leo Tamminen

Old, nasty, not nice and flexible array
*/

template<typename T>
struct Array
{
	Array() = default;
	Array(T * memory, u64 count, u64 capacity);

	Array(Array &&);	
	Array & operator= (Array &&);

	// NO COPYING MOFO
	Array(Array const &) 			= delete;
	void operator = (Array const &) = delete;


	T * data ()					{ return memory_; }
	T const * data ()	const 	{ return memory_; }
	u64 count () const			{ return count_; }
	u64 capacity () const 		{ return capacity_; }

	T & operator[] (u64 index)
	{ 
		Assert(index < count_);
		return memory_[index];
	}

	// Todo(Leo): use enable if to make a version of this to return value for simpler types
	T const & operator[] (u64 index) const
	{ 
		Assert(index < count_);
		return memory_[index];
	}

	T & push(T value);
	void push_many(std::initializer_list<T> values);
	T * push_return_pointer();
	T * push_return_pointer(T value);

	void flush()
	{
		count_ = 0;
	}

	T * begin() {return data();}
	T const * begin() const {return memory_;}

	T * end() {return data() + count();}
	T const * end() const {return memory_ + count();}

	T & last() { return memory_[count_ - 1]; }
	T last() const { return memory_[count_ - 1]; }

private:
	T * memory_;
	u64 count_;
	u64 capacity_;
};

template<typename T>
Array<T>::Array(T * memory, u64 count, u64 capacity) : 	memory_(memory),
														count_(count),
														capacity_(capacity) {}

template<typename T>
Array<T>::Array(Array<T> && old) : 	memory_(old.memory_),
									capacity_(old.capacity_),
									count_(old.count_)
{
	old.memory_ 	= nullptr;
	old.capacity_ 	= 0;
	old.count_ 		= 0;
};

template<typename T>
Array<T> & Array<T>::operator= (Array<T> && old)
{
	/* Note(Leo): Prevent assigning to an already used array. User code must
	be responsible for clearing memory in case of array of arrays, because
	by default allocating array does not initialize to zero. 

	NOTE(Leo): This has proven super good, do not remove. */
	Assert(this->memory_ == nullptr);

	this->memory_ 	= old.memory_;
	this->capacity_ = old.capacity_;
	this->count_ 	= old.count_;

	old.memory_ 	= nullptr;
	old.capacity_ 	= 0;
	old.count_ 		= 0;

	return *this;
};



template<typename T>
T & Array<T>::push(T value)
{
	Assert(count_ < capacity_);
#if 0
	/* Note(Leo): We could use this, but when we have an array of arrays, 
	it bypasses the assertion in move assignment operator, so we might end
	up assigning to an array that is already in use */
	T * address = memory_ + count_;
	new(address) T(std::move(value));
#else
	memory_[count_] = std::move(value);
#endif
	++count_;

	return memory_[count_ - 1];
}

template<typename T>
T * Array<T>::push_return_pointer()
{
	Assert(count_ < capacity_);

	memory_[count_] = {};
	T * result = &memory_[count_];

	++count_;

	return result;
}

template<typename T>
T * Array<T>::push_return_pointer(T value)
{
	Assert(count_ < capacity_);

	memory_[count_] = std::move(value);
	T * result = &memory_[count_];

	++count_;

	return result;
}

template<typename T>
void Array<T>::push_many(std::initializer_list<T> values)
{
	for (auto item : values)
	{
		push(item);
	}
}

template<typename T>
void reset_array(Array<T> & array)
{
	// TODO(Leo): rechek this when there is granular deallocation possibility
	new (&array) Array<T>(nullptr, 0, 0);
}