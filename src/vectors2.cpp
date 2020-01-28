#include <type_traits>

template<typename T, u32 D>
struct BaseVec
{
	using scalar_type = T;
	static constexpr u32 dimension = D;
};



template<typename T>
constexpr bool32 isVectorType = std::is_base_of_v<BaseVec<typename T::scalar_type, T::dimension>, T>;

struct v2 : BaseVec<float, 2> {	float x, y; };
struct v3 : BaseVec<float, 3> { float x, y, z; };
struct v4 : BaseVec<float, 4> { float x, y, z, w; };


static_assert(isVectorType<v2>, "v2 is not vector");

void test_vectors_2()
{
	BaseVec<float, 2> b;


	v2 a;
	std::cout << sizeof(b) << ", " << sizeof(a) << "\n";
}