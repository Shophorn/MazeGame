/*=============================================================================
Leo Tamminen
shophorn @ internet

Default gui to be used in game scenes.
=============================================================================*/

namespace default_scene_gui
{
	struct SceneGui
	{
		int32 gameGuiButtonCount;
		ArenaArray<Rectangle> gameGuiButtons;
		ArenaArray<GuiHandle> gameGuiButtonHandles;
		bool32 showGameMenu;

		int32 selectedGuiButtonIndex;
	};

	internal uint64
	get_alloc_size() { return sizeof (SceneGui); };

	internal void
	load(void * scenePtr, MemoryArena * persistentMemory, MemoryArena * transientMemory, game::PlatformInfo * platformInfo);

	internal MenuResult
	update(void * guiPtr, game::Input * input, game::RenderInfo * renderer, game::PlatformInfo * platform);
}


global_variable SceneGuiInfo
defaultSceneGui = make_scene_gui_info(	default_scene_gui::get_alloc_size,
										default_scene_gui::load,
										default_scene_gui::update);

internal MenuResult
default_scene_gui::update(void * guiPtr, game::Input * input, game::RenderInfo * renderer, game::PlatformInfo * platform)
{
	SceneGui * gui = reinterpret_cast<SceneGui*>(guiPtr);

	// Overlay menu
	if (input->start.IsClicked())
	{
		gui->selectedGuiButtonIndex = 0;
		gui->showGameMenu = !gui->showGameMenu;
	}

	MenuResult gameMenuResult = MENU_NONE;
	if (gui->showGameMenu)
	{
		if (input->down.IsClicked())
		{
			gui->selectedGuiButtonIndex += 1;
			gui->selectedGuiButtonIndex %= gui->gameGuiButtonCount;
		}

		if (input->up.IsClicked())
		{
			gui->selectedGuiButtonIndex -= 1;
			if (gui->selectedGuiButtonIndex < 0)
			{
				gui->selectedGuiButtonIndex = gui->gameGuiButtonCount - 1;
			}
		}

		if (input->confirm.IsClicked())
		{
			switch (gui->selectedGuiButtonIndex)
			{
				case 0: 
					std::cout << "Continue Game\n";
					gameMenuResult = SCENE_CONTINUE;
					break;
				
				case 1:
					std::cout << "Quit to menu\n";
					gameMenuResult = SCENE_EXIT;
					break;
			}
		}
	}
	if (gameMenuResult == SCENE_CONTINUE)
	{
		gui->showGameMenu = false;
	}

	/// Output Render info
	// Todo(Leo): Get info about limits of render output array sizes and constraint to those
	{

		if (gui->showGameMenu)
		{
			/// ----- GUI -----
			Vector2 windowSize = {(real32)platform->windowWidth, (real32)platform->windowHeight};
			Vector2 halfWindowSize = windowSize / 2.0f;

			for (int  guiButtonIndex = 0; guiButtonIndex < gui->gameGuiButtonCount; ++guiButtonIndex)
			{
				Vector2 guiTranslate = gui->gameGuiButtons[guiButtonIndex].position / halfWindowSize - 1.0f;
				Vector2 guiScale = gui->gameGuiButtons[guiButtonIndex].size / halfWindowSize;

				if (guiButtonIndex == gui->selectedGuiButtonIndex)
				{
					guiTranslate = (gui->gameGuiButtons[guiButtonIndex].position - Vector2{15, 15}) / halfWindowSize - 1.0f;
					guiScale = (gui->gameGuiButtons[guiButtonIndex].size + Vector2{30, 30}) / halfWindowSize;
				}

				Matrix44 guiTransform = Matrix44::Translate({guiTranslate.x, guiTranslate.y, 0}) * Matrix44::Scale({guiScale.x, guiScale.y, 1.0});

				renderer->render_gui(gui->gameGuiButtonHandles[guiButtonIndex], guiTransform);
			}
		}
		else
		{
			for (int  guiButtonIndex = 0; guiButtonIndex < gui->gameGuiButtonCount; ++guiButtonIndex)
			{
				Matrix44 guiTransform = {};
				renderer->render_gui(gui->gameGuiButtonHandles[guiButtonIndex], guiTransform);
			}
		}
	}

	return gameMenuResult;
}

internal void 
default_scene_gui::load(void * guiPtr, MemoryArena * persistentMemory, MemoryArena * transientMemory, game::PlatformInfo * platformInfo)
{
	SceneGui * gui = reinterpret_cast<SceneGui*>(guiPtr);

    auto push_mesh = [platformInfo] (MeshAsset * asset) -> MeshHandle
    {
    	auto handle = platformInfo->graphicsContext->PushMesh(asset);
    	return handle;
    };

	struct MaterialCollection {
		MaterialHandle basic;
	} materials;

	// Create MateriaLs
	{
		TextureAsset whiteTextureAsset = make_texture_asset(push_array<uint32>(transientMemory, {0xffffffff}), 1, 1);
		TextureAsset blackTextureAsset = make_texture_asset(push_array<uint32>(transientMemory, {0xff000000}), 1, 1);

		TextureHandle whiteTexture = platformInfo->graphicsContext->PushTexture(&whiteTextureAsset);
		TextureHandle blackTexture = platformInfo->graphicsContext->PushTexture(&blackTextureAsset);

		auto load_and_push_texture = [transientMemory, platformInfo](const char * path) -> TextureHandle
		{
			auto asset = load_texture_asset(path, transientMemory);
			auto result = platformInfo->graphicsContext->PushTexture(&asset);
			return result;
		};

		auto tilesTexture 	= load_and_push_texture("textures/tiles.jpg");
		auto lavaTexture 	= load_and_push_texture("textures/lava.jpg");
		auto faceTexture 	= load_and_push_texture("textures/texture.jpg");

		auto push_material = [platformInfo](MaterialType type, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
		{
			MaterialAsset asset = { type, a, b, c };
			MaterialHandle handle = platformInfo->graphicsContext->PushMaterial(&asset);
			return handle;
		};

		materials.basic = push_material(	MaterialType::Character,
												lavaTexture,
												faceTexture,
												blackTexture);
	}

	gui->gameGuiButtonCount 	= 2;
	gui->gameGuiButtons 		= push_array<Rectangle>(persistentMemory, gui->gameGuiButtonCount);
	gui->gameGuiButtonHandles 	= push_array<GuiHandle>(persistentMemory, gui->gameGuiButtonCount);

	gui->gameGuiButtons[0] = {380, 200, 180, 40};
	gui->gameGuiButtons[1] = {380, 260, 180, 40};

	MeshAsset quadAsset 	= mesh_primitives::create_quad(transientMemory);
 	MeshHandle quadHandle 	= push_mesh(&quadAsset);

	for (int guiButtonIndex = 0; guiButtonIndex < gui->gameGuiButtonCount; ++guiButtonIndex)
	{
		gui->gameGuiButtonHandles[guiButtonIndex] = platformInfo->graphicsContext->PushGui(quadHandle, materials.basic);
	}

	gui->selectedGuiButtonIndex = 0;
	gui->showGameMenu = false;
}
