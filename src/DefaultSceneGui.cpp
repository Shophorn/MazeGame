/*=============================================================================
Leo Tamminen
shophorn @ internet

Default gui to be used in game scenes.
=============================================================================*/
struct SceneGui
{
	StaticArray<Rectangle, 2> buttons;
	int32 selectedIndex;

	bool32 showGameMenu;
	MaterialHandle material;
};

internal SceneGui
make_scene_gui(	MemoryArena * transientMemory,
				platform::Graphics * graphics,
				platform::Functions * functions)
{
	SceneGui result = {};

	#pragma message("Make 'Checkpoint' struct for MemoryArena")
	/* Todo(Leo): We should make transientMemory to yield a temporary allocation object. */

	auto textureAsset 	= load_texture_asset("textures/texture.jpg", transientMemory);
	auto texture 		= functions->push_texture(graphics, &textureAsset);

	result.material 	= functions->push_gui_material(graphics, texture);

	result.buttons 		= {Rectangle{810, 500, 300, 120},
							Rectangle{810, 640, 300, 120}};

	result.selectedIndex 	= 0;
	result.showGameMenu 	= false;	

	return result;
}

internal MenuResult
update_scene_gui(	SceneGui * gui,
					game::Input * input,
					platform::Graphics * graphics,
					platform::Functions * functions)
{
	// Overlay menu
	if (is_clicked(input->start))
	{
		gui->selectedIndex = 0;
		gui->showGameMenu = !gui->showGameMenu;
	}

	MenuResult gameMenuResult = MENU_NONE;
	if (gui->showGameMenu)
	{
		if (is_clicked(input->down))
		{
			gui->selectedIndex += 1;
			gui->selectedIndex %= gui->buttons.count();
		}

		if (is_clicked(input->up))
		{
			gui->selectedIndex -= 1;
			if (gui->selectedIndex < 0)
			{
				gui->selectedIndex = gui->buttons.count() - 1;
			}
		}

		if (is_clicked(input->confirm))
		{
			switch (gui->selectedIndex)
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

	if (gui->showGameMenu)
	{
		for(int i = 0; i < gui->buttons.count(); ++i)
		{
			Rectangle rect = gui->buttons[i];
			float4 color = {1,1,1,1};

			if (i == gui->selectedIndex)
			{
				rect 	= scale_rectangle(gui->buttons[i], {1.2f, 1.2f});
				color 	= {1, 0.4f, 0.4f, 1}; 
			}
			functions->draw_gui(graphics, rect.position, rect.size, gui->material, color);
		}
	}

	return gameMenuResult;
}


/*
namespace default_scene_gui
{
	struct SceneGui
	{
		StaticArray<Rectangle, 2> buttons;

		bool32 showGameMenu;
		int32 gameGuiButtonCount;
		int32 selectedGuiButtonIndex;
		MaterialHandle material;

	};

	internal uint64
	get_alloc_size() { return sizeof (SceneGui); };

	internal void
	load(	void * scenePtr,
			MemoryArena * persistentMemory,
			MemoryArena * transientMemory,
			game::PlatformFunctions * PlatformFunctions,
			platform::Graphics * graphics);

	internal MenuResult
	update(	void * guiPtr,
			game::Input * input,
			game::RenderInfo * renderer,
			game::PlatformFunctions * platform,
			platform::Graphics * graphics);
}


global_variable SceneGuiInfo
defaultSceneGui = make_scene_gui_info(	default_scene_gui::get_alloc_size,
										default_scene_gui::load,
										default_scene_gui::update);


internal MenuResult
default_scene_gui::update(	void * guiPtr,
							game::Input * input,
							game::RenderInfo * renderer,
							game::PlatformFunctions * platform,
							platform::Graphics * graphics)
{
	SceneGui * gui = reinterpret_cast<SceneGui*>(guiPtr);

	// Overlay menu
	if (is_clicked(input->start))
	{
		gui->selectedGuiButtonIndex = 0;
		gui->showGameMenu = !gui->showGameMenu;
	}

	MenuResult gameMenuResult = MENU_NONE;
	if (gui->showGameMenu)
	{
		if (is_clicked(input->down))
		{
			gui->selectedGuiButtonIndex += 1;
			gui->selectedGuiButtonIndex %= gui->gameGuiButtonCount;
		}

		if (is_clicked(input->up))
		{
			gui->selectedGuiButtonIndex -= 1;
			if (gui->selectedGuiButtonIndex < 0)
			{
				gui->selectedGuiButtonIndex = gui->gameGuiButtonCount - 1;
			}
		}

		if (is_clicked(input->confirm))
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

	if (gui->showGameMenu)
	{
		for(int i = 0; i < gui->buttons.count(); ++i)
		{
			Rectangle rect = gui->buttons[i];
			float4 color = {1,1,1,1};

			if (i == gui->selectedGuiButtonIndex)
			{
				rect 	= scale_rectangle(gui->buttons[i], {1.2f, 1.2f});
				color 	= {1, 0.4f, 0.4f, 1}; 
			}
			renderer->draw_gui(graphics, rect.position, rect.size, gui->material, color);
		}
	}

	return gameMenuResult;
}

internal void 
default_scene_gui::load(void * guiPtr,
						MemoryArena * persistentMemory,
						MemoryArena * transientMemory,
						game::PlatformFunctions * PlatformFunctions,
						platform::Graphics * graphics)
{
	SceneGui * gui = reinterpret_cast<SceneGui*>(guiPtr);

	auto textureAsset 	= load_texture_asset("textures/texture.jpg", transientMemory);
	auto texture 		= PlatformFunctions->push_texture(graphics, &textureAsset);
	gui->material 		= PlatformFunctions->push_gui_material(graphics, texture);

	gui->gameGuiButtonCount 	= 2;
	gui->buttons = {Rectangle{810, 500, 300, 120}, Rectangle{810, 640, 300, 120}};

	gui->selectedGuiButtonIndex = 0;
	gui->showGameMenu = false;
}
*/