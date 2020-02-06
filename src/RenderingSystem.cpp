/*=============================================================================
Leo Tamminen
shophorn @ internet

Rendering systems as they are seen in game.
=============================================================================*/
// Todo(Leo): this is stupid and redundant, but let's go with it for a while
struct Renderer
{
	ModelHandle handle;
};

struct RenderSystemEntry
{
	Handle<Transform3D> transform;
	Handle<Renderer> renderer;
};

struct GuiRendererSystemEntry
{
	Handle<Rectangle> transform;
	Handle<Renderer> renderer;
};

internal void
update_render_system(	ArenaArray<RenderSystemEntry> entries,
						platform::Graphics * graphics,
						platform::Functions * functions)
{
	for (s32 i = 0; i < entries.count(); ++i)
	{
		functions->draw_model(graphics, entries[i].renderer->handle, get_matrix(entries[i].transform), true);
	}
}