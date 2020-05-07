/*=============================================================================
Leo Tamminen
shophorn @ internet

Immediate mode gui structure.
=============================================================================*/

struct Gui
{
	// References
	MaterialHandle 	material;

	// Properties
	v2 				startPosition = {810, 500};
	v2 				buttonSize = {300, 120};
	f32 			padding = 20;

	// Per frame state
	game::Input * 	input;
	v2 				currentPosition;
	s32 			currentSelectableIndex;

	// Per visible period state
	s32 			selectedIndex;
	s32 			selectableCountLastFrame;
};

internal Gui make_gui()
{
	auto textureAsset = load_texture_asset("assets/textures/no_gui_texture.png", global_transientMemory);
	auto texture = platformApi->push_texture(platformGraphics, &textureAsset);

	Gui gui = {};
	gui.material = platformApi->push_gui_material(platformGraphics, texture);

	return gui;
}

Gui * global_currentGui = nullptr;

void gui_start(Gui & gui, game::Input * input)
{
	Assert(global_currentGui == nullptr);

	global_currentGui 			= &gui;
	gui.input 					= input;
	gui.currentPosition 		= gui.startPosition;
	gui.currentSelectableIndex 	= 0;

	if (gui.selectableCountLastFrame > 0)
	{
		if (is_clicked(input->down))
		{
			gui.selectedIndex += 1;
			gui.selectedIndex %= gui.selectableCountLastFrame;
		}
		if(is_clicked(input->up))
		{
			gui.selectedIndex -= 1;
			if (gui.selectedIndex < 0)
			{
				gui.selectedIndex += gui.selectableCountLastFrame;
			}
		}
	}
}

void gui_end()
{
	// Todo(Leo): move all draw calls here
	global_currentGui->selectableCountLastFrame = global_currentGui->currentSelectableIndex;
	global_currentGui = nullptr;
}

bool gui_button(v4 color)
{
	Gui & gui = *global_currentGui;

	v2 size 	= gui.buttonSize;
	v2 position = gui.currentPosition;

	bool isSelected = (gui.currentSelectableIndex == gui.selectedIndex);
	++gui.currentSelectableIndex;

	if (isSelected)
	{
		position -= v2{0.1f * size.x, 0.1f*size.y};
		size *= 1.2f;
	}
	platformApi->draw_gui(platformGraphics, position, size, gui.material, color);
	gui.currentPosition.y += gui.buttonSize.y + gui.padding;

	return isSelected && is_clicked(gui.input->confirm);
}