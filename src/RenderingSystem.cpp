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
								entry.castShadows);
	}
}