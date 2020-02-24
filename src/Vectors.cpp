/*=============================================================================
Leo Tamminen

Vector function templates and template instantiations

TODO(Leo):
	- Full testing
	- SIMD / SSE / AVX
=============================================================================*/


// ------------------ DEFINITIONS ---------------------------------
template<typename S, u32 D>
struct Vector;

template<typename S>
struct Vector<S, 2>
{ 
	S x, y;
};

using vector2 = Vector<float, 2>;
using v2 = Vector<float, 2>;
using point2 = Vector<u32, 2>;

template<typename S>
struct Vector<S, 3>
{ 
	S x, y, z; 

	static Vector const up;
};
using v3 	= Vector<float, 3>;

template<> v3 const v3::up = {0,0,1};


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
};

using vector4 	= Vector<float, 4>;
using v4 		= Vector<float,4>;
using float4 	= Vector<float, 4>;

using upoint4 = Vector<u32, 4>;

namespace vector
{
	template<typename S>
	constexpr S epsilon = 0.000001f;

	template <typename S, u32 D> S * 				begin(Vector<S, D> * vec);
	template <typename S, u32 D> S const * 			const_begin(Vector<S, D> const * vec);

	template<typename S, u32 D> S 					dot(Vector<S,D> const & a, Vector<S,D> const & b);
	template<typename S, u32 D>	Vector<S,D>			interpolate(Vector<S,D> a, Vector<S,D> b, S t);
	template<typename S, u32 D> Vector<S,D>			normalize(Vector<S,D> vec);
	template<typename S, u32 D> Vector<S,D> 		normalize_or_zero(Vector<S,D> vec);
	template<typename S, u32 D> S					get_length(Vector<S,D> const & vec);
	template<typename S, u32 D> S 					get_sqr_length(Vector<S, D> const &);
	template<typename S, u32 D> Vector<S, D>		clamp_length(Vector<S,D> vec, S maxLength);
	template<typename S, u32 D> Vector<S, D> 		scale(Vector<S,D> vec, Vector<S,D> const & scale);
	template<typename S, u32 D> void 				dissect (Vector<S,D> const & vec, Vector<S,D> * outDirection, S * outLength);
	template<typename S, u32 D> Vector<S,D>			project (Vector<S,D> vec, Vector<S,D> const & projectionTarget);
	template<typename S, u32 D> S 					get_sqr_distance(Vector<S,D> const & a, Vector<S,D> const & b);

	template<typename TNew, typename Told> TNew		convert(Told const & old);

	// 3-dimensional vectors specific
	template<typename S> Vector<S, 3>				cross(Vector<S,3> lhs, Vector<S,3> const & rhs);
	template<typename S> Vector<S, 3> 				rotate(Vector<S,3> vec, Vector<S,3> const & axis, S angleInRadians);
	template<typename S> S 							get_signed_angle(Vector<S,3> const & from, Vector<S,3> const & to, Vector<S,3> const & axis);
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

template<typename S, u32 D> Vector<S,D>
vector::normalize(Vector<S,D> vec)
{
	/* Note(Leo): We deliberately do handle bad cases here, so that any code where there is risk
	to  normalize 0 length vector should handle that themself. so that*/
	vec /= get_length(vec);
	return vec;
}

template<typename S, u32 D> Vector<S,D>
vector::normalize_or_zero(Vector<S,D> vec)
{
	float sqrMagnitude = get_sqr_length(vec);
	if (sqrMagnitude < epsilon<S>)
	{
		return vec;
	}
	vec /= Root2(sqrMagnitude);
	return vec;
}

template<typename S, u32 D> S
vector::get_length(Vector<S,D> const & vec)
{
	S result = 0;

	S const * pVec = const_begin(&vec);
	for (int i = 0; i < D; ++i)
	{
		result += pVec[i] * pVec[i];
	}
	result = Root2(result);
	return result;
}

template<typename S, u32 D> S
vector::get_sqr_length(Vector<S,D> const & vec)
{
	S result = 0;

	S const * pVec = const_begin(&vec);
	for (int i = 0; i < D; ++i)
	{
		result += pVec[i] * pVec[i];
	}
	return result;
}

template<typename S, u32 D> Vector<S,D>
vector::clamp_length(Vector<S,D> vec, S maxLength)
{
	auto length = get_length(vec);
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
vector::convert(TOld const & old)
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
	*outLength = get_length(vec);

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