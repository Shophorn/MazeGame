/*=============================================================================
Leo Tamminen

Asset things for :MAZEGAME:
=============================================================================*/

enum HandleType
{
	INVALID,

	MESH,
	TEXTURE,
	MATERIAL,
	MODEL,
	PIPELINE,	
};

template<HandleType T>
struct BaseHandle
{
	u64 index_ =  -1;
	operator u64() { return index_; }	
	static constexpr HandleType type = T;

	inline static const BaseHandle Null = {};
};

using MeshHandle  				= BaseHandle<MESH>;
using TextureHandle  			= BaseHandle<TEXTURE>;
using MaterialHandle  			= BaseHandle<MATERIAL>;
using ModelHandle  				= BaseHandle<MODEL>;
using PipelineHandle 			= BaseHandle<PIPELINE>;

template<HandleType T>
bool32
is_valid_handle(BaseHandle<T> handle)
{
	return (BaseHandle<T>::Null.index_ != handle.index_);
}

template <HandleType T>
bool32 operator==(BaseHandle<T> a, BaseHandle<T> b)
{
	return a.index_ == b.index_;
}


struct Vertex
{
	v3 position;
	v3 normal;
	v3 color;
	v2 texCoord;

	upoint4 boneIndices;
	v4 boneWeights;
};

enum struct IndexType : u32 { UInt16, UInt32 };

struct MeshAsset
{
	ArenaArray<Vertex> vertices;
	ArenaArray<u16> indices;

	// TODO(Leo): would this be a good way to deal with different index types?
	// union
	// {
	// 	ArenaArray<u16> indices16;
	// 	ArenaArray<u32> indices32;
	// };

	IndexType indexType = IndexType::UInt16;
};

internal MeshAsset
make_mesh_asset(ArenaArray<Vertex> vertices, ArenaArray<u16> indices)
{
	MeshAsset result =
	{
		.vertices 	= vertices,
		.indices 	= indices,
		.indexType 	= IndexType::UInt16,
	};
	return result;
}

using Pixel = u32;

struct TextureAsset
{
	ArenaArray<Pixel> pixels;

	s32 	width;
	s32 	height;
	s32 	channels;
};

inline float
get_red(Pixel color)
{
	u32 value = (color & 0x00ff0000) >> 16;
	return (float)value / 255.0f; 
}

inline float
get_green(Pixel color)
{
	u32 value = (color & 0x0000ff00) >> 8;
	return (float)value / 255.0f; 
}

inline Pixel 
get_pixel(TextureAsset * texture, u32 x, u32 y)
{
	DEBUG_ASSERT(x < texture->width && y < texture->height, "Invalid pixel coordinates!");

	u64 index = x + y * texture->width;
	return texture->pixels[index];
}

inline u8
value_to_byte(float value)
{
	return (u8)(value * 255);
}

inline u32
make_pixel(float4 color)
{
	u32 pixel = (u32)value_to_byte(color.w) << 24
				| (u32)value_to_byte(color.x) << 16
				| (u32)value_to_byte(color.y) << 8
				| (u32)value_to_byte(color.z);
	return pixel;
}

internal Pixel
get_closest_pixel(TextureAsset * texture, v2 texcoord)
{
	u32 u = round_to<u32>(texture->width * texcoord.x) % texture->width;
	u32 v = round_to<u32>(texture->height * texcoord.y) % texture->height;

	u64 index = u + v * texture->width;
	return texture->pixels[index];
}





enum struct MaterialType : s32
{
	Character,
	Terrain
};

struct MaterialAsset
{
    PipelineHandle pipeline;
    ArenaArray<TextureHandle> textures;
};

MaterialAsset
make_material_asset(PipelineHandle pipeline, ArenaArray<TextureHandle> textures)
{
	MaterialAsset result = 
	{
		.pipeline = pipeline,
		.textures = textures,
	};
	return result;
}

// template<int TextureCount>
// struct MaterialAsset2
// {
// 	PipelineHandle shader;

// 	s32 textureCount;

// 	TextureHandle textures[TextureCount];
// };

// template<typename ... Ts>
// MaterialAsset2<2> make_material_asset(Ts ... textures)
// {
// 	return {};
// }

