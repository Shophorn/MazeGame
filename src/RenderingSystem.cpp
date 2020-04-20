/*=============================================================================
Leo Tamminen
shophorn @ internet
=============================================================================*/
struct RenderSystemEntry
{
	Transform3D * 	transform;
	ModelHandle 	model;
	bool32 			castShadows = true;
};

internal void
update_render_system(	Array<RenderSystemEntry> const & entries,
						platform::Graphics * graphics)
{
	for (auto entry : entries)
	{
		platformApi->draw_model(	graphics,
								entry.model,
								get_matrix(*entry.transform),
								entry.castShadows,
								nullptr, 0);
	}
}

struct AnimatedRenderer
{
	// References to external resources
	Transform3D	* 		transform;
	AnimatedSkeleton * 	skeleton;
	ModelHandle 		model;

	// Memory where animated bone matrices are stores
	Array<m44> bones;

	// Options
	bool32 castShadows = true;
};


AnimatedRenderer make_animated_renderer (Transform3D * transform, AnimatedSkeleton * skeleton, ModelHandle model, MemoryArena & allocator) //Array<m44> boneMemory)
{
	auto result = AnimatedRenderer {
			.transform 	= transform,
			.skeleton 	= skeleton,
			.model 		= model,
			.bones 		= allocate_array<m44>(allocator, skeleton->bones.count(), ALLOC_FILL | ALLOC_NO_CLEAR)
		};	
	return result;		
}

internal void
render_animated_models(	Array<AnimatedRenderer> const & entries,
						platform::Graphics * graphics)
{

}