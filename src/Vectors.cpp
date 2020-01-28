/*=============================================================================
Leo Tamminen

:MAZEGAME:'s vector structures and functions.

TODO(Leo): I would like to get some unary operator type functions (like normalize())
as member functions, but I have not yet figured good way to do that

TODO(Leo):
	- Full testing
	- SIMD / SSE / AVX
=============================================================================*/


// ------------------ DEFINITIONS ---------------------------------
#define VECTOR_TEMPLATE 	template<typename Scalar, u32 Dimension>
#define VECTOR_TYPE			VectorBase<Scalar, Dimension>

#define VECTOR_LOOP_ELEMENTS for (int i = 0; i < Dimension; ++i)

#define VECTOR_SUBSCRIPT_OPERATORS 											\
	scalar_type & operator [](int index) { return components[index]; }			\
	scalar_type operator [] (int index) const { return components[index]; }

VECTOR_TEMPLATE
union VectorBase
{
	using scalar_type = Scalar;
	static constexpr int dimension = Dimension;

	scalar_type components [dimension];

	static_assert(dimension > 1, "No vectors of 1 dimension allowed");

	VECTOR_SUBSCRIPT_OPERATORS;
};

template<typename Scalar>
union VectorBase<Scalar, 2>
{
	using scalar_type = Scalar;
	static constexpr int dimension = 2;

	struct { scalar_type x, y; };
	struct { scalar_type u, v; };

	scalar_type components [dimension];

	VECTOR_SUBSCRIPT_OPERATORS;
};

using vector2 = VectorBase<float, 2>;
using point2 = VectorBase<u32, 2>;

template<typename Scalar>
union VectorBase<Scalar, 3>
{
	using scalar_type = Scalar;
	static constexpr int dimension = 3;

	struct { scalar_type x, y, z; };
	struct { scalar_type r, g, b; };

	scalar_type components [dimension];

	VECTOR_SUBSCRIPT_OPERATORS;
};

#define VECTOR_3_TEMPLATE 	template<typename Scalar>
#define VECTOR_3_TYPE		VectorBase<Scalar, 3>

using vector3 	= VectorBase<float, 3>;
using float3 	= VectorBase<float, 3>;

struct world
{
	/*
	Note(Leo): Right handed coordinate system, x right, y, forward, z up.
	*/

	static constexpr vector3 left 		= {-1,  0,  0};
	static constexpr vector3 right 		= { 1,  0,  0};
	static constexpr vector3 back 		= { 0, -1,  0};
	static constexpr vector3 forward 	= { 0,  1,  0};
	static constexpr vector3 down 		= { 0,  0, -1};
	static constexpr vector3 up 		= { 0,  0,  1};
};

template<typename Scalar>
union VectorBase<Scalar, 4>
{
	using scalar_type = Scalar;
	static constexpr int dimension = 4;

	struct { scalar_type x, y, z, w; };
	struct { scalar_type r, g, b, a; };

	scalar_type components [dimension];

	VECTOR_SUBSCRIPT_OPERATORS;
};

using vector4 = VectorBase<float, 4>;
using float4 = VectorBase<float, 4>;

#undef VECTOR_SUBSCRIPT_OPERATORS


namespace vector_meta
{
	template<typename TVector3>
	using Enable3 = typename std::enable_if<TVector3::dimension == 3, TVector3>::type;

	template<typename> constexpr bool32 is_vector_template = false;
	template<typename T, u32 D> constexpr bool32 is_vector_template<VectorBase<T,D>> = true;

	template<typename T>
	using Enable = typename std::enable_if<is_vector_template<T>, T>::type;
}


namespace vector
{
	using namespace vector_meta;

	template<typename TVector>
	using Scalar = typename TVector::scalar_type;

	template<typename TVector> Scalar<TVector> 		dot(TVector a, TVector b);
	template<typename TVector> Enable3<TVector>		cross(TVector lhs, TVector rhs);
	
	template<typename TVector> TVector 				interpolate(TVector a, TVector b, Scalar<TVector> t);
	template<typename TVector> TVector 				normalize(TVector vec);
	template<typename TVector> Scalar<TVector>		get_length(TVector vec); 			
	template<typename TVector> TVector 				clamp_length(TVector vec, Scalar<TVector> maxLength);

	template<typename TNew, typename TOld> TNew		convert(TOld vec);

