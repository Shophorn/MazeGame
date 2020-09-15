template<typename T>
struct PointerStrip
{
	using type = T;
};

template<typename T>
struct PointerStrip<T*>
{
	using type = typename PointerStrip<T>::type;
};

template<typename T>
using pointer_strip = typename PointerStrip<T>::type;

template<typename T, typename U>
constexpr bool is_same_type = false;

template<typename T>
constexpr bool is_same_type<T,T> = true;
