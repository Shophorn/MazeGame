/*=============================================================================
Leo Tamminen
shophorn @ internet

Rendering systems as they are seen in game.
=============================================================================*/
// Todo(Leo): this is stupid and redundant, but let's go with it for a while
struct Renderer
{
	RenderedObjectHandle handle;
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
update_render_system(game::RenderInfo * renderer, ArenaArray<RenderSystemEntry> entries)
{
	for (int32 i = 0; i < entries.count(); ++i)
	{
		renderer->draw(	entries[i].renderer->handle,
							get_matrix(entries[i].transform));
	}
}