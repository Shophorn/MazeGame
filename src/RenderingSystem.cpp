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
update_render_system(	ArenaArray<RenderSystemEntry> entries,
						platform::Graphics * graphics,
						platform::Functions * functions)
{
	for (auto entry : entries)
	{
		functions->draw_model(	graphics,
								entry.model,
								get_matrix(entry.transform),
								entry.castShadows,
								nullptr, 0);
	}
}

struct AnimatedRenderer
{
	Transform3D * transform;
	ModelHandle model;
	bool32 castShadows = true;
	ArenaArray<m44> bones;
};

internal void
render_animated_models(	ArenaArray<AnimatedRenderer> entries,
						platform::Graphics * graphics,
						platform::Functions * functions)
{
	for (auto entry : entries)
	{
		functions->draw_model( 	graphics,
								entry.model,
								get_matrix(entry.transform),
								entry.castShadows,
								entry.bones.begin(),
								entry.bones.count());
	}
}