	template<typename TVector> TVector 				scale(TVector vec, TVector const & scale);
	template<typename TVector> Enable3<TVector> 	rotate(TVector vec, TVector axis, Scalar<TVector> angleInRadians);

	template<typename TVector> void 				dissect (TVector vec, TVector * outDirection, Scalar<TVector> * outLength);
	template<typename TVector> TVector 				project (TVector vec, TVector const & projectionTarget);
	template<typename TVec> Scalar<Enable3<TVec>> 	get_signed_angle(TVec from, TVec to, TVec axis);
	template<typename TVector> Scalar<TVector> 		get_sqr_distance(TVector const & a, TVector const & b);
}


namespace vector_operators
{
	#define VECTOR_OPERATOR template<typename TVec> vector_meta::Enable<TVec>

	VECTOR_OPERATOR & 	operator += (TVec & a, TVec const & b);
	VECTOR_OPERATOR 	operator + 	(TVec a, TVec const & b);
	VECTOR_OPERATOR &	operator -= (TVec & a, TVec const & b);
	VECTOR_OPERATOR 	operator - 	(TVec a, TVec const & b);

	VECTOR_OPERATOR &	operator *= (TVec & vec, vector::Scalar<TVec> s);
	VECTOR_OPERATOR 	operator * 	(TVec vec, vector::Scalar<TVec> s);
	VECTOR_OPERATOR &	operator /= (TVec & vec, vector::Scalar<TVec> s);
	VECTOR_OPERATOR 	operator / 	(TVec vec, vector::Scalar<TVec> s);

	VECTOR_OPERATOR		operator - 	(TVec vec);

	#undef VECTOR_OPERATOR
}
using namespace vector_operators;


template<typename TVector> vector_meta::Enable<TVector> &
vector_operators::operator += (TVector & a, TVector const & b)
{
	for (int i = 0; i < TVector::dimension; ++i)
	{
		a[i] += b[i];
	}
	return a;
}

template<typename TVector> vector_meta::Enable<TVector>
vector_operators::operator + (TVector a, TVector const & b)
{
	a += b;
	return a;
}


template<typename TVector> vector_meta::Enable<TVector> &
vector_operators::operator -= (TVector & a, TVector const & b)
{
	for (int i = 0; i < TVector::dimension; ++i)
	{
		a[i] -= b[i];
	}
	return a;
}

template<typename TVector> vector_meta::Enable<TVector>
vector_operators::operator - (TVector a, TVector const & b)
{
	a -= b;
	return a;
}

template <typename TVector> vector_meta::Enable<TVector> &
vector_operators::operator *= (TVector & vec, vector::Scalar<TVector> s)
{
	for (int i = 0; i < TVector::dimension; ++i)
	{
		vec[i] *= s;
	}
	return vec;
}

template <typename TVector> vector_meta::Enable<TVector>
vector_operators::operator * (TVector vec, vector::Scalar<TVector> s)
{
	vec *= s;
	return vec;
}

template <typename TVector> vector_meta::Enable<TVector> &
vector_operators::operator /= (TVector & vec, vector::Scalar<TVector> s)
{
	for (int i = 0; i < TVector::dimension; ++i)
	{
		vec[i] /= s;
	}
	return vec;
}

template <typename TVector> vector_meta::Enable<TVector>
vector_operators::operator / (TVector vec, vector::Scalar<TVector> s)
{
	vec /= s;
	return vec;
}

template<typename TVector> vector_meta::Enable<TVector>
vector_operators::operator - (TVector vec)
{
	for(int i = 0; i < TVector::dimension; ++i)
	{
		vec[i] = -vec[i];
	}
	return vec;
}

template<typename TVector> vector::Scalar<TVector>
vector::dot(TVector a, TVector b)
{
	Scalar<TVector> result = 0;
	for (int i = 0; i < TVector::dimension; ++i)
	{
		result += a[i] * b[i];
	}
	return result;
}

template<typename TVector> vector_meta::Enable3<TVector>
vector::cross(TVector lhs, TVector rhs)
{
	lhs = {
		lhs.y * rhs.z - lhs.z * rhs.y,
		lhs.z * rhs.x - lhs.x * rhs.z,
		lhs.x * rhs.y - lhs.y * rhs.x
	};
	return lhs;
}

