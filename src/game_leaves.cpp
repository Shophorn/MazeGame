/*
Leo Tamminen

Leaves
*/

struct Leaves
{
	s32 			capacity;
	s32 			count;

	v3 				position;
	quaternion 		rotation;

	s32 			colourIndex;

	// Todo(Leo): maybe put these into a struct
	v3 * 			localPositions;
	quaternion * 	localRotations;
	f32 * 			localScales;

	f32 * 			swayPositions;
	v3 * 			swayAxes;

	MaterialHandle	material;
};
 
internal void flush_leaves(Leaves & leaves)
{
	leaves.count = 0;
}

internal Leaves make_leaves(MemoryArena & allocator, s32 capacity)
{
	Leaves leaves 			= {};

	leaves.capacity 		= capacity;
	leaves.localPositions 	= push_memory<v3>(allocator, leaves.capacity, ALLOC_GARBAGE);
	leaves.localRotations 	= push_memory<quaternion>(allocator, leaves.capacity, ALLOC_GARBAGE);
	leaves.localScales 		= push_memory<f32>(allocator, leaves.capacity, ALLOC_GARBAGE);
	leaves.swayPositions 	= push_memory<f32>(allocator, leaves.capacity, ALLOC_GARBAGE);
	leaves.swayAxes 		= push_memory<v3>(allocator, leaves.capacity, ALLOC_GARBAGE);

	return leaves;
}

internal void draw_leaves(Leaves & leaves, f32 elapsedTime, v2 leafScale = {1,1}, v3 colour = {})
{
	s32 drawCount = f32_min(leaves.capacity, leaves.count);

	m44 * leafTransforms = push_memory<m44>(*global_transientMemory, drawCount, ALLOC_GARBAGE);
	for (s32 i = 0; i < drawCount; ++i)
	{
		v3 position 			= leaves.position + quaternion_rotate_v3(leaves.rotation, leaves.localPositions[i]);

		constexpr f32 swayRange = 0.5;
		leaves.swayPositions[i] = mod_f32(leaves.swayPositions[i] + elapsedTime, swayRange * 4);
		f32 sway 				= mathfun_pingpong_f32(leaves.swayPositions[i], swayRange * 2) - swayRange;
		quaternion rotation 	= quaternion_axis_angle(leaves.swayAxes[i], sway);
		rotation 				= leaves.localRotations[i] * leaves.rotation * rotation;

		v3 scale 	= make_uniform_v3(leaves.localScales[i]);
		scale.x 	*= leafScale.x;
		scale.y 	*= leafScale.y;

		leafTransforms[i] 		= transform_matrix(	position, rotation,	scale);
	}

	graphics_draw_leaves(platformGraphics, drawCount, leafTransforms, leaves.colourIndex, colour, leaves.material);
}
