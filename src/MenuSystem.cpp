struct MenuGuiState
{
	ArenaArray<GuiRendererSystemEntry> guiRenderSystem;

	int32 menuGuiButtonCount;
	int32 selectedGuiIndex;
};

internal void
update_gui_render_system(ArenaArray<GuiRendererSystemEntry> system, game::RenderInfo * renderer, game::PlatformInfo * platform, int32 selectedGuiIndex)
{
	Vector2 windowSize = {(float)platform->windowWidth, (float)platform->windowHeight};
	Vector2 halfWindowSize = windowSize / 2.0f;

	for (int i = 0; i < system.count(); ++i)
	{
		GuiRendererSystemEntry & entry = system[i];

		Vector2 translation = entry.transform->position / halfWindowSize - 1.0f;
		Vector2 scale 		= entry.transform->size / halfWindowSize;

		if (i == selectedGuiIndex)
		{
			translation = (entry.transform->position - Vector2{15, 15}) / halfWindowSize - 1.0f;
			scale = (entry.transform->size + Vector2{30, 30}) / halfWindowSize;
		}

		Matrix44 transform = Matrix44::Translate({translation.x, translation.y, 0}) * Matrix44::Scale({scale.x, scale.y, 1.0});
		renderer->render_gui(entry.renderer->handle, transform);
	}
}


internal void
update_menu_gui_navigation_system(MenuGuiState * guiState, game::Input * input)
{
	if (is_clicked(input->down))
	{
		guiState->selectedGuiIndex += 1;
		guiState->selectedGuiIndex %= guiState->menuGuiButtonCount;
	}

	if (is_clicked(input->up))
	{
		guiState->selectedGuiIndex -= 1;
		if (guiState->selectedGuiIndex < 0)
		{
			guiState->selectedGuiIndex = guiState->menuGuiButtonCount - 1;
		}
	}
}

internal MenuResult
update_menu_gui_event_system(MenuGuiState * guiState, game::Input * input)
{
	MenuResult result = MENU_NONE;
	if (is_clicked(input->confirm))
	{
		switch (guiState->selectedGuiIndex)
		{
			case 0: 
				std::cout << "Load 3d Scene\n";
				result = MENU_LOADLEVEL_3D;
				break;

			case 1: 
				std::cout << "Load 2d Scene\n";
				result = MENU_LOADLEVEL_2D;
				break;

			case 2: std::cout << "Credits\n"; break;
			
			case 3:
				std::cout << "Exit\n";
				result = MENU_EXIT;
				break;
		}
	}
	return result;
}

internal MenuResult 
update_menu_gui(	void *					guiPtr,
					game::Input * 			input,
					game::RenderInfo * 		outRenderInfo,
					game::PlatformInfo * 	platform)
{
	auto * gui = reinterpret_cast<MenuGuiState*>(guiPtr);


	update_menu_gui_navigation_system(gui, input);
	update_gui_render_system(gui->guiRenderSystem, outRenderInfo, platform, gui->selectedGuiIndex);
	
	auto result = update_menu_gui_event_system(gui, input);
	return result;
}

internal void
load_menu_gui(void * guiPtr, MemoryArena * persistentMemory, MemoryArena * transientMemory, game::PlatformInfo * platformInfo)
{
	auto * gui = reinterpret_cast<MenuGuiState*>(guiPtr);

	int estimatedGuiCount = 10;

	allocate_for_handle<Rectangle>(persistentMemory, estimatedGuiCount);
	allocate_for_handle<GuiRenderer>(persistentMemory, estimatedGuiCount);

 	gui->guiRenderSystem = reserve_array<GuiRendererSystemEntry>(persistentMemory, estimatedGuiCount);

 	MaterialHandle material;
	// Create MateriaLs
	{
		ShaderHandle shader = platformInfo->graphicsContext->push_shader("shaders/vert_gui.spv", "shaders/frag_gui.spv");

		TextureAsset blackTextureAsset = {};
		blackTextureAsset.pixels = push_array<uint32>(transientMemory, 1);
		blackTextureAsset.pixels[0] = 0xff000000;
		blackTextureAsset.width = 1;
		blackTextureAsset.height = 1;
		blackTextureAsset.channels = 4;

		TextureHandle blackTexture = platformInfo->graphicsContext->PushTexture(&blackTextureAsset);

	    TextureAsset textureAssets [] = {
	        load_texture_asset("textures/lava.jpg", transientMemory),
	        load_texture_asset("textures/texture.jpg", transientMemory),
	    };

		TextureHandle texB = platformInfo->graphicsContext->PushTexture(&textureAssets[0]);
		TextureHandle texC = platformInfo->graphicsContext->PushTexture(&textureAssets[1]);

		MaterialAsset characterMaterialAsset = make_material_asset(shader, texB, texC, blackTexture);
		material = platformInfo->graphicsContext->PushMaterial(&characterMaterialAsset);
	}

	MeshAsset quadAsset = mesh_primitives::create_quad(transientMemory, true);
 	MeshHandle quadHandle = platformInfo->graphicsContext->PushMesh(&quadAsset);

 	for (int i = 0; i < 4; ++i)
 	{
 		Handle<Rectangle> rectanle = make_handle<Rectangle>({380, (float)(200 + 60 * i), 180, 40});

 		GuiHandle graphicsHandle = platformInfo->graphicsContext->PushGui(quadHandle, material);
 		Handle<GuiRenderer> renderer = make_handle<GuiRenderer>({graphicsHandle});

 		push_one(gui->guiRenderSystem, {rectanle, renderer});
 	}
	gui->menuGuiButtonCount 	= 4;
	gui->selectedGuiIndex = 0;
}

global_variable SceneGuiInfo menuSceneGuiGui = 
{
	.get_alloc_size = []() { return sizeof(MenuGuiState); },
	.load 			= load_menu_gui,
	.update 		= update_menu_gui,
};