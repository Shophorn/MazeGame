struct DynamicMesh
{
	Array2<Vertex> 	vertices;
	Array2<u16> 	indices;
};

internal DynamicMesh push_dynamic_mesh(MemoryArena & allocator, u64 vertexCapacity, u64 indexCapacity)
{
	DynamicMesh mesh =
	{
		.vertices 	= push_array_2<Vertex>(allocator, vertexCapacity, ALLOC_CLEAR),
		.indices 	= push_array_2<u16>(allocator, indexCapacity, ALLOC_CLEAR)
	};
	return mesh;
}

internal void flush_dynamic_mesh(DynamicMesh & mesh)
{
	mesh.vertices.count = 0;
	mesh.indices.count 	= 0;
};
