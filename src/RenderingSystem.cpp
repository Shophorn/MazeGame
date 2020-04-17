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
	Transform3D * transform;
	ModelHandle model;
	bool32 castShadows = true;
	Array<m44> bones;
};

internal void
render_animated_models(	Array<AnimatedRenderer> const & entries,
						platform::Graphics * graphics)
{
	for (auto const & entry : entries)
	{
		platformApi->draw_model( 	graphics,
								entry.model,
								get_matrix(*entry.transform),
								entry.castShadows,
								entry.bones.data(),
								entry.bones.count());
	}
}