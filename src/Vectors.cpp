/*=============================================================================
Leo Tamminen

Vector function templates and template instantiations

TODO(Leo):
	- Full testing
	- SIMD / SSE / AVX
=============================================================================*/
template<typename S, u32 D>
struct Vector;

namespace vector_impl_
{
	template<typename S, u32 D> S square_magnitude(void const * vec)
	{
		S result = 0;
		for (int i = 0; i < D; ++i)
		{
			// Note(Leo): There is a 'hidden' subscript operator in the end of expression....
			S element = reinterpret_cast<S const *>(vec)[i];
			result += element * element;
		}
		return result;
	}

	template <typename S, u32 D> void divide(void * vec, S divisor)
	{
		Assert(divisor != 0);
		
		for (int i = 0; i < D; ++i)
		{
			reinterpret_cast<S*>(vec)[i] /= divisor;
		}
	}

	template <typename S, s32 D> S sum(void const * vec)
	{
		S sum = 0;
		for(int i = 0; i < D; ++i)
		{
			sum += reinterpret_cast<S const *>(vec)[i];
		}
		return sum;
	}
}

// ------------------ DEFINITIONS ---------------------------------
template<typename S>
struct Vector<S, 2>
{ 
	S x, y;

	S magnitude() const 		{ return math::square_root(vector_impl_::square_magnitude<S, 2>(this)); }
	S square_magnitude() const 	{ return vector_impl_::square_magnitude<S, 2>(this); }

	Vector normalized() const 	{ Vector result = *this; vector_impl_::divide<S, 2>(&result, magnitude()); return result; }

};

using v2 = Vector<float, 2>;
using point2 = Vector<u32, 2>;


f32 dot_product(v2 a, v2 b)
{
	f32 p = { a.x * b.x + a.y * b.y };
	return p;
}

f32 magnitude(v2 v)
{
	f32 m = math::square_root(v.x * v.x + v.y * v.y);
	return m;
}

v2 divide(v2 v, f32 f)
{
	v.x /= f;
	v.y /= f;
	return v;
}

v2 normalize(v2 v)
{
	v = divide(v, magnitude(v));
	return v;
}



template<typename S>
struct Vector<S, 3>
{ 
	S x, y, z; 

	Vector<S, 2> * xy () { return reinterpret_cast<Vector<S,2>*>(this); }
	Vector<S, 2> const * xy() const { return reinterpret_cast<Vector<S, 2> const *>(this); }

	static Vector const right;
	static Vector const forward;
	static Vector const up;

	S magnitude() const 		{ return math::square_root(vector_impl_::square_magnitude<S, 3>(this)); }
	S square_magnitude() const 	{ return vector_impl_::square_magnitude<S, 3>(this); }

	Vector normalized() const 	{ Vector result = *this; vector_impl_::divide<S, 3>(&result, magnitude()); return result; }
};
using v3 	= Vector<float, 3>;

template<typename S> Vector<S,3> constexpr Vector<S,3>::right 	= {1,0,0};
template<typename S> Vector<S,3> constexpr Vector<S,3>::forward = {0,1,0};
template<typename S> Vector<S,3> constexpr Vector<S,3>::up 		= {0,0,1};

f32 magnitude(v3 v)
{
	using namespace math;

	f32 m = square_root(pow2(v.x) + pow2(v.y) + pow2(v.z));
	return m;
}

v3 divide(v3 v, f32 f)
{
	v.x /= f;
	v.y /= f;
	v.z /= f;

	return v;
}

v3 normalize(v3 v)
{
	v = divide(v, magnitude(v));
	return v;
}






struct world
{
	/*
	Note(Leo): Right handed coordinate system, x right, y, forward, z up.
	*/

	static constexpr v3 left 		= {-1,  0,  0};
	static constexpr v3 right 		= { 1,  0,  0};
	static constexpr v3 back 		= { 0, -1,  0};
	static constexpr v3 forward 	= { 0,  1,  0};
	static constexpr v3 down 		= { 0,  0, -1};
	static constexpr v3 up 			= { 0,  0,  1};
};

template<typename S>
struct Vector<S, 4>
{
	S x, y, z, w;

	S magnitude() const 		{ return math::square_root(vector_impl_::square_magnitude<S, 4>(this)); }
	S square_magnitude() const 	{ return vector_impl_::square_magnitude<S, 4>(this); }

	Vector normalized() const 	{ Vector result = *this; vector_impl_::divide<S, 4>(&result, magnitude()); return result; }
};