template<typename TVector> TVector
vector::interpolate(TVector a, TVector b, typename TVector::scalar_type t)
{
	for (int i = 0; i < TVector::dimension; ++i)
	{
		a[i] = ::interpolate(a[i], b[i], t);
	}
	return a;
}

template<typename TVector> TVector
vector::normalize(TVector vec)
{
	vec /= get_length(vec);
	return vec;
}

template <typename TVector> vector::Scalar<TVector>
vector::get_length(TVector vec)
{
	Scalar<TVector> result = 0;
	for (int i = 0; i < TVector::dimension; ++i)
	{
		result += vec[i] * vec[i];
	}
	result = Root2(result);
	return result;
}

template<typename TVector> TVector
vector::clamp_length(TVector vec, Scalar<TVector> maxLength)
{
	auto length = get_length(vec);
	if (length > maxLength)
	{
		vec *= (maxLength / length);
	}
	return vec;
}

template<typename TNew, typename TOld> TNew
vector::convert(TOld old)
{
	TNew result = {};
	for (int i = 0; i < TNew::dimension && i < TOld::dimension; ++i)
	{
		result[i] = old[i];
	}
	return result;
}

template<typename TVector> TVector
vector::scale(TVector vec, TVector const & scale)
{
	for (int i = 0; i < TVector::dimension; ++i)
	{
		vec[i] *= scale[i];
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
// 	Scalar x1 = Cosine(angleInRadians) / rejectionLength;
// 	Scalar x2 = Sine(angleInRadians) / get_length(w);

// 	VECTOR_3_TYPE rotatedRejection = (rejection * x1 + w * x2) * rejectionLength	;

// 	vec = projection + rotatedRejection;
// 	return vec;
// }

template<typename TVector> void
vector::dissect(TVector vec, TVector * outDirection, Scalar<TVector> * outLength)
{
	*outLength = get_length(vec);

	if (Abs(*outLength) == 0.0f)
	{
		*outLength = 0;
		*outDirection = {0, 0, 0};
	}
	else
	{
		*outDirection = vec / (*outLength);
	}
}


template<typename TVector> TVector
vector::project(TVector vec, TVector const & projectionTarget)
{
	Scalar<TVector> c = dot(projectionTarget, vec) / dot(vec, vec);
	return projectionTarget * c;
}

template<typename TVector> vector::Scalar<vector_meta::Enable3<TVector>>
vector::get_signed_angle(TVector from, TVector to, TVector axis)
{
	// Todo(Leo): Check if this relly is correct
	auto a = dot(cross(from, to), axis);
	auto b = dot(to, from);

	auto result = ArcTan2(a, b);
	return result;
}

template<typename TVector> vector::Scalar<TVector>
vector::get_sqr_distance(TVector const & a, TVector const & b)
{
	Scalar<TVector> distance = 0;
	for (u32 i = 0; i < TVector::dimension; ++i)
	{
		Scalar<TVector> s = a[i] - b[i];
		distance += s * s;
	}
	return distance;
}

template<typename TVector> vector_meta::Enable3<TVector>
vector::rotate(TVector vec, TVector axis, Scalar<TVector> angleInRadians)
{
	auto projection 		= project(vec, axis);
	auto rejection 			= vec - projection;

	auto w 					= cross(axis, rejection);
	auto rejectionLength 	= get_length(rejection);
	auto x1 				= Cosine(angleInRadians) / rejectionLength;
	auto x2 				= Sine(angleInRadians) / get_length(w);

	auto rotatedRejection 	= (rejection * x1 + w * x2) * rejectionLength;

	vec = projection + rotatedRejection;
	return vec;	
}

#if MAZEGAME_INCLUDE_STD_IOSTREAM

namespace std
{	
	VECTOR_TEMPLATE ostream &
	operator << (ostream & os, VECTOR_TYPE vec)
	{
		os << "(" << vec[0];
		for (int i = 1; i < Dimension; ++i)
		{
			os << "," << vec[i];
		}
		os << ")";
		return os;
	}
}

#endif


/* Note(Leo): Undefine macros here, since they definetly are not intended
to be used around. */
#undef VECTOR_TEMPLATE
#undef VECTOR_TYPE

#undef VECTOR_3_TEMPLATE
#undef VECTOR_3_TYPE

#undef VECTOR_LOOP_ELEMENTS
