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
	RENDERED_OBJECT,
	PIPELINE,	
};

template<HandleType T>
struct BaseHandle
{
	uint64 index =  -1;
	operator uint64() { return index; }	
	static constexpr HandleType type = T;

	inline static const BaseHandle Null = {};
};

using MeshHandle  				= BaseHandle<MESH>;
using TextureHandle  			= BaseHandle<TEXTURE>;
using MaterialHandle  			= BaseHandle<MATERIAL>;
using RenderedObjectHandle  	= BaseHandle<RENDERED_OBJECT>;
using PipelineHandle 			= BaseHandle<PIPELINE>;

struct Vertex
{
	Vector3 position 	= {};
	Vector3 normal 		= {};
	Vector3 color 		= {};
	Vector2 texCoord 	= {};
};

enum struct IndexType : uint32 { UInt16, UInt32 };

struct MeshAsset
{
	ArenaArray<Vertex> vertices;
	ArenaArray<uint16> indices;

	// TODO(Leo): would this be a good way to deal with different index types?
	// union
	// {
	// 	ArenaArray<uint16> indices16;
	// 	ArenaArray<uint32> indices32;
	// };

	IndexType indexType = IndexType::UInt16;
};

internal MeshAsset
make_mesh_asset(ArenaArray<Vertex> vertices, ArenaArray<uint16> indices)
{
	MeshAsset result =
	{
		.vertices = vertices,
		.indices = indices,
		.indexType = IndexType::UInt16,
	};
	return result;
}


struct TextureAsset
{
	ArenaArray<uint32> pixels;

	int32 	width;
	int32 	height;
	int32 	channels;
};


enum struct MaterialType : int32
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

// 	int32 textureCount;

// 	TextureHandle textures[TextureCount];
// };

// template<typename ... Ts>
// MaterialAsset2<2> make_material_asset(Ts ... textures)
// {
// 	return {};
// }