using v4 		= Vector<float,4>;
using float4 	= Vector<float, 4>;

using point4_u32 = Vector<u32, 4>;

namespace vector
{
	template<typename S>
	constexpr S epsilon = 0.000001f;

	template <typename S, u32 D> S * 				begin(Vector<S, D> * vec);
	template <typename S, u32 D> S const * 			const_begin(Vector<S, D> const * vec);

	template<typename S, u32 D> S 					dot(Vector<S,D> const & a, Vector<S,D> const & b);
	template<typename S, u32 D>	Vector<S,D>			interpolate(Vector<S,D> a, Vector<S,D> b, S t);
	template<typename S, u32 D> Vector<S,D> 		normalize_or_zero(Vector<S,D> vec);
	template<typename S, u32 D> Vector<S, D>		clamp_length(Vector<S,D> vec, S maxLength);
	template<typename S, u32 D> Vector<S, D> 		scale(Vector<S,D> vec, Vector<S,D> const & scale);
	template<typename S, u32 D> void 				dissect (Vector<S,D> const & vec, Vector<S,D> * outDirection, S * outLength);
	template<typename S, u32 D> Vector<S,D>			project (Vector<S,D> vec, Vector<S,D> const & projectionTarget);
	template<typename S, u32 D> S 					get_sqr_distance(Vector<S,D> const & a, Vector<S,D> const & b);

	template<typename TNew, typename Told> TNew		convert_to(Told const & old);

	// 3-dimensional vectors specific
	template<typename S> Vector<S, 3>				cross(Vector<S,3> lhs, Vector<S,3> const & rhs);
	template<typename S> Vector<S, 3> 				rotate(Vector<S,3> vec, Vector<S,3> const & axis, S angleInRadians);
	template<typename S> S 							get_signed_angle(Vector<S,3> const & from, Vector<S,3> const & to, Vector<S,3> const & axis);

	template<typename S, u32 D> S coeff_sum(Vector<S,D> const &vec)
	{
		return vector_impl_::sum<S,D>(&vec);
	}
}

namespace vector_operators_
{
	#define VECTOR_OPERATOR template<typename S, u32 D> Vector<S,D>

	VECTOR_OPERATOR & 	operator += (Vector<S,D> &, Vector<S,D> const &);
	VECTOR_OPERATOR 	operator + 	(Vector<S,D>, 	Vector<S,D> const &);
	VECTOR_OPERATOR &	operator -= (Vector<S,D> &, Vector<S,D> const &);
	VECTOR_OPERATOR 	operator - 	(Vector<S,D>, 	Vector<S,D> const &);

	VECTOR_OPERATOR &	operator *= (Vector<S,D> &, S);
	VECTOR_OPERATOR 	operator * 	(Vector<S,D>, 	S);
	VECTOR_OPERATOR 	operator *	(S, Vector<S, D>);
	VECTOR_OPERATOR &	operator /= (Vector<S,D> &, S);
	VECTOR_OPERATOR 	operator / 	(Vector<S,D>, 	S);

	VECTOR_OPERATOR		operator - 	(Vector<S,D>);

	#undef VECTOR_OPERATOR
}

/* Note(Leo): these are in namespace to begin with only that we get errors
if either definition or declaration is changed. */
using namespace vector_operators_;

template<typename S, u32 D> Vector<S,D> &
vector_operators_::operator += (Vector<S,D> & a, Vector<S,D> const & b)
{
	S * pA 			= vector::begin(&a);
	S const * pB	= vector::const_begin(&b);

	for (int i = 0; i < D; ++i)
	{
		pA[i] += pB[i];
	}
	return a;
}

template <typename S, u32 D> Vector<S,D>
vector_operators_::operator + (Vector<S,D> a, Vector<S,D> const & b)
{
	a += b;
	return a;
}


template <typename S, u32 D> Vector<S,D> &
vector_operators_::operator -= (Vector<S,D> & a, Vector<S,D> const & b)
{
	S * pA 			= vector::begin(&a);
	S const * pB 	= vector::const_begin(&b);
	
	for (int i = 0; i < D; ++i)
	{
		pA[i] -= pB[i];
	}
	return a;
}

template <typename S, u32 D> Vector<S,D>
vector_operators_::operator - (Vector<S,D> a, Vector<S,D> const & b)
{
	a -= b;
	return a;
}

