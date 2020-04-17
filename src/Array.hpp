#include <vector>

global_variable int global_arrayEmptyDctorCounter 		= 0;
global_variable int global_arrayHasContentDctorCounter = 0;

template<typename T>
struct Array
{
	Array() = default;
	Array(T * memory, u64 count, u64 capacity);

	Array(Array &&);	
	Array & operator= (Array && old)
	{
		// TODO(Leo): IMPORTANT we must do this, but first we need a debugger to find out where it fails
		Assert(this->memory_ == nullptr);

		this->memory_ 	= old.memory_;
		this->capacity_ = old.capacity_;
		this->count_ 	= old.count_;

		old.memory_ 	= nullptr;
		old.capacity_ 	= 0;
		old.count_ 		= 0;

		return *this;
	};

	// ~Array()
	// { 

	// 	std::cout << "Hello from array dctor";
	// 	if (memory_ != nullptr)
	// 	{
	// 		global_arrayHasContentDctorCounter++;
	// 		std:: cout << " NOT EMPTY, type = " << typeid(T).name() << ", counter: " << global_arrayHasContentDctorCounter <<"\n";
	// 	}
	// 	else
	// 	{
	// 		global_arrayEmptyDctorCounter++;
	// 		std::cout << ", counter: " << global_arrayEmptyDctorCounter << "\n";
	// 	}
	// }

	// NO COPYING MOFO
	Array(Array const &) 			= delete;
	void operator = (Array const &) = delete;


	T * data ()		{ return memory_; }
	T const * data ()	const { return memory_; }
	u64 count () const	{ return count_; }
	u64 capacity () const { return capacity_; }

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

	void push(T value);
	void push_many(std::initializer_list<T> values);

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
void Array<T>::push(T value)
{
	Assert(count_ < capacity_);
	memory_[count_] = std::move(value);
	++count_;
}

template<typename T>
void Array<T>::push_many(std::initializer_list<T> values)
{
	for (auto item : values)
	{
		push(item);
	}
}
