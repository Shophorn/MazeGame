struct MenuGuiState
{
	ArenaArray<Rectangle> buttons;
	int32 selectedIndex = 0;
	MaterialHandle material;
};

internal void
update_menu_gui_navigation_system(MenuGuiState * gui, game::Input * input)
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
}

internal MenuResult
update_menu_gui_event_system(MenuGuiState * guiState, game::Input * input)
{
	MenuResult result = MENU_NONE;
	if (is_clicked(input->confirm))
	{
		switch (guiState->selectedIndex)
		{
			case 0: 
				std::cout << "Load 3d Scene\n";
				result = MENU_LOADLEVEL_3D;
				break;

			case 1: 
				std::cout << "Load 2d Scene\n";
				result = MENU_LOADLEVEL_2D;
				break;

			case 2:
				std::cout << "Exit\n";
				result = MENU_EXIT;
				break;
		}
	}
	return result;
}

internal MenuResult 
update_menu_gui(	void *						guiPtr,
					game::Input * 				input,
					game::RenderInfo * 			renderer,
					game::PlatformInfo * 		platform,
					platform::GraphicsContext * graphics)
{
	auto * gui = reinterpret_cast<MenuGuiState*>(guiPtr);

	update_menu_gui_navigation_system(gui, input);

	for(int i = 0; i < gui->buttons.count(); ++i)
	{
		Rectangle rect = gui->buttons[i];
		float4 color = {1,1,1,1};

		if (i == gui->selectedIndex)
		{
			rect 	= scale_rectangle(gui->buttons[i], {1.2f, 1.2f});
			color 	= {1, 0.4f, 0.4f, 1}; 
		}
		renderer->draw_gui(graphics, rect.position, rect.size, gui->material, color);
	}
	
	auto result = update_menu_gui_event_system(gui, input);
	return result;
}

internal void
load_menu_gui(	void * guiPtr,
				MemoryArena * persistentMemory,
				MemoryArena * transientMemory,
				game::PlatformInfo * platformInfo,
				platform::GraphicsContext * graphics)
{
	auto * gui = reinterpret_cast<MenuGuiState*>(guiPtr);

	auto textureAsset 	= load_texture_asset("textures/texture.jpg", transientMemory);
	auto texture 		= platformInfo->push_texture(graphics, &textureAsset);
	gui->material 		= platformInfo->push_gui_material(graphics, texture);

	gui->buttons = push_array<Rectangle>(persistentMemory, {{810, 500, 300, 100},
															{810, 620, 300, 100},
															{810, 740, 300, 100}});
	gui->selectedIndex = 0;
}

global_variable SceneGuiInfo menuSceneGuiGui = 
{
	.get_alloc_size = []() { return sizeof(MenuGuiState); },
	.load 			= load_menu_gui,
	.update 		= update_menu_gui,
};