template <typename S, u32 D> Vector<S,D> &
vector_operators_::operator *= (Vector<S,D> & vec, S value)
{
	auto * pVec = vector::begin(&vec);

	for (int i = 0; i < D; ++i)
	{
		pVec[i] *= value;
	}
	return vec;
}

template <typename S, u32 D> Vector<S,D>
vector_operators_::operator * (Vector<S,D> vec, S value)
{
	vec *= value;
	return vec;
}

template <typename S, u32 D> Vector<S,D>
vector_operators_::operator * (S value, Vector<S,D> vec)
{
	vec *= value;
	return vec;
}

template <typename S, u32 D> Vector<S,D> &
vector_operators_::operator /= (Vector<S,D> & vec, S value)
{
	auto * pVec = vector::begin(&vec);
	for (int i = 0; i < D; ++i)
	{
		pVec[i] /= value;
	}
	return vec;
}

template <typename S, u32 D> Vector<S,D>
vector_operators_::operator / (Vector<S,D> vec, S value)
{
	vec /= value;
	return vec;
}

template <typename S, u32 D> Vector<S,D>
vector_operators_::operator - (Vector<S,D> vec)
{
	auto * pVec = vector::begin(&vec);
	for(int i = 0; i < D; ++i)
	{
		pVec[i] = -pVec[i];
	}
	return vec;
}

template <typename T, u32 D> T * 
vector::begin(Vector<T, D> * vec)
{
	return reinterpret_cast<T*> (vec);
}

template <typename T, u32 D> T const * 
vector::const_begin(Vector<T, D> const * vec)
{
	return reinterpret_cast<T const *>(vec);
}

template<typename S, u32 D> S
vector::dot(Vector<S,D> const & a, Vector<S,D> const & b)
{
	S const * pA = const_begin(&a);
	S const * pB = const_begin(&b);

	S result = 0;
	for (int i = 0; i < D; ++i)
	{
		result += pA[i] * pB[i];
	}
	return result;
}

template<typename S> Vector<S,3>
vector::cross(Vector<S,3> lhs, Vector<S,3> const & rhs)
{
	lhs = {
		lhs.y * rhs.z - lhs.z * rhs.y,
		lhs.z * rhs.x - lhs.x * rhs.z,
		lhs.x * rhs.y - lhs.y * rhs.x
	};
	return lhs;
}

template<typename S, u32 D> Vector<S,D>
vector::interpolate(Vector<S, D> a, Vector<S, D> b, S t)
{
	S * pA = begin(&a);
	S const * pB = const_begin(&b);

	for (int i = 0; i < D; ++i)
	{
		pA[i] = ::interpolate(pA[i], pB[i], t);
	}
	return a;	
}

// template<typename S, u32 D> Vector<S,D>
// vector::normalize(Vector<S,D> vec)
// {
// 	return vec.normalized();

// 	 Note(Leo): We deliberately do handle bad cases here, so that any code where there is risk
// 	to  normalize 0 length vector should handle that themself. so that
// 	// vec /= vec.magnitude();
// 	// return vec;
// }

template<typename S, u32 D> Vector<S,D>
vector::normalize_or_zero(Vector<S,D> vec)
{
	float sqrMagnitude = vec.square_magnitude();
	if (sqrMagnitude < epsilon<S>)
	{
		return vec;
	}

	return vec.normalized();

	vec /= math::square_root(sqrMagnitude);
	return vec;
}

template<typename S, u32 D> Vector<S,D>
vector::clamp_length(Vector<S,D> vec, S maxLength)
{
	auto length = vec.magnitude();
	if (length > maxLength)
	{
		vec *= (maxLength / length);
	}
	return vec;
}

namespace vector_meta_
{
	template<typename S, u32 D>
	struct VectorInfo
	{
		using scalar_type = S;
		constexpr static u32 dimension = D;
	};

	template<template<typename, u32> typename V, typename S, u32 D>
	constexpr VectorInfo<S,D> get_info(V<S,D> const &) { return {}; }
 	
	template<typename TVector>
 	using Info = decltype(get_info(std::declval<TVector>()));

	template<typename> constexpr bool32 isVectorTemplate = false;
	template<typename T, u32 D> constexpr bool32 isVectorTemplate<Vector<T,D>> = true;
}

