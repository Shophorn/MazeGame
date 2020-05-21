/*=============================================================================
Leo Tamminen
shophorn @ internet

Immediate mode gui structure.
=============================================================================*/

struct Gui
{
	// References
	MaterialHandle 	material;

	Font 			font;
	MaterialHandle 	fontMaterial;

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
	global_currentGui->selectableCountLastFrame = global_currentGui->currentSelectableIndex;
	global_currentGui = nullptr;
}

void gui_text (char const * text, v2 position, v4 color)
{
	Gui & gui = *global_currentGui;

	f32 size = 40;
	s32 firstCharacter = ' ';
	s32 charactersPerDirection = 10;
	f32 characterUvSize = 1.0f / charactersPerDirection;

	while(*text != 0)
	{
		if (*text == ' ')
		{
			position.x += size * gui.font.spaceWidth;
			++text;
			continue;
		}

		s32 index = *text - firstCharacter;

		platformApi->draw_screen_rect(	platformGraphics,
										{position.x + gui.font.leftSideBearings[index], position.y},
										{gui.font.characterWidths[index] * size, size},
										gui.font.uvPositionsAndSizes[index].xy,
										gui.font.uvPositionsAndSizes[index].zw,
										gui.fontMaterial,
										color);

		position.x += size * gui.font.advanceWidths[index];
		++text;
	}
};

bool gui_button(char const * label)
{
	Gui & gui = *global_currentGui;

	bool isSelected = (gui.currentSelectableIndex == gui.selectedIndex);
	++gui.currentSelectableIndex;

	v4 color = isSelected ? colors::mutedYellow : colors::white;

	gui_text(label, gui.currentPosition, color);

	gui.currentPosition.y += gui.buttonSize.y + gui.padding;

	bool result = isSelected && is_clicked(gui.input->confirm);
	return result;
}

void gui_generate_font_material(Gui & gui)
{
	// Todo(Leo): We need something like this, but this won't work since TextureHandles do have default index -1, 
	// but if we have allocted this in array etc, it may not have been initialized to it properly...
	Assert(gui.font.atlasTexture >= 0);

	auto guiPipeline = platformApi->push_pipeline (	platformGraphics,
													"assets/shaders/gui_vert3.spv",
					    							"assets/shaders/gui_frag2.spv",
					    							{
										                .textureCount           = 1,
										                .pushConstantSize       = sizeof(v2) * 4 + sizeof(v4),

										                .primitiveType          = platform::RenderingOptions::PRIMITIVE_TRIANGLE_STRIP,
										                .cullMode               = platform::RenderingOptions::CULL_NONE,

														.enableDepth 			= false,
										                .useVertexInput         = false,
										                .useSceneLayoutSet      = false,
										                .useMaterialLayoutSet   = true,
										                .useModelLayoutSet      = false,
										                .enableTransparency     = true
										            });

	MaterialAsset guiFontMaterialInfo = {guiPipeline, allocate_array<TextureHandle>(*global_transientMemory, {gui.font.atlasTexture})};
	gui.fontMaterial = platformApi->push_material(platformGraphics, &guiFontMaterialInfo);
}