/*=============================================================================
Leo Tamminen

Asset things for :MAZEGAME:
=============================================================================*/
enum struct HandleType
{ 
	Invalid,

	Mesh = 1, 
	Texture,
	Material,
	RenderedObject,
	Gui
};

template<HandleType Type>
struct BaseHandle
{
	uint64 index =  -1;
	operator uint64() { return index; }	
	static constexpr HandleType type = Type;
};

using MeshHandle = 				BaseHandle<HandleType::Mesh>;
using TextureHandle = 			BaseHandle<HandleType::Texture>;
using MaterialHandle = 			BaseHandle<HandleType::Material>;
using RenderedObjectHandle = 	BaseHandle<HandleType::RenderedObject>;
using GuiHandle =				BaseHandle<HandleType::Gui>;

struct Vertex
{
	Vector3 position;
	Vector3 normal;
	Vector3 color;
	Vector2 texCoord;
};

enum struct IndexType : uint32 { UInt16, UInt32 };

struct MeshAsset
{
	ArenaArray<Vertex> vertices;
	ArenaArray<uint16> indices;

	// TODO(Leo): would this be a good way to deal with different index types
	union
	{
		ArenaArray<uint16> indices16;
		ArenaArray<uint32> indices32;
	};

	IndexType indexType;

	void * indexData;
};


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
    // Note(Leo): Ignore now, later we use this to differentiate different material layouts
    MaterialType type;

    TextureHandle albedo;
    TextureHandle metallic;
    TextureHandle testMask;
};