template<typename TNew, typename TOld> TNew
vector::convert_to(TOld const & old)
{	

	using namespace vector_meta_;

	constexpr bool32 valid = isVectorTemplate<TNew> && isVectorTemplate<TOld>;
	static_assert(valid, "Can only convert Vector types");

	/* Note(Leo): this is to stop compiling these for invalid types. static_assert
	does not necessrily stop there, but merely reports an error. */
	if constexpr(valid)
	{
		using SNew = typename Info<TNew>::scalar_type;
		constexpr u32 dimension = math::min(Info<TNew>::dimension, Info<TOld>::dimension);
		

		TNew result = {};	
	
		auto * pResult 		= begin(&result);
		auto const * pOld 	= const_begin(&old);


		for (u32 i = 0; i < dimension; ++i)
		{
			pResult[i] = static_cast<SNew>(pOld[i]);
		}
		return result;
	}
}

template<typename S, u32 D> Vector<S,D>
vector::scale(Vector<S,D> vec, Vector<S,D> const & scale)
{
	S * pVec 		= begin(&vec);
	S const * pScale = const_begin(&scale);

	for (int i = 0; i < D; ++i)
	{
		pVec[i] *= pScale[i];
	}
	return vec;
}


// VECTOR_3_TEMPLATE VECTOR_3_TYPE
// rotate(VECTOR_3_TYPE vec, VECTOR_3_TYPE axis, Scalar angleInRadians)
// {
// 	// https://math.stackexchange.com/questions/511370/how-to-rotate-one-vector-about-another
// 	VECTOR_3_TYPE projection = project(vec, axis);
// 	VECTOR_3_TYPE rejection = vec - projection;

// 	VECTOR_3_TYPE w = cross(axis, rejection);
	
// 	Scalar rejectionLength = get_length(rejection);
// 	Scalar x1 = cosine(angleInRadians) / rejectionLength;
// 	Scalar x2 = sine(angleInRadians) / get_length(w);

// 	VECTOR_3_TYPE rotatedRejection = (rejection * x1 + w * x2) * rejectionLength	;

// 	vec = projection + rotatedRejection;
// 	return vec;
// }

template<typename S, u32 D> void
vector::dissect(Vector<S,D> const & vec, Vector<S,D> * outDirection, S * outLength)
{
	*outLength = vec.magnitude();

	if (*outLength < epsilon<S>)
	{
		*outLength = 0;
		*outDirection = {0, 0, 0};
	}
	else
	{
		*outDirection = vec / (*outLength);
	}
}


template<typename S, u32 D> Vector<S,D>
vector::project(Vector<S,D> vec, Vector<S,D> const & projectionTarget)
{
	S c = dot(projectionTarget, vec) / dot(vec, vec);
	return projectionTarget * c;
}

template<typename S> S
vector::get_signed_angle(Vector<S,3> const & from, Vector<S,3> const & to, Vector<S,3> const & axis)
{
	// Todo(Leo): Check if this relly is correct
	S a = dot(cross(from, to), axis);
	S b = dot(to, from);

	S result = arc_tan_2(a, b);
	return result;
}

template<typename S, u32 D> S
vector::get_sqr_distance(Vector<S,D> const & a, Vector<S,D> const & b)
{
	S const * pA = const_begin(&a);
	S const * pB = const_begin(&b);

	S distance = 0;
	for (u32 i = 0; i < D; ++i)
	{
		S s = pA[i] - pB[i];
		distance += s * s;
	}
	return distance;
}

template<typename S> Vector<S,3>
vector::rotate(Vector<S,3> vec, Vector<S,3> const & axis, S angleInRadians)
{
	auto projection = dot(axis, vec) * axis;
	auto rejection = vec - projection;

	auto rotatedRejection = cosine(angleInRadians) * rejection
							+ sine(angleInRadians) * cross(axis, rejection);

	// Vector<S,3> projection 	= project(vec, axis);
	// Vector<S,3> rejection 	= vec - projection;

	// Vector<S,3> w 			= cross(axis, rejection);
	// S rejectionLength 		= get_length(rejection);
	// S x1 					= cosine(angleInRadians) / rejectionLength;
	// S x2 					= sine(angleInRadians) / get_length(w);

	// Vector<S,3> rotatedRejection = (rejection * x1 + w * x2) * rejectionLength;

	vec = projection + rotatedRejection;
	return vec;	
}

#if MAZEGAME_INCLUDE_STD_IOSTREAM

// namespace std
// {	
	template<typename S, u32 D> std::ostream &
	operator << (std::ostream & os, Vector<S,D> const & vec)
	{
		S const * pVec = vector::const_begin(&vec);
		os << "(" << pVec[0];
		for (int i = 1; i < D; ++i)
		{
			os << "," << pVec[i];
		}
		os << ")";
		return os;
	}
// }

#endif